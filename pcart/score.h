#pragma once

#include <pcart/variable.h>

namespace pcart {

template <typename T>
struct LeafStats;

template <>
struct LeafStats<RealVar> {
	double avg;
	double stddev;
	size_t dataCount;

	template <typename T>
	LeafStats(const RealVar& var, const pair<T, double>* data, size_t dataCount)
		: avg(0.0),
		  stddev(0.0),
		  dataCount(dataCount)
	{
		for(size_t i = 0; i < dataCount; ++i) {
			avg += data[i].second;
		}
		avg /= (double)dataCount;
		for(size_t i = 0; i < dataCount; ++i) {
			double d = data[i].second - avg;
			stddev += d * d;
		}
		stddev = sqrt(stddev / (double)dataCount);
	}

	double score(const RealVar& var) {
		return 0.0;
	}
};
template <>
struct LeafStats<CatVar> {
	std::vector<size_t> catCount;
	size_t dataCount;

	template <typename T>
	LeafStats(const CatVar& var, const pair<T, size_t>* data, size_t dataCount)
		: catCount(var.cats.size(), 0),
		  dataCount(dataCount)
	{
		for(size_t i = 0; i < dataCount; ++i) {
			++catCount[data[i].second];
		}
	}

	double score(const CatVar& var) {
		return 0.0;
	}
};

}
