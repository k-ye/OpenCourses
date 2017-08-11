# @pre Should create subfiles of "data/" before using
# read args
args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(1e5, 1)
} else if (length(args) != 2) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

numData <- args[1]
job.id <- args[2]

source("ykuang_ps3_task2_part2_util.R")
set.seed(39)
data <- data.gen(t=numData)

if (job.id == 1) {
  result <- batch(data)
  save(result, file="data/ps3_task2_part2_batch.rda")
} else if (job.id == 2) {
  result <- SGD(data)
  save(result, file="data/ps3_task2_part2_SGD.rda")
} else if (job.id == 3) {
  result <- ASGD(data)
  save(result, file="data/ps3_task2_part2_ASGD.rda")
} else if (job.id == 4) {
  result <- Implicit(data)
  save(result, file="data/ps3_task2_part2_implicit.rda")
} else {
  print(job.id)
}