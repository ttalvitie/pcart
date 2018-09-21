#pragma once

#include <pcart/score.h>
#include <pcart/variable.h>

namespace pcart {

template <typename T>
struct Leaf;
typedef Leaf<RealVar> RealLeaf;
typedef Leaf<CatVar> CatLeaf;

template <typename T>
struct Split;
typedef Split<RealVar> RealSplit;
typedef Split<CatVar> CatSplit;

typedef std::variant<RealLeaf, CatLeaf, RealSplit, CatSplit> Tree;
typedef std::unique_ptr<Tree> TreePtr;

struct BaseLeaf {};

template <typename T>
struct Leaf : public BaseLeaf {
	shared_ptr<const T> var;
	LeafStats<T> stats;
};

struct BaseSplit {
	unique_ptr<Tree> leftChild;
	unique_ptr<Tree> rightChild;
};

template <>
struct Split<RealVar> : public BaseSplit {
	RealVarPtr var;
	double splitVal;
};

template <>
struct Split<CatVar> : public BaseSplit {
	CatVarPtr var;
	uint64_t leftCatMask;
	uint64_t rightCatMask;
};

struct TreeResult {
	double dataScore;
	double structureScore;
	double totalScore;
	TreePtr tree;
};

void iterateTrees(
	const vector<VarPtr>& predictors,
	const VarPtr& response,
	const vector<vector<double>>& data,
	function<void(const TreeResult&)> f
);

void printTree(const TreePtr& tree, ostream& out = cout);

}
