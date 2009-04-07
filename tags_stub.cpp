#include <istream>
#include "tags.h"
#include "logger.h"

using namespace std;

void ignore(istream& i, int count);

ProtectTag::ProtectTag(RECORDHEADER h, istream& in):ControlTag(h,in)
{
	LOG(NOT_IMPLEMENTED,"Protect Tag");
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}

DefineSoundTag::DefineSoundTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(NOT_IMPLEMENTED,"DefineSound Tag");
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}

DefineFontInfoTag::DefineFontInfoTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(NOT_IMPLEMENTED,"DefineFontInfo Tag");
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}

StartSoundTag::StartSoundTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(NOT_IMPLEMENTED,"StartSound Tag");
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(NOT_IMPLEMENTED,"SoundStreamHead2 Tag");
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}

