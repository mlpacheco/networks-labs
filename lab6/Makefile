.PHONY: all clean

CC = gcc
HT_OBJECT = hashtable.o
DS_OBJECT = dropsendto.o
OR_OBJECT = overlayrouter.o
TS_OBJECT = turboserver.o
OBJECTS = overlaybuild.o turboclient.o
HEADERS = hashtable.h dropsendto.h
CFLAGS = -lm

all: $(OBJECTS) $(HT_OBJECT) $(OR_OBJECT) $(DS_OBJECT) $(TS_OBJECT)

$(HT_OBJECT): EXTRA_FLAG := -c
$(DS_OBJECT): EXTRA_FLAG := -c
$(OR_OBJECT): EXTRA_OBJ := $(HT_OBJECT)
$(TS_OBJECT): EXTRA_OBJT := $(DS_OBJECT)

%.o: %.c $(HEADERS)
	$(CC) $(EXTRA_FLAG) $(CFLAGS) $< $(EXTRA_OBJ) $(EXTRA_OBJT) -o $@

clean:
	rm -f *.o
