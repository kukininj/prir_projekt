CC = gcc
MPICC = mpicc

CFLAGS = -Wall -Wextra -O3

RM = rm

openmp: md5.c openmp.c
	$(CC) $(CFLAGS) -o md5 openmp.c -lm -fopenmp

mpi: md5.c mpi.c
	$(MPICC) $(CFLAGS) -o md5 mpi.c -lm

mpi_openmp: md5.c mpi_openmp.c
	$(MPICC) $(CFLAGS) -o md5 mpi_openmp.c -lm -fopenmp

clean:
	@$(RM) md5
