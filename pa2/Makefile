.PHONY: all clean

CC = gcc
OBJECTS = prog2_arq.o prog2_gbn.o

%.o: %.c
	$(CC) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
