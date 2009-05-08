LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o vm.o asobjects.o

lightspark: main.o $(LIBOBJS) 
	g++ -pthread -g -o $@ $^ `pkg-config --cflags --libs gl sdl libcurl libxml-2.0` -lrt  -lz  

libls.so: $(LIBOBJS) 
	g++ -pthread -shared -g -o $@ $^ `pkg-config --cflags --libs sdl gl`  -lrt 

%.o: %.cpp
	g++ -pthread -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG `pkg-config --cflags --libs libxml-2.0`

.PHONY: clean
clean:
	-rm -f main.o $(LIBOBJS) lightspark libls.so
