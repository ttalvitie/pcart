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

struct BaseSplit {
	std::unique_ptr<Tree> leftChild;
	std::unique_ptr<Tree> rightChild;
};

template <typename T>
struct Leaf {
	const T* var;
	LeafStats<T> stats;
};

template <>
struct Split<RealVar> : public BaseSplit {
	const RealVar* var;
	double splitVal;
};

template <>
struct Split<CatVar> : public BaseSplit {
	const CatVar* var;
	uint64_t leftCatMask;
	uint64_t rightCatMask;
};

}
