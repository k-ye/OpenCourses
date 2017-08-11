
rASL <- function(n, x0=0, m=0, b=1) {
    X <- rexp(n)
    Z <- rnorm(n)
    Y <- x0 + m*X + sqrt(b*X)*Z
    return(Y)
}

