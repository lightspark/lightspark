LLVMLIBS = `llvm-config --libfiles engine interpreter`
EXTRALIBS = `pkg-config --cflags --libs gl sdl libcurl libxml-2.0 libpcrecpp libavcodec`
prefix = /usr
bindir = $(prefix)/bin
datarootdir = $(prefix)/share
datadir = $(datarootdir)

LIBOBJS = objs/swf.o objs/swftypes.o objs/tags.o objs/geometry.o objs/actions.o objs/frame.o objs/input.o objs/streams.o \
	  objs/tags_stub.o objs/logger.o objs/vm.o objs/asobjects.o objs/abc.o objs/abc_codesynt.o objs/abc_opcodes.o objs/flashdisplay.o \
	  objs/flashevents.o objs/textfile.o objs/thread_pool.o objs/flashgeom.o objs/flashnet.o objs/flashsystem.o objs/flashutils.o \
	  objs/compat.o objs/abc_interpreter.o objs/flashexternal.o objs/flashtext.o objs/flashmedia.o objs/flv.o objs/netutils.o \
	  objs/fastpaths.o objs/timer.o

default: objs-dir lightspark.so

lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o $(LIBOBJS)
	g++ -shared -g -o $@ $^ $(EXTRALIBS) -lz $(LLVMLIBS) -lGLEW

%.o: %.cpp
	g++ -g -fPIC -c -o $@ $^ -I include -I ../ `nspr-config --cflags` -D MOZ_X11

objs-dir:
	make -C objs

install: lightspark.so
	mkdir -p ~/.mozilla/plugins
	cp $^ ~/.mozilla/plugins

clean:
	-rm -f objs/*.o
	-rm -f *.o lightspark.so
