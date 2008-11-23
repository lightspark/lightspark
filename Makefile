clash: main.o swf.o swftypes.o
	g++ -g -o $@ $^

%.o: %.cpp
	g++ -g -c -o $@ $^
