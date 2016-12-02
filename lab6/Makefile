.PHONY: all clean

CC = gcc
OBJECTS = turboclient.o turboserver.o overlayrouter.o overlaybuild.o
LIB = -lm

%.o: %.c
	$(CC) $(LIB) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
