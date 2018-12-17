#include <random>
#include <pcart/tree.h>
#include <pcart/bits.h>

using namespace pcart;

double readDouble() {
	double x;
	cin >> x;
	if(!isfinite(x)) {
		fail("Input contains infinite value ", x);
	}
	return x;
}

void writeTree(const TreePtr& tree) {
	lambdaVisit(*tree,
		[&](const RealSplit& split) {
			cout << "SPLIT REAL " << split.var->name << " " << split.splitVal << "\n";
			writeTree(split.leftChild);
			writeTree(split.rightChild);
		},
		[&](const CatSplit& split) {
			cout << "SPLIT CAT " << split.var->name;
			for(size_t i = 0; i < split.var->cats.size(); ++i) {
				if(split.leftCatMask & bit64(i)) {
					cout << " " << i;
				}
			}
			cout << " -1";
			for(size_t i = 0; i < split.var->cats.size(); ++i) {
				if(split.rightCatMask & bit64(i)) {
					cout << " " << i;
				}
			}
			cout << " -1\n";
			writeTree(split.leftChild);
			writeTree(split.rightChild);
		},
		[&](const RealLeaf& leaf) {
			cout << "LEAF REAL " << leaf.stats.avg << " " << leaf.stats.stddev << "\n";
		},
		[&](const CatLeaf& leaf) {
			cout << "LEAF CAT";
			for(size_t x : leaf.stats.catCount) {
				cout << " " << x;
			}
			cout << "\n";
		}
	);
}

int main() {
	cin.exceptions(cin.eofbit | cin.failbit | cin.badbit);

	bool useStructureScore = true;
	while(true) {
		while(isspace(cin.peek())) {
			cin.get();
		}
		if(cin.peek() != '%') {
			break;
		}
		string flag;
		cin >> flag;
		if(flag == "%NO_USE_STRUCTURE_SCORE") {
			useStructureScore = false;
		} else {
			fail("Unknown flag");
		}
	}

	size_t predictorCount, dataCount;
	cin >> predictorCount >> dataCount;

	vector<VarPtr> predictors;
	for(size_t i = 0; i < predictorCount; ++i) {
		stringstream varName;
		varName << i;
		string type;
		cin >> type;
		if(type == "REAL") {
			double minVal = readDouble();
			double maxVal = readDouble();
			size_t maxSubdiv;
			cin >> maxSubdiv;
			predictors.push_back(createRealVar(varName.str(), i, minVal, maxVal, maxSubdiv));
		} else if(type == "CAT") {
			size_t catCount;
			cin >> catCount;
			predictors.push_back(createCatVar(varName.str(), i, vector<string>(catCount)));
		} else {
			fail("Unknown predictor variable type ", type);
		}
	}

	VarPtr response;
	string type;
	cin >> type;
	if(type == "REAL") {
		double minVal = readDouble();
		double maxVal = readDouble();
		size_t maxSubdiv;
		cin >> maxSubdiv;
		double nu = readDouble();
		double lambda = readDouble();
		double barmu = readDouble();
		double a = readDouble();
		response = createRealVar("response", predictorCount, minVal, maxVal, maxSubdiv, nu, lambda, barmu, a);
	} else if(type == "CAT") {
		size_t catCount;
		cin >> catCount;
		double alpha = readDouble();
		response = createCatVar("response", predictorCount, vector<string>(catCount), alpha);
	} else if(type == "BDEU_CAT") {
		size_t catCount;
		cin >> catCount;
		double ess = readDouble();
		response = createBDeuCatVar("response", predictorCount, vector<string>(catCount), ess);
	} else {
		fail("Unknown response variable type ", type);
	}

	vector<vector<double>> data(dataCount);
	for(size_t i = 0; i < dataCount; ++i) {
		data[i].resize(predictorCount + 1);
		for(double& x : data[i]) {
			x = readDouble();
		}
	}

	TreeResult res = optimizeTree(predictors, response, data, useStructureScore);

	cout << "OK\n";
	cout.precision(16);
	cout << res.totalScore() << "\n";

	writeTree(res.tree);

	return 0;
}
