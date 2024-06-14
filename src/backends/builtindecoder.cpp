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

#include "backends/builtindecoder.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/net/flashnet.h"
#include "swf.h"

using namespace lightspark;

BuiltinStreamDecoder::BuiltinStreamDecoder(std::istream& _s, NetStream* _ns, uint32_t _buffertime):
	stream(_s),prevSize(0),decodedAudioBytes(0),decodedVideoFrames(0),decodedTime(0),frameRate(0.0),netstream(_ns),headerbuf(nullptr),headerLen(0),buffertime(_buffertime)
{
	STREAM_TYPE t=classifyStream(stream);
	if(t==FLV_STREAM)
	{
		FLV_HEADER h(stream);
		valid=h.isValid();
		hasvideo=h.hasVideo();
	}
	else
		valid=false;
}

BuiltinStreamDecoder::~BuiltinStreamDecoder()
{
	if (headerLen)
	{
		delete headerbuf;
		headerLen = 0;
	}
}

BuiltinStreamDecoder::STREAM_TYPE BuiltinStreamDecoder::classifyStream(std::istream& s)
{
	char buf[3];
	s.read(buf,3);
	STREAM_TYPE ret;
	if(strncmp(buf,"FLV",3)==0)
		ret=FLV_STREAM;
	else
		ret=UNKOWN_STREAM;

	s.seekg(0);
	return ret;
}

bool BuiltinStreamDecoder::decodeNextFrame()
{
	UI32_FLV PreviousTagSize;
	stream >> PreviousTagSize;
	// It seems that Adobe simply ignores invalid values for PreviousTagSize
	//assert_and_throw(PreviousTagSize==prevSize);

	//Check tag type and read it
	UI8 TagType;
	stream >> TagType;
	switch(TagType)
	{
		case 8:
		{
			AudioDataTag tag(stream);
			prevSize=tag.getTotalLen();
			if (tag.packetLen == 0)
				return false;
			if (tag.isHeader() && tag.SoundFormat == AAC)
			{
				if (audioDecoder)
					break;
				if (headerLen)
					delete headerbuf;
				// store the aac header, don't pass it as initData to the FFMpegAudioDecoder constructor
				headerbuf = new uint8_t[tag.packetLen];
				memcpy(headerbuf,tag.packetData,tag.packetLen);
				headerLen = tag.packetLen;
			}
			if(audioDecoder==nullptr)
			{
				audioCodec=tag.SoundFormat;
				switch(tag.SoundFormat)
				{
					case AAC:
#ifdef ENABLE_LIBAVCODEC
						audioDecoder=new FFMpegAudioDecoder(netstream->getSystemState()->getEngineData(), tag.SoundFormat,nullptr,0,buffertime);// tag.packetData, tag.packetLen);
#else
						audioDecoder=new NullAudioDecoder();
#endif
						tag.releaseBuffer();
						break;
					case MP3:
#ifdef ENABLE_LIBAVCODEC
						audioDecoder=new FFMpegAudioDecoder(netstream->getSystemState()->getEngineData(), tag.SoundFormat,nullptr,0,buffertime);
#else
						audioDecoder=new NullAudioDecoder();
#endif
						decodedAudioBytes+=audioDecoder->decodeData(tag.packetData,tag.packetLen,decodedTime);
						//Adjust timing
						if (audioDecoder->getBytesPerMSec())
							decodedTime=decodedAudioBytes/audioDecoder->getBytesPerMSec();
						break;
					default:
						throw RunTimeException("Unsupported SoundFormat");
				}
			}
			else
			{
				assert_and_throw(audioCodec==tag.SoundFormat);
				if (headerLen)
				{
					// add aac header to this packet
					uint8_t buf[headerLen+tag.packetLen];
					memcpy(buf,headerbuf,headerLen);
					memcpy(buf+headerLen,tag.packetData,tag.packetLen);
					
					decodedAudioBytes+=audioDecoder->decodeData(buf,tag.packetLen+headerLen,decodedTime);
					delete headerbuf;
					headerLen = 0;
				}
				else
					decodedAudioBytes+=audioDecoder->decodeData(tag.packetData,tag.packetLen,decodedTime);
				//Adjust timing
				if (audioDecoder->getBytesPerMSec())
					decodedTime=decodedAudioBytes/audioDecoder->getBytesPerMSec();
			}
			break;
		}
		case 9:
		{
			VideoDataTag tag(stream);
			prevSize=tag.getTotalLen();
			//If the framerate is known give the right timing, otherwise use decodedTime from audio
			uint32_t frameTime=(frameRate!=0.0)?(decodedVideoFrames*1000/frameRate):decodedTime;

			if(videoDecoder==nullptr)
			{
				//If the isHeader flag is on then the decoder becomes the owner of the data
				if(tag.isHeader())
				{
					//The tag is the header, initialize decoding
#ifdef ENABLE_LIBAVCODEC
					videoDecoder=new FFMpegVideoDecoder(tag.codec,tag.packetData,tag.packetLen, frameRate);
#else
					videoDecoder=new NullVideoDecoder();
#endif
					tag.releaseBuffer();
				}
				else
				{
					//First packet but no special handling
#ifdef ENABLE_LIBAVCODEC
					videoDecoder=new FFMpegVideoDecoder(tag.codec,nullptr,0,frameRate);
#else
					videoDecoder=new NullVideoDecoder();
#endif
					videoDecoder->decodeData(tag.packetData,tag.packetLen, frameTime);
					videoDecoder->framesdecoded++;
					if (videoDecoder->frameRate != 0)
						frameRate = videoDecoder->frameRate;
					decodedVideoFrames++;
				}
			}
			else
			{
				if(tag.isHeader())
				{
					//The tag is the header, initialize decoding
					videoDecoder->switchCodec(tag.codec,tag.packetData,tag.packetLen,frameRate);
					tag.releaseBuffer();
				}
				else
				{
					videoDecoder->decodeData(tag.packetData,tag.packetLen, frameTime);
					videoDecoder->framesdecoded++;
					if (videoDecoder->frameRate != 0)
						frameRate = videoDecoder->frameRate;
					decodedVideoFrames++;
				}
			}
			break;
		}
		case 18:
		{
			ScriptDataTag tag(stream);
			prevSize=tag.getTotalLen();
			if (tag.methodName == "onMetaData")
			{
				// set framerate from metadata, if available
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=getSys()->getUniqueStringId("framerate");
				m.ns.emplace_back(getSys(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
				m.isAttribute = false;
				auto it = tag.dataobjectlist.begin();
				while (it != tag.dataobjectlist.end())
				{
					ASObject* o = asAtomHandler::getObject((*it));
					if(o && o->hasPropertyByMultiname(m,true,false,o->getInstanceWorker()))
					{
						asAtom v=asAtomHandler::invalidAtom;
						o->getVariableByMultiname(v,m,GET_VARIABLE_OPTION::NONE,o->getInstanceWorker());
						frameRate = asAtomHandler::toNumber(v);
						break;
					}
					it++;
				}
			}
			
			netstream->sendClientNotification(tag.methodName,tag.dataobjectlist);
			break;
		}
		default:
			LOG(LOG_ERROR,"Unexpected tag type " << (int)TagType << " in FLV");
			return false;
	}
	return true;
}

void BuiltinStreamDecoder::jumpToPosition(number_t position)
{
	if (position != 0)
		LOG(LOG_NOT_IMPLEMENTED,"jumpToPosition for BuiltinStreamDecoder");
}
