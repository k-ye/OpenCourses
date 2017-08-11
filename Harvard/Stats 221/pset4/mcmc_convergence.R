## Metropolis-Hastings and convergence
## Fall, 2014, Panos Toulis ptoulis@fas.harvard.edu
## 
library(mvtnorm)
library(coda)
d = 5  # dimensions
mu.vector <- c(1:d)
rho = 0.5  # autocorrelation between components
Sigma <- (1-rho) * diag(d) + rho * matrix(1, nrow=d, ncol=d)
# We will sample from 
# y ~ N(m, S)  where m = (1, 2, 3...d)  and S = r I + (1-r) U  i.e., equicorrelation

NormalMHExample<-function(mcmc.niters, oracle=F) {
  # Runs Metropolis-Hastings for the MVN example
  # Returns  sims x d  matrix with samples.
  #
  # Use oracle=T if you need "perfect sampling"
  #
  # the chain matrix
  theta.mh <- matrix(0, nrow = mcmc.niters, ncol = d)
  # Start from overdispersed
  theta.mh[1, ] <- rmvnorm(n=1, mean=mu.vector, sigma=2 * Sigma)
  pb = txtProgressBar(style=3)
  
  nacc <- 0  # count of acceptance
  for (i in 2:mcmc.niters) {
    # 1. Get current state of the chain
    theta.old <- theta.mh[i-1,]  # prev parameters
    # 2. Propose new state (theta.new)
    if(oracle) {
      # Here we sample from the actual distribution. 
      # We do this to check the effectiveness of the convergence checks.
      theta.new <- rmvnorm(n=1, mean=mu.vector, sigma=Sigma)
    } else {
      # Propose  theta.new ~ N(theta.old, s * I)  i.e., around the current state
      theta.new <- rmvnorm(n=1, mean=theta.old, sigma=2 * diag(d))  # new params
    }
    setTxtProgressBar(pb, value=i/mcmc.niters)
    # 3. Compute MH ratio (the terms from the propNoosal cancel out by symmetry)
    log.a = min(0, dmvnorm(theta.new, mean=mu.vector, sigma=Sigma, log=T) - 
                   dmvnorm(theta.old, mean=mu.vector, sigma=Sigma, log=T))
    # 4. accept-reject
    if(runif(1) < exp(log.a)) {
      theta.mh[i,] <- theta.new
      nacc <- nacc + 1
    } else {
      theta.mh[i,] <- theta.old
    }
  }
  # 5. (optional) Print some statistics to make sure it is fine.
  print(sprintf("Acceptance rate %.2f%%", 100 * nacc/mcmc.niters))
  burnin = 0.3 * mcmc.niters
  return(theta.mh[-c(1:burnin),])
}

# Run the chain
run.chain <- function(show.diagnostics=F, oracle=F) {
  mh.draws <- NormalMHExample(mcmc.niters = 5000, oracle=oracle)
  
  ##  Do the diagnostics here.
  if (show.diagnostics) {
    library(coda)
    mcmc.chain <- mcmc(mh.draws)
    fns <- c("summary", "plot", "autocorr.plot", "rejectionRate")
    for (fn in fns) {
      readline(sprintf("Press [ENTER] for %s", fn))
      do.call(fn, args=list(mcmc.chain))
    }
  }
  print("Geweke diagnostic")
  print(geweke.diag(mh.draws))
  
  print("Raftery diagnostic")
  print(raftery.diag(mh.draws, r=0.005))
  
  print("Heidelberg diagnostic")
  print(heidel.diag(mh.draws))
}

rubin.gelman <- function(nchains=5) {
  mh.list <- list()
  print("Calculating Gelman-Rubin diag")
  pb = txtProgressBar(style=3)
  for(i in 1:nchains) {
    mh.list[[i]] <- mcmc(NormalMHExample(mcmc.niters = 2000, oracle = F))
    setTxtProgressBar(pb, value=i/nchains)
  }
  mh.list <- mcmc.list(mh.list)
  print(gelman.diag(mh.list))
}


