# User-overridable flags:
CXXFLAGS = -g -O0 -D_GLIBCXX_NO_DEBUG
prefix = /usr
bindir = $(prefix)/bin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
#PKG_BUILD_FLAG=-DLIGHTSPARK_PKG_BUILD   # Read data from /usr/share

LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o vm.o \
	  asobjects.o abc.o flashdisplay.o flashevents.o textfile.o

# TODO: library?
all: lightspark
lightspark: main.o $(LIBOBJS) 
	$(CXX) -pthread `pkg-config --cflags --libs gl sdl libcurl libxml-2.0` -lz `llvm-config --cxxflags --ldflags --libs core jit native` $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

libls.so: $(LIBOBJS)
	$(CXX) -pthread -shared `pkg-config --cflags --libs sdl gl` $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $@ $^

%.o: %.cpp
	$(CXX) -pthread `pkg-config --cflags libxml-2.0` `llvm-config --cxxflags core` $(PKG_BUILD_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^

.PHONY: all clean install
clean:
	-rm -f main.o $(LIBOBJS) lightspark libls.so
install: all
	install -d $(DESTDIR)$(bindir) $(DESTDIR)$(datadir)/lightspark
	install lightspark $(DESTDIR)$(bindir)/lightspark
	install -m644 lightspark.frag $(DESTDIR)$(datadir)/lightspark/lightspark.frag
	install -m644 lightspark.vert $(DESTDIR)$(datadir)/lightspark/lightspark.frag
	# TODO: library/plugin install

