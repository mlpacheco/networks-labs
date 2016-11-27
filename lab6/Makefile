.PHONY: all clean

CC = gcc
OBJECTS = turboclient.o turboserver.o

%.o: %.c
	$(CC) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
