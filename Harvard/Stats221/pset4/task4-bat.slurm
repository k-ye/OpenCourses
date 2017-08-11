#!/bin/bash

#SBATCH -J task4 # name for job array
#SBATCH -o task4.out #Standard output
#SBATCH -e task4.err #Standard error
#SBATCH -p stats #Partition
#SBATCH -t 80 #Running time of 40 mins
#SBATCH --mem-per-cpu 4000 #Memory Request
#SBATCH -n 1 # Number of cores
#SBATCH -N 1 # All cores on one machine
#SBATCH --mail-type=END #Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user=yekuang@g.harvard.edu #Email to be sent

# first arg = number of data to simulate
Rscript yk_ps4_part4.R 1000000 200000 $SLURM_ARRAY_TASK_ID