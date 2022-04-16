# Weight in bytes
Header-only C++17 which library which approximates the allocation size of any struct/class and it's members.

## Quick start
Allocation size of vector-of-vector<char>
```cpp
auto vecs = std::vector<std::vector<char>>{};
vecs.resize(10);
std::fill(vecs.begin(), vecs.end(), vec);
assert(wib::weight_in_bytes(vecs) == 10*1024);
```

Allocation size of class
```cpp
#define WIB_ENABLE_PFR // User boost::pfr for introspection
#include <wib/wib.hpp>

struct Town {
  struct Street {
    std::string name_{};
    std::vector<int> numbers_{};
  };
  struct Citizen {
    std::string name_{};
    int age_{};
  };
  std::vector<Citizen> citizens_{};
  std::vector<Street> streets_{};
};

auto main() {
  auto town = Town{};
  // Insert data with heap allocation
  town.citizens_.emplace("Karl Peter Andersson", 42);
  town.citizens_.emplace("Emma Britta Larsson", 52);
  town.streets_.emplace("Big main street", {1,3,5,7,9});
  town.streets_.emplace("Small side street", {2,4,6,8,10});
  
  // Get size of heap allocation
  auto size = wib::weight_in_bytes(town);
  size_t expected_size = 
    sizeof(Town::Citizen) * town.citizens_.size() + 
      town.citizens_[0].name_.size() +
      town.citizens_[1].name_.size() +
    sizeof(Town::Street) * town.streets_.size() +
      sizeof(int) * town.streets_[0].numbers_.size() +
      sizeof(int) * town.streets_[1].numbers_.size();
  assert(size == expected_size);
}
```

## Features
* Handles containers, pointers, std::optional, std::tuple, std::variant, std::any out of the box
* std::any is introspected by providing a type-list of possible types
* Automatic introspection of classes provided via via boost::pfr, cista
* Multiple pointers to the same element counts as a single allocation
* Containers with internal buffers (such as std::string) are taken account for

## Reference
Retrieve heap allocation size of any type.
```cpp
template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto wib::weight_in_bytes(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->size_t;
```

List types which coudn't be inspected
```cpp
template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto wib::unknown_types(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->typeindex_set_t;
```

## Feature examples

### Pointers to same element
Pointers are tracked, so many pointers to the same object will not add additional size.
```cpp
auto sptr = std::make_shared<int>();
auto array = std::array<std::shared_ptr<int>, 100>{};
array.fill(sptr);
assert(wib::weight_in_bytes(sptr) == wib::weight_in_bytes(array));
```

### Provide as_tuple() for introspection
```cpp
class Custom {
public:
  auto as_tuple() const { return std::tie(a,b,c,s); }
private:
  double a,b,c;
  std::string s;
};
auto custom = Custom{};
assert(wib::unknown_types(custom).size() == 0);
```

### Unused capacity
For containers with possibly unused capacity (std::vector, std::string), the current allocation is taken into account, not the number of elements.
```cpp
auto vec = std::vector<char>{};
assert(wib::weight_in_bytes(vec) == 0);
vec.resize(1000);
assert(wib::weight_in_bytes(vec) == 0);
vec.clear();
// vec still holds an allocation of 1000 bytes
assert(wib::weight_in_bytes(vec) == 1000);
```

### Internal buffer containers
For containers which keeps small numbers of elements inside them, no allocation is reported:
```cpp
// Example using std::string
auto str = std::string{"abc"};
assert(wib::weight_in_bytes(str) == 0);
str.resize(1000);
assert(wib::weight_in_bytes(str) >= 1000);
// Example with boost::container::small_vector
auto sv = boost::container::small_vector<char, 16>{};
sv.resize(16);
assert(wib::weight_in_bytes(sv) == 0);
sv.resize(17);
assert(wib::weight_in_bytes(sv) >= 17);
```

### Introspcting std::any
```cpp
using bytevec_t = std::vector<char>;
const auto s = std::string(200);
const auto v = bytevec_t(400};
// No types provided
{
  auto a = std::any{};
	a = s;
  assert(wib::weight_in_bytes(a) == 0);
  assert(wib::unknown_types(a).size() == 1);
}
// Provide type-list as a std::tuple
{
  using types = std::tuple<std::string, bytevec_t>;
  auto a = std::any{};
	a = s;
  assert(wib::weight_in_bytes<types>(a) => 200);
  assert(wib::unknown_types(a).size() == 0);
  a = v;
  assert(wib::weight_in_bytes<types>(a) >= 400);
  assert(wib::unknown_types(a).size() == 0);
}
```

## Configuration
* Define WIB_ENABLE_PFR to utilize boost::pfr
* Define WIB_ENABLE_CISTA to utilize introspection via Cista
* Define WIB_CEREAL to enable introspection via Cereal 

## Notes
* structs/classes smaller than the size of pointer is assumed to not heap-allocate
* std::weak_ptr's are assumed to be non-owning and ignored
* std::basic_string_view<T> are assumed to be non-owning and ignored
* std::function is not handled
* std::unique_ptr's to arrays (std::unique_ptr<T[]>), only uses takes the first element into account as the size cannot be determined.

## Future updates:
Customization via free functions in addition 
