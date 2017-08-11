posterior.N <- function(data, N) {
  S <- sum(data)
  M <- length(data)
  data <- as.matrix(data)
  combs <- lchoose(N, data)
  pr <- sum(combs) + lbeta(S+1, M*N - S + 1) - log(N)
  return(pr)
}

data.path <- 'data/%s'
name.impala <- 'impala.txt'
name.water <- 'waterbuck.txt'
name.data <- name.water

data <- read.table(sprintf(data.path, name.data), head=T, stringsAsFactors=F)[,1]

limit <- Inf
N.iter <- max(data)
i <- 1
sum <- 0
threshold <- 1e-5
diff <- Inf

i <- 1
while(diff >= threshold) {
  prob <- exp(posterior.N(data, N.iter))
  sum.new <- sum + prob
  diff <- abs(1 / sum.new - 1 / sum)
  sum <- sum.new
  N.iter <- N.iter + 1
  i <- i + 1
  if (i %% 1e4 == 0) {
    print(sprintf("Iter %d, Diff: %f", i, diff))
  }
}

const <- sum

N.iter <- max(data)
prob.sum <- 0
while(N.iter <= 100) {
  prob <- exp(posterior.N(data, N.iter)) / const
  prob.sum <- prob.sum + prob
  N.iter <- N.iter + 1
}

print(1 - prob.sum)
