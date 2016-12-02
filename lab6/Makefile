.PHONY: all clean

CC = gcc
OBJECTS = turboclient.o turboserver.o overlayrouter.c overlaybuild.c
LIB = -lm

%.o: %.c
	$(CC) $(LIB) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
