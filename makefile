CC = gcc

CFLAGS = -Wall -Wextra -O3

RM = rm

all: md5.c main.c
	$(CC) $(CFLAGS) -o md5 main.c -fopenmp -lm

clean:
	@$(RM) md5
