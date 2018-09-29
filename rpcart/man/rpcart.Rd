\name{opt.pcart.cat}
\alias{opt.pcart.cat}
\alias{opt.pcart.cat.bdeu}
\title{opt.pcart.cat}
\usage{
opt.pcart.cat(data, predictors, response, alpha=0.5)
opt.pcart.cat.bdeu(data, predictors, response, ess=1.0)
}
\description{
Optimizes PCART decision tree to given categorical data, and returns the score. Data is given as a data frame, and the list of predictors and response should be labels of the data frame (not all labels of the data have to be used). For prior, opt.pcart.cat uses the same hyperparameter alpha for every category of every leaf (defaulting to Jeffrey's prior), whereas the opt.pcart.cat.bdeu uses BDeu with equivalent sample size given by parameter ess.
}
\examples{
TODO
}
