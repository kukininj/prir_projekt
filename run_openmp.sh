#!/bin/bash

make openmp

echo "threads,time,hashes_checked"

for n in 1 2 4 8 16 24 32 40 48 56 64
do
  for i in {1..10}
  do
    ./md5 $n
  done
done
