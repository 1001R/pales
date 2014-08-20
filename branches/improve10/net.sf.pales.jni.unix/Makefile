JNI_INCDIR := /usr/lib/jvm/java-7-openjdk-amd64/include
CFLAGS := -Wall -std=gnu99 -fPIC
CPPFLAGS := -D_GNU_SOURCE -I$(JNI_INCDIR) -I$(JNI_INCDIR)/linux
LDFLAGS := -lpthread
CC := gcc
ifdef DEBUG
CFLAGS += -g -O0
else
CFLAGS += -O2
endif

all: libpales.so palestest

clean:
	rm -f *.so *.o palestest

libpales.so: unix.o palesjni.o database.o
	$(CC) -shared -o $@ $^ 

palestest: palestest.o unix.o database.o
	$(CC) -o $@ $^ $(LDFLAGS)

