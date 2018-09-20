#include <pcart/variable.h>

namespace pcart {

RealVarPtr createRealVar(string name, size_t dataIdx, double minVal, double maxVal, size_t maxSubdiv) {
	RealVar var;

	var.name = move(name);
	var.dataSrcIdx = dataIdx;
	var.minVal = minVal;
	var.maxVal = maxVal;
	var.maxSubdiv = maxSubdiv;

	var.nu = 0.0;
	var.lambda = 0.0;
	var.barmu = 0.0;
	var.a = 0.0;

	return make_shared<RealVar>(move(var));
}

CatVarPtr createCatVar(string name, size_t dataIdx, vector<string> catNames) {
	CatVar var;

	var.name = move(name);
	var.dataSrcIdx = dataIdx;

	var.cats.resize(catNames.size());
	for(size_t i = 0; i < catNames.size(); ++i) {
		var.cats[i].name = catNames[i];
		var.cats[i].alpha = 0.5;
	}

	return make_shared<CatVar>(move(var));
}

}
