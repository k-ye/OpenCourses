#!/bin/bash

#SBATCH -J task5 # name for job array
#SBATCH -o task5.out #Standard output
#SBATCH -e task5.err #Standard error
#SBATCH -p stats #Partition
#SBATCH -t 110 #Running time of 120 mins
#SBATCH --mem-per-cpu 3000 #Memory Request
#SBATCH -n 1 # Number of cores
#SBATCH -N 1 # All cores on one machine
#SBATCH --mail-type=END #Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user=yekuang@g.harvard.edu #Email to be sent

# first arg = number of theta simulation
# second arg = number of Y simulation
# third arg = job ID

Rscript ykuang_ps2_task5.R 3 40 $SLURM_ARRAY_TASK_ID
