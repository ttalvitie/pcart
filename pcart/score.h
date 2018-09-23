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
		if(dataCount == 0) {
			return 0.0;
		}

		double n = (double)dataCount;

		double mu = avg;
		double vari = stddev * stddev;

		double s = (n - 1.0) * vari;
		double mud = mu - var.barmu;
		double t = (n * var.a / (n + var.a)) * mud * mud;

		double score = 0.0;
		score -= 0.5 * n * log(pi);
		score += 0.5 * var.nu * log(var.lambda * var.nu);
		score += 0.5 * log(var.a);
		score -= 0.5 * log(n + var.a);
		score += lgamma(0.5 * (n + var.nu));
		score -= lgamma(0.5 * var.nu);
		score -= 0.5 * (n + var.nu) * log(s + t + var.nu * var.lambda);

		return score;
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
		if(dataCount == 0) {
			return 0.0;
		}

		double score = 0.0;
		
		double alpha_sum = 0.0;
		for(const CatInfo& info : var.cats) {
			score -= lgamma(info.alpha);
			alpha_sum += info.alpha;
		}
		score += lgamma(alpha_sum);

		double count_sum = 0.0;
		for(size_t i = 0; i < var.cats.size(); ++i) {
			score += lgamma((double)catCount[i] + var.cats[i].alpha);
		}
		score -= lgamma((double)dataCount + alpha_sum);

		return score;
	}
};

struct StructureScoreTerms {
	double leafPenaltyTerm;
	double normalizerTerm;
};

StructureScoreTerms computeStructureScoreTerms(const vector<VarPtr>& predictors);

}
