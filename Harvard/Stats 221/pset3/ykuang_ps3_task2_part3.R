# @pre Should create subfiles of "data/" before using
# read args
source("ykuang_ps3_task2_part2_util.R")
args <- as.numeric(commandArgs(trailingOnly = TRUE))

if (length(args) == 0) {
  args <- c(1e5, 1)
} else if (length(args) != 2) {
  stop("Not correct no. of args")
} else {
  print("args fine")
}

numData <- args[1]
job.id <- args[2]

stopifnot(numData>0, job.id>0)

alphas.len <- 10
# generate alpha candidats
alphas <- c(60, 80, 100, 120, 150, 200, 250, 300, 400, 500)
# the original form is: gamma0 / (1 + gamma0*alpha_*t)
# since we are experimenting with: alpha / (alpha/gamma0 + t)
# to transform to the original form, we know that:
# now = alpha*gamma0 / (alpha + gamma0*t) = gamma0 / (1 + gamma0*t/alpha_) = origin
# therefore, alpha_ = 1 / alpha
m <- 200

methodName = ""
method = NA

if (job.id == 1) {
  method = SGD
  methodName = "SGD"
} else if (job.id == 2) {
  method = ASGD
  methodName = "ASGD"
} else if (job.id == 3){
  method = Implicit
  methodName = "Implicit"
} else {
  print(job.id)
}

lr.test <- function(params, t) {
  a <- params$a
  gamma0 <- params$gamma0
  isImplicit <- params$isImplicit
  if (isImplicit)
    return(a / (a + t))
  else
    return(a / (a/gamma0 + t))
}

for (alpha.idx in 1:alphas.len) {
  A = A.gen()
  alpha <- alphas[alpha.idx]
  gamma0 <- 1 / sum(diag(A))
  
  lr.params <- list()
  if (methodName == 'Implicit')
    lr.params <- list(a=alpha, gamma0=gamma0, isImplicit=TRUE)
  else
    lr.params <- list(a=alpha, gamma0=gamma0, isImplicit=FALSE)
  results = list()
  for (i in 1:m) {
    set.seed(job.id*length(alphas)*m + i)
    data <- data.gen(A=A, t=numData)
    
    result <- method(data, lr.params, lr.test, sampler=50, saveTheta=T)
    results[[i]] = list(thetas=result$thetas, theta.est=result$theta.est)
  }
  
  results[['theta.true']] <- data$theta.true
  results[['A']] <- A
  
  filename = sprintf("data/ps3_task2_part3_%s_%d_newlr.rda", methodName, alpha.idx)
  save(results, file=filename)
}