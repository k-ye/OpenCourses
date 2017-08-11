# @pre Should create subfiles of "task3/" before using
# read args
args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(3, 40, 1)
} else if (length(args) != 3) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

# i = j = k = 1
num.theta.sim <- args[1] # simulate times for theta-vector
num.Y.sim <- args[2] # simulate times for Y for each theta-vector

stopifnot(num.theta.sim > 0, num.Y.sim > 0)

job.id <- args[3]
set.seed(job.id)
# find the correct parameter set for each job.id
nodePerParam <- 3
param.set <- c(rep(c(1.6,0.7),nodePerParam), rep(c(2.5,1.3),nodePerParam), rep(c(5.2,1.3),nodePerParam), rep(c(4.9,1.6),nodePerParam))
dim(param.set) <- c(2,12)
param.set <- t(param.set)

#args <- c(1.6, 0.7, 1)
mu <- param.set[job.id, 1] # mean
std <- param.set[job.id, 2] # variance

stopifnot(std > 0)

source("poissonLogN_MCMC.R")
source("ykuang_ps2_functions.R")

# function to generate theta
log.theta.gen <- function()
{
  return(rnorm(J, mean=mu, sd=std))
}

# function to generate w
w.gen <- function()
{
  return(rep(1,J))
}

J <- 1000
N <- 2

# initialize
w <- w.gen()
log.theta <- array(NA, dim=c(J, num.theta.sim))
# initialize posterior mean and std
post.mean <- list()
post.std <- list()
for (i in 1:num.theta.sim)
{
  post.mean[[i]] <- array(NA, dim=c(J, num.Y.sim))
  post.std[[i]] <- array(NA, dim=c(J, num.Y.sim))
}

# record the coverage of each theta_j
coverage.95 <- array(NA, dim=c(J, num.theta.sim))
coverage.68 <- array(NA, dim=c(J, num.theta.sim))

for (i in 1:num.theta.sim) {
  log.theta[,i] <- log.theta.gen()
  
  bool.95 <- array(NA, dim=c(J, num.Y.sim))
  bool.68 <- array(NA, dim=c(J, num.Y.sim))
  
  for (j in 1:num.Y.sim) {
    # Y is a JxN matrix, simulate Y
    Y <- simYgivenTheta(exp(log.theta[,i]), w, N)
    # draw estimated logTheta using MCMC
    mcmc.result <- poisson.logn.mcmc(Y, w)
    mcmc.log.theta <- mcmc.result$logTheta
    
    # store post mean and std
    post.mean[[i]][,j] <- apply(mcmc.log.theta, 1, mean)
    post.std[[i]][,j] <- apply(mcmc.log.theta, 1, sd)
    # compute the coverage of 95 and 68
    bool.95[,j] <- computeCoverage(mcmc.log.theta, log.theta[,i], 0.025, 0.975)
    bool.68[,j] <- computeCoverage(mcmc.log.theta, log.theta[,i], 0.16, 0.84)
  }
  # compute the coverage
  coverage.95[,i] <- apply(bool.95, 1, function(x) sum(x) / length(x))
  coverage.68[,i] <- apply(bool.68, 1, function(x) sum(x) / length(x))
}

result <- list(logTheta=log.theta, postMean=post.mean, postStd=post.std, cover95=coverage.95, cover68=coverage.68)
save(result, file=sprintf("task3/task3_id%d_mu%.1f_std%.1f.rda", job.id, mu, std))
