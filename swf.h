#include <stdio.h>
#include <iostream>
#include "swftypes.h"
class SWF_HEADER
{
private:
	UI8 Signature[3];
	UI8 Version;
	UI32 FileLenght;
	RECT FrameSize;
	UI16 FrameRate;
	UI16 FrameCount;
public:
	SWF_HEADER(FILE* in);
	const RECT& getFrameSize(){ return FrameSize; }
};

