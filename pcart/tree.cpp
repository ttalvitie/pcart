#include <pcart/tree.h>

#include <pcart/cell.h>
#include <pcart/score.h>

namespace pcart {

namespace {

template <typename R>
vector<pair<Cell, typename R::Val>> extractData(
	const CellCtx& cellCtx,
	const shared_ptr<R>& response,
	const vector<vector<double>>& dataSrc
) {
	vector<pair<Cell, typename R::Val>> data(dataSrc.size());
	for(size_t i = 0; i < dataSrc.size(); ++i) {
		data[i].first = cellCtx.pointCell(dataSrc[i]);
		data[i].second = response->parseDataSrcVal(dataSrc[i]);
	}
	return data;
}

bool useAdaptCell(const RealVar& response) {
	return true;
}
bool useAdaptCell(const CatVar& response) {
	return !response.bdeu;
}

template <typename R>
double optimizeTreeRecursion(
	const CellCtx& cellCtx,
	const shared_ptr<const R>& response,
	double leafPenaltyTerm,
	unordered_map<Cell, double>& mem,
	Cell cell,
	double cellSize,
	pair<Cell, typename R::Val>* data,
	size_t dataCount
) {
	auto it = mem.find(cell);
	if(it == mem.end()) {
		LeafStats<R> stats(*response, data, dataCount);
		double score = stats.dataScore(*response, cellSize) + leafPenaltyTerm;
		cellCtx.iterateDataSplits(
			cell, data, dataCount, false, useAdaptCell(*response),
			[&](
				const VarPtr& var,
				Cell left, Cell right,
				double leftCoef, double rightCoef,
				size_t splitPos,
				auto& lazySplit
			) {
				double splitScore = optimizeTreeRecursion(
					cellCtx, response, leafPenaltyTerm, mem,
					left, leftCoef * cellSize, data, splitPos
				);
				splitScore += optimizeTreeRecursion(
					cellCtx, response, leafPenaltyTerm, mem,
					right, rightCoef * cellSize, data + splitPos, dataCount - splitPos
				);
				score = max(score, splitScore);
			}
		);
		it = mem.emplace(cell, score).first;
	}
	return it->second;
}

template <typename R>
TreeResult extractOptimumTree(
	const CellCtx& cellCtx,
	const shared_ptr<const R>& response,
	double leafPenaltyTerm,
	const unordered_map<Cell, double>& mem,
	Cell cell,
	double cellSize,
	pair<Cell, typename R::Val>* data,
	size_t dataCount
) {
	TreeResult best;
	Leaf<R> leaf(response, data, dataCount);
	best.dataScore = leaf.stats.dataScore(*response, cellSize);
	best.structureScore = leafPenaltyTerm;
	best.tree = make_unique<Tree>(move(leaf));

	cellCtx.iterateDataSplits(
		cell, data, dataCount, false, useAdaptCell(*response),
		[&](
			const VarPtr& var,
			Cell left, Cell right,
			double leftCoef, double rightCoef,
			size_t splitPos,
			auto& lazySplit
		) {
			auto leftIt = mem.find(left);
			auto rightIt = mem.find(right);
			if(leftIt == mem.end() || rightIt == mem.end()) {
				fail("Internal error in extractOptimumTree");
			}
			double splitScore = leftIt->second + rightIt->second;
			if(splitScore > best.totalScore()) {
				TreeResult leftRes = extractOptimumTree(
					cellCtx, response, leafPenaltyTerm, mem,
					left, leftCoef * cellSize, data, splitPos
				);
				TreeResult rightRes = extractOptimumTree(
					cellCtx, response, leafPenaltyTerm, mem,
					right, rightCoef * cellSize, data + splitPos, dataCount - splitPos
				);
				best.dataScore = leftRes.dataScore + rightRes.dataScore;
				best.structureScore = leftRes.structureScore + rightRes.structureScore;
				best.tree = lazySplit(move(leftRes.tree), move(rightRes.tree));
				if(abs(best.totalScore() - splitScore) > 0.01) {
					fail("Internal error in extractOptimumTree");
				}
			}
		}
	);

	return best;
}

bool catsOrder(
	const pair<CatVarPtr, uint64_t>& a,
	const pair<CatVarPtr, uint64_t>& b
) {
	return a.first.get() < b.first.get();
}

void fixAdaptedTreeSplitsRecursion(
	vector<pair<CatVarPtr, uint64_t>>& cats,
	TreePtr& tree
) {
	lambdaVisit(*tree,
		[&](CatSplit& split) {
			auto it = lower_bound(
				cats.begin(), cats.end(),
				make_pair(split.var, 0),
				catsOrder
			);
			if(
				it == cats.end() ||
				it->first != split.var ||
				(split.leftCatMask & ~it->second) ||
				(split.rightCatMask & ~it->second)
			) {
				fail("Internal error in optimizeTree");
			}

			split.rightCatMask = it->second ^ split.leftCatMask;

			uint64_t orig = it->second;
			it->second = split.leftCatMask;
			fixAdaptedTreeSplitsRecursion(cats, split.leftChild);
			it->second = split.rightCatMask;
			fixAdaptedTreeSplitsRecursion(cats, split.rightChild);
			it->second = orig;
		},
		[&](RealSplit& split) {
			fixAdaptedTreeSplitsRecursion(cats, split.leftChild);
			fixAdaptedTreeSplitsRecursion(cats, split.rightChild);
		},
		[&](BaseLeaf& leaf) {}
	);
}

void fixAdaptedTreeSplits(const vector<VarPtr>& predictors, TreePtr& tree) {
	vector<pair<CatVarPtr, uint64_t>> cats;
	for(const VarPtr& var : predictors) {
		lambdaVisit(var,
			[&](const CatVarPtr& var) {
				cats.emplace_back(var, ones64(var->cats.size()));
			},
			[&](const RealVarPtr& var) {}
		);
	}
	sort(cats.begin(), cats.end(), catsOrder);
	fixAdaptedTreeSplitsRecursion(cats, tree);
}

template <typename R>
TreeResult optimizeTree(
	const vector<VarPtr>& predictors,
	const shared_ptr<const R>& response,
	const vector<vector<double>>& dataSrc
) {
	StructureScoreTerms sst = computeStructureScoreTerms(predictors);

	CellCtx cellCtx(predictors);
	typedef typename R::Val Val;
	vector<pair<Cell, Val>> data = extractData(cellCtx, response, dataSrc);

	unordered_map<Cell, double> mem;
	double score = optimizeTreeRecursion(
		cellCtx, response, sst.leafPenaltyTerm, mem,
		cellCtx.root(), 1.0, data.data(), data.size()
	);

	TreeResult ret = extractOptimumTree(
		cellCtx, response, sst.leafPenaltyTerm, mem,
		cellCtx.root(), 1.0, data.data(), data.size()
	);
	if(abs(ret.totalScore() - score) > 0.01) {
		fail("Internal error in optimizeTree");
	}

	ret.structureScore += sst.normalizerTerm;

	if(useAdaptCell(*response)) {
		fixAdaptedTreeSplits(predictors, ret.tree);
	}

	return ret;
}

template <typename R>
void iterateTreesRecursion(
	const CellCtx& cellCtx,
	const shared_ptr<const R>& response,
	Cell cell,
	double cellSize,
	pair<Cell, typename R::Val>* data,
	size_t dataCount,
	double leafPenaltyTerm,
	function<TreeResult(TreeResult)> f
) {
	cellCtx.iterateDataSplits(
		cell, data, dataCount, true, false,
		[&](
			const VarPtr& var,
			Cell left, Cell right,
			double leftCoef, double rightCoef,
			size_t splitPos,
			auto& lazySplit
		) {
			TreeResult res;
			res.tree = lazySplit(nullptr, nullptr);
			BaseSplit& split = *lambdaVisit(*res.tree,
				[&](BaseSplit& split) {
					return &split;
				},
				[&](BaseLeaf&) {
					fail("Internal error");
					return (BaseSplit*)nullptr;
				}
			);

			iterateTreesRecursion<R>(
				cellCtx, response, left, leftCoef * cellSize, data, splitPos, leafPenaltyTerm,
				[&](TreeResult leftRes) {
					iterateTreesRecursion<R>(
						cellCtx, response, right, rightCoef * cellSize, data + splitPos, dataCount - splitPos, leafPenaltyTerm,
						[&](TreeResult rightRes) {
							res.dataScore = leftRes.dataScore + rightRes.dataScore;
							res.structureScore = leftRes.structureScore + rightRes.structureScore;
							split.leftChild = move(leftRes.tree);
							split.rightChild = move(rightRes.tree);
			
							res = f(move(res));
			
							leftRes.tree = move(split.leftChild);
							rightRes.tree = move(split.rightChild);
			
							return rightRes;
						}
					);
					return leftRes;
				}
			);
		}
	);

	Leaf<R> leaf(response, data, dataCount);

	TreeResult leafRes;
	leafRes.dataScore = leaf.stats.dataScore(*response, cellSize);
	leafRes.structureScore = leafPenaltyTerm;
	leafRes.tree = make_unique<Tree>(move(leaf));

	f(move(leafRes));
}

template <typename R>
void iterateTrees(
	const vector<VarPtr>& predictors,
	const shared_ptr<const R>& response,
	const vector<vector<double>>& dataSrc,
	function<void(const TreeResult&)> f
) {
	StructureScoreTerms sst = computeStructureScoreTerms(predictors);

	CellCtx cellCtx(predictors);
	typedef typename R::Val Val;
	vector<pair<Cell, Val>> data = extractData(cellCtx, response, dataSrc);

	iterateTreesRecursion<R>(
		cellCtx, response, cellCtx.root(), 1.0, data.data(), data.size(), sst.leafPenaltyTerm,
		[&](TreeResult src) {
			TreeResult res;
			res.dataScore = src.dataScore;
			res.structureScore = sst.normalizerTerm + src.structureScore;
			res.tree = move(src.tree);
			f((const TreeResult&)res);
			src.tree = move(res.tree);
			return src;
		}
	);
}

void printTreeRecursion(const TreePtr& tree, ostream& out, string pre1, string pre) {
	out << pre1;
	lambdaVisit(*tree,
		[&](const RealLeaf& leaf) {
			out << "-- " << leaf.var->name << ":";
			out << " " << leaf.stats.avg;
			out << " +- " << leaf.stats.stddev;
			out << " x" << leaf.stats.dataCount;
			out << "\n";
		},
		[&](const CatLeaf& leaf) {
			out << "-- " << leaf.var->name << ": ";
			for(size_t i = 0; i < leaf.var->cats.size(); ++i) {
				if(i) {
					out << ", ";
				}
				out << leaf.var->cats[i].name << " x" << leaf.stats.catCount[i];
			}
			out << "\n";
		},
		[&](const RealSplit& split) {
			out << "-+ " << split.var->name << " (< or >=) " << split.splitVal << "\n";
		},
		[&](const CatSplit& split) {
			auto writeMask = [&](uint64_t mask) {
				bool first = true;
				for(size_t i = 0; i < split.var->cats.size(); ++i) {
					if(mask & bit64(i)) {
						if(!first) {
							out << ", ";
						}
						first = false;
						out << split.var->cats[i].name;
					}
				}
			};
			out << "-+ " << split.var->name << " in {";
			writeMask(split.leftCatMask);
			out << "} or {";
			writeMask(split.rightCatMask);
			out << "}\n";
		}
	);
	lambdaVisit(*tree,
		[&](const BaseLeaf& leaf) {},
		[&](const BaseSplit& split) {
			printTreeRecursion(split.leftChild, out, pre + " |", pre + " |");
			printTreeRecursion(split.rightChild, out, pre + " `", pre + "  ");
		}
	);
}

}

TreeResult optimizeTree(
	const vector<VarPtr>& predictors,
	const VarPtr & response,
	const vector<vector<double>>& data
) {
	return lambdaVisit(response, [&](const auto& response) {
		return optimizeTree(predictors, response, data);
	});
}

void iterateTrees(
	const vector<VarPtr>& predictors,
	const VarPtr& response,
	const vector<vector<double>>& dataSrc,
	function<void(const TreeResult&)> f
) {
	lambdaVisit(response, [&](const auto& response) {
		iterateTrees(predictors, response, dataSrc, f);
	});
}

void printTree(const TreePtr& tree, ostream& out) {
	printTreeRecursion(tree, cout, "", "");
}

}
