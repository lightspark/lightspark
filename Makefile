clash: main.o swf.o swftypes.o tags.o 
	g++ -g -o $@ $^ -lSDL -lGL -pg

%.o: %.cpp
	g++ -g -c -o $@ $^ -pg
