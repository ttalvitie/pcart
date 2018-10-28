#include <iostream>
#include <random>

#include <pcart/bits.h>
#include <pcart/tree.h>

using namespace pcart;
using namespace std;

typedef variant<uint64_t, pair<double, double>> VarRange;

pair<double, double> checkTreeRecursion(
	const TreePtr& tree,
	const vector<VarPtr>& pred,
	const VarPtr& resp,
	double leafPenaltyTerm,
	bool fullTable,
	const vector<const vector<double>*> data,
	vector<VarRange>& ranges,
	double cellSize
) {
	auto getPredIdx = [&](const VarPtr& var) -> size_t {
		for(size_t i = 0; i < pred.size(); ++i) {
			if(var == pred[i]) {
				return i;
			}
		}
		fail();
		return (size_t)-1;
	};
	return lambdaVisit(*tree,
		[&](const RealSplit& split) {
			size_t i = getPredIdx(split.var);
			pair<double, double>& range = get<pair<double, double>>(ranges[i]);
			if(fullTable) {
				if(split.splitVal < range.first || split.splitVal > range.second) fail();
			} else {
				if(abs(0.5 * (range.first + range.second) - split.splitVal) > 1e-6) fail();
			}

			vector<const vector<double>*> leftData;
			vector<const vector<double>*> rightData;
			for(const vector<double>* point : data) {
				if((*point)[split.var->dataSrcIdx] < split.splitVal) {
					leftData.push_back(point);
				} else {
					rightData.push_back(point);
				}
			}
			pair<double, double> origRange = range;

			range.second = split.splitVal;
			auto leftp = checkTreeRecursion(split.leftChild, pred, resp, leafPenaltyTerm, fullTable, leftData, ranges, 0.5 * cellSize);

			range = origRange;
			range.first = split.splitVal;
			auto rightp = checkTreeRecursion(split.rightChild, pred, resp, leafPenaltyTerm, fullTable, rightData, ranges, 0.5 * cellSize);

			range = origRange;

			return make_pair(leftp.first + rightp.first, leftp.second + rightp.second);
		},
		[&](const CatSplit& split) {
			size_t i = getPredIdx(split.var);
			uint64_t& range = get<uint64_t>(ranges[i]);
			if(split.leftCatMask & ~range) fail();
			if(split.rightCatMask & ~range) fail();
			if(split.leftCatMask & split.rightCatMask) fail();
			if((split.leftCatMask | split.rightCatMask) != range) {
				fail("Split category masks do not form a partition of the remaining categories");
			}

			vector<const vector<double>*> leftData;
			vector<const vector<double>*> rightData;
			for(const vector<double>* point : data) {
				size_t cat = (size_t)(*point)[split.var->dataSrcIdx];
				if(split.leftCatMask & bit64(cat)) {
					leftData.push_back(point);
				} else if(split.rightCatMask & bit64(cat)) {
					rightData.push_back(point);
				} else {
					fail();
				}
			}

			uint64_t origRange = range;

			range = split.leftCatMask;
			double leftCellSize = cellSize * (double)popcount64(range) / (double)popcount64(origRange);
			auto leftp = checkTreeRecursion(split.leftChild, pred, resp, leafPenaltyTerm, fullTable, leftData, ranges, leftCellSize);

			range = split.rightCatMask;
			double rightCellSize = cellSize * (double)popcount64(range) / (double)popcount64(origRange);
			auto rightp = checkTreeRecursion(split.rightChild, pred, resp, leafPenaltyTerm, fullTable, rightData, ranges, rightCellSize);

			range = origRange;

			return make_pair(leftp.first + rightp.first, leftp.second + rightp.second);
		},
		[&](const RealLeaf& leaf) {
			if((VarPtr)leaf.var != resp) fail();
			if(leaf.stats.dataCount != data.size()) fail();

			double avg = 0.0;
			for(const vector<double>* point : data) {
				avg += (*point)[leaf.var->dataSrcIdx];
			}
			avg /= (double)data.size();
			double stddev = 0.0;
			for(const vector<double>* point : data) {
				double d = (*point)[leaf.var->dataSrcIdx] - avg;
				stddev += d * d;
			}
			stddev = sqrt(stddev / (double)data.size());

			if(abs(avg - leaf.stats.avg) > 1e-5) fail();
			if(abs(stddev - leaf.stats.stddev) > 1e-5) fail();

			return make_pair(leaf.stats.dataScore(*leaf.var, cellSize), leafPenaltyTerm);
		},
		[&](const CatLeaf& leaf) {
			if((VarPtr)leaf.var != resp) fail();
			if(leaf.stats.dataCount != data.size()) fail();

			vector<size_t> counts(leaf.var->cats.size());
			for(const vector<double>* point : data) {
				++counts.at((size_t)(*point)[leaf.var->dataSrcIdx]);
			}

			if(counts != leaf.stats.catCount) fail();

			return make_pair(leaf.stats.dataScore(*leaf.var, cellSize), leafPenaltyTerm);
		}
	);
}

void checkTree(
	const TreeResult& treeResult,
	const vector<VarPtr>& pred,
	const VarPtr& resp,
	const vector<vector<double>>& data,
	bool fullTable = false
) {
	StructureScoreTerms sst;
	if(fullTable) {
		sst = StructureScoreTerms{ 0.0, 0.0 };
	} else {
		sst = computeStructureScoreTerms(pred);
	}

	const TreePtr& tree = treeResult.tree;
	vector<VarRange> ranges;
	for(const VarPtr& var : pred) {
		ranges.push_back(lambdaVisit(var,
			[&](const RealVarPtr& var) -> VarRange {
				return make_pair(var->minVal, var->maxVal);
			},
			[&](const CatVarPtr& var) -> VarRange {
				return ones64(var->cats.size());
			}
		));
	}

	vector<const vector<double>*> dataPtrs;
	for(const vector<double>& point : data) {
		dataPtrs.push_back(&point);
	}

	double dataScore, structureScore;
	tie(dataScore, structureScore) = checkTreeRecursion(tree, pred, resp, sst.leafPenaltyTerm, fullTable, dataPtrs, ranges, 1.0);
	structureScore += sst.normalizerTerm;
	bool checkDataScore;
	lambdaVisit(resp,
		[&](const RealVarPtr& var) {
			checkDataScore = true;
		},
		[&](const CatVarPtr& var) {
			checkDataScore = !var->bdeu;
		}
	);
	if(checkDataScore && abs(dataScore - treeResult.dataScore) > 1e-5) fail();
	if(abs(structureScore - treeResult.structureScore) > 1e-5) fail();
}

int main() {
	RealVarPtr A = createRealVar("A", 0, -127.0, 51.0, 2);
	CatVarPtr B = createBDeuCatVar("B", 1, { "x", "y", "z" });
	CatVarPtr C = createCatVar("C", 2, { "u", "v" });
	RealVarPtr D = createRealVar("D", 3, 1.0, 3.0, 1);
	CatVarPtr E = createCatVar("E", 4, { "a", "b" });

	vector<VarPtr> vars = { A, B, C, D, E };

	mt19937 rng(1234);
	auto randInt = [&](int a, int b) {
		return rng() % (b - a + 1);
	};
	auto randReal = [&](double a, double b) {
		double t = (double)rng() / (double)((uint64_t)1 << 32);
		return (1 - t) * a + t * b;
	};
	vector<vector<double>> data;
	for(int i = 0; i < 1000; ++i) {
		vector<double> point(5);
		point[1] = randInt(0, 2);
		if(point[1] == 1) {
			point[0] = randReal(-60.0, -10.0);
		} else {
			point[0] = randReal(-30.0, 10.0);
		}

		point[3] = randReal(1.5, 2.5);

		double x = randReal(-1.0, 1.0);
		point[0] += 30.0 * x;
		point[3] += 0.1 * x;

		point[2] = (double)((point[0] > 0.0) != (point[3] > 2.0));

		point[4] = 1.0 - point[2];
		if(point[3] > 2.5) {
			point[4] = 0.0;
		}

		data.push_back(move(point));
	}

	for(uint64_t predMask = 0; predMask < bit64(vars.size()); ++predMask) {
		if(predMask == 3) {
			// slow case
			continue;
		}
		for(size_t respIdx = 0; respIdx < vars.size(); ++respIdx) {
			if(predMask & bit64(respIdx)) {
				continue;
			}
			vector<VarPtr> pred;
			for(size_t i = 0; i < vars.size(); ++i) {
				if(predMask & bit64(i)) {
					pred.push_back(vars[i]);
				}
			}

			if(pred.size() > 2) {
				continue;
			}

			VarPtr resp = vars[respIdx];

			double totalStructureProb = 0.0;
			double bestScore = -numeric_limits<double>::infinity();
			iterateTrees(
				pred, resp, data,
				[&](const TreeResult& treeResult) {
					totalStructureProb += exp(treeResult.structureScore);
					checkTree(treeResult, pred, resp, data);
					bestScore = max(bestScore, treeResult.totalScore());
				}
			);
			if(abs(totalStructureProb - 1.0) > 1e-5) fail();
			TreeResult opt = optimizeTree(pred, resp, data);
			checkTree(opt, pred, resp, data);
			if(abs(bestScore - opt.totalScore()) > 1e-5) fail();

			TreeResult ft = optimizeFullTable(pred, resp, data, 7);
			checkTree(ft, pred, resp, data, true);
		}
	}

	{
		TreeResult opt = optimizeTree({ A, B, C, D }, E, data);
		checkTree(opt, { A, B, C, D }, E, data);
		if(abs(opt.totalScore() + 58.045) > 0.1) fail();
	}

	{
		TreeResult opt = optimizeTree({ C, D, E }, B, data);
		checkTree(opt, { C, D, E }, B, data);
		if(abs(opt.totalScore() + 1093.61) > 0.1) fail();
	}

	{
		TreeResult opt = optimizeTree({ B, C, D, E }, A, data);
		checkTree(opt, { B, C, D, E }, A, data);
		if(abs(opt.totalScore() + 4133.78) > 0.1) fail();
	}

	for(int c = 0; c < 3; ++c)  {
		// Missing categories, different types of response variables
		CatVarPtr A = createCatVar("A", 0, { "p", "q" });
		CatVarPtr B = createCatVar("B", 1, { "a", "b", "c" });
		VarPtr C;
		switch(c) {
			case 0: C = createCatVar("C", 2, { "x", "y" }); break;
			case 1: C = createBDeuCatVar("C", 2, { "x", "y" }); break;
			case 2: C = createRealVar("C", 2, 0.0, 1.0, 3); break;
		}

		vector<vector<double>> data2;
		for(int i = 0; i < 50; ++i) {
			data2.push_back({ 0, 0, 0 });
		}
		for(int i = 0; i < 50; ++i) {
			data2.push_back({ 0, 1, 1 });
		}
		for(int i = 0; i < 50; ++i) {
			data2.push_back({ 1, 0, 0 });
		}
		for(int i = 0; i < 50; ++i) {
			data2.push_back({ 1, 1, 0 });
		}
		for(int i = 0; i < 50; ++i) {
			data2.push_back({ 1, 2, 1 });
		}

		vector<VarPtr> pred = { A, B };
		VarPtr resp = C;

		double bestScore = -numeric_limits<double>::infinity();
		iterateTrees(
			pred, resp, data2,
			[&](const TreeResult& treeResult) {
				checkTree(treeResult, pred, resp, data2);
				bestScore = max(bestScore, treeResult.totalScore());
			}
		);

		TreeResult opt = optimizeTree(pred, resp, data2);
		checkTree(opt, pred, resp, data2);

		if(abs(opt.totalScore() - bestScore) > 0.0001) fail();
	}

	return 0;
}
