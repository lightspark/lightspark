lightspark: main.o libls.so
	g++ -g -o $@ $^ -lSDL -pg -lrt -lGL 

libls.so: swf.o swftypes.o tags.o geometry.o actions.o frame.o input.o streams.o 
	g++ -shared -g -o $@ $^ -lSDL -pg -lrt -lGL 

%.o: %.cpp
	g++ -fPIC -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG

