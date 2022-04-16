
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//#define WIB_PFR_ENABLED
//#define WIB_CISTA_ENABLED
#define WIB_CEREAL_ENABLED
#include "../include/weight_in_bytes.h"

#include <iostream>
#include <map>
#include <type_traits>
#include <mutex>
#include <array>
#include <chrono>
#include <string>
#include <cstddef>
#include <any>



namespace {
using byte_t = char;
static_assert(sizeof(byte_t) == 1);
using bytevec_t = std::vector<byte_t>;

}


TEST_CASE("std::any - custom types") {

	{
		auto a = std::any{};
		using array_of_bytevecs_t = std::array<bytevec_t, 128>;
		auto arr = array_of_bytevecs_t{};
		arr.fill(bytevec_t(1024));
		a = arr;
		// No types passed
		REQUIRE(wib::unknown_typeindexes(a).size() == 1);
		REQUIRE(wib::weight_in_bytes(a) == 0);
		// Types passed
		using any_types = std::tuple<array_of_bytevecs_t>;
		REQUIRE(wib::unknown_typeindexes<any_types>(a).size() == 0);
		REQUIRE(
			wib::weight_in_bytes<any_types>(a) ==
			sizeof(arr) + 1024 * arr.size()
		);
		REQUIRE(
			wib::weight_in_bytes(arr) ==
			1024 * arr.size()
		);
	}
	{
		struct type_a {
			auto weight_in_bytes() const { return 42; }
			std::array<char, 512> space{};
		};
		struct type_b {
			auto weight_in_bytes() const { return 84; }
			std::array<char, 512> space{};
		};
		auto v = std::vector<std::any>{};
		v.reserve(2);
		v.push_back(type_a{});
		v.push_back(type_b{});
		REQUIRE(wib::unknown_typeindexes(v).size() == 2);
		REQUIRE(
			wib::weight_in_bytes(v) ==
			sizeof(std::any) * 2
		);
		using any_types = std::tuple<type_a, type_b>;
		REQUIRE(wib::unknown_typeindexes<any_types>(v).size() == 0);
		REQUIRE(
			wib::weight_in_bytes<any_types>(v) ==
			(sizeof(std::any) * v.capacity()) + // vector allocation
			(sizeof(type_a) + type_a{}.weight_in_bytes()) + // element 0
			(sizeof(type_b) + type_b{}.weight_in_bytes()) // element 1
		);

	}
}




TEST_CASE("small objects") {
	struct str {
		char c{};
	};
	static_assert(sizeof(str) < sizeof(void*));
	REQUIRE(wib::unknown_typeindexes(str{}).size() == 0);
	REQUIRE(wib::unknown_typeindexes(float{}).size() == 0);
	REQUIRE(wib::unknown_typeindexes(double{}).size() == 0);
	REQUIRE(wib::weight_in_bytes(str{}) == 0);
	REQUIRE(wib::weight_in_bytes(float{}) == 0);
	REQUIRE(wib::weight_in_bytes(double{}) == 0);

};


TEST_CASE("c-array") {
	bytevec_t arr[100]{};
	for (auto&& v : arr) {
		v.resize(128);
	}
	auto unknown = wib::unknown_typeindexes(arr);
	REQUIRE(unknown.size() == 0);
	REQUIRE(wib::weight_in_bytes(arr) == 128 * 100);
};


TEST_CASE("continous memory containers") {

	// std::array
	{
		REQUIRE(wib::weight_in_bytes(std::array<int, 1000>{}) == 0);
	}
	// std::array of vectors
	{
		std::array<bytevec_t, 100> arr{};
		for (auto&& v : arr) {
			v.resize(128);
		}
		auto unknown = wib::unknown_typeindexes(arr);
		REQUIRE(unknown.size() == 0);
		REQUIRE(wib::weight_in_bytes(arr) == 128 * 100);
		for (auto& v : arr) {
			v.clear();
		}
		// Capacity is preserved
		REQUIRE(wib::weight_in_bytes(arr) == 128 * 100);
	}

	// std::vector
	{
		auto vec = bytevec_t{};
		REQUIRE(wib::weight_in_bytes(vec) == 0);
		vec.resize(512);
		REQUIRE(wib::weight_in_bytes(vec) == 512);
		vec.clear();
		REQUIRE(wib::weight_in_bytes(vec) == 512);
		vec.shrink_to_fit();
		REQUIRE(wib::weight_in_bytes(vec) == 0);
	}

	// std::vector of arrays
	{
		using array_t = std::array<int, 45>;
		auto vec = std::vector<array_t>{};
		vec.resize(68);
		REQUIRE(
			wib::weight_in_bytes(vec) == 
			vec.capacity() * sizeof(array_t)
		);
	}

};





TEST_CASE("std::vector<bool>") {
	auto v = std::vector<bool>{};
	REQUIRE(wib::weight_in_bytes(v) == 0);
	v.resize(100, false);
	REQUIRE(wib::weight_in_bytes(v) == v.capacity() / 8);
};


TEST_CASE("std::string") {
	// std::string
	{
		auto str = std::string{};
		REQUIRE(wib::weight_in_bytes(str) == 0);
		str.resize(5);
		REQUIRE(wib::weight_in_bytes(str) == 0);
		str.resize(25);
		REQUIRE(wib::weight_in_bytes(str) >= 25);
	}
	// std::string, std::string_view
	{
		auto str = std::string(2048, ' ');
		REQUIRE(wib::weight_in_bytes(str) >= 2048);
		auto sv = std::string_view{ str };
		REQUIRE(wib::weight_in_bytes(sv) == 0);
	}
};


TEST_CASE("std::optional") {
	auto v = std::optional<bytevec_t>{};
	REQUIRE(wib::weight_in_bytes(v) == 0);
	v.emplace();
	REQUIRE(wib::weight_in_bytes(v) == 0);
	v->resize(3000);
	REQUIRE(wib::weight_in_bytes(v) >= 3000);
};







TEST_CASE("std::shared_ptr") {
	
	using value_t = std::array<byte_t, 128>;
	using sptr_t = std::shared_ptr<value_t>;
	auto sptr = std::make_shared<value_t>();
	REQUIRE(
		wib::weight_in_bytes(sptr) ==
		sizeof(value_t)
	);
	// All point to the same value
	auto vec = std::vector<sptr_t>{};
	vec.resize(100);
	std::fill(vec.begin(), vec.end(), sptr);
	REQUIRE(
		wib::weight_in_bytes(vec) ==
		sizeof(value_t) +
		vec.capacity() * sizeof(sptr_t)
	);

	// array of shared_ptrs
	auto arr = std::array<sptr_t, 100>{};
	arr.fill(sptr);
	REQUIRE(
		wib::weight_in_bytes(arr) ==
		sizeof(value_t)
	);
};


TEST_CASE("raw pointer") {
	auto uptr = std::make_unique<bytevec_t>();
	uptr->resize(128);
	auto arr = std::array<bytevec_t*, 12>{};
	arr.fill(uptr.get());
	REQUIRE(
		wib::weight_in_bytes(arr,wib::efollow_raw_pointers::True) ==
		128 + sizeof(bytevec_t)
	);
	REQUIRE(
		wib::weight_in_bytes(arr,wib::efollow_raw_pointers::False) ==
		0
	);
};





TEST_CASE("std::set") {
	auto s = std::set<bytevec_t>{};
	REQUIRE(wib::weight_in_bytes(s) == 0);
	s.insert(bytevec_t{});
	REQUIRE(wib::weight_in_bytes(s) == sizeof(bytevec_t));
	s.clear();
	s.insert(bytevec_t(256));
	REQUIRE(wib::weight_in_bytes(s) == sizeof(bytevec_t) + 256);
	s.insert(bytevec_t(128));
	REQUIRE(
		wib::weight_in_bytes(s) == 
		sizeof(bytevec_t) + 256 +
		sizeof(bytevec_t) + 128
	);
};






TEST_CASE("std::map") {
	
	{
		auto m = std::map<int, double>{};
		REQUIRE(wib::weight_in_bytes(m) == 0);
		for (int i = 0; i < 1000; ++i) {
			m[i] = double(i);
		}
		REQUIRE(
			wib::weight_in_bytes(m) ==
			1000 * sizeof(int) + 1000 * sizeof(double)
		);
	}
	{
		auto m = std::unordered_map<int, bytevec_t>{};
		REQUIRE(wib::weight_in_bytes(m) == 0);
		for (int i = 0; i < 1000; ++i) {
			m[i] = bytevec_t{};
			m[i].resize(500);
		}
		REQUIRE(
			wib::weight_in_bytes(m) ==
			1000 * sizeof(int) + 
			1000 * sizeof(bytevec_t) +
			1000 * 500
		);
	}

	{
		auto m = std::multimap<int, double>{};
		for (int i = 0; i < 1000; ++i) {
			m.insert({ 0, double(i) });
		}
		REQUIRE(
			wib::weight_in_bytes(m) ==
			1000 * sizeof(int) + 1000 * sizeof(double)
		);
	}
}




TEST_CASE("std::variant") {
	auto v = std::variant<int, std::string>{};
	v = int(0);
	REQUIRE(wib::weight_in_bytes(v) == 0);
	v = std::string(1024, ' ');
	REQUIRE(wib::weight_in_bytes(v) >= 1024);
	v = int(0);
	REQUIRE(wib::weight_in_bytes(v) == 0);
};



TEST_CASE("std::tuple") {
	{
		auto v = std::tuple<
			int, 
			std::vector<int>, 
			std::vector<double>
		>{};
		REQUIRE(wib::weight_in_bytes(v) == 0);
		std::get<2>(v).resize(1024);
		REQUIRE(wib::weight_in_bytes(v) == 1024 * sizeof(double));
		std::get<1>(v).resize(1024);
		REQUIRE(wib::weight_in_bytes(v) == 1024 * sizeof(double) + 1024 * sizeof(int));
	}
};



TEST_CASE("custom access") {

	{
		class str_t {
		public:
			str_t() {}
			auto as_tuple() const { return std::tie(a, b, c, d); }
			auto set_size(size_t sz) -> void { 
				b.resize(sz); 
				d.resize(sz);
			}
		private:
			int a{};
			bytevec_t b{};
			double c{};
			bytevec_t d{};
		};
		str_t s{};
		REQUIRE(wib::unknown_typeindexes(s).size() == 0);
		REQUIRE(wib::weight_in_bytes(s) == 0);
		s.set_size(1024);
		REQUIRE(wib::weight_in_bytes(s) == 1024*2);
	}

	{
		struct str_t {
			auto weight_in_bytes() const { return 54; }
			bytevec_t dummy_a{};
		};
		str_t s{};
		s.dummy_a.resize(2048);
		REQUIRE(wib::unknown_typeindexes(s).size() == 0);
		REQUIRE(wib::weight_in_bytes(s) == 54);
	}


}

TEST_CASE("unknown_types") {
	REQUIRE(wib::unknown_typeindexes(std::chrono::steady_clock::now()).size() == 1);
	REQUIRE(wib::unknown_typeindexes(std::mutex{}).size() == 1);
}

TEST_CASE("boost::pfr::for_each_field and cista::for_each_field") {
	struct empty_struct_t {};
	const auto empty_struct = empty_struct_t{};
	struct inner_t {
		bytevec_t v{};
		float f{};
		bool b{};

	};
	struct str_t {
		int x{};
		std::string y{};
		inner_t z{};
	};
	str_t s{};
	s.z.v.resize(2000);
#if defined(WIB_PFR_ENABLED) || defined(WIB_CISTA_ENABLED)
	REQUIRE(wib::unknown_typeindexes(s).size() == 0);
	REQUIRE(wib::weight_in_bytes(s) == 2000);
	REQUIRE(wib::unknown_typeindexes(empty_struct).size() == 0);
	REQUIRE(wib::weight_in_bytes(empty_struct) == 0);
#else
	REQUIRE(wib::unknown_typeindexes(s).size() == 1);
	REQUIRE(wib::weight_in_bytes(s) == 0);
	REQUIRE(wib::unknown_typeindexes(empty_struct).size() == 0);
	REQUIRE(wib::weight_in_bytes(empty_struct) == 0);
#endif
}








namespace {
struct cereal_serialize_enabled {
	template <typename Ar>
	auto serialize(Ar& ar) -> void {
		ar(v0, v1);
		ar(v2);
	}
	bytevec_t v0{};
	bytevec_t v1{};
	bytevec_t v2{};
};
struct cereal_serialize_disabled {
	auto serialize() -> void {}
};
struct cereal_save_enabled {
	template <typename Ar>
	auto save(Ar& ar) const -> void {
		ar(v0, v1);
		ar(v2);
	}
	bytevec_t v0{};
	bytevec_t v1{};
	bytevec_t v2{};
};
struct cereal_save_disabled {
	auto save() -> void {}
};
}
TEST_CASE("cereal-serialize") {
	{
		static_assert(wib::detail::type_traits::has_cereal_serialize_v<cereal_serialize_enabled>);
		static_assert(!wib::detail::type_traits::has_cereal_serialize_v<cereal_serialize_disabled>);
		static_assert(wib::detail::type_traits::has_cereal_save_v<cereal_save_enabled>);
		static_assert(!wib::detail::type_traits::has_cereal_save_v<cereal_save_disabled>);
	}
	{
		auto v = cereal_serialize_enabled{};
		v.v0.resize(1024);
		v.v1.resize(512);
		v.v2.resize(256);
		REQUIRE(
			wib::weight_in_bytes(v) ==
			1024 + 512 + 256
		);
	}
	{
		auto v = cereal_save_enabled{};
		v.v0.resize(1024);
		v.v1.resize(512);
		v.v2.resize(256);
		REQUIRE(
			wib::weight_in_bytes(v) ==
			1024 + 512 + 256
		);
	}
};

