library(dplyr)
library(ggplot2)
library(reshape2)
library(grid)

data.len <- 277
thetas.all <- array(NA, dim=c(17, data.len))
start <- 1
h <- 5
thetas.time <- rep(NA, data.len)
for (i in 1:11) {
  load(sprintf("out/pset5_1router_local_c2_%d.RData", i))
  len <- dim(thetas.c1)[2]
  thetas.all[, start:(start-1+len)] <- thetas.c1
  thetas.time[start:(start-1+len)] <- colnames(thetas.c1)
  start <- start + len
}
colnames(thetas.all) <- thetas.time
rownames(thetas.all) <- c(
  "fddi->fddi","fddi->switch","fddi->local","fddi->corp","switch->fddi",
  "switch->switch","switch->local","switch->corp","local->fddi","local->switch",
  "local->local","local->corp","corp->fddi","corp->switch","corp->local",
  "corp->corp", "phi"
)

thetas.all <- t(thetas.all)
thetas.smooth.all <- thetas.all
for (i in (1+h):(data.len-h)) {
  thetas.smooth.all[i, ] <- as.vector(apply(thetas.smooth.all[(i-h):(i+h), ], 2, mean))
}

# save post mean, variance for smooothed EM
log.thetas.mean <- apply(log(thetas.all), 2, mean)
log.thetas.cov <- cov(log(thetas.all))
V <- cov(diff(log(thetas.all)))
smoothed.params <- list(eta.hat0=log.thetas.mean, Sigma.hat0=log.thetas.cov, V=V)
save(smoothed.params, file=sprintf("out/1router_smoothed_params.RData"))

thetas.all.df <- data.frame(time=as.POSIXct(rownames(thetas.all), format="%H:%M:%S"), 
                          thetas.all[, c(13:16, 9:12, 5:8, 1:4, 17)])
rownames(thetas.all.df) <- NULL

thetas.OD <- melt(thetas.all.df, id='time')
thetas.OD <- subset(thetas.OD, thetas.OD$variable != 'phi')

OD.plot <- ggplot(thetas.OD, aes(x=time, y=value)) +
  geom_line(colour="#39CCCC")  + scale_x_datetime(
    breaks=as.POSIXct(sprintf("%i:00:00", seq(0, 24, 4)), format="%H:%M:%S"),
    labels=seq(0, 24, 4)
  ) +
  scale_y_continuous(
    limits=c(0, 1e6),
    breaks=seq(0, 1e6, 200e3),
    labels=c("0", "200K", "400K", "600K", "800K", "1M")
  ) +
  xlab("hour of day") +
  ylab("bytes/sec") +
  facet_wrap( ~ variable, ncol=4)
# for estimated
data.org <- data.frame(time=thetas.all.df$time)
data.org$`origin corp` <- rowSums(thetas.all[, grepl("^corp", colnames(thetas.all))])
data.org$`origin local` <- rowSums(thetas.all[, grepl("^local", colnames(thetas.all))])
data.org$`origin switch` <- rowSums(thetas.all[, grepl("^switch", colnames(thetas.all))])
data.org$`origin fddi` <- rowSums(thetas.all[, grepl("^fddi", colnames(thetas.all))])
data.org <- melt(data.org, id='time')
data.org$`label` <- 'estimate'
# for smoothed
data.smooth.org <- data.frame(time=thetas.all.df$time)
data.smooth.org$`origin corp` <- rowSums(thetas.smooth.all[, grepl("^corp", colnames(thetas.smooth.all))])
data.smooth.org$`origin local` <- rowSums(thetas.smooth.all[, grepl("^local", colnames(thetas.smooth.all))])
data.smooth.org$`origin switch` <- rowSums(thetas.smooth.all[, grepl("^switch", colnames(thetas.smooth.all))])
data.smooth.org$`origin fddi` <- rowSums(thetas.smooth.all[, grepl("^fddi", colnames(thetas.smooth.all))])
data.smooth.org <- melt(data.smooth.org, id='time')
data.smooth.org$`label` <- 'smoothed'
data.all.org <- rbind(data.org, data.smooth.org)

org.plot <- data.all.org %>%
  ggplot(aes(x=time, y=value, fill=label, color=label)) +
  geom_line() +
  scale_x_datetime(
    breaks=as.POSIXct(sprintf("%i:00:00", seq(0, 24, 4)), format="%H:%M:%S"),
    labels=seq(0, 24, 4)
  ) +
  scale_y_continuous(
    limits=c(0, 1e6)
  ) + 
  scale_color_manual(labels=c("estimate", "smoothed"), 
                     values=c("#39CCCC", "#B10DC9"),
                     guide=FALSE) +
  xlab("") +
  ylab("") +
  theme(axis.ticks.y=element_blank(), axis.text.y=element_blank(), 
        legend.position="top", legend.title=element_blank()) +
  facet_wrap( ~ variable, nrow=4)

data.dst <- data.frame(time=thetas.all.df$time)
data.dst$`fddi destination` <- rowSums(thetas.all[, grepl("fddi$", colnames(thetas.all))])
data.dst$`switch destination` <- rowSums(thetas.all[, grepl("switch$", colnames(thetas.all))])
data.dst$`local destination` <- rowSums(thetas.all[, grepl("local$", colnames(thetas.all))])
data.dst$`corp destination` <- rowSums(thetas.all[, grepl("corp$", colnames(thetas.all))])
data.dst <- melt(data.dst, id="time")
data.dst$`label` <- "estimate"

data.smooth.dst <- data.frame(time=thetas.all.df$time)
data.smooth.dst$`fddi destination` <- rowSums(thetas.smooth.all[, grepl("fddi$", colnames(thetas.smooth.all))])
data.smooth.dst$`switch destination` <- rowSums(thetas.smooth.all[, grepl("switch$", colnames(thetas.smooth.all))])
data.smooth.dst$`local destination` <- rowSums(thetas.smooth.all[, grepl("local$", colnames(thetas.smooth.all))])
data.smooth.dst$`corp destination` <- rowSums(thetas.smooth.all[, grepl("corp$", colnames(thetas.smooth.all))])
data.smooth.dst <- melt(data.smooth.dst, id="time")
data.smooth.dst$`label` <- "smoothed"
data.all.dst <- rbind(data.dst, data.smooth.dst)

dst.plot <- data.all.dst %>%
  ggplot(aes(x=time, y=value, fill=label, color=label)) +
  geom_line() +
  scale_x_datetime(
    breaks=as.POSIXct(sprintf("%i:00:00", seq(0, 24, 4)), format="%H:%M:%S"),
    labels=seq(0, 24, 4)
  ) + 
  scale_color_manual(labels=c("estimate", "smoothed"), 
                     values=c("#39CCCC", "#B10DC9")) +
  xlab("") +
  ylab("") +
  scale_y_continuous(
    limits=c(0, 1e6),
    breaks=seq(0, 1e6, 200e3),
    labels=c("0", "200K", "400K", "600K", "800K", "1M")
  ) +
  theme(legend.position="top", legend.title=element_blank()) +
  facet_wrap( ~ variable, ncol=4)

data.total <- data.frame(time=thetas.all.df$time)
data.total$`total` <- rowSums(subset(thetas.all, select=-c(phi)))
data.total <- melt(data.total, id="time")
data.total$`label` <- "estimate"

data.smooth.total <- data.frame(time=thetas.all.df$time)
data.smooth.total$`total` <- rowSums(subset(thetas.smooth.all, select=-c(phi)))
data.smooth.total <- melt(data.smooth.total, id="time")
data.smooth.total$`label` <- "smoothed"
data.all.total <- rbind(data.total, data.smooth.total)

total.plot <- data.all.total %>%
  ggplot(aes(x=time, y=value, fill=label, color=label)) +
  geom_line() +
  scale_y_continuous(
    limits=c(0, 1e6)
  ) +
  scale_color_manual(labels=c("estimate", "smoothed"), 
                     values=c("#39CCCC", "#B10DC9")) +
  xlab("") +
  ylab("") +
  theme(axis.ticks.x=element_blank(), axis.text.x=element_blank(),
        axis.ticks.y=element_blank(), axis.text.y=element_blank(),
        legend.position="top", legend.title=element_blank()) +
  facet_wrap( ~ variable, nrow=1)

grid.newpage()
pushViewport(viewport(layout=grid.layout(2,2,heights=c(0.3,0.7), widths = c(0.8, 0.2))))
print(dst.plot, vp=viewport(layout.pos.row=1,layout.pos.col=1))
print(total.plot, vp=viewport(layout.pos.row=1,layout.pos.col=2))
print(OD.plot, vp=viewport(layout.pos.row=2,layout.pos.col=1))
print(org.plot, vp=viewport(layout.pos.row=2,layout.pos.col=2))
