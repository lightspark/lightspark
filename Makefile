LIBOBJS = swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o

lightspark: main.o $(LIBOBJS) 
	g++ -pthread -g -o $@ $^ -lSDL -pg -lrt -lGL 

libls.so: $(LIBOBJS) 
	g++ -pthread -shared -g -o $@ $^ -lSDL -pg -lrt -lGL 

%.o: %.cpp
	g++ -pthread -pg -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG

.PHONY: clean
clean:
	-rm -f main.o $(LIBOBJS) lightspark libls.so
