data.path <- 'data/%s_chain_%d.rda'
name.impala <- 'impala'
name.water <- 'waterbuck'
name.data <- name.water

num.chains <- 10
probs <- rep(NA, num.chains)
accepts <- rep(NA, num.chains)
for (i in 1:num.chains) {
  load(sprintf(data.path, name.data, i))
  
  mcmc.chain <- result$chain
  sim.N <- mcmc.chain[, 1]
  accepts[i] <- result$accept
  probs[i] <- sum(sim.N > 100) / length(sim.N)
}