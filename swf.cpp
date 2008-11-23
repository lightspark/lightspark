#include <iostream>
#include "swf.h"
#include "string.h"

SWF_HEADER::SWF_HEADER(std::ifstream& in)
{
	//Valid only on little endian platforms
	in >> Signature[0] >> Signature[1] >> Signature[2];

	if(Signature[0]!='F' || Signature[1]!='W' || Signature[2]!='S')
		throw "bad file\n";
	in >> Version >> FileLenght >> FrameSize >> FrameRate >> FrameCount;
}

