U.gen <- function(d=100) {
  # Generate a dxd matrix U which satisfied U %*% t(U) = I
  M <- array(runif(d^2), dim=c(d,d))
  U <- qr.Q(qr(M))
  return(U)
}

expect.risk <- function(theta.t, theta.true, A) {
  # Calculate the expected risk difference
  risk <- t(theta.t - theta.true) %*% A %*% (theta.t - theta.true)
  return(as.numeric(risk))
}

eucl.norm <- function(x) {
  return(sqrt(sum(x^2)))
}