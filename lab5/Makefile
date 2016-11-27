.PHONY: all clean

CC = gcc
JC = javac
OBJECTS = audiostreamd.o audiolisten.o 
CLASSES = FileClient.class

%.o: %.c
	$(CC) -pthread $< -o $@

%.class: %.java
	$(JC) $<

all: $(OBJECTS) $(CLASSES)

clean:
	rm -f *.o *.class
