# Weight In Bytes
Header-only C++17 which library which approximates the size of memory allocated by any struct/class and it's members.
Class members are automatically reflected via boost::pfr, Cista, or manually by providing an as_tuple() member function for you class. 
Additionally, if your code base uses Cereal for serialization, it's serialization functions can be hijacked in order to reflect class members.


## Quick start
```cpp
#define WIB_ENABLE_PFR // Use boost::pfr for automatic reflection
#include <wib/wib.hpp>

struct Street {
  std::string name_{};
  std::vector<int> numbers_{};
};
struct Citizen {
  std::string name_{};
  int age_{};
};
struct Town {
  std::vector<Citizen> citizens_{};
  std::vector<Street> streets_{};
};

auto main() {
  auto town = Town{};

  // Insert data with heap allocation
  town.citizens_.emplace("Karl-Petter Andersson", 42);
  town.citizens_.emplace("Emma-Britta Larsson", 52);
  town.streets_.emplace("Big main street", {1,3,5,7,9,11,13,15,17});
  town.streets_.emplace("Small side street", {2,4,6});
  
  // Examine the allocated size of 'town' (this it it!)
  size_t size = wib::weight_in_bytes(town);

  // Under the hood, approximately the following summation is executed:
  size_t expected_size = 
    sizeof(Town::Citizen) * town.citizens_.capacity() + 
      town.citizens_[0].name_.capacity() +
      town.citizens_[1].name_.capacity() +
    sizeof(Town::Street) * town.streets_.capacity() +
      sizeof(int) * town.streets_[0].numbers_.capacity() +
      sizeof(int) * town.streets_[1].numbers_.capacity();
  assert(size == expected_size);

  // Verify we where able to examine all types in 'town'
  auto unknown_types = wib::unknown_types(town);
  assert(unknown_types.size() == 0);
}
```

## Features
* Handles containers, pointers, std::optional, std::tuple, std::variant, std::any out of the box
* std::any is introspected by providing a type-list of possible types
* Automatic reflection of class members are provided via boost::pfr or Cista
* Automatic reflection of class members can utilize Cereal serialization functions (conside work in progress)
* Multiple pointers to the same element counts as a single allocation
* Containers with internal buffers (such as std::string) are not reported as allocated until the contained data is allocated on the heap

## Public interface
Approximate heap allocation size of any object:
```cpp
template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto wib::weight_in_bytes(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->size_t;
```

List types which coudn't be reflected:
```cpp
template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto wib::unknown_types(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->typeindex_set_t;
```

## Features by example


### Provide as_tuple() for reflection
```cpp
class Custom {
public:
  Custom() {
    s.resize(3000);
  }
  auto as_tuple() const { return std::tie(a,b,c,s); }
private:
  double a,b,c;
  std::string s;
};
auto custom = Custom{};
assert(wib::unknown_types(custom).size() == 0);
assert(wib::weight_in_bytes(custom) == 3000);
```

### Provide weight_in_bytes() member function 
For special cases, a precalculated size be provided via weight_in_bytes() const -> size_t member function.
```cpp
class GLTexture {
public:
  GLTexture() {
    glGenTextures(1, &textureid_);
    glBindTexture(GL_TEXTURE_2D, textureid_); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R, width, height, 0, GL_R8, GL_UNSIGNED_BYTE, nullptr);
  }
  auto weight_in_bytes() const -> size_t { return width*height;}
private:
  GLint textureid_;
  int width = 1024;
  int height = 1024;
};
auto texture = GLTexture{};
assert(wib::weight_in_bytes(texture) == 1024*1024);
```

### Introspecting std::any
```cpp
using bytevec_t = std::vector<char>;
const auto s = std::string(200);
const auto v = bytevec_t(400};
// No types provided
{
  auto a = std::any{};
  a = s;
  // The std::string is not recognized
  assert(wib::weight_in_bytes(a) == 0);
  assert(wib::unknown_types(a).size() == 1);
}
// Provide type-list as a std::tuple
{
  using types = std::tuple<std::string, bytevec_t>;
  auto a = std::any{};
  a = s;
  // The std::string is recognized
  assert(wib::weight_in_bytes<types>(a) == 200);
  assert(wib::unknown_types(a).size() == 0);
  // The bytevec_t is recognized
  a = v;
  assert(wib::weight_in_bytes<types>(a) == 400);
  assert(wib::unknown_types(a).size() == 0);
}
```

### Utilize Cereal for reflection
Note that hijacking Cereal is work in progress
```cpp
#define WIB_CEREAL
#include <wib/wib.hpp>
class MyClass {
public:
  MyClass() {
    a.resize(100);
    b.resize(100);
    c.resize(100);
  }
  template <typename Ar>
  auto serialize(Ar& ar) -> void {
    ar(a, b);
    ar(c);
  }
private:
  std::string a,b,c;
};
auto main() {
  auto myclass = MyClass{};
  // The serialize(Ar&ar) member function is hijacked to introspect the members of MyClass
  assert(wib::weight_in_bytes(myclass) == 300);
  assert(wib::unknown_types(myclass).size() == 0);
}
```

### Pointers to same element
Pointers are tracked, so many pointers to the same object will not add additional size.
```cpp
using bytes_t = std::array<char, 1000>;
auto pair = std::pair<
  std::shared_ptr<bytes_t>,
  std::shared_ptr<bytes_t>
>{};
assert(wib::weight_in_bytes(pair) == 0);
myclass.first = std::make_shared<bytes_t>();
assert(wib::weight_in_bytes(pair) == 1000));
myclass.second = myclass.first;
assert(wib::weight_in_bytes(pair) == 1000); // Pointers refers to same object
myclass.second = std::make_shared<bytes_t>();
assert(wib::weight_in_bytes(pair) == 2000); // Pointers refers to different objects
```

### Unused capacity
For containers with possibly unused capacity (std::vector, std::string), the current allocation is taken into account, not the number of elements.
```cpp
auto vec = std::vector<char>{};
assert(wib::weight_in_bytes(vec) == 0);
vec.resize(1000);
assert(wib::weight_in_bytes(vec) == 0);
vec.clear();
// vec still holds the allocation of 1000 bytes
assert(wib::weight_in_bytes(vec) == 1000);
// Remove superflous allocation
vec.shrink_to_fit();
assert(wib::weight_in_bytes(vec) == 0);
```

### Containers with internal buffers
For containers which keeps small numbers of elements inside them, no allocation is reported:
```cpp
// Example using std::string, assuming an internal buffer of 15 chars
auto str = std::string{"abc"};
// str is smaller than the internal buffer size, nothing is allocated
assert(wib::weight_in_bytes(str) == 0);
// Assign a string of 20 chars
str = "12345678901234567890";
assert(wib::weight_in_bytes(str) == 20);

// Example with boost::container::small_vector<T, N>
auto sv = boost::container::small_vector<char, 16>{};
sv.resize(16);
assert(wib::weight_in_bytes(sv) == 0);
sv.resize(17);
assert(wib::weight_in_bytes(sv) >= 17);
```


## Configuration
* Define WIB_ENABLE_PFR to utilize boost::pfr for automatic reflection
* Define WIB_ENABLE_CISTA to utilize Cista for automatic reflection
* (If your codebase uses Cereal) Define WIB_CEREAL to utilize MyClass::serialize(Ar&ar) or MyClass::save(Ar& ar) for reflection.


## Notes
* Overhead for structure of containers other than continous memory allocated is not included
* structs/classes smaller than the size of pointer is assumed to not heap-allocate
* std::weak_ptr's are assumed to be non-owning and ignored
* std::basic_string_view<T> are assumed to be non-owning and ignored
* allocated storage of std::function is not handled
* std::unique_ptr's to arrays (std::unique_ptr<T[]>), only uses takes the first element into account as the size cannot be determined.
* Unit-tests is available in test/test.cpp (uses Catch)



## Future updates
* More accurate overhead calculation of containers
* Add non-intrusive version of as_tuple()
* Improve Cereal support to utilize non-intrusive and versioned Cereal functions
* Improve overhead for standard library containers
* Add more reflection possibilities (boost::describe and boost:serialize)


## License
Use however you want, crediting is nice but not nessesary.
