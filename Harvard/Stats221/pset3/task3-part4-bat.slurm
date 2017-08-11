#!/bin/bash

#SBATCH -J task3_part4 # name for job array
#SBATCH -o task3_part4.out #Standard output
#SBATCH -e task3_part4.err #Standard error
#SBATCH -p stats #Partition
#SBATCH -t 120 #Running time of 180 mins
#SBATCH --mem-per-cpu 8000 #Memory Request
#SBATCH -n 1 # Number of cores
#SBATCH -N 1 # All cores on one machine
#SBATCH --mail-type=END #Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user=yekuang@g.harvard.edu #Email to be sent

# first arg = number of data to simulate
Rscript ykuang_ps3_task3_part4.R $SLURM_ARRAY_TASK_ID