# @pre Should create subfiles of "task4/", "plot/" and "dat/" before using.
library('ggplot2')

param.set <- c(c(1.6,0.7),c(2.5,1.3), c(5.2,1.3),c(4.9,1.6))
dim(param.set) <- c(2,4)
param.set <- t(param.set)

task.num = 4
taskStr = sprintf("task%d", task.num)

originW <- (read.table('weights.txt'))$V1
logw <- log(rep(originW, 3))

for (i in 1:4)
{
  mu = param.set[i,1]
  std = param.set[i,2]
  
  logTheta <- c()
  cover68 <- c()
  cover95 <- c()
  
  for (j in 1:3)
  {
    id = (i-1)*3 + j
    dataName = sprintf('%s/%s_id%d_mu%.1f_std%.1f.rda', taskStr, taskStr, id, mu, std)
    load(dataName)
    
    logTheta <- c(logTheta, c(result$logTheta))
    cover68 <- c(cover68, c(result$cover68))
    cover95 <- c(cover95, c(result$cover95))
  }
  df <- data.frame(logTheta, cover68, cover95)
  df.mat <- t(as.matrix(df))
  lw.draw <- rep(logw, 2)
  lt.draw <- rep(logTheta, 2)
  cv.draw <- c(cover68, cover95)
  cv.mask <- rep(0.68, length(cover68))
  cv.mask <- c(cv.mask, rep(0.95, length(cover95)))
  df.draw <- data.frame(lt.draw, cv.draw, cv.mask)
  
  ggplot(df.draw, aes(lt.draw, cv.draw, colour = factor(cv.mask))) + geom_point(alpha=0.5)+labs(colour = "Coverage") +
    geom_smooth() + ylim(0,1) + ggtitle(sprintf('mu=%.1f std=%.1f', mu, std))+ ylim(0,1)+
    geom_hline(aes(yintercept=0.68), linetype="dashed")+geom_hline(aes(yintercept=0.95),linetype="dashed")+
    ylab('Coverage') + xlab('log(Theta)')+
    theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  ggsave(file=sprintf('plot/ykuang_ps2_%s_plot%d.png', taskStr, i), width=6, heigh=5, dpi=300) 
  write(df.mat, file=sprintf('dat/ykuang_ps2_%s_par%d_theta.dat', taskStr, i), ncolumns=3)
  #save(df, file=sprintf('out/ykuang_ps2_%s_part%d_theta.dat', taskStr, i)) 
  
  df <- data.frame(logw, cover68, cover95)
  df.mat <- t(as.matrix(df))
  df.draw <- data.frame(lw.draw, cv.draw, cv.mask)
  
  ggplot(df.draw, aes(lw.draw, cv.draw, colour = factor(cv.mask))) + geom_point(alpha=0.5)+labs(colour = "Coverage") +
    geom_smooth() + ylim(0,1) + ggtitle(sprintf('mu=%.1f std=%.1f', mu, std))+ ylim(0,1)+
    geom_hline(aes(yintercept=0.68), linetype="dashed")+geom_hline(aes(yintercept=0.95),linetype="dashed")+
    ylab('Coverage') + xlab('log(w)')+
    theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  ggsave(file=sprintf('plot/ykuang_ps2_%s_plot%d.png', taskStr, i+4), width=6, heigh=5, dpi=300) 
  write(df.mat, file=sprintf('dat/ykuang_ps2_%s_par%d_w.dat', taskStr, i), ncolumns=3)
  #save(df, file=sprintf('out/ykuang_ps2_%s_part%d_w.dat', taskStr, i)) 
}