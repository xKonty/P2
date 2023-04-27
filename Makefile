CC=gcc
CFLAGS=-std=gnu99 -pthread -Wall -Wextra -Werror -pedantic

main:
	$(CC) $(CFLAGS) -o proj2 proj2.c -lrt

test:
	./proj2 4 1 10000 100 200