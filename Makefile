clash: main.o swf.o swftypes.o tags.o gltags.o
	g++ -g -o $@ $^ -lSDL -lGL

%.o: %.cpp
	g++ -g -c -o $@ $^
