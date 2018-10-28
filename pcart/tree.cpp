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
		LeafStats<R> stats(*response, dataCount, [&](size_t i) { return data[i].second; });
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
	Leaf<R> leaf(response, dataCount, [&](size_t i) { return data[i].second; });
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

vector<double> computeDisc(const vector<double>& vals, size_t count) {
	vector<double> ret;
	for(size_t i = 1; i < count; ++i) {
		double p = (double)vals.size() * (double)i / (double)count - 0.5;
		double pp = max(p, 0.0);
		size_t a = min((size_t)floor(pp), vals.size() - 1);
		size_t b = min((size_t)ceil(pp), vals.size() - 1);
		if(a == b) {
			ret.push_back(vals[a]);
		} else {
			double t = p - (double)a;
			ret.push_back((1.0 - t) * vals[a] + t * vals[b]);
		}
	}
	return ret;
}

template <typename F>
void cycleDiscsRecursion(
	vector<pair<RealVarPtr, vector<vector<double>>>>& opt,
	vector<pair<RealVarPtr, vector<double>>>& res,
	size_t i,
	F f
) {
	if(i == opt.size()) {
		f((const vector<pair<RealVarPtr, vector<double>>>&)res);
		return;
	}
	for(vector<double>& disc : opt[i].second) {
		swap(res[i].second, disc);
		cycleDiscsRecursion(opt, res, i + 1, f);
		swap(res[i].second, disc);
	}
}

template <typename F>
void cycleDiscs(vector<pair<RealVarPtr, vector<vector<double>>>>& opt, F f) {
	vector<pair<RealVarPtr, vector<double>>> res(opt.size());
	for(size_t i = 0; i < opt.size(); ++i) {
		res[i].first = opt[i].first;
	}
	cycleDiscsRecursion(opt, res, 0, f);
}

template <typename R>
double computeFullTableScore(
	const vector<pair<RealVarPtr, vector<double>>>& realPred,
	const vector<CatVarPtr>& catPred,
	const shared_ptr<const R>& response,
	const vector<vector<double>>& data
) {
	vector<vector<size_t>> parts;
	parts.emplace_back();
	parts.back().resize(data.size());
	for(size_t i = 0; i < data.size(); ++i) {
		parts.back()[i] = i;
	}

	double cellSize = 1.0;
	for(const pair<RealVarPtr, vector<double>>& p : realPred) {
		const RealVar& var = *p.first;
		const vector<double>& splits = p.second;

		cellSize /= (double)splits.size() + 1.0;

		vector<vector<size_t>> newParts;
		for(const vector<size_t>& part : parts) {
			vector<vector<size_t>> subs(splits.size() + 1);
			for(size_t pi : part) {
				double val = var.parseDataSrcVal(data[pi]);
				size_t si = upper_bound(splits.begin(), splits.end(), val) - splits.begin();
				subs[si].push_back(pi);
			}
			for(vector<size_t>& sub : subs) {
				if(!sub.empty()) {
					newParts.push_back(move(sub));
				}
			}
		}
		parts = move(newParts);
	}
	for(const CatVarPtr& var : catPred) {
		cellSize /= (double)var->cats.size();

		vector<vector<size_t>> newParts;
		for(const vector<size_t>& part : parts) {
			vector<vector<size_t>> subs(var->cats.size());
			for(size_t pi : part) {
				size_t val = var->parseDataSrcVal(data[pi]);
				subs[val].push_back(pi);
			}
			for(vector<size_t>& sub : subs) {
				if(!sub.empty()) {
					newParts.push_back(move(sub));
				}
			}
		}
		parts = move(newParts);
	}

	double score = 0.0;
	for(const vector<size_t>& part : parts) {
		score += LeafStats<R>(*response, part.size(), [&](size_t i) {
			return response->parseDataSrcVal(data[part[i]]);
		}).dataScore(*response, cellSize);
	}
	return score;
}

template <typename R>
TreePtr createFullTableTreeRecursion2(
	const vector<CatVarPtr>& catPred,
	size_t catPredIdx,
	const shared_ptr<const R>& response,
	const vector<const vector<double>*>& data
) {
	if(catPredIdx == catPred.size()) {
		return make_unique<Tree>(Leaf<R>(response, data.size(), [&](size_t i) {
			return response->parseDataSrcVal(*data[i]);
		}));
	}

	const CatVarPtr& var = catPred[catPredIdx];

	vector<vector<const vector<double>*>> subs(var->cats.size());
	for(const vector<double>* point : data) {
		size_t val = var->parseDataSrcVal(*point);
		subs[val].push_back(point);
	}

	TreePtr ret;
	for(size_t si = 0; si < subs.size(); ++si) {
		TreePtr tree = createFullTableTreeRecursion2(catPred, catPredIdx + 1, response, subs[si]);
		if(si) {
			ret = make_unique<Tree>(CatSplit{
				{move(ret), move(tree)},
				var,
				ones64(si),
				bit64(si)
			});
		} else {
			ret = move(tree);
		}
	}

	return ret;
}

template <typename R>
TreePtr createFullTableTreeRecursion1(
	const vector<pair<RealVarPtr, vector<double>>>& realPred,
	size_t realPredIdx,
	const vector<CatVarPtr>& catPred,
	const shared_ptr<const R>& response,
	const vector<const vector<double>*>& data
) {
	if(realPredIdx == realPred.size()) {
		return createFullTableTreeRecursion2(catPred, 0, response, data);
	}

	const RealVarPtr& var = realPred[realPredIdx].first;
	const vector<double>& splits = realPred[realPredIdx].second;

	vector<vector<const vector<double>*>> subs(splits.size() + 1);
	for(const vector<double>* point : data) {
		double val = var->parseDataSrcVal(*point);
		size_t si = upper_bound(splits.begin(), splits.end(), val) - splits.begin();
		subs[si].push_back(point);
	}
	
	TreePtr ret;
	for(size_t si = 0; si < subs.size(); ++si) {
		TreePtr tree = createFullTableTreeRecursion1(realPred, realPredIdx + 1, catPred, response, subs[si]);
		if(si) {
			ret = make_unique<Tree>(RealSplit{
				{move(ret), move(tree)},
				var,
				splits[si - 1]
			});
		} else {
			ret = move(tree);
		}
	}

	return ret;
}

template <typename R>
TreePtr createFullTableTree(
	const vector<pair<RealVarPtr, vector<double>>>& realPred,
	const vector<CatVarPtr>& catPred,
	const shared_ptr<const R>& response,
	const vector<vector<double>>& data
) {
	vector<const vector<double>*> dataPtrs(data.size());
	for(size_t i = 0; i < data.size(); ++i) {
		dataPtrs[i] = &data[i];
	}

	return createFullTableTreeRecursion1(realPred, 0, catPred, response, dataPtrs);
}

template <typename R>
TreeResult optimizeFullTable(
	const vector<VarPtr>& predictors,
	const shared_ptr<const R>& response,
	const vector<vector<double>>& data,
	size_t maxDiscretizationBins
) {
	if(data.empty()) {
		fail("optimizeFullTable called with empty data");
	}
	if(maxDiscretizationBins < 2) {
		fail("optimizeFullTable: maxDiscretizationBins should be at least 2");
	}
	size_t discCount = maxDiscretizationBins - 1;

	vector<pair<RealVarPtr, vector<vector<double>>>> realPredOpts;
	vector<CatVarPtr> catPred;
	for(const VarPtr& var : predictors) {
		lambdaVisit(var,
			[&](const RealVarPtr& var) {
				vector<double> vals;
				for(const vector<double>& point : data) {
					vals.push_back(var->parseDataSrcVal(point));
				}
				sort(vals.begin(), vals.end());
				vector<vector<double>> discs(discCount);
				for(size_t i = 0; i < discCount; ++i) {
					discs[i] = computeDisc(vals, i + 2);
				}
				realPredOpts.emplace_back(var, move(discs));
			},
			[&](const CatVarPtr& var) {
				catPred.push_back(var);
			}
		);
	}

	bool first = true;
	double bestScore;
	vector<pair<RealVarPtr, vector<double>>> bestRealPred;
	cycleDiscs(realPredOpts, [&](const vector<pair<RealVarPtr, vector<double>>>& realPred) {
		double score = computeFullTableScore(realPred, catPred, response, data);
		if(first || score > bestScore) {
			bestScore = score;
			bestRealPred = realPred;
		}
		first = false;
	});

	TreeResult ret;
	ret.tree = createFullTableTree(bestRealPred, catPred, response, data);
	ret.dataScore = bestScore;
	ret.structureScore = 0.0;
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

	Leaf<R> leaf(response, dataCount, [&](size_t i) { return data[i].second; });

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

TreeResult optimizeFullTable(
	const vector<VarPtr>& predictors,
	const VarPtr& response,
	const vector<vector<double>>& data,
	size_t maxDiscretizationBins
) {
	return lambdaVisit(response, [&](const auto& response) {
		return optimizeFullTable(predictors, response, data, maxDiscretizationBins);
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
