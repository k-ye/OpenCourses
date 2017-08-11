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

timings = list()
timing <- NA
methodName = ""

if (job.id == 1) {
  timing <- run.SGD(dim.n=1e5, dim.p=1e3, nreps=20, useImplicit=F)
  methodName = "SGD"
} else {
  timing <- run.SGD(dim.n=1e5, dim.p=1e3, nreps=20, useImplicit=T)
  methodName = "Imp"
}

timings = list(timing=timing, method=methodName)
save(timings, file=sprintf("data/task3_part4_%s.rda", methodName))