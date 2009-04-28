LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o vm.o asobjects.o

lightspark: main.o $(LIBOBJS) 
	g++ -pthread -pg -g -o $@ $^ -lSDL -lrt -lGL -lz -lcurl -lxml2

libls.so: $(LIBOBJS) 
	g++ -pthread -shared -g -o $@ $^ -lSDL -lrt -lGL 

%.o: %.cpp
	g++ -pthread -pg -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG -I /usr/include/libxml2

.PHONY: clean
clean:
	-rm -f main.o $(LIBOBJS) lightspark libls.so
