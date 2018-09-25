#include <random>
#include <pcart/tree.h>

using namespace std;
using namespace pcart;

int main() {
	// Define variables (see ../pcart/variable.h for info on setting hyperparameters; we use the defaults)
	VarPtr A = createRealVar("A", 0, -2.0, 2.0, 2); // data index 0, range [-2, 2], max 2 subdivisions
	VarPtr B = createCatVar("B", 1, { "a", "b", "c" }); // data index 1, categories {a, b, c}
	VarPtr C = createCatVar("C", 2, { "x", "y" }); // data index 2, categories {x, y}
	VarPtr D = createCatVar("D", 3, { "0", "1" }); // data index 3, categories {0, 1}

	// Random generator
	mt19937 rng(random_device{}());

	// Try different data set sizes
	for(size_t N = 32; N <= 4096; N *= 2) {
		// Generate data from a decision tree that predicts D from {A, B, C} and try to learn it back
		vector<vector<double>> data(N);
		for(size_t i = 0; i < N; ++i) {
			data[i].resize(4);

			// As defined by the data index parameter, the value indices are as follows
			double& valA = data[i][0];
			double& valB = data[i][1];
			double& valC = data[i][2];
			double& valD = data[i][3];

			// In this example, we generate A, B, C uniformly at random
			valA = uniform_real_distribution<double>(-2.0, 2.0)(rng);

			// For categorical variables, the categories are given by 0-based integer indices
			valB = (double)uniform_int_distribution<int>(0, 2)(rng);
			valC = (double)uniform_int_distribution<int>(0, 1)(rng);

			// Determine D using a decision tree
			if(valA < 0.0) {
				if(valB == 0.0 || valB == 2.0) { // B = a or B = c
					if(valC == 0.0) { // C = x
						if(valA < -1.0) {
							valD = bernoulli_distribution(0.1)(rng);
						} else { // A >= -1
							if(valB == 0.0) { // B = a
								valD = bernoulli_distribution(0.2)(rng);
							} else { // B = c
								valD = bernoulli_distribution(0.9)(rng);
							}
						}
					} else { // C = y
						valD = bernoulli_distribution(0.5)(rng);
					}
				} else { // B = b
					valD = bernoulli_distribution(0.4)(rng);
				}
			} else { // A >= 1
				if(valC == 0.0) { // C = x
					valD = bernoulli_distribution(0.1)(rng);
				} else { // C = y
					valD = bernoulli_distribution(0.2)(rng);
				}
			}
		}

		// Optimize a decision tree based on the data
		TreeResult res = optimizeTree({A, B, C}, D, data);

		// Print the tree and its score
		cout << "Sample size: " << N << '\n';
		cout << "Score: " << res.totalScore() << '\n';
		printTree(res.tree);
		cout << '\n';
	}

	/*
	If you are lucky, with larger data sizes you will get trees that look
	like the generating tree, for example:

	Sample size: 4096
	Score: -2043.86
	-+ A (< or >=) 0
	 |-+ B in {a, c} or {b}
	 | |-+ C in {x} or {y}
	 | | |-+ A (< or >=) -1
	 | | | |-- D: 0 x292, 1 x29
	 | | | `-+ B in {a} or {c}
	 | | |   |-- D: 0 x125, 1 x23
	 | | |   `-- D: 0 x15, 1 x152
	 | | `-- D: 0 x328, 1 x346
	 | `-- D: 0 x413, 1 x285
	 `-+ C in {x} or {y}
	   |-- D: 0 x940, 1 x107
	   `-- D: 0 x843, 1 x198
	*/

	return 0;
}
