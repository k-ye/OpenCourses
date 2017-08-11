simYgivenTheta <- function(theta, w, N)
{
  # compute the length of theta
  J <- length(theta)
  # init Y
  Y <- array(NA, dim=c(J, N))
  
  for (i in 1:J) {
    Y[i,] <- rpois(N, w[i]*theta[i])
  }
  return(Y)
}

# find quantile indicated by low and high in vector x, return a vector
findQuantile <- function(x, low, high)
{
  qt <- quantile(x, c(low, high))
  lowQt <- qt[[1]]
  highQt <- qt[[2]]
  return(c(lowQt, highQt))
}

# check if x is in a interval
isInCI <- function(x, CI)
{
  return((x >= CI[1]) & (x <= CI[2]))
}

# compute the coverage for a vector of log.theta
computeCoverage <- function(mcmc.log.theta, log.theta, low, high)
{
  J <- length(log.theta)
  covered <- rep(NA, J)
  for (k in 1:J) {
    CI <- findQuantile(mcmc.log.theta[k,], low, high)
    covered[k] <- isInCI(log.theta[k], CI)
  }
  return(covered)
}


report.post <- function(task.id, param.id, raw.str.format) 
{
  #
  # Args
  #   task.id: 3-5, specify which task to report
  #   param.id: 1-4, specify which parameter to report
  #   raw.str.format: '%s/%s_id%d_mu%.1f_std%.1f.rda', specify the format of path to look for raw data
  #
  # Report the post mean and post std of log theta
  
  param.set <- c()
  param1 = param2 = param3 = 0
  if (task.id < 5) {
    param.set <- c(c(1.6,0.7),c(2.5,1.3), c(5.2,1.3),c(4.9,1.6))
    dim(param.set) <- c(2,4)
  } else {
    param.set <- c(c(1.6, 0, 1.3), c(1.6, -0.7, 1.3), c(1.6, 0.7, 1.3), c(1.6, 0, 2.6))
    dim(param.set) <- c(3,4)
  }
  param.set <- t(param.set)
  
  taskStr = sprintf("task%d", task.id)
  
  param1 <- param.set[param.id, 1]
  param2 <- param.set[param.id, 2]
  if (task.id == 5) {
    param3 <- param.set[param.id, 3] 
  }
  
  post.mean <- c()
  post.std <- c()
  for (j in 1:3)
  {
    id = (param.id-1)*3 + j
    if (task.id < 5) {
      dataName = sprintf(raw.str.format, taskStr, taskStr, id, param1, param2)
    } else {
      dataName = sprintf(raw.str.format, taskStr, taskStr, id, param1, param2, param3)
    }
    
    load(dataName)
    
    post.mean <- c(post.mean, unlist(result$postMean))
    post.std <- c(post.std, unlist(result$postStd))
  }
  df <- data.frame(post.mean, post.std)
  summary(df)
  #return(df)
}