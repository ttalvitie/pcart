#include <iostream>

#include <pcart/tree.h>

using namespace pcart;

int main() {
	VarPtr A = createRealVar("A", 0, 0.0, 1.0, 1);
	VarPtr B = createCatVar("B", 1, { "x", "y", "z" });
	VarPtr C = createCatVar("C", 2, { "u", "v" });
	VarPtr D = createRealVar("D", 3, -1.0, 1.0, 2);

	vector<vector<double>> data = {
		{0, 2, 0, 0.5}
	};
	iterateTrees(
		{ A, B }, D, data,
		[&](const TreeResult& treeResult) {
			printTree(treeResult.tree);
			cout << "\n";
		}
	);
}
