#!/bin/bash

GOPATH=$(greadlink -f ../..)
export GOPATH
echo "GOPATH=$GOPATH"

set -e

# bash means pain
# https://unix.stackexchange.com/questions/88216/bash-variables-in-for-loop-range
num_run=100
for ((i=1;i<=num_run;i++)); do
  echo "Test iteration: $i"
  go test -run 2A
  go test -run 2B
  # go test -run 2C

  # enable this when A, B and C are all done
  # go test
done