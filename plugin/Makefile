lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o ../swf.o ../actions.o ../swftypes.o ../streams.o ../frame.o ../tags.o \
		../input.o ../geometry.o ../tags_stub.o ../logger.o ../vm.o ../asobjects.o
	g++ -shared -g -o $@ $^ -lGL -lz

%.o: %.cpp
	g++ -fPIC -g -c -o $@ $^ -I include -I /usr/include/nspr -I ../ -D MOZ_X11 -D_GLIBCXX_NO_DEBUG

install: lightspark.so
	mkdir -p ~/.mozilla/plugins
	cp $^ ~/.mozilla/plugins

