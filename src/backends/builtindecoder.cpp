/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

using namespace lightspark;

BuiltinStreamDecoder::BuiltinStreamDecoder(std::istream& _s):
	stream(_s),prevSize(0),decodedAudioBytes(0),decodedVideoFrames(0),decodedTime(0),frameRate(0.0)
{
	STREAM_TYPE t=classifyStream(stream);
	if(t==FLV_STREAM)
	{
		FLV_HEADER h(stream);
		valid=h.isValid();
	}
	else
		valid=false;
}

BuiltinStreamDecoder::STREAM_TYPE BuiltinStreamDecoder::classifyStream(std::istream& s)
{
	char buf[3];
	s.read(buf,3);
	STREAM_TYPE ret;
	if(strncmp(buf,"FLV",3)==0)
		ret=FLV_STREAM;
	else
		throw ParseException("File signature not recognized");

	s.seekg(0);
	return ret;
}

bool BuiltinStreamDecoder::decodeNextFrame()
{
	UI32_FLV PreviousTagSize;
	stream >> PreviousTagSize;
	assert_and_throw(PreviousTagSize==prevSize);

	//Check tag type and read it
	UI8 TagType;
	stream >> TagType;
	switch(TagType)
	{
		case 8:
		{
			AudioDataTag tag(stream);
			prevSize=tag.getTotalLen();

			if(audioDecoder==NULL)
			{
				audioCodec=tag.SoundFormat;
				switch(tag.SoundFormat)
				{
					case AAC:
						assert_and_throw(tag.isHeader())
#ifdef ENABLE_LIBAVCODEC
						audioDecoder=new FFMpegAudioDecoder(tag.SoundFormat, tag.packetData, tag.packetLen);
#else
						audioDecoder=new NullAudioDecoder();
#endif
						tag.releaseBuffer();
						break;
					case MP3:
#ifdef ENABLE_LIBAVCODEC
						audioDecoder=new FFMpegAudioDecoder(tag.SoundFormat,NULL,0);
#else
						audioDecoder=new NullAudioDecoder();
#endif
						decodedAudioBytes+=audioDecoder->decodeData(tag.packetData,tag.packetLen,decodedTime);
						//Adjust timing
						decodedTime=decodedAudioBytes/audioDecoder->getBytesPerMSec();
						break;
					default:
						throw RunTimeException("Unsupported SoundFormat");
				}
			}
			else
			{
				assert_and_throw(audioCodec==tag.SoundFormat);
				decodedAudioBytes+=audioDecoder->decodeData(tag.packetData,tag.packetLen,decodedTime);
				//Adjust timing
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

			if(videoDecoder==NULL)
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
					videoDecoder=new FFMpegVideoDecoder(tag.codec,NULL,0,frameRate);
#else
					videoDecoder=new NullVideoDecoder();
#endif
					videoDecoder->decodeData(tag.packetData,tag.packetLen, frameTime);
					decodedVideoFrames++;
				}
			}
			else
			{
				videoDecoder->decodeData(tag.packetData,tag.packetLen, frameTime);
				decodedVideoFrames++;
			}
			break;
		}
		case 18:
		{
			metadataTag=ScriptDataTag(stream);
			prevSize=metadataTag.getTotalLen();

			//The frameRate of the container overrides the stream
			
			if(metadataTag.metadataDouble.find("framerate") != metadataTag.metadataDouble.end())
				frameRate=metadataTag.metadataDouble["framerate"];
			break;
		}
		default:
			LOG(LOG_ERROR,_("Unexpected tag type ") << (int)TagType << _(" in FLV"));
			return false;
	}
	return true;
}

bool BuiltinStreamDecoder::getMetadataInteger(const char* name, uint32_t& ret) const
{
	auto it=metadataTag.metadataInteger.find(name);
	if(it == metadataTag.metadataInteger.end())
		return false;

	ret=it->second;
	return true;
}

bool BuiltinStreamDecoder::getMetadataDouble(const char* name, double& ret) const
{
	auto it=metadataTag.metadataDouble.find(name);
	if(it == metadataTag.metadataDouble.end())
		return false;

	ret=it->second;
	return true;
}
