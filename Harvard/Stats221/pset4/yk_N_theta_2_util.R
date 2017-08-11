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
  shape1 <- Q.pars$shape1
  shape2 <- Q.pars$shape2
  
  N.old <- param.old[1]
  
  N.prop <- max(max(data), round(rexp(1, rate=1/N.old)))
  theta.prop <- rbeta(1, shape1=shape1, shape2=shape2)
  
  return(c(N.prop, theta.prop))
}

prob.Q <- function(param.prop, param.old, data, Q.pars) {
  shape1 <- Q.pars$shape1
  shape2 <- Q.pars$shape2
  # old generated params
  N.old <- param.old[1]
  theta.old <- param.old[2]
  # prop generated params
  N.prop <- param.prop[1]
  theta.prop <- param.prop[2]
  
  prob.N <- dexp(N.old, rate=1/N.prop, log=T)
  prob.theta <- dbeta(theta.old, shape1=shape1, shape2=shape2, log=T)
  return(prob.N + prob.theta)
}

Q.pars.gen <- function(data, shape1, shape2) {
  return(list(shape1=shape1, shape2=shape2))
}

mcmc.chain.gen <- function(data, shape1=6, shape2=6, num.iter=1e4, burn.len=1e3, seed=1) {
  set.seed(seed)
  # generate N and theta
  theta.init <- runif(1, 0, 1)
  N.init <- max(data)
  param.init <- c(N.init, theta.init)
  Q.pars <- Q.pars.gen(data, shape1, shape2)
  
  mcmc.chain <- mcmc(data, Q.pars, param.init, prop.func, accept.func=param.accept.func, 
                     num.iter, burn.len, seed)
  return(mcmc.chain)
}