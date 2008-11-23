#include "swf.h"
#include "tags.h"
#include <iostream>
#include <fstream>

using namespace std;

int main()
{
	ifstream f("flash.swf",ifstream::in);
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;

	try
	{
		TagFactory factory(f);
		while(1)
		{
			factory.readTag();
		}
	}
	catch(const char* s)
	{
		cout << s << endl;
	}
}

