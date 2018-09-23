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

template <typename R>
void iterateTreesRecursion(
	const CellCtx& cellCtx,
	const shared_ptr<const R>& response,
	Cell cell,
	pair<Cell, typename R::Val>* data,
	size_t dataCount,
	double leafPenaltyTerm,
	function<TreeResult(TreeResult)> f
) {
	cellCtx.iterateDataSplits(
		cell, data, dataCount, true,
		[&](const VarPtr& var, Cell left, Cell right, size_t splitPos, auto& lazySplit) {
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
				cellCtx, response, left, data, splitPos, leafPenaltyTerm,
				[&](TreeResult leftRes) {
					iterateTreesRecursion<R>(
						cellCtx, response, right, data + splitPos, dataCount - splitPos, leafPenaltyTerm,
						[&](TreeResult rightRes) {
							res.dataScore = leftRes.dataScore + rightRes.dataScore;
							res.structureScore = leftRes.structureScore + rightRes.structureScore;
							res.totalScore = res.dataScore + res.structureScore;
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

	TreeResult leafRes;
	LeafStats<R> stats(*response, data, dataCount);
	leafRes.dataScore = stats.score(*response);
	leafRes.structureScore = leafPenaltyTerm;
	leafRes.totalScore = leafRes.dataScore + leafRes.structureScore;
	leafRes.tree = make_unique<Tree>(Leaf<R>{ {}, response, move(stats)});
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
		cellCtx, response, cellCtx.root(), data.data(), data.size(), sst.leafPenaltyTerm,
		[&](TreeResult src) {
			TreeResult res;
			res.dataScore = src.dataScore;
			res.structureScore = sst.normalizerTerm + src.structureScore;
			res.totalScore = res.dataScore + res.structureScore;
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
					if(mask &bit64(i)) {
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

