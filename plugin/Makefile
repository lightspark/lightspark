LIBOBJS = ../swf.o ../swftypes.o ../tags.o ../geometry.o ../actions.o ../frame.o ../input.o ../streams.o \
		../tags_stub.o ../logger.o ../vm.o ../asobjects.o ../abc.o ../flashdisplay.o ../flashevents.o \
		../textfile.o ../thread_pool.o

lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o $(LIBOBJS)
	g++ -shared -g -o $@ $^ `pkg-config --cflags --libs gl sdl libcurl libxml-2.0` -lz \
		`llvm-config --cxxflags --ldflags --libs core jit native`

%.o: %.cpp
	g++ -fPIC -g -c -o $@ $^ -I include -I /usr/include/nspr -I ../ -D MOZ_X11 -D_GLIBCXX_NO_DEBUG `pkg-config --cflags libxml-2.0` `llvm-config --cxxflags core`

install: lightspark.so
	mkdir -p ~/.mozilla/plugins
	cp $^ ~/.mozilla/plugins

