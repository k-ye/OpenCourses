# @pre Should create subfiles of "data/" before using
# read args
args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(1e6, 1)
} else if (length(args) != 2) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

numData <- args[1]
job.id <- args[2]

source("ykuang_ps3_task2_part1_util.R")
set.seed(39)
data <- data.gen(t=numData)

if (job.id == 1) {
  result <- batch(data)
  save(result, file="data/ps3_task2_part1_batch.rda")
} else if (job.id == 2) {
  result <- SGD(data)
  save(result, file="data/ps3_task2_part1_SGD.rda")
} else if (job.id == 3) {
  # ASGD Good
  result <- ASGD(data, TRUE)
  save(result, file=sprintf("data/ps3_task2_part1_ASGD%s.rda", ""))
} else if (job.id == 4) {
  # ASGD Bad
  result <- ASGD(data, FALSE)
  save(result, file=sprintf("data/ps3_task2_part1_ASGD%s.rda", "_BAD"))
} else if (job.id == 5) {
  result <- Implicit(data)
  save(result, file="data/ps3_task2_part1_implicit.rda")
} else {
  print(job.id)
}