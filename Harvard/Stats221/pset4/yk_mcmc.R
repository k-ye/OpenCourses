source('ps4_util.R')

param.accept.func <- function(param.prop, param.old, data, Q.pars) {
  accept <- log.posterior(param.prop, data) + prob.Q(param.old, param.prop, data, Q.pars)
  accept <- accept - (log.posterior(param.old, data) + prob.Q(param.prop, param.old, data, Q.pars))
  return(accept)
}

param.decide <- function(accept, param.old, param.prop) {
  # Decide if going to accept the proposed parameter
  accept <- min(0, accept)
  u <- log(runif(1, min=0, max=1))
  
  param.new <- param.old
  is.accept <- 0
  if (u < accept) {
    param.new <- param.prop
    is.accept <- 1 
  }
  return(list(param.new=param.new, is.accept=is.accept))
}

mcmc <- function(data, Q.pars, param.init, prop.func, accept.func, 
                 num.iter=5e4, burn.len=1e4, seed=1) {
  # basic mcmc function
  set.seed(seed)
  num.param <- length(param.init)
  params <- array(NA, dim=c(num.iter, num.param))
  params[1, ] <- param.init
  accept.num <- 0
  
  for(i in 2:num.iter) {
    if (i %% 1e4 == 0)
      print(sprintf("Iteration: %d", i))
    param.old <- as.vector(params[i-1,])
    # generate proposed params
    param.prop <- prop.func(param.old, data, Q.pars)
    # calculate accept rate
    accept <- accept.func(param.prop, param.old, data, Q.pars)
    # decide if accept the proposed param or not
    decide <- param.decide(accept, param.old, param.prop)
    param.new <- decide$param.new
    accept.num = accept.num + decide$is.accept
    params[i,] <- param.new
  }
  params <- params[burn.len:num.iter,]
  print(sprintf("Total accept: %d, accept ratio: %2.f%%", accept.num, accept.num/num.iter*100))
  result <- list(chain=params, accept=accept.num/num.iter*100)
  return(result)
}

plot.chain <- function(mcmc.chain) {
  mcmc.niters = nrow(mcmc.chain)
  burnin = 0.1 * mcmc.niters
  mcmc.chain = mcmc.chain[burnin:mcmc.niters, ]
  f = kde2d(x=mcmc.chain[, 1], y=mcmc.chain[, 2], n=100)
  
  image(f, xlim=c(min(mcmc.chain[, 1]), max(mcmc.chain[, 1])), 
        ylim=c(min(mcmc.chain[, 2]), max(mcmc.chain[, 2])))
}

