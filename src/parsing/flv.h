/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef PARSING_FLV_H
#define PARSING_FLV_H 1

#include "compat.h"
#include <istream>
#include <map>
#include "swftypes.h"
#include "backends/decoder.h"

namespace lightspark
{
union asAtom;

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
	unsigned int skipAmount() const {return dataOffset;}
	bool isValid() const { return valid; }
	bool hasAudio() const { return _hasAudio; }
	bool hasVideo() const { return _hasVideo; }
};

class VideoTag
{
protected:
	uint32_t dataSize;
	uint32_t timestamp;
	uint32_t totalLen;
public:
	VideoTag() {}
	VideoTag(std::istream& s);
	uint32_t getDataSize() const { return dataSize; }
	uint32_t getTotalLen() const { return totalLen; }
};

class ScriptDataTag: public VideoTag
{
public:
	tiny_string methodName;
	std::list<asAtom> dataobjectlist;
	ScriptDataTag() {}
	ScriptDataTag(std::istream& s);
};

class VideoDataTag: public VideoTag
{
private:
	bool _isHeader;
public:
	int frameType;
	LS_VIDEO_CODEC codec;
	uint8_t* packetData;
	uint32_t packetLen;
	VideoDataTag(std::istream& s);
	void releaseBuffer()
	{
		packetData=NULL;
		packetLen=0;
	}
	~VideoDataTag();
	bool isHeader() const { return _isHeader; }
};

class AudioDataTag: public VideoTag
{
private:
	bool _isHeader;
public:
	LS_AUDIO_CODEC SoundFormat;
	uint32_t SoundRate;
	bool is16bit;
	bool isStereo;
	uint8_t* packetData;
	uint32_t packetLen;
	AudioDataTag(std::istream& s);
	~AudioDataTag();
	void releaseBuffer()
	{
		packetData=NULL;
		packetLen=0;
	}
	bool isHeader() const { return _isHeader; }
};

}

#endif /* PARSING_FLV_H */
