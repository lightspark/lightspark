clash: main.o swf.o swftypes.o tags.o geometry.o
	g++ -g -o $@ $^ -lSDL -lGL -pg 
%.o: %.cpp
	g++ -g -O0 -c -o $@ $^ -pg -D_GLIBCXX_DEBUG

