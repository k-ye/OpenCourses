# Copyright (c) 2013
# Panos Toulis, ptoulis@fas.harvard.edu
#
# Using the simulation setup as in glmnet JoSS paper(Friedman, Hastie, Tibshirani)
# http://www.jstatsoft.org/v33/i01/
#
# EXAMPLE run:
#  run.glmnet(1e4, 10)
#
rm(list=ls())
library(mvtnorm)
library(glmnet)
library(elasticnet)

# genjerry, genx2 are functions taken from the above paper.
# These functions generate the simulation data.
# NOTE: use function sample.data(..) instead.
genjerry = function(x, snr){
  # generate data according to Friedman's setup
  n=nrow(x)
  p=ncol(x)
  b=((-1)^(1:p))*exp(-2*((1:p)-1)/20)
  # b=sample(c(-0.8, -0.45, 0.45, 0.9, 1.4), size=p, replace=T)
  # ((-1)^(1:p))*(1:p)^{-0.65}#exp(-2*((1:p)-1)/20)
  f=x%*%b
  e=rnorm(n)
  k=sqrt(var(f)/(snr*var(e)))
  y=f+k*e
  return(list(y=y, beta=b))
}

genx2 = function(n,p,rho){
  #    generate x's multivariate normal with equal corr rho
  # Xi = b Z + Wi, and Z, Wi are independent normal.
  # Then Var(Xi) = b^2 + 1
  #  Cov(Xi, Xj) = b^2  and so cor(Xi, Xj) = b^2 / (1+b^2) = rho
  z=rnorm(n)
  if(abs(rho)<1){
    beta=sqrt(rho/(1-rho))
    x0=matrix(rnorm(n*p),ncol=p)
    A = matrix(z, nrow=n, ncol=p, byrow=F)
    x= beta * A + x0
  }
  if(abs(rho)==1){ x=matrix(z,nrow=n,ncol=p,byrow=F)}
  
  return(x)
}

sample.data <- function(dim.n, dim.p, rho=0.0, snr=1) {
  # Samples the dataset according to Friedman et. al.
  #
  # 1. sample covariates
  X = genx2(dim.n, dim.p, rho)
  # 2. ground truth params.
  theta = ((-1)^(1:dim.p))*exp(-2*((1:dim.p)-1)/20)
  
  f= X %*% theta
  e = rnorm(dim.n)
  k= sqrt(var(f)/(snr*var(e)))
  y=f+k*e
  return(list(y=y, X=X, theta=theta))
}

dist <- function(x, y) {
  if(length(x) != length(y))
    stop("MSE should compare vectors of same length")
  sqrt(mean((x-y)^2))
}

# Main function to run this experiment.
run.glmnet <- function(dim.n, dim.p,
                       rho.values=c(0.0, 0.1, 0.2, 0.5, 0.9, 0.95),
                       nreps=3, 
                       type.gs="naive") {
  ## Runs glmnet() for various param values.
  ## type.gaussian: naive, covariance
  niters = 0
  #cols = c("rho", "rep", "time", "mse")
  cols = c("rho", "time", "mse")
  timings = matrix(nrow=0, ncol=length(cols))
  colnames(timings) <- cols
  rownames(timings) = NULL
  total.iters = nreps * length(rho.values)
  
  pb = txtProgressBar(style=3)
  
  seeds=sample(1:1e9, size=total.iters)
  for(rho in rho.values) {
    dt.mean = 0
    mse.mean = 0
    for(i in 1:nreps) {
        niters = niters + 1
        set.seed(seeds[niters])
        # 1. (For every repetition) Sample the dataset
        dataset = sample.data(dim.n=dim.n, dim.p=dim.p, rho=rho, snr=3.0)
        true.theta = dataset$theta
        x = dataset$X
        y = dataset$y
        stopifnot(nrow(x) == dim.n, ncol(x) == dim.p)
        # 1b. Define metrics:
        #   dt = time for the method to finish
        #   mse = Distance (e.g. RMSE) of the estimates to the ground truth.
        #         (q1, q2, q3) representing the quartiles (since glmnet returns grid of estimates)
        #         Implicit has (x, x, x) i.e., the same value in all places.
        new.dt = 0
        new.mse = NA
        # 2. Run the method.
        new.dt = system.time({ fit = glmnet(x, y, alpha=1, standardize=FALSE, type.gaussian=type.gs)})[1]
        new.mse = median(apply(fit$beta, 2, function(est) dist(est, true.theta)))
        
        dt.mean = dt.mean + new.dt
        mse.mean = mse.mean + new.mse
        
        setTxtProgressBar(pb, niters/total.iters)
    }
    dt.mean = dt.mean / nreps
    mse.mean= mse.mean / nreps
    # 3. Tabulate timings
    timings = rbind(timings, c(rho, 
                               dt.mean, 
                               mse.mean))
  }
  return(timings)
}


# Main function to run this experiment.
run.lars <- function(dim.n, dim.p,
                       rho.values=c(0.0, 0.1, 0.2, 0.5, 0.9, 0.95),
                       nreps=3) {
  ## Runs glmnet() for various param values.
  ##
  niters = 0
  cols = c("rho", "time")
  timings = matrix(nrow=0, ncol=length(cols))
  colnames(timings) <- cols
  rownames(timings) = NULL
  total.iters = nreps * length(rho.values)
  
  pb = txtProgressBar(style=3)
  
  seeds=sample(1:1e9, size=total.iters)
  for(rho in rho.values) {
    dt.mean = 0
    for(i in 1:nreps) {
      niters = niters + 1
      set.seed(seeds[niters])
      # 1. (For every repetition) Sample the dataset
      dataset = sample.data(dim.n=dim.n, dim.p=dim.p, rho=rho, snr=3.0)
      true.theta = dataset$theta
      x = dataset$X
      y = dataset$y
      stopifnot(nrow(x) == dim.n, ncol(x) == dim.p)
      # 1b. Define metrics:
      #   dt = time for the method to finish
      #   mse = Distance (e.g. RMSE) of the estimates to the ground truth.
      #         (q1, q2, q3) representing the quartiles (since glmnet returns grid of estimates)
      #         Implicit has (x, x, x) i.e., the same value in all places.
      new.dt = 0
      # 2. Run the method.
      new.dt = system.time({ fit = enet(x, y, lambda=0)})[1]
      dt.mean = dt.mean + new.dt
      # 3. Tabulate timings
      setTxtProgressBar(pb, niters/total.iters)
    }
    dt.mean = dt.mean / nreps
    timings = rbind(timings, c(rho, dt.mean))
  }
  return(timings)
}

source('ykuang_ps3_task2_part2_util.R')
# Main function to run this experiment.
run.SGD <- function(dim.n, dim.p,
                       rho.values=c(0.0, 0.1, 0.2, 0.5, 0.9, 0.95),
                       nreps=10,
                    useImplicit=FALSE) {
  ## Runs glmnet() for various param values.
  ## type.gaussian: naive, covariance
  niters = 0
  #cols = c("rho", "rep", "time", "mse")
  cols = c("rho", "time", "mse")
  timings = matrix(nrow=0, ncol=length(cols))
  colnames(timings) <- cols
  rownames(timings) = NULL
  total.iters = nreps * length(rho.values)
  
  pb = txtProgressBar(style=3)
  
  seeds=sample(1:1e9, size=total.iters)
  for(rho in rho.values) {
    dt.mean = 0
    mse.mean = 0
    for(i in 1:nreps) {
      niters = niters + 1
      set.seed(seeds[niters])
      # 1. (For every repetition) Sample the dataset
      dataset = sample.data(dim.n=dim.n, dim.p=dim.p, rho=rho, snr=3.0)
      
      theta.true = dataset$theta
      theta.init = rep(0, length(theta.true))
      X = dataset$X
      Y = dataset$y
      #A = t(X) %*% X / dim(X)[1]
      A = 0
      
      data <- list(X=X, Y=Y, theta.true=theta.true, theta.init=theta.init, A=A, rho=rho)
      stopifnot(nrow(X) == dim.n, ncol(X) == dim.p)
      # 1b. Define metrics:
      #   dt = time for the method to finish
      #   mse = Distance (e.g. RMSE) of the estimates to the ground truth.
      #         (q1, q2, q3) representing the quartiles (since glmnet returns grid of estimates)
      #         Implicit has (x, x, x) i.e., the same value in all places.
      new.dt = 0
      new.mse = NA
      # 2. Run the method.
      b <- sqrt(rho/(1-rho))
      gamma0 <- 1 / (dim.p * (b^2+1))
      a <- 1
      lr.params <- list(gamma0=gamma0, a=a)
      if (useImplicit) {
        new.dt = system.time({ result = Implicit(data, lr.params, saveRisk=F) })[1]
      }
      else
        new.dt = system.time({ result = SGD(data, lr.params, saveRisk=F) })[1]
      new.mse = dist(result$theta.est, theta.true)
      
      dt.mean = dt.mean + new.dt
      mse.mean = mse.mean + new.mse
      
      setTxtProgressBar(pb, niters/total.iters)
    }
    dt.mean = dt.mean / nreps
    mse.mean= mse.mean / nreps
    # 3. Tabulate timings
    timings = rbind(timings, c(rho, 
                               dt.mean, 
                               mse.mean))
  }
  return(timings)
}
