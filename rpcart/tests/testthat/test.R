context("rpcart")
library(rpcart)

test_that("opt.pcart.cat works", {
  data = data.frame(a=c("a", "b", "a", "b"), b=c(5, 2, 3, 1), c=c(TRUE, FALSE, TRUE, FALSE), d=c(5, 4, 3, 3))
  expect_equal(opt.pcart.cat(data, c("a", "b", "d"), c("c")), -4.05, tolerance=0.1)
})
