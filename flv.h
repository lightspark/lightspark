/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef FLV_H
#define FLV_H

#include "compat.h"
#include <istream>
#include "swftypes.h"

namespace lightspark
{

enum FLV_AUDIO_CODEC { LINEAR_PCM_PLATFORM_ENDIAN=0, ADPCM=1, MP3=2, LINEAR_PCM_LE=3, AAC=10 };

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
	VideoTag(std::istream& s);
	uint32_t getDataSize() const { return dataSize; }
	uint32_t getTotalLen() const { return totalLen; }
};

class ScriptDataTag: public VideoTag
{
public:
	double frameRate;
	ScriptDataTag(std::istream& s);
};

class ScriptDataString
{
private:
	uint32_t size;
	tiny_string val;
public:
	ScriptDataString(std::istream& s);
	const tiny_string& getString() const { return val; }
	uint32_t getSize() const { return size; }
};

class ScriptECMAArray
{
public:
	double frameRate;
	ScriptECMAArray(std::istream& s);
};

class VideoDataTag: public VideoTag
{
private:
	bool _isHeader;
public:
	int frameType;
	int codecId;
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
	FLV_AUDIO_CODEC SoundFormat;
	uint32_t SoundRate;
	bool is16bit;
	bool isStereo;
	uint8_t* packetData;
	uint32_t packetLen;
	AudioDataTag(std::istream& s);
	~AudioDataTag();
	bool isHeader() const { return _isHeader; }
};

};

#endif
