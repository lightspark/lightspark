lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o
	g++ -shared -o $@ $^

%.o: %.cpp
	g++ -fPIC -c -o $@ $^ -I include -I /usr/include/nspr -D MOZ_X11

install: lightspark.so
	cp $^ ~/.mozilla/plugins
