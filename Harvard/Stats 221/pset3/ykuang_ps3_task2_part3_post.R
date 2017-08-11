library('ggplot2')
source('ykuang_ps3_task2_util.R')

file.path = "data/ps3_task2_part3_%s_%d.rda"
file.suffix.list = c('SGD', 'ASGD', 'Implicit')

alphas.len = 10
alphas <- c(60, 80, 100, 120, 150, 200, 250, 300, 400, 500)

#alphas.len = 11
#alphas <- seq(0.05,9.55, length.out=alphas.len)
m = 200

lr.test <- function(params, t) {
  a <- params$a
  gamma0 <- params$gamma0
  isImplicit <- params$isImplicit
  if (isImplicit)
    return(a / (a + t))
  else
    return(a / (a/gamma0 + t))
}

sqrt.norm <- function(X) {
  sqrt(mean(X^2)  )
}

# Bias
for (method in file.suffix.list) {  
  t <- c()
  label <- c()
  bias.all <- c()
  plot.df <- NA
  
  for (alpha.idx in 1:alphas.len) {
    file <- sprintf(file.path, method, alpha.idx)
    load(file)
    
    thetas <- array(0, dim = dim(results[[1]]$thetas))
    d <- length(results$theta.true)
    sample.num <- dim(thetas)[2]
    theta.true <- results$theta.true
    bias <- rep(0, sample.num)
    alpha <- alphas[alpha.idx]
    for (i in 1:m) {
      theta.t <- results[[i]]$thetas
      bias.t.norm <- apply(theta.t, 2, function(x) norm(x - theta.true, type="2"))
      bias <- bias + bias.t.norm
    }
    #limit <- alpha^2 * solve(2*alpha*A-diag(1,d)) %*% A
    bias <- bias / m
    new.t <- seq(from=1, to=1e4, by=50)
    new.label <- rep(sprintf('alpha=%.2f', alphas[alpha.idx]), length(new.t))
    t <- c(t, new.t)
    bias.all <- c(bias.all, bias)
    label <- c(label, new.label)
  }
  plot.df <- data.frame(t, bias.all, label)
  
  ggplot(data=plot.df, aes(x=t, y=bias.all)) + geom_line(aes(colour=label), size = 1) + scale_y_log10() + 
    ylab('Bias') + xlab('Training size t')+ ggtitle(sprintf("Bias with different alpha in %s", method)) +
  theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  
  savePlot = FALSE
  if (savePlot)
    ggsave(file=sprintf('fig/ykuang_ps2_part3_%s.png', method), width=10, heigh=7, dpi=300) 
}

# Variance
for (method in file.suffix.list) {  
  t <- c()
  label <- c()
  var.all <- c()
  limit.all <- c()
  plot.df <- NA
  
  for (alpha.idx in 1:alphas.len) {
    file <- sprintf(file.path, method, alpha.idx)
    load(file)
    
    thetas <- array(0, dim = dim(results[[1]]$thetas))
    d <- length(results$theta.true)
    sample.num <- dim(thetas)[2]
    var <- rep(NA, sample.num)
    limit <- rep(NA, sample.num)
    alpha <- alphas[alpha.idx]
    
    A <- results$A
    I = diag(d)
    if (method == 'ASGD')
      Sigma.theoretical <- solve(A)
    else
      Sigma.theoretical <- alpha * solve(2 * alpha * A - I) %*% A
    gamma0 <- 1 / sum(diag(A))
    
    for (n in 1:sample.num) {
      var.mat <- array(NA, dim=c(m, d))
      for (i in 1:m) {
        theta.t <- results[[i]]$thetas[,n]
        var.mat[i,] <- theta.t
      }
      var.n <- cov(var.mat)
      # calculate trace
      var[n] <- sum(diag(var.n))
      # calculate limit
      if (method=='ASGD') {
        limit[n] <- sqrt.norm(var.n - Sigma.theoretical / n / 50)
      } else {
        isImplicit = ifelse(method=='Implicit', TRUE, FALSE)
        var.n <- var.n / lr.test(list(a=alpha, gamma0=gamma0, isImplicit=isImplicit), n * 50)
        limit[n] <- sqrt.norm(var.n - Sigma.theoretical)
      }
    }
    new.t <- seq(from=1, to=1e4, by=50)
    new.label <- rep(sprintf('alpha=%.2f', alphas[alpha.idx]), length(new.t))
    t <- c(t, new.t)
    var.all <- c(var.all, var)
    limit.all <- c(limit.all, limit)
    label <- c(label, new.label)
  }
  plot.df <- data.frame(t, var.all, alpha=label)
  
  ggplot(data=plot.df, aes(x=t, y=var.all)) + geom_line(aes(colour=label), size = 1) + #scale_y_log10()+
    ylab('Variance') + xlab('Training size t')+ ggtitle(sprintf("Variance with different alpha in %s", method)) +
    theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  
  savePlot = FALSE
  if (savePlot)
    ggsave(file=sprintf('fig/ykuang_ps2_part3_%s_var.png', method), width=10, heigh=7, dpi=300) 
  
  plot.df <- data.frame(t, limit.all, alpha=label)
  
  ggplot(data=plot.df, aes(x=t, y=limit.all)) + geom_line(aes(colour=label), size = 1) + #scale_y_log10()+
    ylab('Variance error') + xlab('Training size t')+ ggtitle(sprintf("Error of variance with different alpha in %s", method)) +
    theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  
  if (savePlot)
    ggsave(file=sprintf('fig/ykuang_ps2_part3_%s_var_error.png', method), width=10, heigh=7, dpi=300) 
}