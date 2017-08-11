# @pre Should create subfiles of "data/" before using
# read args
source('glmnet.R')

args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(1)
} else if (length(args) != 1) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

job.id <- args[1]
Ns <- c(1e3, 5e3, 1e2, 1e2, 1e2, 1e2)
ps <- c(1e2, 1e2, 1e3, 5e3, 2e4, 5e4)

test.len <- length(Ns)
timings <- list()
methodName = ""

for(i in 1:test.len) {
  timing <- NA
  if (job.id == 1) {
    timing <- run.glmnet(Ns[i], ps[i])
    methodName = "glm.naive"
  } else if (job.id == 2) {
    timing <- run.glmnet(Ns[i], ps[i], type.gs="covariance")
    methodName = "glm.cov"
  } else if (job.id == 3) {
    timing <- run.lars(Ns[i], ps[i])
    methodName = "lars"
  } else if (job.id == 4) {
    timing <- run.SGD(Ns[i], ps[i], nreps=10)
    methodName = "SGD"
  } else {
    timing <- run.SGD(Ns[i], ps[i], nreps=10, useImplicit=T)
    methodName = "Implicit"
  }
  
  timings[[i]] = timing
}

timings[['method']] = methodName
