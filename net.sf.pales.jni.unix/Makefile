JNI_INCDIR := /usr/lib/jvm/java-7-openjdk-amd64/include
CXXFLAGS := -Wall -Wno-parentheses -Wno-sign-compare -std=c++11 -fPIC
CPPFLAGS := -D_GNU_SOURCE -I$(JNI_INCDIR) -I$(JNI_INCDIR)/linux
LDFLAGS := -lpthread
ifdef DEBUG
CFLAGS += -g -O0
OUTDIR := debug
else
CFLAGS += -O2
OUTDIR := release
endif
VPATH = $(OUTDIR)

%.o: %.cc %.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $(OUTDIR)/$@ -c $< 

all: libpales.so palestest

clean:
	rm -f $(OUTDIR)/*.so $(OUTDIR)/*.o $(OUTDIR)/palestest

libpales.so: unix.o palesjni.o database.o
	$(CXX) -shared -o $(OUTDIR)/$@ $^

palestest: palestest.o unix.o database.o syserror.o
	$(CXX) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

