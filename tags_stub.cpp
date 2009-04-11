/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

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

