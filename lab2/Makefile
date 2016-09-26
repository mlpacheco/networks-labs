.PHONY: all clean

CC = gcc
OBJECTS = tcpserver.o tcpclient.o mypingd.o myping.o

%.o: %.c
	$(CC) $< -o $@

all: $(OBJECTS)

clean:
	rm -f *.o
