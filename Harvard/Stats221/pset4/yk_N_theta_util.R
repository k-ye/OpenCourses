source('yk_mcmc.R')

# param: [1]:N [2]:theta
# sample: N , theta
log.prior <- function(param) {
  N <- param[1]
  return(log(1 / N))
}

log.lik <- function(param, data) {
  # Log-likelihood of the data
  N <- param[1]
  theta <- param[2]
  sum(dbinom(data, size=N, prob=theta, log=T))
}

log.posterior <- function(param, data) {
  log.lik(param, data) + log.prior(param)
}

prop.func <- function(param.old, data, Q.pars) {
  std1 <- Q.pars$std1
  std2 <- Q.pars$std2
  
  N.old <- param.old[1]
  theta.old <- param.old[2]
  
  N.prop <- round(rnorm.trunc(N.old, std1, max(data)))
  theta.prop <- rnorm(1, mean=theta.old, sd=std2)
  theta.prop <- max(1e-5, theta.prop)
  theta.prop <- min(1, theta.prop)
  
  return(c(N.prop, theta.prop))
}

prob.Q <- function(param.prop, param.old, data, Q.pars) {
  # get params for Q (prop distribution)
  std1 <- Q.pars$std1
  std2 <- Q.pars$std2
  # old generated params
  N.old <- param.old[1]
  theta.old <- param.old[2]
  # prop generated params
  N.prop <- param.prop[1]
  theta.prop <- param.prop[2]
  
  prob.N <- dnorm(N.old, mean=N.prop, sd=std1, log=T)
  prob.theta <- dnorm(theta.old, mean=theta.prop, sd=std2, log=T)
  return(prob.N + prob.theta)
}

Q.pars.gen <- function(data, std1, std2) {
  return(list(std1=std1, std1=std1, std2=std2))
}

mcmc.chain.gen <- function(data, std1=1, std2=0.1, num.iter=1e4, burn.len=1e3, seed=1) {
  set.seed(seed)
  # generate N and theta
  theta.init <- runif(1, 0, 1)
  N.init <- max(data)
  param.init <- c(N.init, theta.init)
  Q.pars <- Q.pars.gen(data, std1, std2)
  
  mcmc.chain <- mcmc(data, Q.pars, param.init, prop.func, accept.func=param.accept.func, 
                     num.iter, burn.len, seed)
  return(mcmc.chain)
}