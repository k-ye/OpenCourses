#library(MASS)
# @pre Should create subfiles of "data/" before using
# read args
args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(1e6, 2e5, 1)
} else if (length(args) != 3) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

chain.len <- args[1]
burn.len <- args[2]
job.id <- args[3]

library(MASS)
#source('yk_lambda_theta_util.R')
source('yk_N_theta_util.R')
#source('yk_S_theta_util.R')
#source('yk_N_theta_2_util.R')

data.path <- 'data/%s.txt'
plot.path <- 'plot/%s_%d.png'
name.impala <- 'impala'
name.water <- 'waterbuck'

name.data <- ifelse(job.id == 2, name.water, name.impala)
data <- read.table(sprintf(data.path, name.data), head=T, stringsAsFactors=F)[,1]

# sample N and theta 
std1 <- sd(data)
std2 <-0.02

num.chains <- 10
prob.N100 <- rep(NA, num.chains)
savePlot <- FALSE
for (i in 1:num.chains) {
  result <- mcmc.chain.gen(data, std1=std1, std2=std2, num.iter=chain.len, burn.len=burn.len, seed=i)
  #result <- mcmc.chain.gen(data, shape1, shape2, num.iter=chain.len, burn.len=burn.len, seed=i)
  mcmc.chain <- result$chain
  sim.N <- mcmc.chain[,1]
  sim.theta <- mcmc.chain[,2]
  prob.N100[i] <- (sum(sim.N>100) / length(sim.N))
  
  if (savePlot) {
    slicer <- seq(from=1, to=length(sim.N), by=50)
    sim.N <- sim.N[slicer]
    sim.theta <- sim.theta[slicer]
    ct.data <- kde2d(sim.N, sim.theta, n=100)
    png(sprintf(plot.path, name.data, i), width=1200, height=900)
    #plot.chain(mcmc.chain)
    plot(sim.N, sim.theta, xlab='Simulate N',  ylab='Simulated theta', main=sprintf("MCMC simulation for %s, %d", name.data, i))
    #grid()
    contour(ct.data, col='red', add=TRUE, lwd=2, labcex=1.2)
    dev.off()
    save(result, file=sprintf("data/%s_chain_%d.rda", name.data, i)) 
  }
}
#save(prob.N100, file=sprintf("data/%s_Ngr100.rda", name.data))