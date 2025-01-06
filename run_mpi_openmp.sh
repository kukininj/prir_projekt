#!/bin/bash

make mpi_openmp

echo "threads,time,hashes_checked"

for n in 1 2 3 4 5 6 7
do
  for i in {1..10}
  do
    mpirun -n $n --oversubscribe ./md5 $n
  done
done
