context("rpcart")
library(rpcart)

test_that("opt.pcart.cat works", {
  data <- data.frame(
    a=c(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1),
    b=c(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1),
    c=c(0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2)
  )
  result <- opt.pcart.cat(data, c("a", "b"), "c")
  expect_equal(result$score, -15.30213, tolerance=0.001)
  expect(
    result$tree == "-+ a: 0 | 1\n |-+ b: 0 | 1\n | |-- c: 4 0 0\n | `-- c: 0 4 0\n `-+ b: 0 | 1\n   |-- c: 0 4 0\n   `-- c: 0 0 4\n" ||
    result$tree == "-+ b: 0 | 1\n |-+ a: 0 | 1\n | |-- c: 4 0 0\n | `-- c: 0 4 0\n `-+ a: 0 | 1\n   |-- c: 0 4 0\n   `-- c: 0 0 4\n"
  )
})

test_that("opt.pcart.cat.bdeu works", {
  data <- data.frame(
    a=c(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1),
    b=c(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1),
    c=c(0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2)
  )
  result <- opt.pcart.cat.bdeu(data, c("a", "b"), "c")
  expect_equal(result$score, -11.9985, tolerance=0.001)
  expect(
    result$tree == "-+ a: 0 | 1\n |-+ b: 0 | 1\n | |-- c: 4 0 0\n | `-- c: 0 4 0\n `-+ b: 0 | 1\n   |-- c: 0 4 0\n   `-- c: 0 0 4\n" ||
    result$tree == "-+ b: 0 | 1\n |-+ a: 0 | 1\n | |-- c: 4 0 0\n | `-- c: 0 4 0\n `-+ a: 0 | 1\n   |-- c: 0 4 0\n   `-- c: 0 0 4\n"
  )
})
