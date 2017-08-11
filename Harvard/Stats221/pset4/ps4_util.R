rgamma.trunc <- function(upper.bound, s, r) {
  x <- upper.bound + 10
  while(x > upper.bound) {
    x <- rgamma(1, shape=s, rate=r)
  }
  return(x)
}

rnorm.trunc <- function(mean, std, lower) {
  x <- lower - 10
  while (x <= lower) {
    x <- rnorm(1, mean=mean, sd=std)
  }
  return(x)
}