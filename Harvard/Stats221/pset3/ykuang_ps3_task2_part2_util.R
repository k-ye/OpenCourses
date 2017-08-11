library('MASS')
source('ykuang_ps3_task2_util.R')

theta.true.gen <- function(d=100) {
  return(rep(1, d))
}

theta.init.gen <- function(d=100) {
  #return(runif(d, min=-10, max=10))
  return(rep(0, d))
}

A.gen <- function(d=100) {
  # get A for task2, part 1
  # eigenvalue: evenly distributed from 0.01 to 1
  # dimension: 100x100
  
  diag.all <- seq(0.01, 1, length.out=d)
  U <- U.gen()
  A <- U %*% diag(diag.all) %*% t(U)
  return(A)
}

data.gen <- function(A=A.gen(), d=100, t=1e5) {
  # Generate the data for task2 part 2 according to the toy model
  # x is 100 dimension ~ N(0, A)
  # y is scalar ~ N(0, x.T * theta.true)
  # Return:
  #   A list of data, X indicates sample feature, Y indicates label
  stopifnot(d>0, t>0)
  #A <- A.gen()
  theta.true <- theta.true.gen()
  theta.init <- theta.init.gen()
  X <- mvrnorm(n=t, mu=rep(0, d), Sigma=A)
  Y <- apply(X, 1, function(x) rnorm(n=1, mean=t(x) %*% theta.true, sd=1))
  
  data <- list(X=X, Y=Y, theta.true=theta.true, theta.init=theta.init, A=A)
  return(data)
}

l.func <- function(x, y, theta) {
  # Calculate the value of l(theta, d) in task2 part 1
  # Args:
  #   x: a single sample vector
  #   y: a single scalar
  #   theta: params
  #   A: matrix used for l()
  result <- as.numeric(0.5 * (y - as.numeric(t(theta) %*% x))^2)
  return(result)
}

l.grad.func <- function(x, y, theta) {
  # Calculate the value of partial derivative of l(theta, d) 
  # on theta in task2 part 1
  # Args: same as l.func()
  #
  # Return:
  #   a vector that has the same dimension of theta, grad
  
  grad <- as.vector((as.numeric(t(theta) %*% x) - y) * x)
  return(grad)
}

# for batch gradient descent
batch <- function(data, saveTheta=F) {
  # use Stochastic Gradient Descent to find proper theta
  # Args:
  #   data: all the data needed
  X <- data$X
  Y <- data$Y
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 50
  theta.t <- theta.init
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)
  
  X.sub.mat <- X[1:100,]
  X.y.sub <- t(X.sub.mat) %*% as.vector(Y[1:100])
  X.sub.mat <- t(X.sub.mat) %*% X.sub.mat
  theta.t <- solve(X.sub.mat) %*% X.y.sub
  
  thetas[1,] <- theta.init
  thetas[2:100,] <- theta.t
  
  for(t in 101:data.T) {
    X.t <- as.vector(X[t,])
    X.sub.mat <- X.sub.mat + X.t %*% t(X.t)
    X.y.sub <- X.y.sub + X.t * Y[t]
    theta.t <- solve(X.sub.mat) %*% X.y.sub
    if (t %% sampler == 0 || t == data.T) {
      risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
      #print(sprintf("current iteration: %d", t))
    }
  }
  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for stochastic gradient descent
lr.sgd <- function(params, t) {
  # learning rate for SGD
  #   lr.t = lr.0 * (1 + a*lr.0*t)^(-1)
  # Args:
  #   a, gamma0: init params
  #   t: current iteration step
  gamma0 <- params$gamma0
  a <- params$a
  return(gamma0 * (1+a*gamma0*t)^-1)
}

SGD <- function(data, lr.params=NA, lr.func=lr.sgd, sampler=50, saveRisk=T, saveTheta=F) {
  # use Stochastic Gradient Descent to find proper theta
  # lr.params: learning rate parameters
  # lr.func: function to calculate learning rate
  # sampler: interval to sample risk and theta.est
  
  X <- data$X
  Y <- data$Y
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  theta.t <- theta.init
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)
  
  if (all(is.na(lr.params))) {
    a <- min(eigen(A)$values)
    gamma0 <- 1 / sum(diag(A))
    lr.params <- list(a=a, gamma0=gamma0)
  }
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.func(lr.params, t)

    theta.t <- theta.t - lr.t*l.grad.func(data.t, Y[t], theta.t)
    theta.t <- as.vector(theta.t)
    
    if (t %% sampler == 0 || t == data.T) {
      if (saveRisk) {
        risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      }
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
      #print(sprintf("current iteration: %d", t))
    }
  }
  
  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for averaged SGD
lr.asgd <- function(params, t) {
  # learning rate for ASGD
  #   lr.t = lr.0 * (1 + a*lr.0*t)^(-2/3)
  # Args:
  #   a, gamma0: init params
  #   t: current iteration step
  gamma0 <- params$gamma0
  a <- params$a
  return(gamma0 * (1+a*gamma0*t)^-(2/3))
}

ASGD <- function(data, lr.params=NA, lr.func=lr.asgd, sampler=50, saveRisk=T, saveTheta=F) {
  X <- data$X
  Y <- data$Y
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  theta.t <- theta.init
  theta.mean <- theta.init
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)
  
  if (all(is.na(lr.params))) {
    a <- min(eigen(A)$values)
    gamma0 <- 1 / sum(diag(A))
    lr.params <- list(a=a, gamma0=gamma0)
  }
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.func(lr.params, t)
    
    theta.t <- theta.t - lr.t*l.grad.func(data.t, Y[t], theta.t)
    theta.t <- as.vector(theta.t)
    theta.mean <- (1 - 1/(t+1)) * theta.mean + theta.t / (t+1)

    if (t %% sampler == 0 || t == data.T) {
      if (saveRisk)
        risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.mean
      }
      #print(sprintf("current iteration: %d", t))
    }
  }
  
  result <- list(risks=risks, theta.est=theta.mean, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# for Implicit SGD
lr.implicit<- function(params, t) {
  # learning rate for Implicit
  # Args:
  #   t: current iteration step
  return(lr.sgd(params, t)) 
}

Implicit <- function(data, lr.params=NA, lr.func=lr.implicit, sampler=50, saveRisk=T, saveTheta=F) {
  # use Stochastic Gradient Descent to find proper theta
  X <- data$X
  Y <- data$Y
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  theta.t <- theta.init
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)

  if (all(is.na(lr.params))) {
    a <- min(eigen(A)$values)
    gamma0 <- 1 / sum(diag(A))
    lr.params <- list(a=a, gamma0=gamma0)
  }
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.func(lr.params, t)
    # Sherman-Morrison: (I + uv^T)'
    theta.t <- theta.t + lr.t * (Y[t] - t(data.t) %*% theta.t) / (1 + lr.t * t(data.t) %*% data.t) * data.t
    theta.t <- as.vector(theta.t)
    
    if (t %% sampler == 0 || t == data.T) {
      if (saveRisk)
        risks[t / sampler] <- expect.risk(theta.t, theta.true, A)
      if (saveTheta) {
        thetas[, t / sampler] <- theta.t
      }
      #print(sprintf("current iteration: %d", t))
    }
  }

  result <- list(risks=risks, theta.est=theta.t, theta.true=theta.true, A=A)
  if (saveTheta) {
    result[['thetas']] <- thetas
  }
  return(result)
}

# outdated, not supporting anymore
ASGD.sparse <- function(data, alpha=-1, saveTheta=F) {
  # use Averaged Stochastic Gradient Descent to find proper theta, sparce condition
  X <- data$X
  Y <- data$Y
  theta.true <- data$theta.true
  theta.init <- data$theta.init
  A <- data$A
  
  data.T <- dim(X)[1]
  d <- dim(X)[2]
  # init theta
  sampler <- 50
  theta.mean <- theta.init
  thetas <- matrix(NA, nrow=d, ncol=data.T/sampler)
  risks <- rep(NA, data.T/sampler)
  
  a <- ifelse(alpha==-1, min(eigen(A)$values), alpha)
  gamma0 <- 1 / sum(diag(A))
  
  alpha.t <- 1
  beta.t <- 1
  eta.t <- 0
  tao.t <- 0
  u.t <- theta.init
  u.hat.t <- theta.init
  lambda <- 0
  
  for(t in 1:data.T) {
    data.t <- as.vector(X[t,])
    lr.t <- lr.asgd(a, gamma0, t)
    
    grad.t <- l.grad.func(data.t, Y[t], u.t/alpha.t)
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