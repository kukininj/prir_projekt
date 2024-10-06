CC = clang

CFLAGS = -Wall -Wextra -O3

md5: md5.c main.c
	@$(CC) $(CFLAGS) -o md5 main.c

clean:
	@$(RM) md5
