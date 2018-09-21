#pragma once

#include <pcart/common.h>

#if defined(_MSC_VER) // Only for MSVC
#   include <intrin.h>
#   pragma intrinsic(_BitScanReverse64) // topOne64
#	pragma warning (disable : 4146) // bottomOne64
#endif

namespace pcart {

inline constexpr uint64_t ones64(size_t count) {
	return (count == 64)
		? (uint64_t)-1
		: (((uint64_t)1 << count) - (uint64_t)1);
}

inline constexpr uint64_t bit64(size_t pos) {
	return (uint64_t)1 << pos;
}

inline int clz64(uint64_t x) {
#if defined(_MSC_VER)
	unsigned long idx;
	_BitScanReverse64(&idx, x);
	return 64 - idx;
#else
	static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "The case unsigned long long != uint64_t is not implemented");
	return __builtin_clzll(x);
#endif
}

inline constexpr uint64_t bottomOne64(uint64_t x) {
	return x & -x;
}

}
