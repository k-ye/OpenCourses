# Copyright (c) 2014
# Panos Toulis, ptoulis@fas.harvard.edu
# Helper code for pset3 for Stat 221, Fall 2014
# rm(list=ls())
#
# EXAMPLE run:
#   A = generate.A(p=10)
#   d = sample.data(dim.n=1e5, A)
#   sgd(d, alpha=2)  # should give 'biased' results
#   sgd(d, alpha=100) # better results
#
# Frequentist variance (for a quick demo):
#   run.sgd.many(nreps=500, alpha=100, nlist=c(1e3, 5e3, 1e4), p=5, verbose=T)
library(mvtnorm)

random.orthogonal <- function(p) {
  # Get an orthogonal matrix.
  B = matrix(runif(p^2), nrow=p)
  qr.Q(qr(B))
}

generate.A <- function(p) {
  # Create A matrix (variance of the covariates xn)
  Q = random.orthogonal(p)
  lambdas = seq(0.01, 1, length.out=p)
  A = Q %*% diag(lambdas) %*% t(Q)
  return(A)
}

sample.data <- function(dim.n, A, 
                        model="gaussian") {
  # Samples the dataset. Returns a list with (Y, X, A ,true theta)
  dim.p = nrow(A)
  # This call will make the appropriate checks on A.
  X = rmvnorm(dim.n, mean=rep(0, dim.p), sigma=A)
  theta = matrix(1, ncol=1, nrow=dim.p)
  epsilon = rnorm(dim.n, mean=0, sd=1)
  # Data generation
  y = X %*% theta  + epsilon
  
  return(list(Y=y, X=X, A=A, theta=theta))
}

check.data <- function(data) {
  # Do this to check the data object.
  # 
  nx = nrow(data$X)
  ny = length(data$Y)
  p = ncol(data$X)
  stopifnot(nx==ny, p==length(data$theta))
  lambdas = eigen(cov(data$X))$values
  print(lambdas)
  print(mean(data$Y))
  print(var(data$Y))
  print(1 + sum(cov(data$X)))
}

plot.risk <- function(data, est) {
  # est = p x niters 
  est.bias = apply(est, 2, function(colum) 
    log(t(colum-data$theta) %*% data$A %*% (colum-data$theta)))
  
  est.bias = apply(est, 2, function(colum) 
    t(colum-data$theta) %*% data$A %*% (colum-data$theta))
  print(sprintf("Risk of first estimate = %.3f Risk of last estimate %.3f", 
                head(est.bias, 1), tail(est.bias, 1)))
  plot(est.bias, type="l", lty=3)
}

lr <- function(alpha, n) {
  ## learning rate
  alpha / (alpha + n)
}

sgd <- function(data, alpha, plot=T) {
  # check.data(data)
  # Implements implicit
  n = nrow(data$X)
  p = ncol(data$X)
  # matrix of estimates of SGD (p x iters)
  theta.sgd = matrix(0, nrow=p, ncol=1)
  # params for the learning rate seq.
  # gamma0 = 1 / (sum(seq(0.01, 1, length.out=p)))
  trace = sum(diag(data$A))  # NOTE: data snooping.
  I = diag(p)
  
  for(i in 1:n) {
    xi = data$X[i, ]
    theta.old = theta.sgd[, i]
    # NOTE: This is assuming implicit. Use rate 
    #  an = alpha / (alpha * trace(A) + n)  if standard.
    ai = lr(alpha, i)
    
    # make computations easier.
    xi.norm = sum(xi^2)
    lpred = sum(theta.old * xi)
    fi = 1 / (1 + ai * sum(xi^2))
    yi = data$Y[i]
    # Implicit SGD
    theta.new = (theta.old  - ai * fi * lpred * xi) +  
      (ai * yi * xi - ai^2 * fi * yi * xi.norm * xi)
    # Standard SGD
    # Uncomment this line for standard SGD.
    # theta.new = (theta.old - ai * lpred * xi) + ai * yi * xi
    
    theta.sgd = cbind(theta.sgd, theta.new)
  }
  
  if(plot) {
    print("Last estimate")
    print(theta.sgd[, ncol(theta.sgd)])
    plot.risk(data, theta.sgd)
  } else {
    return(theta.sgd)
  }
}

sqrt.norm <- function(X) {
  sqrt(mean(X^2)  )
}


# Replicate this code on Odyssey
# to get the frequentist variance of SGD.
run.sgd.many <- function(nreps, alpha, 
                         nlist=as.integer(seq(100, 1e5, length.out=10)),
                         p=10, verbose=F) {
  ## Run SGD (implicit) with multiple n(=SGD iterations)
  #
  # Example:
  #   run.sgd.many(nreps=1000, alpha=2, p=4)
  #
  # Plots || Empirical variance - Theoretical ||
  #
  A = generate.A(p)
  dist.list = c()  # vector of || . || distances
  for(n in nlist) {
    # matrix of the last iterate theta_n
    last.theta = matrix(NA, nrow=0, ncol=p)
    # Compute theoretical variance
    data0 = sample.data(n, A)
    I = diag(p)
    Sigma.theoretical <- alpha * solve(2 * alpha * A - I) %*% A
    stopifnot(all(eigen(Sigma.theoretical)$values > 0))
    
    # Get many replications for each n
    for(j in 1:nreps) {
      data = sample.data(n, A)
      out = sgd(data, alpha, plot=F)
      # Every replication stores the last theta_n
      last.theta <- rbind(last.theta, out[, n])
    }
    
    print(sprintf("n = %d", n))
    print("Avg. estimates for theta*")
    print(colMeans(last.theta))
    # Store the distance.
    empirical.var = (1 / lr(alpha, n)) * cov(last.theta)
    if(verbose) {
      print("Empirical variance")
      print(round(empirical.var, 4))
      print("Theoretical ")
      print(round(Sigma.theoretical,4))
    }
    dist.list <- c(dist.list, sqrt.norm(empirical.var - Sigma.theoretical))
    plot(dist.list, type="l")
    print("Vector of ||empirical var.  - theoretical var.||")
    print(dist.list)
  }
}

