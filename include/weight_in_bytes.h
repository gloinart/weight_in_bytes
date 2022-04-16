#pragma once
#include <typeindex>
#include <unordered_set>
#include <tuple>


// Public interface
namespace wib {
enum class efollow_raw_pointers{False, True};
using typeindex_set_t = std::unordered_set<std::type_index>;
using empty_typelist_t = std::tuple<>;

template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto weight_in_bytes(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->size_t;

template <typename AnyTypeList = empty_typelist_t, typename T>
[[nodiscard]] auto unknown_typeindexes(
  const T& value,
  efollow_raw_pointers follow_raw_pointers = efollow_raw_pointers::False
)->typeindex_set_t;
}






// Configuration
#ifdef WIB_PFR_ENABLED
  #include <boost/pfr.hpp>
#endif
#ifdef WIB_CISTA_ENABLED
  #include <cista/reflection/for_each_field.h>
#endif
#ifdef WIB_CEREAL_ENABLED
  // Do nothing
#endif

#if defined(WIB_CISTA_ENABLED) && defined(WIB_PFR_ENABLED)
namespace {
  static_assert(false, "Cannot enable both cista and boost::pfr");
}
#endif








// Includes
#include <type_traits>
#include <typeinfo>
#include <memory>
#include <variant>
#include <optional>
#include <any>











// Type-traits
namespace wib::detail::type_traits {

namespace introspection {
  // Taken from http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4502.pdf
  template <typename...>
  using void_t = void;
  // Primary template handles all types not supporting the operation.
  template <typename, template <typename> class, typename = void_t<>>
  struct detect : std::false_type {};
  // Specialization recognizes/validates only types supporting the archetype.
  template <typename T, template <typename> class Op>
  struct detect<T, Op, void_t<Op<T>>> : std::true_type {};
}


template <typename T> constexpr auto is_smart_ptr_f(const std::unique_ptr<T>&) { return std::true_type{}; }
template <typename T> constexpr auto is_smart_ptr_f(const std::shared_ptr<T>&) { return std::true_type{}; }
template <typename T> constexpr auto is_smart_ptr_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_smart_ptr_v = decltype(is_smart_ptr_f(std::declval<T>()))::value;

template <typename T> constexpr auto is_weak_ptr_f(const std::weak_ptr<T>&) { return std::true_type{}; }
template <typename T> constexpr auto is_weak_ptr_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_weak_ptr_v = decltype(is_weak_ptr_f(std::declval<T>()))::value;


template <typename ...Ts> constexpr auto is_variant_f(const std::variant<Ts...>&) { return std::true_type{}; }
template <typename T> constexpr auto is_variant_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_variant_v = decltype(is_variant_f(std::declval<T>()))::value;

template <typename A, typename B> constexpr auto is_pair_f(const std::pair<A, B>&) { return std::true_type{}; }
template <typename T> constexpr auto is_pair_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_pair_v = decltype(is_pair_f(std::declval<T>()))::value;

template <typename ...Ts> constexpr auto is_tuple_f(const std::tuple<Ts...>&) { return std::true_type{}; }
template <typename T> constexpr auto is_tuple_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_tuple_v = decltype(is_tuple_f(std::declval<T>()))::value;

template <typename T> constexpr auto is_optional_f(const std::optional<T>&) { return std::true_type{}; }
template <typename T> constexpr auto is_optional_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_optional_v = decltype(is_optional_f(std::declval<T>()))::value;

template <typename T> constexpr auto is_string_view_f(const std::basic_string_view<T>&) { return std::true_type{}; }
template <typename T> constexpr auto is_string_view_f(const T&) { return std::false_type{}; }
template <typename T>
constexpr auto is_string_view_v = decltype(is_string_view_f(std::declval<T>()))::value;

template <typename T> constexpr auto is_any_f(const T&) { return std::false_type{}; }
inline constexpr auto is_any_f(const std::any&) { return std::true_type{}; }
template <typename T>
constexpr auto is_any_v = decltype(is_any_f(std::declval<T>()))::value;

template<class T> using begin_t = decltype(std::begin(std::declval<T&>()));
template <typename T> constexpr auto has_begin_v = introspection::detect<T, begin_t>::value;

template<class T> using end_t = decltype(std::end(std::declval<T&>()));
template <typename T> constexpr auto has_end_v = introspection::detect<T, end_t>::value;

template<class T> using data_t = decltype(std::declval<T&>().data());
template <typename T> constexpr auto has_data_v = introspection::detect<T, data_t>::value;

template<class T> using capacity_t = decltype(std::declval<T&>().capacity());
template <typename T> constexpr auto has_capacity_v = introspection::detect<T, capacity_t>::value;

template<class T> using weight_in_bytes_t = decltype(std::declval<T&>().weight_in_bytes());
template <typename T> constexpr auto has_weight_in_bytes_v = introspection::detect<T, weight_in_bytes_t>::value;

template<class T> using as_tuple_t = decltype(std::declval<T&>().as_tuple());
template <typename T> constexpr auto as_tuple_v = introspection::detect<T, as_tuple_t>::value;

template<class T> using cereal_serialize_t = decltype(std::declval<T&>().serialize(
  std::declval<T&>() // Just use anything as Archive parameter
));
template <typename T> constexpr auto has_cereal_serialize_v = introspection::detect<T, cereal_serialize_t>::value;
template<class T> using cereal_save_t = decltype(std::declval<const T&>().save(
  std::declval<T&>() // Just use anything as Archive parameter
));
template <typename T> constexpr auto has_cereal_save_v = introspection::detect<T, cereal_save_t>::value;

template <class T> using key_type_t = typename T::key_type;
template <class T> using mapped_type_t = typename T::mapped_type;
template <class T>
constexpr auto is_map_v =
  introspection::detect<T, key_type_t>::value &&
  introspection::detect<T, mapped_type_t>::value;

}
























// Utilities
namespace wib::detail::wibutil {
  
template <typename F>
struct scope_exit_t {
  constexpr scope_exit_t(F&& f) noexcept : f_{ std::forward<F>(f) } {}
  constexpr scope_exit_t() noexcept = delete;
  constexpr scope_exit_t(const scope_exit_t&) noexcept = delete;
  constexpr scope_exit_t(scope_exit_t&&) noexcept = delete;
  constexpr scope_exit_t& operator=(const scope_exit_t&) noexcept = delete;
  constexpr scope_exit_t& operator=(scope_exit_t&&) noexcept = delete;
  ~scope_exit_t() { f_(); }
  const F f_;
};

// Iterate tuple
template <typename Tpl, typename F, size_t Idx = 0>
[[nodiscard]] auto tuple_for_each(const Tpl& tpl, const F& f) -> void {
  constexpr auto is_last = Idx >= std::tuple_size_v<Tpl>;
  if constexpr (is_last) {
    return;
  }
  else {
    f(std::get<Idx>(tpl));
    tuple_for_each<Tpl, F, Idx + 1>(tpl, f);
  }
}


// Try visit std::any with a type-list from provided tuple
template <typename TupleTypeList, typename F, size_t Idx = 0>
[[nodiscard]] auto try_visit_any(const std::any& a, const F& f) -> bool {
  constexpr auto is_last = Idx >= std::tuple_size_v<TupleTypeList>;
  if constexpr (is_last) {
    return false;
  }
  else {
    using value_t = std::tuple_element_t<Idx, TupleTypeList>;
    const auto* ptr = std::any_cast<value_t>(&a);
    if (ptr != nullptr) {
      f(*ptr);
      return true;
    }
    return try_visit_any<TupleTypeList, F, Idx + 1>(a, f);
  }
}


// Inspect members via cereal archive proxy
template <typename F>
struct cereal_archive_inspector {
  F f_{};
  template <typename ...Ts>
  auto operator()(const Ts&... members) const -> void {
    auto tpl = std::tie(members...);
    tuple_for_each(tpl, f_);
  }
};


}





















namespace wib::detail {


using byteptr_t = const uint8_t*;
using address_set_t = std::unordered_set<byteptr_t>;



struct data_t {
  const efollow_raw_pointers follow_raw_pointers{};
  address_set_t& handled_addresses;
  typeindex_set_t* unknown_types{nullptr};
  size_t& current_depth;
};


template <typename AnyTypeList, typename T>
[[nodiscard]] auto get_heap_allocated_bytes(
  const T& value, 
  data_t& io_data
) -> size_t {
  // Skip plain simple types (int, float, char, bool, enum etc)
  if constexpr (
    std::is_arithmetic_v<T> ||
    std::is_enum_v<T>
  ) {
    return 0;
  }

  const auto current_depth = io_data.current_depth;
  ++io_data.current_depth;
  const auto scope_exit = wibutil::scope_exit_t{ 
    [&io_data]() { --io_data.current_depth; } 
  };
  const auto record_unknown_types = io_data.unknown_types != nullptr;

  auto accumulate_pointer_f = [&io_data](const auto* ptr) -> size_t {
    if (ptr == nullptr) {
      return 0;
    }
    const auto* byteptr = reinterpret_cast<byteptr_t>(ptr);
    if (
      const auto is_handled = io_data.handled_addresses.count(byteptr) > 0;
      is_handled
     ) {
      return 0;
    }
    io_data.handled_addresses.insert(byteptr);
    using value_t = std::remove_pointer_t<decltype(ptr)>;
    return get_heap_allocated_bytes<AnyTypeList>(*ptr, io_data) + sizeof(value_t);
  };

  auto is_inside_self_f = [
    my_address_first = reinterpret_cast<byteptr_t>(std::addressof(value)),
    my_address_last = reinterpret_cast<byteptr_t>(std::addressof(value)) + sizeof(value)
  ](const auto* address) noexcept -> bool {
    return
      reinterpret_cast<byteptr_t>(address) >= my_address_first &&
      reinterpret_cast<byteptr_t>(address) < my_address_last;
  };

  // non-allocating std types
  if constexpr (
    type_traits::is_string_view_v<T> ||
    type_traits::is_weak_ptr_v<T>
  ) {
    return 0;
  }
  // has custom weight_in_bytes
  else if constexpr(type_traits::has_weight_in_bytes_v<T>) {
    return value.weight_in_bytes();
  }
  // has custom as_tuple
  else if constexpr (type_traits::as_tuple_v<T>) {
    return get_heap_allocated_bytes<AnyTypeList>(value.as_tuple(), io_data);
  }
  // objects smaller than a pointer is assumed to not heap allocate if it does not have a customized version
  else if constexpr (sizeof(value) < sizeof(const void*)) {
    return 0;
  }
  // std::optional
  else if constexpr (type_traits::is_optional_v<T>) {
    return value.has_value() ?
      get_heap_allocated_bytes<AnyTypeList>(*value, io_data) :
      0;
  }
  // smart_ptr
  else if constexpr (type_traits::is_smart_ptr_v<T>) {
    return accumulate_pointer_f(value.get());
  }
  // raw pointer
  else if constexpr (std::is_pointer_v<T>) {
    return io_data.follow_raw_pointers == efollow_raw_pointers::True ?
      accumulate_pointer_f(value):
      0;
  }
  // std::variant
  else if constexpr (type_traits::is_variant_v<T>) {
    if (value.valueless_by_exception()) {
      return 0;
    }
    const auto bytes = std::visit(
      [&io_data](const auto& variant_value) -> size_t {
        return get_heap_allocated_bytes<AnyTypeList>(variant_value, io_data);
      }, 
      value
    );
    return bytes;
  }
  // std::pair
  else if constexpr (type_traits::is_pair_v<T>) {
    const auto bytes = 
      get_heap_allocated_bytes<AnyTypeList>(value.first, io_data) +
      get_heap_allocated_bytes<AnyTypeList>(value.second, io_data);
    return bytes;
  }
  // std::tuple
  else if constexpr (type_traits::is_tuple_v<T>) {
    auto allocation_bytes = size_t{ 0 };
    wibutil::tuple_for_each(value, [&allocation_bytes, &io_data](auto&& element) -> void {
      allocation_bytes += get_heap_allocated_bytes<AnyTypeList>(element, io_data);
    });
    return allocation_bytes;
  }
  // container
  else if constexpr (
    type_traits::has_begin_v<T> &&
    type_traits::has_end_v<T>
  ) {
    auto accumulate_range_f = [&io_data](const auto& range) noexcept -> size_t {
      auto bytes = size_t{ 0 };
      for (auto&& element : range) {
        bytes += get_heap_allocated_bytes<AnyTypeList>(element, io_data);
      }
      return bytes;
    };
    constexpr auto is_vector_bool = 
      std::is_same_v<std::vector<bool>, T>;
    constexpr auto is_continuous_memory =
      type_traits::has_data_v<T> &&
      type_traits::has_capacity_v<T>;
    constexpr auto is_map = 
      type_traits::is_map_v<T>;
    if constexpr (is_vector_bool) {
      return value.capacity() / 8;
    }
    else if constexpr (is_continuous_memory) {
      using value_type = typename T::value_type;
      const auto is_stack_allocated = is_inside_self_f(value.data());
      const auto allocation_bytes = 
        value.data() == nullptr ? size_t{ 0 }:
        is_stack_allocated ? size_t{ 0 }:
        sizeof(value_type) * value.capacity();
      return allocation_bytes + accumulate_range_f(value);
    }
    else if constexpr (is_map) {
      auto allocation_bytes = size_t{ 0 };
      for (auto&& kvp : value) {
        // We might be dealing with a small map of some sort
        if (!is_inside_self_f(std::addressof(kvp.first))) {
          allocation_bytes += sizeof(kvp.first);
        }
        if (!is_inside_self_f(std::addressof(kvp.second))) {
          allocation_bytes += sizeof(kvp.second);
        }
      }
      return allocation_bytes + accumulate_range_f(value);
    }
    else {
      auto allocation_bytes = size_t{ 0 };
      for (const auto& element : value) {
        if (!is_inside_self_f(std::addressof(element))) {
          allocation_bytes += sizeof(element);
        }
      }
      return allocation_bytes + accumulate_range_f(value);
    }
  }
  // std::any
  else if constexpr (type_traits::is_any_v<T>) {
    if (!value.has_value()) {
      return 0;
    }
    auto bytes = size_t{ 0 };
    auto visitor = [&bytes, &io_data, is_inside_self_f](const auto& casted_value) -> void {
      bytes += get_heap_allocated_bytes<AnyTypeList>(casted_value, io_data);
      // Element might be allocated in small storage
      if (!is_inside_self_f(std::addressof(casted_value))) {
        bytes += sizeof(casted_value);
      }
    };
    const auto handled =
      wibutil::try_visit_any<AnyTypeList>(value, visitor) ? true :
      false;
    if (!handled && record_unknown_types) {
      io_data.unknown_types->emplace(value.type());
    }
    return bytes;
  }
  // access members via cereal
#ifdef WIB_CEREAL_ENABLED
  else if constexpr (
    type_traits::has_cereal_serialize_v<T> ||
    type_traits::has_cereal_save_v<T> 
  ) {
    auto bytes = size_t{ 0 };
    auto visitor = [&io_data, &bytes](auto&& member) -> void {
      bytes += get_heap_allocated_bytes<AnyTypeList>(member, io_data);
    };
    using visitor_t = decltype(visitor);
    auto archive_inspector = wibutil::cereal_archive_inspector<visitor_t>{visitor};
    if constexpr (type_traits::has_cereal_serialize_v<T>) {
      auto* value_mut = const_cast<T*>(std::addressof(value)); // Sorry...
      value_mut->serialize(archive_inspector);
    }
    else {
      value.save(archive_inspector);
    }
    return bytes;
  }
#endif
  // access members via struct-to-tuple decomposition
  else if constexpr (std::is_aggregate_v<T>) {
#if defined(WIB_PFR_ENABLED)
    if constexpr (boost::pfr::tuple_size_v<T> == 0) {
      return 0;
    }
    else {
      auto bytes = size_t{ 0 };
      boost::pfr::for_each_field(value, [&bytes, &io_data](auto&& member) {
        bytes += get_heap_allocated_bytes<AnyTypeList>(member, io_data);
      });
      return bytes;
    }
#elif defined(WIB_CISTA_ENABLED)
    auto bytes = size_t{ 0 };
    cista::for_each_field(value, [&bytes, &io_data](auto&& member) {
      bytes += get_heap_allocated_bytes<AnyTypeList>(member, io_data);
    });
    return bytes;
#endif
  }

  // store unknown type
  if(record_unknown_types) {
    const auto& type_info = typeid(T);
    io_data.unknown_types->emplace(type_info);
  }

  // could not approximate bytes
  return 0;
}


}







































namespace wib {

template <typename AnyTypeList, typename T>
auto weight_in_bytes(
  const T& value,
  const efollow_raw_pointers follow_raw_pointers
) -> size_t {
  static_assert(detail::type_traits::is_tuple_v<AnyTypeList>);
  auto handled_addresses = detail::address_set_t{};
  auto current_depth = size_t{ 0 };
  auto io_data = detail::data_t{
    follow_raw_pointers,
    handled_addresses,
    nullptr,
    current_depth
  };
  return detail::get_heap_allocated_bytes<AnyTypeList>(value, io_data);
}

template <typename AnyTypeList, typename T>
auto unknown_typeindexes(
  const T& value,
  const efollow_raw_pointers follow_raw_pointers
) -> typeindex_set_t {
  static_assert(detail::type_traits::is_tuple_v<AnyTypeList>);
  auto handled_addresses = detail::address_set_t{};
  auto current_depth = size_t{ 0 };
  auto unknown_types = typeindex_set_t{};
  auto io_data = detail::data_t{
    follow_raw_pointers,
    handled_addresses,
    std::addressof(unknown_types),
    current_depth
  };
  [[maybe_unused]] const auto bytes = detail::get_heap_allocated_bytes<AnyTypeList>(
    value,
    io_data
  );
  return unknown_types;
}


}





