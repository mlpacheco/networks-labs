.PHONY: all clean

CC = gcc
OBJECTS = pa3.o

pa3.o: prog3.c node0.c node1.c node2.c node3.c
	$(CC) prog3.c node0.c node1.c node2.c node3.c -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
