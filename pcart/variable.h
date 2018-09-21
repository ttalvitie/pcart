#pragma once
#pragma once

#include <pcart/common.h>

namespace pcart {

struct BaseVar {
	string name;
	size_t dataSrcIdx;
};

struct RealVar : public BaseVar {
	typedef double Val;

	double minVal;
	double maxVal;
	size_t maxSubdiv;

	double nu;
	double lambda;
	double barmu;
	double a;

	double parseDataSrcVal(const vector<double>& src) const {
		if(dataSrcIdx >= src.size()) {
			fail("Too short vector in data source");
		}
		double val = src[dataSrcIdx];
		if(val < minVal || val > maxVal) {
			fail("Value ", val, " in data source is outside the allowed range [", minVal, ", ", maxVal, "] for variable ", name);
		}
		return val;
	}
};

struct CatInfo {
	string name;
	double alpha;
};
struct CatVar : public BaseVar {
	typedef size_t Val;

	vector<CatInfo> cats;

	size_t parseDataSrcVal(const vector<double>& src) const {
		if(dataSrcIdx >= src.size()) {
			fail("Too short vector in data source");
		}
		double srcVal = src[dataSrcIdx];
		size_t val = (size_t)srcVal;
		if(srcVal != (double)val || val >= cats.size()) {
			fail("Invalid value ", srcVal, " for variable ", name, " in data source");
		}
		return val;
	}
};

typedef shared_ptr<const RealVar> RealVarPtr;
typedef shared_ptr<const CatVar> CatVarPtr;
typedef variant<RealVarPtr, CatVarPtr> VarPtr;

RealVarPtr createRealVar(string name, size_t dataIdx, double minVal, double maxVal, size_t maxSubdiv);
CatVarPtr createCatVar(string name, size_t dataIdx, vector<string> catNames);

}
