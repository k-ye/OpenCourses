#!/bin/bash

#SBATCH -J ykuang_1router # name for job array
#SBATCH -o ykuang_1router.out #Standard output
#SBATCH -e ykuang_1router.err #Standard error
#SBATCH -p stats #Partition
#SBATCH --mem-per-cpu 4000 #Memory Request
#SBATCH -n 1 # Number of cores
#SBATCH -N 1 # All cores on one machine
#SBATCH --mail-type=END #Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user=yekuang@g.harvard.edu #Email to be sent

Rscript ykuang_1router.R $SLURM_ARRAY_TASK_ID