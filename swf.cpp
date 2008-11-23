#include <iostream>
#include <stdio.h>
#include "swf.h"
#include "string.h"

SWF_HEADER::SWF_HEADER(FILE* in)
{
	//Valid only on little endian platforms
	fread(Signature,sizeof(*Signature),3,in);
	if(Signature[0]!='F' || Signature[1]!='W' || Signature[2]!='S')
		throw "bad file\n";
	fread(&Version,sizeof(Version),1,in);
	fread(&FileLenght,sizeof(FileLenght),1,in);
	FrameSize=RECT(in);
	fread(&FrameRate,sizeof(FrameRate),1,in);
	fread(&FrameCount,sizeof(FrameCount),1,in);
}

