library(mvtnorm)

data.load <- function(data.path) {
  data <- read.table(data.path, header=TRUE, sep=',')
  new.nme <- gsub("->", " ", data$nme)
  data$nme <- new.nme
  data <- subset(data, grepl("^dst |^src", data$nme))
  data$time <- as.POSIXct(data$time, format="(%m/%d/%y %H:%M:%S)")
  
  time.start <- min(data$time)
  time.end <- max(data$time)
  obsv.data <- c()
  obsv.time <- c()
  while(time.start <= time.end) {
    data.sub <- subset(data, data$time > (time.start - 30) & data$time < (time.start + 30))
    obsv.data <- cbind(obsv.data, data.sub$value[1:7])
    obsv.time <- c(obsv.time, strftime(unique(data.sub$time),'%H:%M:%S'))
    time.start <- time.start + 60 * 5
  }
  colnames(obsv.data) <- obsv.time
  rownames(obsv.data) <- data.sub$nme[1:7]
  return(obsv.data)
}

data2.load <- function(data.path) {
  data <- read.table(data.path, header=TRUE, sep=',')
  new.nme <- gsub("->", " ", data$nme)
  data$nme <- new.nme
  data <- subset(data, grepl("^dst |^ori", data$nme))
  data$time <- as.POSIXct(data$time, format="(%m/%d/%y %H:%M:%S)")
  
  time.start <- min(data$time)
  time.end <- max(data$time)
  obsv.data <- c()
  obsv.time <- c()
  while(time.start <= time.end) {
    data.sub <- subset(data, data$time > (time.start - 30) & data$time < (time.start + 30))
    obsv.data <- cbind(obsv.data, data.sub$value[1:15])
    obsv.time <- c(obsv.time, strftime(unique(data.sub$time),'%H:%M:%S'))
    time.start <- time.start + 60 * 5
  }
  colnames(obsv.data) <- obsv.time
  rownames(obsv.data) <- data.sub$nme[1:15]
  return(obsv.data)
}

A.gen <- function(dim) {
  A <- array(0, dim=c((2*dim-1), dim^2))
  for (i in 1:dim) {
    A[i, (((i-1)*dim+1):(i*dim))] <- rep(1, dim)
  }
  for (i in 1:(dim-1)) {
    template <- rep(0, dim)
    template[i] <- 1
    A[i+dim, ] <- rep(template, dim)
  }
  return(A)
}

m.kstep <- function(lambda.k, Sigma.k, A, y.window) {
  # Return: I x windows.size matrix
  I <- dim(A)[2]
  w <- dim(y.window)[2]
  
  term1 <- Sigma.k %*% t(A) %*% solve(A %*% Sigma.k %*% t(A))
  ms <- array(NA, dim=c(I, w))
  for(i in 1:w) {
    ms[, i] <- as.vector(lambda.k + term1 %*% (y.window[, i] - A %*% lambda.k))
  }
  #m.tk <- as.vector(lambda.k + Sigma.k %*% t(A) %*% solve(A %*% Sigma.k %*% t(A)) %*% (y.t - A %*% lambda.k))
  return(ms)
}

R.kstep <- function(lambda.k, Sigma.k, A) {
  # Return: I x I matrix
  term1 <- Sigma.k %*% t(A) %*% solve(A %*% Sigma.k %*% t(A))
  R.k <- as.matrix(Sigma.k - term1 %*% A %*% Sigma.k)
  return(R.k)
}

a.kstep <- function(R.k, m.T) {
  # Return: I x 1 vector
  len.T <- dim(m.T)[2]
  R.ii <- as.vector(diag(R.k))
  a.k <- R.ii + rowSums(m.T^2) / len.T
  return(a.k)
}

b.kstep <- function(m.T) {
  # Return: I x 1 vector
  b.k <- apply(m.T, 1, mean)
  return(b.k)
}

f.func <- function(theta.k, c, a.k, b.k) {
  # Return: (I+1) x 1 vector
  I <- length(theta.k) - 1
  lambda.k <- theta.k[1:I]
  phi.k <- theta.k[I+1]
  
  f1 <- c * phi.k * lambda.k^c + (2-c) * lambda.k^2 - 2*(1-c) * lambda.k * b.k - c * a.k
  f2 <- sum(lambda.k^(1-c) * (lambda.k - b.k))
  f <- c(f1, f2)
  return(f)
}

f.jacobian <- function(theta.k, c, a.k, b.k) {
  # Return: (I+1) x (I+1) matrix
  I <- length(theta.k) - 1
  lambda.k <- theta.k[1:I]
  phi.k <- theta.k[I+1]
  f.Jcb <- matrix(0, nr=(I+1), nc=(I+1))
  
  f1.lambda <- phi.k * c^2 * lambda.k^(c-1) + 2*(2-c) * lambda.k - 2*(1-c) * b.k
  f1.phi <- c * lambda.k^c
  f2.lambda <- (2-c) * lambda.k^(1-c) - (1-c) * lambda.k^(-c) * b.k
  f.Jcb[1:I, 1:I] <- diag(f1.lambda)
  f.Jcb[I+1, 1:I] <- f2.lambda
  f.Jcb[1:I, I+1] <- f1.phi
  return(f.Jcb)
}

locally_iid_EM <- function(data, c, A, w.range, verbose=F, max.iter=5e5, threshold=5e-2) {
  I <- dim(A)[2]
  w <- 11
  h <- floor(w / 2)
  w.len <- length(w.range)
  
  # store all thetas
  thetas <- array(NA, dim=c((I+1), w.len))
  colnames(thetas) <- colnames(data)[w.range]
  for (idx in 1:w.len) {
    t <- w.range[idx]
    y.window <- data[,(t-h):(t+h)]
    lambda0 <- rep(sum(y.window)/(w * sum(A)), I)
    phi0 <- c^2 * var(c(y.window)) / (mean(y.window)^c)
    theta.t <- c(lambda0, phi0)
    
    cur.iter <- 1
    while (TRUE) {
      # step 1, calculate all parameters in step k
      lambda.k <- theta.t[1:I]
      phi.k <- theta.t[I+1]
      Sigma.k <- phi.k * diag(lambda.k^c)
      
      ms.k <- m.kstep(lambda.k, Sigma.k, A, y.window)
      R.k <- R.kstep(lambda.k, Sigma.k, A)
      a.k <- a.kstep(R.k, ms.k)
      b.k <- b.kstep(ms.k)
      # step 2, Calculate f(*)
      f.theta.k <- f.func(theta.t, c, a.k, b.k)
      f.jcb.k <- f.jacobian(theta.t, c, a.k, b.k)
      # step 3, Update theta.k+1 = theta.k - solve(f.jcb(theta.k)) * f(theta.k)
      theta.t <- theta.t - solve(f.jcb.k) %*% f.theta.k
      
      f.norm <- norm(f.theta.k, type="2") #sum(abs(theta.t))
      cur.iter <- cur.iter + 1
      if (verbose & cur.iter %% 1e3 == 0) {
        print(sprintf("Current iteration: %d, norm2 is: %f", cur.iter, f.norm))
      }
      
      if (f.norm < threshold) {
        print(sprintf("Below threshold now, norm2 is: %f, using %d iterations", f.norm, cur.iter))
        break
      }
      if (cur.iter > max.iter) {
        print(sprintf("Exceeded max iteration limit, norm2 is: %f", f.norm))
        break
      }
    }
    thetas[, idx] <- theta.t
  }
  return(thetas)
}