source('ykuang_functions.R')

data.path <- "dat/1router_allcount.dat"
args <- as.numeric(commandArgs(trailingOnly = TRUE))
if (length(args) == 0) {
  args <- c(1)
} else if (length(args) != 1) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}
job.id <- args[1]
w.range <- NA

if (job.id < 11) {
  w.range <- ((job.id-1)*25 + 1):(job.id*25) + 5
} else {
  w.range <- 256:282
}

data <- data.load(data.path)
A <- A.gen(4)

c <- 1
thetas.local <- locally_iid_EM(data, c=c, A, w.range, verbose=T, max.iter=5e5, threshold=5e-2)
save(thetas.local, file=sprintf("out/pset5_1router_local_c%d_%d.RData", c, job.id))
