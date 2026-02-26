CC = gcc
CFLAGS = -g -Wall

all: memgrind

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o memgrind memgrind.c mymalloc.c

tests: tests.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o tests tests.c mymalloc.c

clean:
	rm -f memgrind tests
	rm -rf *.dSYM