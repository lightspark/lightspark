#include "swf.h"
#include <iostream>
#include <fstream>

using namespace std;

int main()
{
	ifstream f("flash.swf",ifstream::in);
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;
}

