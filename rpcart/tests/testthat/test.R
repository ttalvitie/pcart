context("rpcart")
library(rpcart)

test_that("optimize.pcart works", {
  expect_equal(optimize.pcart(), -0.6931472, tolerance=0.001)
})
