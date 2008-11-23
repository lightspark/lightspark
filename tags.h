#include <stdio.h>

class Tag
{

};

class TagFactory
{
private:
	FILE* f;
public:
	TagFactory(FILE* in):f(in){}
	Tag* readTag();
};
