source('yk_mcmc.R')

# param: [1]:mu [2]:theta
# sample: lambda, theta
# lambda = mu * theta
log.prior <- function(param) {
  lambda <- prod(param)
  return(log(1 / lambda))
}

log.lik <- function(param, data) {
  # Log-likelihood of the data
  lambda <- prod(param)
  sum(dpois(data, lambda=lambda, log=T))
  #sum(dbinom(data, size=N, prob=theta, log=T))
}

log.posterior <- function(param, data) {
  log.lik(param, data) + log.prior(param)
}

prop.func <- function(param.old, data, Q.pars) {
  S <- Q.pars$S
  n <- Q.pars$n
  kBound <- Q.pars$kBound
  #std <- Q.pars$std
  shape1 <- Q.pars$shape1
  shape2 <- Q.pars$shape2
  
  lambda.prop <- rgamma.trunc(kBound**2, s=S, r=n)
  #N.old <- param.old[1]
  #theta.prop <- runif(1, 0, 1)
  theta.prop <- rbeta(1, shape1, shape2)
  mu.prop <- lambda.prop / theta.prop
  #S.prop <- rnorm.trunc(N.old, std, max(data) * theta.prop)
  #N.prop <- round(S.prop / theta.prop)
  return(c(mu.prop, theta.prop))
}

prob.Q <- function(param.prop, param.old, Q.pars) {
  # get params for Q (prop distribution)
  S <- Q.pars$S
  n <- Q.pars$n
  #std <- Q.pars$std
  shape1 <- Q.pars$shape1
  shape2 <- Q.pars$shape2
  # old generated params
  mu.old <- param.old[1]
  theta.old <- param.old[2]
  lambda.old <- mu.old * theta.old
  # prop generated params
  mu.prop <- param.prop[1]
  theta.prop <- param.prop[2]
  lambda.prop <- mu.prop * theta.prop
  
  prob.lambda <- dgamma(lambda.old, shape=S, rate=n, log=T)
  #prob.S <- dnorm(S.old, mean=S.prop, sd=std, log=T)
  prob.theta <- dbeta(theta.old, shape1, shape2, log=T)
  return(prob.lambda + prob.theta)
}

Q.pars.gen <- function(data, kBound, shape1, shape2) {
  S <- sum(data) + 1
  n <- length(data)
  return(list(S=S, n=n, kBound=kBound, shape1=shape1, shape2=shape2))
  #return(list(std=std, shape1=shape1, shape2=shape2))
}

mcmc.chain.gen <- function(data, kBound=150, shape1=5, shape2=5, num.iter=1e4, burn.len=1e3, seed=1) {
  set.seed(seed)
  # generate N and theta
  theta.init <- runif(1, 0, 1)
  lambda.init <- mean(data)
  #N.init <- round(S.init / theta.init)
  mu.init <- lambda.init / theta.init
  param.init <- c(mu.init, theta.init)
  Q.pars <- Q.pars.gen(data, kBound, shape1, shape2)
  
  mcmc.chain <- mcmc(data, Q.pars, param.init, prop.func, accept.func=param.accept.func, 
                     num.iter, burn.len, seed)
  return(mcmc.chain)
}