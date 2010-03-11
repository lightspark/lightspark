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

#ifndef FLV_H
#define FLV_H

#include <istream>
#include "swftypes.h"

namespace lightspark
{

class FLV_HEADER
{
private:
	unsigned int dataOffset;
	unsigned int version;
	bool valid;
	bool _hasAudio;
	bool _hasVideo;
public:
	FLV_HEADER(std::istream& in);
	unsigned int skipAmount(){return dataOffset;}
	bool isValid() { return valid; }
	bool hasAudio() { return _hasAudio; }
	bool hasVideo() { return _hasVideo; }
};

class VideoTag
{
protected:
	uint32_t dataSize;
	uint32_t timestamp;
public:
	VideoTag(std::istream& s);
	uint32_t getDataSize() { return dataSize; }
};

class ScriptDataTag: public VideoTag
{
public:
	ScriptDataTag(std::istream& s);
};

class ScriptDataString
{
private:
	uint32_t size;
	tiny_string val;
public:
	ScriptDataString(std::istream& s);
	const tiny_string& getString() { return val; }
	uint32_t getSize() { return size; }
};

};

#endif
