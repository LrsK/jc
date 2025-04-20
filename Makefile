CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -std=c99 -ggdb

all: main
main: main.o
main.o: main.c

clean:
	rm -f main main.o
run: main
	./main

