JNI_INCDIR := /usr/lib/jvm/java-7-openjdk-amd64/include
CXXFLAGS := -Wall -Wno-parentheses -Wno-sign-compare -std=c++11 -fPIC
CPPFLAGS := -D_GNU_SOURCE -I$(JNI_INCDIR) -I$(JNI_INCDIR)/linux
LDFLAGS := -lpthread
ifdef DEBUG
CXXFLAGS += -g -O0
OUTDIR := debug
else
CXXFLAGS += -O2
OUTDIR := release
endif
LIBPALES_OBJS := $(addprefix $(OUTDIR)/, unix.o palesjni.o database.o syserror.o bsd-closefrom.o)
TEST_OBJS := $(addprefix $(OUTDIR)/, palestest.o unix.o database.o syserror.o)

$(OUTDIR)/%.o: %.cc %.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< 

$(OUTDIR)/bsd-closefrom.o: bsd-closefrom.c
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< 

$(OUTDIR)/palestest.o: palestest.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< 

all: libpales.so palestest

clean:
	rm -f $(OUTDIR)/*.so $(OUTDIR)/*.o $(OUTDIR)/palestest

libpales.so: $(LIBPALES_OBJS)
	$(CXX) -shared -o $(OUTDIR)/$@ $^

palestest: $(TEST_OBJS)
	$(CXX) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

