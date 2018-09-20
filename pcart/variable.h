#pragma once
#pragma once

#include <pcart/common.h>

namespace pcart {

struct BaseVar {
	std::string name;
	size_t dataIdx;
};

struct RealVar : public BaseVar {
	double minVal;
	double maxVal;
	size_t maxSubdiv;

	double nu;
	double lambda;
	double barmu;
	double a;
};

struct CatInfo {
	std::string name;
	double alpha;
};
struct CatVar : public BaseVar {
	std::vector<CatInfo> cats;
};

typedef std::variant<RealVar, CatVar> Var;

}
