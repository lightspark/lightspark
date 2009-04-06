LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o tags_stub.o logger.o

lightspark: main.o $(LIBOBJS) 
	g++ -pthread -g -o $@ $^ -lSDL -lrt -lGL -lz 

libls.so: $(LIBOBJS) 
	g++ -pthread -shared -g -o $@ $^ -lSDL -lrt -lGL 

%.o: %.cpp
	g++ -pthread -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG

.PHONY: clean
clean:
	-rm -f main.o $(LIBOBJS) lightspark libls.so
