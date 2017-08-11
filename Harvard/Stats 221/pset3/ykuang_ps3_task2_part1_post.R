library('ggplot2')

file.path = 'data/ps3_task2_part1_%s.rda'
file.suffix.list = c('batch', 'SGD', 'ASGD', 'ASGD_BAD', 'implicit')

t <- c()
log.loss <- c()
label <- c()
for (i in 1:length(file.suffix.list)) {
  load(sprintf(file.path, file.suffix.list[i]))
  
  loss.diff <- result$risks
  loss.len <- length(loss.diff)
  
  new.log.loss <- loss.diff
  #new.t <- sampler
  new.t <- c(floor(1.1^(1:144)), 1e6)
  new.log.loss <- new.log.loss[new.t]
  #new.label <- rep(file.suffix.list[i], loss.len)
  new.label <- rep(file.suffix.list[i], length(new.t))
  
  t <- c(t, new.t)
  log.loss <- c(log.loss, new.log.loss)
  label <- c(label, new.label)
}

plot.df <- data.frame(t, log.loss, label)
ggplot(data=plot.df, aes(x=t, y=log.loss)) + geom_line(aes(colour=label), size=1) + scale_x_log10(breaks=c(1e0, 1e1,1e2,1e3,1e4,1e5,1e6)) + scale_y_log10()+
  ylab('Excess risk') + xlab('Training size t')+ ggtitle("Comparison with different Gradient Descent method")
  theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))

savePlot = FALSE
if (savePlot)
  ggsave(file='fig/ykuang_ps2_part1.png', width=16, heigh=9, dpi=300) 
