.PHONY: all clean

CC = gcc
HT_OBJECT = hashtable.o
OR_OBJECT = overlayrouter.o
OBJECTS = overlaybuild.o turboclient.o turboserver.o myping.o traffic_snd.o
HEADERS = hashtable.h
CFLAGS = -lm

all: $(OBJECTS) $(HT_OBJECT) $(OR_OBJECT)

$(HT_OBJECT): EXTRA_FLAG := -c
$(OR_OBJECT): EXTRA_OBJ := $(HT_OBJECT)

%.o: %.c $(HEADERS)
	$(CC) $(EXTRA_FLAG) $(CFLAGS) $< $(EXTRA_OBJ) -o $@

clean:
	rm -f *.o
