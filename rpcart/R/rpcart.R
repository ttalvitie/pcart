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

  tree <- res[3:length(res)]

  return(list(score=score, tree=tree))
}

to.factor <- function(x) {
  # Ensures that missing levels are not removed
  if(class(x) == "factor") {
    return(x)
  } else {
    return(factor(x))
  }
}

opt.pcart.cat.internal <- function(data, predictors, response, type, param) {
  if(class(data) != "data.frame") {
    stop("data should be a data.frame")
  }
  if(length(response) != 1) {
    stop("response should have length 1")
  }
  vars <- c(predictors, response)
  if(any(duplicated(vars))) {
    stop("no column label should appear twice in c(predictors, response)")
  }

  mydata <- data.frame(lapply(data[vars], to.factor))

  types <- c(rep("CAT", length(predictors)), type)
  varstr <- paste(types, unlist(lapply(lapply(mydata, levels), length)), collapse=" ")
  varstr <- paste(varstr, param)

  datastr <- paste(t(data.matrix(mydata)) - 1, sep=' ', collapse=' ')

  input <- paste(length(predictors), nrow(data), varstr, datastr)

  callresult <- call.pcartcli(input)
  score <- callresult$score
  tree <- callresult$tree

  pos <- 0
  prettify <- function(pre1, pre) {
    pos <<- pos + 1
    line <- strsplit(tree[[pos]], " +")[[1]]
    if(line[1] == "SPLIT") {
      varidx <- as.numeric(line[3]) + 1
      cats <- line[4:(length(line)-1)]
      cats[cats == -1] = "|"
      tree[[pos]] <<- paste0(pre1, "-+ ", predictors[varidx], ": ", paste(cats, collapse=" "))
      prettify(paste0(pre, " |"), paste0(pre, " |"))
      prettify(paste0(pre, " `"), paste0(pre, "  "))
    } else {
      tree[[pos]] <<- paste0(pre1, "-- ", response, ": ", paste(line[3:length(line)], collapse=" "))
    }
  }
  prettify("", "")

  tree <- paste0(paste(tree, collapse="\n"), "\n")

  return(list(score=score, tree=tree))
}

opt.pcart.cat <- function(data, predictors, response, alpha=0.5) {
  return(opt.pcart.cat.internal(data, predictors, response, "CAT", alpha))
}

opt.pcart.cat.bdeu <- function(data, predictors, response, ess=1.0) {
  return(opt.pcart.cat.internal(data, predictors, response, "BDEU_CAT", ess))
}
