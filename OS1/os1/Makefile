CC=gcc
CFLAGS=-c -Wall -I. -fpic -g -fbounds-check
LDFLAGS=-L.

OBJS=process_a.o process_b.o

%.o:	%.c %.h
	$(CC) $(CFLAGS) $< -o $@

all: process_a process_b

process_a: process_a.o
	$(CC) *.o -o $@
	rm $<

process_b: process_b.o
	$(CC) *.o -o $@
	rm $<

clean:
	rm -f $(OBJS) process_a process_b
