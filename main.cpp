#include "swf.h"
#include <iostream>
#include <stdio.h>

using namespace std;

int main()
{
	FILE* f=fopen("flash.swf","r");
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;
}

