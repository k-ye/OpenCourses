# @pre Should create subfiles of "task5/", "plot/" and "dat/" before using.

library('ggplot2')

param.set <- c(c(1.6, 0, 1.3), c(1.6, -0.7, 1.3), c(1.6, 0.7, 1.3), c(1.6, 0, 2.6))
dim(param.set) <- c(3,4)
param.set <- t(param.set)

task.num = 5
taskStr = sprintf("task%d", task.num)

originW <- (read.table('weights.txt'))$V1
logw <- log(rep(originW, 9))
for (i in 1:4)
{
  x0 = param.set[i,1]
  m = param.set[i,2]
  b = param.set[i,3]
  
  logTheta <- c()
  cover68 <- c()
  cover95 <- c()
  
  for (j in 1:3)
  {
    id = (i-1)*3 + j
    dataName = sprintf('%s/%s_id%d_x0%.1f_m%.1f_b%.1f.rda', taskStr, taskStr, id, x0, m, b)
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
  cv.mask <- rep(1, length(cover68))
  cv.mask <- c(cv.mask, rep(2, length(cover95)))
  df.draw <- data.frame(lt.draw, cv.draw, cv.mask)
  
  x_low = -3
  if (i == 4)
    x_low = -5
  
  ggplot(df.draw, aes(lt.draw, cv.draw, colour = factor(cv.mask))) + geom_point(alpha=0.5)+labs(colour = "Coverage") +
    geom_smooth() + ylim(0,1) + ggtitle(sprintf('x0=%.1f m=%.1f b=%.1f', x0, m, b))+ ylim(0,1)+xlim(x_low,5)+
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
    geom_smooth() + ylim(0,1) + ggtitle(sprintf('x0=%.1f m=%.1f b=%.1f', x0, m, b))+ ylim(0,1)+
    geom_hline(aes(yintercept=0.68), linetype="dashed")+geom_hline(aes(yintercept=0.95),linetype="dashed")+
    ylab('Coverage') + xlab('log(w)')+
    theme(axis.title.x = element_text(size=15),axis.text.x= element_text(size=12),axis.title.y = element_text(size=15),axis.text.y= element_text(size=12))
  ggsave(file=sprintf('plot/ykuang_ps2_%s_plot%d.png', taskStr, i+4), width=6, heigh=5, dpi=300) 
  write(df.mat, file=sprintf('dat/ykuang_ps2_%s_par%d_w.dat', taskStr, i), ncolumns=3)
  #save(df, file=sprintf('out/ykuang_ps2_%s_part%d_w.dat', taskStr, i))   
}