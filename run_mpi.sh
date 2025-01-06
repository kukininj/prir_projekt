#!/bin/bash

make mpi

echo "threads,time,hashes_checked"

for n in 1 2 4 8 16 24 32 40 48 56 64
do
  for i in {0..10}
  do
    mpirun -n $n --oversubscribe ./md5
  done
done
