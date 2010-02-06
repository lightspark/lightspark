# User-overridable flags:
CXXFLAGS = -g -O0 -D_GLIBCXX_NO_DEBUG -Wnon-virtual-dtor
CXXFLAGS_RELEASE = -O3 -DNDEBUG -Wnon-virtual-dtor
LLVMLIBS = `llvm-config --libfiles engine interpreter`
EXTRAFLAGS = `pkg-config --cflags gl sdl libcurl libxml-2.0 libpcrecpp`
EXTRALIBS = `pkg-config --cflags --libs gl sdl libcurl libxml-2.0 libpcrecpp`
prefix = /usr
bindir = $(prefix)/bin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
#PKG_BUILD_FLAG=-DLIGHTSPARK_PKG_BUILD   # Read data from /usr/share

LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o vm.o \
	  asobjects.o abc.o abc_codesynt.o abc_opcodes.o flashdisplay.o flashevents.o textfile.o thread_pool.o \
	  flashgeom.o flashnet.o flashsystem.o flashutils.o compat.o abc_interpreter.o flashexternal.o

# TODO: library?
all: lightspark tightspark

release: CXXFLAGS=$(CXXFLAGS_RELEASE)
release: all

lightspark: main.o $(LIBOBJS) 
	$(CXX) -pthread -lz `llvm-config --ldflags` -lGLEW $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -pipe \
		-o $@ $^ $(LLVMLIBS) $(EXTRALIBS)

tightspark: tightspark.o $(LIBOBJS)
	$(CXX) -pthread -lz `llvm-config --ldflags` -lGLEW $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -pipe \
		-o $@ $^ $(LLVMLIBS) $(EXTRALIBS)



%.o: %.cpp
	$(CXX) -pipe -pthread -I`llvm-config --includedir` $(EXTRAFLAGS) $(PKG_BUILD_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^ 

.PHONY: all clean install
clean:
	-rm -f main.o tightspark.o $(LIBOBJS) lightspark tightspark
install: all
	install -d $(DESTDIR)$(bindir) $(DESTDIR)$(datadir)/lightspark
	install lightspark $(DESTDIR)$(bindir)/lightspark
	install -m644 lightspark.frag $(DESTDIR)$(datadir)/lightspark/lightspark.frag
	install -m644 lightspark.vert $(DESTDIR)$(datadir)/lightspark/lightspark.vert
	# TODO: library/plugin install

