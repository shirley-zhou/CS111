CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused -Werror -lrt -pthread

all: sltest

sltest: sltest.c SortedList.c
	$(CC) $(CFLAGS) -o $@ sltest.c SortedList.c
