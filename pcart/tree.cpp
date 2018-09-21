#include <pcart/tree.h>

#include <pcart/cell.h>

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
	Cell cell,
	pair<Cell, typename R::Val>* data,
	size_t dataCount,
	function<void(TreeResult)> f
) {
	cellCtx.iterateDataSplits(
		cell, data, dataCount, true,
		[&](const VarPtr& var, Cell left, Cell right, size_t splitPos) {
			iterateTreesRecursion<R>(
				cellCtx, left, data, splitPos,
				[&](TreeResult leftRes) {
					iterateTreesRecursion<R>(
						cellCtx, right, data + splitPos, dataCount - splitPos,
						[&](TreeResult rightRes) {
							TreeResult res;
							res.dataScore = leftRes.dataScore + rightRes.dataScore;
							res.structureScore = leftRes.structureScore + rightRes.structureScore;
							res.totalScore = res.dataScore + res.structureScore;
							res.tree = make_unique<Tree>(RealSplit{ // TODO
								move(leftRes.tree), move(rightRes.tree),
								nullptr, 0.0 // TODO
							});
							f(move(res));
						}
					);
				}
			);
		}
	);

	TreeResult leafRes;
	leafRes.dataScore = 0.0; // TODO
	leafRes.structureScore = 0.0; // TODO
	leafRes.totalScore = 0.0; // TODO
	leafRes.tree = make_unique<Tree>(RealLeaf{nullptr, LeafStats<RealVar>()}); // TODO
	f(move(leafRes));
}

template <typename R>
void iterateTrees(
	const vector<VarPtr>& predictors,
	const shared_ptr<R>& response,
	const vector<vector<double>>& dataSrc,
	function<void(TreeResult)> f
) {
	CellCtx cellCtx(predictors);
	typedef typename R::Val Val;
	vector<pair<Cell, Val>> data = extractData(cellCtx, response, dataSrc);
	iterateTreesRecursion<R>(cellCtx, cellCtx.root(), data.data(), data.size(), f);
}

}

void iterateTrees(
	const vector<VarPtr>& predictors,
	const VarPtr& response,
	const vector<vector<double>>& dataSrc,
	function<void(TreeResult)> f
) {
	lambdaVisit(response, [&](const auto& response) {
		iterateTrees(predictors, response, dataSrc, f);
	});
}

}
