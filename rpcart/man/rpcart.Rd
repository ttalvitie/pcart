\name{opt.pcart.cat}
\alias{opt.pcart.cat}
\alias{opt.pcart.cat.bdeu}
\title{opt.pcart.cat}
\usage{
opt.pcart.cat(data, predictors, response, alpha=0.5, use_structure_score=TRUE)
opt.pcart.cat.bdeu(data, predictors, response, ess=1.0, use_structure_score=TRUE)
}
\description{
Optimizes PCART decision tree to given categorical data. Data is given as a data frame (each column is converted to a factor), and the list of predictors and response should consist of column labels of the data frame. For the prior of the response variable, opt.pcart.cat uses the Dirichlet prior with the same hyperparameter alpha for every category of every leaf (defaulting to Jeffrey's prior), whereas opt.pcart.cat.bdeu uses BDeu with equivalent sample size given by the parameter ess. By setting use_structure_score to FALSE, the tree structure prior is disabled by setting all tree structure scores to 0.

Returns a list containing two elements: the score and the tree structure as a string (with newlines, print it with cat).
}
\examples{
# XOR data set
data <- data.frame(
  a=c(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1),
  b=c(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1),
  c=c(0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0)
)

# Optimize tree
result <- opt.pcart.cat(data, c("a", "b"), "c")

# Show score and tree structure
print(result$score)
cat(result$tree)
}

