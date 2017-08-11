source('ykuang_ps3_task2_util.R')

A.gen <- function(d=100) {
  # get A for task2, part 1
  # eigenvalue: first 3 are 1, the rest are 0.02
  # dimension: 100x100
  diag1 <- rep(1, 3)
  diag2 <- rep(0.02, d-3)
  diag.all <- c(diag1, diag2)
  
  U <- U.gen()
  A <- U %*% diag(diag.all) %*% t(U)
  return(A)
}

data.gen <- function(d=100, t=1e6) {
  # Generate the data for task2 according to the toy model
  # Args:
  #   d: dimension of features for a sample
  #   t: number of data
  stopifnot(d>0, t>0)
  
  A <- A.gen()
  theta.true <- rep(0, d)
  theta.init <- rep(1, d)
  X <- rnorm(d*t, mean=0, sd=1)
  dim(X) = c(t, d)
  
  data <- list(X=X, theta.init=theta.init, theta.true=theta.true, A=A)
  return(data)
}

l.func <- function(x, theta, A) {
  # Calculate the value of l(theta, d) in task2 part 1
  # Args:
  #   x: a single sample
  #   theta: params
  #   A: matrix used for l()
  result <- as.numeric(t(theta-x) %*% A %*% (theta-x))
  return(result)
}

l.grad.func <- function(x, theta, A) {
  # Calculate the value of partial derivative of l(theta, d) 
  # on theta in task2 part 1
  # Return:
  #   a vector that has the same dimension of theta, grad
  grad <- as.vector(t(theta-x) %*% (A+t(A)))
  return(grad)
}

# for batch gradient descent
batch <- function(data, saveTheta=F) {
  # use Stochastic Gradient Descent to find proper theta
  # Args:
  #   X: data matrix
  #   theta.true: true theta values
  #   A: matrix used to form l() and g()=l'()
  X <- data$X
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]  
  
  # init theta
  sampler <- 1
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  theta.t <- theta.init
  risks <- rep(NA, data.T/sampler)
  
  for(t in 1:data.T) {
    data.t <- X[t,]
    
    theta.t <- ((t-1)*theta.t + data.t) / t
    theta.t <- as.vector(theta.t)
    
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
    }
  }
  
  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for stochastic gradient descent
lr.sgd <- function(t) {
  # learning rate for SGD
  # Args:
  #   t: current iteration step
  return(1/(1+0.02*t))
}

SGD <- function(data, saveTheta=F) {
  # use Stochastic Gradient Descent to find proper theta
  X <- data$X
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 1
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  theta.t <- theta.init
  risks <- rep(NA, data.T/sampler)
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.sgd(t)
    
    theta.t <- theta.t - lr.t*l.grad.func(data.t, theta.t, A)
    theta.t <- as.vector(theta.t)
    
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
    }
  }
  
  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for averaged SGD
lr.asgd <- function(t, useGood) {
  # learning rate for SGD
  # Args:
  #   t: current iteration step
  #   useGood: indicate if using good learning rate
  if (useGood) {
    return((1 + 0.02*t)^(-2/3)) 
  } else {
    return((1 + t)^(-0.5))
  }
}

ASGD <- function(data, useGood=T, saveTheta=F) {
  # use Averaged Stochastic Gradient Descent to find proper theta
  X <- data$X
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 1
  theta.t <- theta.init
  theta.mean <- theta.init
  thetas<- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.asgd(t, useGood)
    
    theta.t <- theta.t - lr.t*l.grad.func(data.t, theta.t, A)
    theta.t <- as.vector(theta.t)

    theta.mean <- (1 - 1/t) * theta.mean + theta.t / t
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.mean, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.mean
      }
    }
  }

  result <- list(risks=risks, theta.est=theta.mean, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

ASGD.sparse <- function(data, useGood=T, saveTheta=F) {
  # use Averaged Stochastic Gradient Descent to find proper theta, sparce condition
  X <- data$X
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 1
  thetas<- matrix(NA, nrow=d, ncol=data.T/sampler)
  theta.mean <- theta.init
  risks <- rep(NA, data.T/sampler)
  
  alpha.t <- 1
  beta.t <- 1
  eta.t <- 0
  tao.t <- 0
  u.t <- theta.init
  u.hat.t <- theta.init
  lambda <- 0
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.asgd(t, useGood)
    
    grad.t <- l.grad.func(data.t, u.t/alpha.t, A)
    alpha.t <- alpha.t / (1-lambda*lr.t)
    eta.t <- ifelse(t==1, 0, 1/t)
    beta.t <- beta.t / (1-eta.t)
    u.t <- u.t - alpha.t * lr.t * grad.t
    u.hat.t <- u.hat.t + tao.t * alpha.t * lr.t * grad.t
    tao.t <- tao.t + eta.t * beta.t / alpha.t
    
    theta.mean <- (tao.t*u.t + u.hat.t) / beta.t
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.mean, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.mean
      }
    }
  }

  result <- list(risks=risks, theta.est=theta.mean, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for Implicit SGD
lr.implicit<- function(t) {
  # learning rate for SGD
  # Args:
  #   t: current iteration step
  
  return(lr.sgd(t)) 
}

Implicit <- function(data, saveTheta=F) {
  # use Implicit Stochastic Gradient Descent to find proper theta
  X <- data$X
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 1
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  theta.t <- theta.init
  risks <- rep(NA, data.T/sampler)
  
  a <- min(eigen(A)$values)
  gamma0 <- 1# / sum(diag(A))
  I.identity <- diag(1, d)
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.implicit(t)
    
    theta.factor <- I.identity + lr.t*(A+t(A))
    theta.t <- solve(theta.factor) %*% (theta.t + lr.t*(A+t(A)) %*% data.t)
    theta.t <- as.vector(theta.t)
    
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
    }
  }

  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}