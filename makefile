CC = clang

CFLAGS = -Wall -Wextra -O3

md5: md5.c main.c
	@$(CC) $(CFLAGS) -o md5 main.c -lm

md5_openmp: md5.c main.c
	@$(CC) $(CFLAGS) -o md5 main.c -fopenmp -lm

clean:
	@$(RM) md5
