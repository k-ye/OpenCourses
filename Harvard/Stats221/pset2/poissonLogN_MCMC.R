#!/usr/bin/env Rscript

poisson.logn.mcmc <- function(Y, w, ndraws=1e4, burnin=1e3,
        mu0=0, sigmasq0=1) {
    # Check inputs
    J <- nrow(Y)
    N <- ncol(Y)
    if( J != length(w) ) {
        error("Error -- need length(w) == J")
    }
    M <- ndraws + burnin

    # Setup data structures
    logTheta <- matrix(NA, J, M)
    mu <- numeric(M)
    sigmasq <- numeric(M)
    
    # Initialize values
    mu[1] <- mu0
    sigmasq[1] <- sigmasq0
    logTheta[,1] <- rowMeans(log(Y+1)) - log(w)
    ySum <- rowSums(Y)

    # Loop for MCMC iterations
    for (m in 2:M) {
        # Draw mu | ...
        mu[m] <- rnorm( 1, mean(logTheta[,m-1]), sqrt(sigmasq[m-1]/J) )

        # Draw sigmasq | ...
        sigmasq[m] <- 1 / rgamma( 1, J/2, crossprod(logTheta[,m-1]-mu[m])/2 )

        # Draw logTheta | ...
        mhOut <- thetaMHStep(ySum, N, w,
                mu[m], sigmasq[m], logTheta[,m-1])
        logTheta[,m] <- mhOut$logTheta
    }

    # Return results
    retval <- list( Y=Y, w=w, ndraws=ndraws, burnin=burnin,
            mu=tail(mu,ndraws), sigmasq=tail(sigmasq,ndraws),
            logTheta=t( tail(t(logTheta),ndraws) ) )
    return(retval)
}

thetaPost <- function(logTheta, ySum, n, w, mu, sigmasq) {
    #
    # Function to compute unnormalized conditional (log) posterior density of
    # intensities given all parameters and unobserved data for Poisson-lognormal
    # model. Used by lambdaMHStep and lambdaInit.
    #
    return( -n*exp(logTheta+log(w)) + (ySum + mu/sigmasq)*logTheta -
                logTheta*logTheta/2/sigmasq )
}

dp.dtheta <- function(logTheta, ySum, n, w, mu, sigmasq) {
    #
    # Function to compute first derivative of unnormalized conditional (log)
    # posterior density of intensities given all parameters and unobserved data
    # for Poisson-lognormal model. Used by thetaMHStep and thetaInit.
    #
    return( -n*exp(logTheta+log(w)) +
            (ySum+mu/sigmasq) - 1./sigmasq * logTheta )
}


d2p.dtheta2 <- function(logTheta, ySum, n, w, mu, sigmasq) {
    #
    # Function to compute second derivative of unnormalized conditional (log)
    # posterior density of intensities given all parameters and unobserved data
    # for Poisson-lognormal model. Used by thetaMHStep and thetaInit.
    #
    return( -n*exp(logTheta+log(w)) - 1./sigmasq )
}

d3p.dtheta3 <- function(logTheta, ySum, n, w, mu, sigmasq) {
    #
    # Function to compute second derivative of unnormalized conditional (log)
    # posterior density of intensities given all parameters and unobserved data
    # for Poisson-lognormal model. Used by thetaMHStep and thetaInit.
    #
    return( -n*exp(logTheta+log(w)) )
}

thetaInit <- function(ySum, n, w, mu, sigmasq, logThetaPrev,
        tol=1e-3, maxIter=100, stepFactor=1) {
    #
    # Function to find conditional posterior means for log-intensities for
    # Poisson-lognormal models. Uses Halley's method (an extension of Newton's
    # method with second derivatives). Used by thetaMHStep to set center
    # of Metropolis-Hastings proposals.
    #
    initModes <- logThetaPrev
    for (iter in seq(maxIter)) {
        initModesPrev <- initModes
        f <- dp.dtheta(initModes, ySum, n, w, mu, sigmasq)
        fprime <- d2p.dtheta2(initModes, ySum, n, w, mu, sigmasq)
        fdoubleprime <- d3p.dtheta3(initModes, ySum, n, w, mu, sigmasq)
        initModes <- initModes - stepFactor * 2.*f*fprime/
                (2.*fprime*fprime - f*fdoubleprime)
#        initModes <- initModes - stepFactor * f/fprime
        #
        deltaMax <- max(abs((initModes-initModesPrev)/
                (initModes+initModesPrev)*2.))
        if (is.nan(deltaMax)) {
            bad <- which(is.nan(initModes))
            print(mu)
            print(sigmasq)
            print(logThetaPrev[bad])
            print(initModesPrev[bad])
            print(f[bad])
            print(fprime[bad])
            print(fdoubleprime[bad])
        }
        if ( deltaMax < tol ) {
            return(initModes)
        }
    }
    #print('bad init')
    return(initModes)
}

thetaMHStep <- function(ySum, n,w,mu,sigmasq,logThetaPrev,propscale=1.) {
    #
    # Function to execute Metropolis-Hastings step on vector of log-intensities
    # for Poisson-lognormal model. Sets centers of proposal distributions using
    # thetaInit.
    #
    J <- length(ySum)
    
    # Initialize modes
    initModes <- thetaInit(ySum, n, w, mu, sigmasq, logThetaPrev)
    
    # Generate proposal
    propSigma <- 1/sqrt(-d2p.dtheta2(initModes, ySum, n, w, mu, sigmasq))
    propSigma <- propscale * propSigma
    proposal <- rnorm( J, initModes, propSigma)
    
    # Calculate LLR & LIR
    llr <- ( thetaPost(proposal, ySum, n, w, mu, sigmasq)-
              thetaPost(logThetaPrev, ySum, n, w, mu, sigmasq))
    lir <- ( dnorm(proposal, initModes, propSigma, log=TRUE) -
             dnorm(logThetaPrev, initModes, propSigma, log=TRUE) )
    
    # Calculate acceptance/rejection values
    accept <- ( log(1-runif(J)) < (llr-lir) )
    
    # Setup return(values)
    logThetaNew <- numeric(J)
    logThetaNew[accept] <- proposal[accept]
    logThetaNew[!accept] <- logThetaPrev[!accept]
    return(list(logTheta=logThetaNew, accept=accept))
}
