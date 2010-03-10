/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "flv.h"
#include "swftypes.h"

using namespace lightspark;

FLV_HEADER::FLV_HEADER(std::istream& in):dataOffset(0),_hasAudio(false),_hasVideo(false)
{
	UI8 Signature[3];
	UI8 Version;
	UI32 DataOffset;

	in >> Signature[0] >> Signature[1] >> Signature[2] >> Version;
	version=Version;
	
	if(Signature[0]=='F' && Signature[1]=='L' && Signature[2]=='V')
	{
		LOG(LOG_NO_INFO, "FLV file: Version " << (int)Version);
		valid=true;
	}
	else
	{
		LOG(LOG_NO_INFO,"No FLV file signature found");
		valid=false;
		return;
	}
	BitStream bs(in);

	if(UB(5, bs)!=0)
	{
		valid=false;
		return;
	}
	_hasAudio=UB(1, bs);
	if(UB(1, bs)!=0)
	{
		valid=false;
		return;
	}
	_hasVideo=UB(1, bs);

	in >> DataOffset;

	dataOffset = DataOffset;
}
