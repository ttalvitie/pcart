#pragma once

#include "variable.h"

namespace pcart {

template <typename T>
struct LeafStats;

template <>
struct LeafStats<RealVar> {
	double avg;
	double stddev;
	size_t dataCount;

	double score(const RealVar& var) {
		return 0.0;
	}
};
template <>
struct LeafStats<CatVar> {
	std::vector<size_t> catCount;
	size_t dataCount;

	double score(const CatVar& var) {
		return 0.0;
	}
};

}
