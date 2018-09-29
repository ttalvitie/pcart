context("rpcart")
library(rpcart)

test_that("opt.pcart.cat works", {
  expect_equal(opt.pcart.cat.bdeu(), -0.6931472, tolerance=0.001)
})
test_that("opt.pcart.cat.bdeu works", {
  expect_equal(opt.pcart.cat(), -0.6931472, tolerance=0.001)
})
