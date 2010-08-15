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

#include "flv.h"
#include "swftypes.h"

using namespace lightspark;
using namespace std;

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

	DataOffset.bswap();
	dataOffset = DataOffset;
	assert_and_throw(dataOffset==9);
}

VideoTag::VideoTag(istream& s)
{
	//Read dataSize
	UI24 DataSize;
	s >> DataSize;
	DataSize.bswap();
	dataSize=DataSize;

	//Read and assemble timestamp
	UI24 Timestamp;
	s >> Timestamp;
	Timestamp.bswap();
	UI8 TimestampExtended;
	s >> TimestampExtended;
	timestamp=Timestamp|(TimestampExtended<<24);
	
	UI24 StreamID;
	s >> StreamID;
	assert_and_throw(StreamID==0);
}

ScriptDataTag::ScriptDataTag(istream& s):VideoTag(s)
{
	unsigned int start=s.tellg();
	tiny_string methodName;

	//Specs talks about an arbitrary number of stuff, actually just a string and an array are expected
	UI8 Type;
	s >> Type;
	if(Type!=2)
		throw ParseException("Unexpected type in FLV");

	ScriptDataString String(s);
	methodName=String.getString();
	//cout << methodName << endl;

	s >> Type;
	if(Type!=8)
		throw ParseException("Unexpected type in FLV");

	ScriptECMAArray ecmaArray(s);
	frameRate=ecmaArray.frameRate;
	//Compute totalLen
	unsigned int end=s.tellg();
	totalLen=(end-start)+11;
}

ScriptDataString::ScriptDataString(std::istream& s)
{
	UI16 Length;
	s >> Length;
	Length.bswap();
	size=Length;

	//TODO: use resize on tiny_string
	char* buf=new char[Length+1];
	s.read(buf,Length);
	buf[Length]=0;

	val=tiny_string(buf,true);

	delete[] buf;
}

ScriptECMAArray::ScriptECMAArray(std::istream& s):frameRate(0)
{
	//numVar is an 'approximation' of array size
	UI32 numVar;
	s >> numVar;
	numVar.bswap();

	while(1)
	{
		ScriptDataString varName(s);
		//cout << varName.getString() << endl;
		UI8 Type;
		s >> Type;
		switch(Type)
		{
			case 0: //double (big-endian)
			{
				union
				{
					uint64_t i;
					double d;
				} tmp;
				s.read((char*)&tmp.i,8);
				tmp.i=be64toh(tmp.i);
				//HACK, extract fps information
				if(varName.getString()=="framerate")
					frameRate=tmp.d;
				break;
			}
			case 1:
			{
				UI8 b;
				s >> b;
				//cout << (int)b << endl;
				break;
			}
			case 2:
			{
				ScriptDataString String(s);
				//cout << String.getString() << endl;
				break;
			}
			case 9: //End of array
			{
				return;
			}
			default:
				cout << (int)Type << endl;
				throw ParseException("Unexpected type in FLV");
		}
	}
}

VideoDataTag::VideoDataTag(istream& s):VideoTag(s),_isHeader(false),packetData(NULL)
{
	unsigned int start=s.tellg();
	UI8 typeAndCodec;
	s >> typeAndCodec;

	frameType=(typeAndCodec>>4);
	int codecId=(typeAndCodec&0xf);

	if(frameType!=1 && frameType!=2)
		throw ParseException("Unexpected frameType in FLV");

	assert_and_throw(codecId==7 || codecId==2);

	if(codecId==2)
	{
		codec=H263;
		//H263 video packet
		//Is everything raw?
		//Compute lenght of raw data
		packetLen=dataSize-1;

		// fix warning
#ifndef NDEBUG
		int ret =
#endif
		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads
		assert(ret==0);

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

		SI24 CompositionTime;
		s >> CompositionTime;
		assert_and_throw(CompositionTime==0); //TODO: what are composition times

		//Compute lenght of raw data
		packetLen=dataSize-5;

		// fix warning
#ifndef NDEBUG
		int ret =
#endif
		aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads
		assert(ret==0);

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

AudioDataTag::AudioDataTag(std::istream& s):VideoTag(s),_isHeader(false)
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
	packetLen=dataSize-headerConsumed;

	// fix warning
#ifndef NDEBUG
	int ret =
#endif
	aligned_malloc((void**)&packetData, 16, packetLen+16); //Ensure no overrun happens when doing aligned reads
	assert(ret==0);

	s.read((char*)packetData,packetLen);
	memset(packetData+packetLen,0,16);

	//Compute totalLen
	unsigned int end=s.tellg();
	totalLen=(end-start)+11;
}

AudioDataTag::~AudioDataTag()
{
	aligned_free(packetData);
}
