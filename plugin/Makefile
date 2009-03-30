lightspark.so: np_entry.o npn_gate.o npp_gate.o plugin.o ../libls.so 
	g++ -shared -o $@ $^

%.o: %.cpp
	g++ -fPIC -c -o $@ $^ -I include -I /usr/include/nspr -I ../ -D MOZ_X11

install: lightspark.so
	cp $^ ~/.mozilla/plugins

