library(dplyr)

data1 <- read.table('dat/1router_allcount.dat', header=TRUE, sep=',')
head(data1)
new.nme <- gsub("->", " ", data1$nme)
data1$nme <- new.nme
data1 <- subset(data1, grepl("^dst |^src", data1$nme))
unique(data1$nme)
data1$time <- as.POSIXct(data1$time, format="(%m/%d/%y %H:%M:%S)")
data1$dir <- gsub(" [[:alpha:]]+", "", data1$nme)
data1$node <- gsub("[[:alpha:]]+ ", "", data1$nme)
#data1 <- select(data1, -c(row.names, nme))
# plot 1
ggplot(data1, aes(x=time, y=value, fill=dir, color=dir)) +
  geom_line() +
  scale_x_datetime(
    breaks=as.POSIXct(sprintf("(02/22/99 %i:00:00)", seq(0, 24, 4)), format="(%m/%d/%y %H:%M:%S)"),
    labels=seq(0, 24, 4)
  ) +
  scale_y_continuous(
    breaks=seq(0, 1e6, 200e3),
    labels=c("0", "200K", "400K", "600K", "800K", "1M")
  ) +
  xlab("hour of day") +
  ylab("bytes/sec") +
  scale_color_manual(
    labels=c("destination", "origin"),
    guide = guide_legend(reverse=TRUE),
    values=c("#39CCCC", "#B10DC9")
  ) +
  theme(
    legend.position="top",
    legend.title=element_blank()
  ) +
  facet_grid(node ~ .)

# 2
task2 <- function(time.center, dataset, winsize=11) {
  # Result:
  # index: index of each pair in dataset$nme, 1-8
  # means: log means
  # vars: log vars
  # group: 11:30 or 15:30
  # regression: fitted values using lm(~)
  # fit1: fitted values using c=1
  # fit2: fitted values using c=2
  time.lower <- time.center - winsize * 5 / 2 * 60
  time.upper <- time.center + winsize * 5 /2 * 60
  
  time.lower <- substr(time.lower, 12, 16)
  time.upper <- substr(time.upper, 12, 16)
  
  data.sub <- subset(dataset, substr(dataset$time, 12, 16) > time.lower & substr(dataset$time, 12, 16) < time.upper)
  nme.set <- unique(data.sub$nme)
  log.means <- rep(NA, length(nme.set))
  log.vars <- rep(NA, length(nme.set))
  nme.index <- 1:length(nme.set)
  group <- rep(paste("Time", substr(time.center, 12, 16), sep=" "), length(nme.set))
  for (i in 1:length(nme.set)) {  
    nme.df <- subset(data.sub, data.sub$nme == nme.set[i])
    log.means[i] <- log(mean(nme.df$value), base=10)
    log.vars[i] <- log(var(nme.df$value), base=10)
  }
  
  coeff <- lm(log.vars ~ log.means) %>% coef() %>% as.vector()
  # c=1, slope is 1, using offset(), can get interception
  coeff.c1 <- lm(log.vars ~ offset(log.means) + 1) %>% coef() %>% as.vector()
  coeff.c2 <- lm(log.vars ~ offset(2 * log.means) + 1) %>% coef() %>% as.vector()
  
  result <- data.frame(index=nme.index, means=log.means, vars=log.vars, group=group,
                            regression=(log.means * coeff[2] + coeff[1]),
                            fit1=(log.means + coeff.c1), fit2=(log.means*2 + coeff.c2))
  return(result)
}

time1.center <- as.POSIXct("01/01/2014 11:30:00", format=("%m/%d/%Y %H:%M:%S"))
time2.center <- as.POSIXct("01/01/2014 15:30:00", format=("%m/%d/%Y %H:%M:%S"))
result1 <- task2(time1.center, data1)
result2 <- task2(time2.center, data1)
results <- rbind(result1, result2)

ggplot(results, aes(x=means, y=vars)) + geom_point() +
  geom_line(aes(x=means, y=regression)) +
  geom_text(aes(label=index, x=means, y=vars, vjust=-.5), size=6) +
  geom_line(aes(x=means, y=fit1), linetype="dashed") + geom_line(aes(x=means, y=fit2), linetype="dashed") +
  scale_x_continuous(
    breaks=seq(4.2, 5.6, 0.2),
    labels=c("4.2", "", "4.6", "", "5.0", "", "5.4", ""),
    limits=c(4.1, 5.6)) + 
  scale_y_continuous(breaks=7:10, limits=c(7, 11)) +
  xlab("log10(mean)") + ylab("log10(var)") +
  facet_grid(. ~ group)

# plot for 2router.data
data2 <- read.table('dat/2router_linkcount.dat', header=TRUE, sep=',')
head(data2)
new.nme <- gsub("->", " ", data2$nme)
data2$nme <- new.nme
data2 <- subset(data2, grepl("^dst |^ori", data2$nme))
unique(data2$nme)
data2$time <- as.POSIXct(data2$time, format="(%m/%d/%y %H:%M:%S)")
data2$dir <- gsub(" [0-9A-Za-z-]+", "", data2$nme)
data2$node <- gsub("[0-9A-Za-z-]+ ", "", data2$nme)

result1 <- task2(time1.center, data2)
result2 <- task2(time2.center, data2)
results <- rbind(result1, result2)

ggplot(results, aes(x=means, y=vars)) + geom_point() +
  geom_line(aes(x=means, y=regression)) +
  geom_text(aes(label=index, x=means, y=vars, vjust=-.5), size=6) +
  geom_line(aes(x=means, y=fit1), linetype="dashed") + 
  geom_line(aes(x=means, y=fit2), linetype="dashed") +
  facet_grid(. ~ group) + 
  xlab("log10(mean)") + ylab("log10(var)")
