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

	template <typename F>
	LeafStats(const RealVar& var, size_t dataCount, F getVal)
		: avg(0.0),
		  stddev(0.0),
		  dataCount(dataCount)
	{
		for(size_t i = 0; i < dataCount; ++i) {
			avg += getVal(i);
		}
		avg /= (double)dataCount;
		for(size_t i = 0; i < dataCount; ++i) {
			double d = getVal(i) - avg;
			stddev += d * d;
		}
		stddev = sqrt(stddev / (double)dataCount);
	}

	double dataScore(const RealVar& var, double cellSize) const {
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

	template <typename F>
	LeafStats(const CatVar& var, size_t dataCount, F getVal)
		: catCount(var.cats.size(), 0),
		  dataCount(dataCount)
	{
		for(size_t i = 0; i < dataCount; ++i) {
			++catCount[getVal(i)];
		}
	}

	double dataScore(const CatVar& var, double cellSize) const {
		if(dataCount == 0) {
			return 0.0;
		}

		double coef = var.bdeu ? cellSize : 1.0;

		double score = 0.0;
		
		double alpha_sum = 0.0;
		for(const CatInfo& info : var.cats) {
			score -= lgamma(info.alpha * coef);
			alpha_sum += info.alpha * coef;
		}
		score += lgamma(alpha_sum);

		for(size_t i = 0; i < var.cats.size(); ++i) {
			score += lgamma((double)catCount[i] + var.cats[i].alpha * coef);
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
