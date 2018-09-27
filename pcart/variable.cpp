#include <pcart/variable.h>

namespace pcart {

RealVarPtr createRealVar(
	string name,
	size_t dataSrcIdx,
	double minVal, double maxVal,
	size_t maxSubdiv,
	double nu,
	double lambda,
	double barmu,
	double a
) {
	RealVar var;

	var.name = move(name);
	var.dataSrcIdx = dataSrcIdx;
	var.minVal = minVal;
	var.maxVal = maxVal;
	var.maxSubdiv = maxSubdiv;

	var.nu = nu;
	var.lambda = lambda;
	var.barmu = barmu;
	var.a = a;

	return make_shared<RealVar>(move(var));
}

CatVarPtr createCatVar(
	string name,
	size_t dataSrcIdx,
	const vector<string>& catNames,
	double alpha
) {
	CatVar var;

	var.name = move(name);
	var.dataSrcIdx = dataSrcIdx;

	var.cats.resize(catNames.size());
	for(size_t i = 0; i < catNames.size(); ++i) {
		var.cats[i].name = catNames[i];
		var.cats[i].alpha = alpha;
	}

	var.bdeu = false;

	return make_shared<CatVar>(move(var));
}
CatVarPtr createBDeuCatVar(
	string name,
	size_t dataSrcIdx,
	const vector<string>& catNames,
	double ess
) {
	CatVar var;

	var.name = move(name);
	var.dataSrcIdx = dataSrcIdx;

	double alpha = ess / (double)catNames.size();

	var.cats.resize(catNames.size());
	for(size_t i = 0; i < catNames.size(); ++i) {
		var.cats[i].name = catNames[i];
		var.cats[i].alpha = alpha;
	}

	var.bdeu = true;

	return make_shared<CatVar>(move(var));
}

}
