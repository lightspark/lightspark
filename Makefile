lightspark: main.o swf.o swftypes.o tags.o geometry.o actions.o frame.o 
	g++ -g -o $@ $^ -lSDL -pg -lrt -lGL 
%.o: %.cpp
	g++ -g -O0 -c -o $@ $^ -D_GLIBCXX_NO_DEBUG

