#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace pcart {

using std::abs;
using std::ceil;
using std::cerr;
using std::cout;
using std::floor;
using std::function;
using std::get;
using std::make_pair;
using std::make_shared;
using std::make_tuple;
using std::make_unique;
using std::max;
using std::min;
using std::move;
using std::ostream;
using std::pair;
using std::round;
using std::shared_ptr;
using std::size_t;
using std::sqrt;
using std::string;
using std::tie;
using std::tuple;
using std::uint64_t;
using std::unique_ptr;
using std::variant;
using std::vector;

inline void stderrPrint() {
	std::cerr << "\n";
}
template <typename F, typename... T>
void stderrPrint(const F& f, const T&... p) {
	std::cerr << f;
	stderrPrint(p...);
}
template <typename... T>
void fail(const T&... p) {
	stderrPrint("FAIL: ", p...);
	abort();
}

template <typename T>
T fromString(const std::string& str) {
	T ret;
	std::stringstream ss(str);
	ss >> ret;
	if (ss.fail()) fail("fromString: Could not convert string '", str, "' to given type.");
	return ret;
}

template<typename... T> struct LambdaVisitor : T... { using T::operator()...; };
template<typename... T> LambdaVisitor(T...)->LambdaVisitor<T...>;

template <typename Variant, typename... Visitor>
constexpr auto lambdaVisit(Variant&& var, Visitor&&... vis) {
	return visit(LambdaVisitor{ vis... }, var);
}

}
