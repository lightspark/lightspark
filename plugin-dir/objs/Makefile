# User-overridable flags:
CXXFLAGS = -Wall -g -O0 -D_GLIBCXX_NO_DEBUG -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS_RELEASE = -Wall -O3 -DNDEBUG -Wnon-virtual-dtor -Woverloaded-virtual
EXTRAFLAGS = `pkg-config --cflags gl sdl libcurl libxml-2.0 libpcrecpp libavcodec`
LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o vm.o \
	  asobjects.o abc.o abc_codesynt.o abc_opcodes.o flashdisplay.o flashevents.o textfile.o thread_pool.o \
	  flashgeom.o flashnet.o flashsystem.o flashutils.o compat.o abc_interpreter.o flashexternal.o \
	  flashtext.o flashmedia.o netutils.o flv.o netutils.o fastpaths.o timer.o

%.o: ../../%.cpp
	$(CXX) -fPIC -pipe -pthread -I`llvm-config --includedir` $(EXTRAFLAGS) $(PKG_BUILD_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^ 

%.o: ../../%.asm
	nasm -f elf64 -o $@ $^

objs: $(LIBOBJS)

release: CXXFLAGS=$(CXXFLAGS_RELEASE)
release: objs
