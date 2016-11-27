.PHONY: all clean

CC = gcc
OBJECTS = turboclient.o turboserver.o
LIB = -lm

%.o: %.c
	$(CC) $(LIB) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
