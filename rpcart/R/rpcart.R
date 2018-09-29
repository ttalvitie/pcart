call.pcartcli <- function(input) {
  sysname <- Sys.info()['sysname']
  machine <- Sys.info()['machine']
  if(sysname == "Windows") {
    if(endsWith(machine, "64")) {
      bin_name <- "pcartcli-win64.exe"
    } else {
      bin_name <- "pcartcli-win32.exe"
    }
  } else if(sysname == "Linux") {
    if(endsWith(machine, "64")) {
      bin_name <- "pcartcli-lin64"
    } else {
      bin_name <- "pcartcli-lin32"
    }
  } else if(sysname == "Darwin") {
    if(endsWith(machine, "64")) {
      bin_name <- "pcartcli-mac64"
    } else {
      bin_name <- "pcartcli-mac32"
    }
  } else {
    stop(paste("Unknown system name", sysname, "-> cannot choose pcartcli executable to run"))
  }

  bin_path <- system.file("bin", bin_name, package="rpcart")
  res <- system2(bin_path, c(), input=input, stdout=TRUE, stderr=TRUE)

  if((!is.null(attr(res, 'status')) && attr(res, 'status') != 0)) {
    stop(paste("Running", bin_name, "failed"))
  }

  score <- as.double(res[2])
  if(res[1] != "OK" || !is.finite(score)) {
    stop(paste("Running", bin_name, "resulted in an invalid output"))
  }

  return(score)
}

opt.pcart.cat.internal <- function(data, predictors, response, type, param) {
  return(call.pcartcli(paste0("0 1\n", type, " 2 0.5\n0\n")))
}

opt.pcart.cat <- function(data, predictors, response, alpha=0.5) {
  return(opt.pcart.cat.internal(data, predictors, response, "CAT", alpha))
}

opt.pcart.cat.bdeu <- function(data, predictors, response, ess=1.0) {
  return(opt.pcart.cat.internal(data, predictors, response, "BDEU_CAT", ess))
}
