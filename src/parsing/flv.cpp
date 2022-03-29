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

#include "parsing/flv.h"
#include "swftypes.h"
#include "compat.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"
#include "amf3_generator.h"

using namespace lightspark;
using namespace std;


FLV_HEADER::FLV_HEADER(std::istream& in):dataOffset(0),_hasAudio(false),_hasVideo(false)
{
	UI8 Signature[3];
	UI8 Version;
	UI32_FLV DataOffset;

	in >> Signature[0] >> Signature[1] >> Signature[2] >> Version;
	version=Version;
	
	if(Signature[0]=='F' && Signature[1]=='L' && Signature[2]=='V')
	{
		LOG(LOG_INFO, "PARSING: FLV file: Version " << (int)Version);
		valid=true;
	}
	else
	{
		LOG(LOG_INFO,"PARSING: No FLV file signature found");
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
	assert_and_throw(dataOffset==9);
}

VideoTag::VideoTag(istream& s)
{
	//Read dataSize
	UI24_FLV DataSize;
	s >> DataSize;
	dataSize=DataSize;

	//Read and assemble timestamp
	UI24_FLV Timestamp;
	s >> Timestamp;
	UI8 TimestampExtended;
	s >> TimestampExtended;
	timestamp=Timestamp|(TimestampExtended<<24);
	
	UI24_FLV StreamID;
	s >> StreamID;
	assert_and_throw(StreamID==0);
}

ScriptDataTag::ScriptDataTag(istream& s):VideoTag(s)
{
	unsigned int start=s.tellg();

	if (dataSize > 0)
	{
		_R<ByteArray> b = _NR<ByteArray>(Class<ByteArray>::getInstanceS(getWorker()));
		uint8_t* data =b->getBuffer(dataSize,true);
		s.read((char*)data,dataSize);
		b->setObjectEncoding(OBJECT_ENCODING::AMF0);
		b->setCurrentObjectEncoding(OBJECT_ENCODING::AMF0);
		b->setPosition(0);
		uint8_t tagtype;
		if (!b->readByte(tagtype))
			throw ParseException("Not enough data to parse tag type");
		if (tagtype != amf0_string_marker)
			throw ParseException("wrong tagtype in ScriptDataTag");
		
		Amf3Deserializer d(b.getPtr());
		methodName=d.parseStringAMF0();
		
		while (b->getPosition() < dataSize)
			dataobjectlist.push_back(d.readObject());
	}

	//Compute totalLen
	unsigned int end=s.tellg();
	totalLen=(end-start)+11;
}


VideoDataTag::VideoDataTag(istream& s):VideoTag(s),_isHeader(false),packetData(nullptr)
{
	unsigned int start=s.tellg();
	UI8 typeAndCodec;
	s >> typeAndCodec;

	frameType=(typeAndCodec>>4);
	int codecId=(typeAndCodec&0xf);

	if(frameType!=1 && frameType!=2)
		throw ParseException("Unexpected frameType in FLV");

	assert_and_throw(codecId==2 || codecId==4 || codecId==7);

	if(codecId==2)
	{
		codec=H263;
		//H263 video packet
		//Compute lenght of raw data
		packetLen=dataSize-1;

		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads

		s.read((char*)packetData,packetLen);
		memset(packetData+packetLen,0,16);
	}
	else if(codecId==4)
	{
		codec=VP6;
		//VP6 video packet
		uint8_t adjustment;
		s.read((char*)&adjustment,1);
		//TODO: support adjustment (FFMPEG accept extradata for it)
		assert(adjustment==0);
		//Compute lenght of raw data
		packetLen=dataSize-2;

		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads

		s.read((char*)packetData,packetLen);
		memset(packetData+packetLen,0,16);
	}
	else if(codecId==7)
	{
		codec=H264;
		//AVCVideoPacket
		UI8 packetType;
		s >> packetType;
		switch(packetType)
		{
			case 0: //Sequence header
				_isHeader=true;
				break;
			case 1: //NALU
			case 2: //End of sequence
				break;
			default:
				throw UnsupportedException("Unexpected packet type in FLV");
		}

		SI24_FLV CompositionTime;
		s >> CompositionTime;
		//assert_and_throw(CompositionTime==0); //TODO: what are composition times

		//Compute lenght of raw data
		packetLen=dataSize-5;

		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads

		s.read((char*)packetData,packetLen);
		memset(packetData+packetLen,0,16);
	}

	//Compute totalLen
	unsigned int end=s.tellg();
	totalLen=(end-start)+11;
}

VideoDataTag::~VideoDataTag()
{
	aligned_free(packetData);
}

AudioDataTag::AudioDataTag(std::istream& s):VideoTag(s),_isHeader(false),packetData(nullptr),packetLen(0)
{
	unsigned int start=s.tellg();
	BitStream bs(s);
	SoundFormat=(LS_AUDIO_CODEC)(int)UB(4,bs);
	switch(UB(2,bs))
	{
		case 0:
			SoundRate=5500;
			break;
		case 1:
			SoundRate=11000;
			break;
		case 2:
			SoundRate=22000;
			break;
		case 3:
			SoundRate=44000;
			break;
	}
	is16bit=UB(1,bs);
	isStereo=UB(1,bs);

	uint32_t headerConsumed=1;
	//Special handling for AAC data
	if(SoundFormat==AAC)
	{
		UI8 t;
		s >> t;
		_isHeader=(t==0);
		headerConsumed++;
	}
	if (dataSize > headerConsumed)
	{
		packetLen=dataSize-headerConsumed;
	
		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads
	
		s.read((char*)packetData,packetLen);
		memset(packetData+packetLen,0,16);
	}
	//Compute totalLen
	unsigned int end=s.tellg();
	totalLen=(end-start)+11;
}

AudioDataTag::~AudioDataTag()
{
	if (packetData)
		aligned_free(packetData);
}
