/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_BUILTINDECODER_H
#define BACKENDS_BUILTINDECODER_H 1

#include "backends/decoder.h"
#include "parsing/flv.h"

namespace lightspark
{
class NetStream;

class BuiltinStreamDecoder: public StreamDecoder
{
private:
	std::istream& stream;
	unsigned int prevSize;
	LS_AUDIO_CODEC audioCodec;
	uint32_t decodedAudioBytes;
	uint32_t decodedVideoFrames;
	//The decoded time is computed from the decodedAudioBytes to avoid drifts
	uint32_t decodedTime;
	double frameRate;
	ScriptDataTag metadataTag;
	enum STREAM_TYPE { FLV_STREAM=0, UNKOWN_STREAM=1 };
	STREAM_TYPE classifyStream(std::istream& s);
	NetStream* netstream;
	uint8_t* headerbuf;
	uint32_t headerLen;
	uint32_t buffertime;
public:
	BuiltinStreamDecoder(std::istream& _s, NetStream* _ns, uint32_t _buffertime);
	~BuiltinStreamDecoder();
	bool decodeNextFrame() override;
	void jumpToPosition(number_t position) override;
};

}

#endif /* BACKENDS_BUILTINDECODER_H */
