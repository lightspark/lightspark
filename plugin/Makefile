lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o ../swf.o ../actions.o ../swftypes.o ../streams.o ../frame.o ../tags.o \
		../input.o ../geometry.o 
	g++ -shared -g -o $@ $^ 

%.o: %.cpp
	g++ -fPIC -g -c -o $@ $^ -I include -I /usr/include/nspr -I ../ -D MOZ_X11

install: lightspark.so
	mkdir -p ~/.mozilla/plugins
	cp $^ ~/.mozilla/plugins

