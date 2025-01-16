#!/bin/bash

make mpi_openmp > /dev/null

echo "threads,time,hashes_checked"

for n in 2 4 6 8
do
  for m in 1 2 4 6 8
  do
    for i in {1..10}
    do
      mpirun -n $n --oversubscribe ./md5 $m
    done
  done
done
