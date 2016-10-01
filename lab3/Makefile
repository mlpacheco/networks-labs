.PHONY: all clean

CC = gcc
OBJECTS = fileserver.o fileclient.o traffic_snd.o traffic_rcv.o

%.o: %.c
	$(CC) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
