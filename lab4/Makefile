.PHONY: all clean

CC = gcc
OBJECTS = mytunnel.o tunneld.o wetalk.o

%.o: %.c
	$(CC) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
