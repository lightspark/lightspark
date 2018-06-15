/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/abc.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/class.h"
#include "compat.h"
#include <iostream>
#include "backends/audio.h"
#include "backends/rendering.h"
#include "backends/streamcache.h"
#include "scripting/argconv.h"
#include <unistd.h>

using namespace lightspark;
using namespace std;

SoundTransform::SoundTransform(Class_base* c): ASObject(c,T_OBJECT,SUBTYPE_SOUNDTRANSFORM),volume(1.0),pan(0.0),leftToLeft(1.0),leftToRight(0),rightToLeft(0),rightToRight(1.0)
{
}

void SoundTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c,volume);
	REGISTER_GETTER_SETTER(c,pan);
	REGISTER_GETTER_SETTER(c,leftToLeft);
	REGISTER_GETTER_SETTER(c,leftToRight);
	REGISTER_GETTER_SETTER(c,rightToLeft);
	REGISTER_GETTER_SETTER(c,rightToRight);
}

ASFUNCTIONBODY_GETTER_SETTER(SoundTransform,volume)
ASFUNCTIONBODY_GETTER_SETTER(SoundTransform,pan)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(SoundTransform,leftToLeft)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(SoundTransform,leftToRight)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(SoundTransform,rightToLeft)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(SoundTransform,rightToRight)

ASFUNCTIONBODY_ATOM(SoundTransform,_constructor)
{
	SoundTransform* th=obj.as<SoundTransform>();
	ARG_UNPACK_ATOM(th->volume, 1.0)(th->pan, 0.0);
}

void Video::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("videoWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("videoHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),Video::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),Video::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, deblocking);
	REGISTER_GETTER_SETTER(c, smoothing);
}

ASFUNCTIONBODY_GETTER_SETTER(Video, deblocking);
ASFUNCTIONBODY_GETTER_SETTER(Video, smoothing);

void Video::buildTraits(ASObject* o)
{
}

void Video::finalize()
{
	DisplayObject::finalize();
	netStream.reset();
}

Video::Video(Class_base* c, uint32_t w, uint32_t h)
	: DisplayObject(c),width(w),height(h),videoWidth(0),videoHeight(0),
	  initialized(false),netStream(NullRef),deblocking(0),smoothing(false)
{
}

Video::~Video()
{
}

void Video::renderImpl(RenderContext& ctxt) const
{
	Mutex::Lock l(mutex);
	if(skipRender())
		return;

	//Video is especially optimized for GL rendering
	//It needs special treatment for SOFTWARE contextes
	if(ctxt.contextType != RenderContext::GL)
	{
		LOG(LOG_NOT_IMPLEMENTED, "Video::renderImpl on SOFTWARE context is not yet supported");
		return;
	}

	if(!netStream.isNull() && netStream->lockIfReady())
	{
		//All operations here should be non blocking
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();

		ctxt.setProperties(this->getBlendMode());
		const MATRIX totalMatrix=getConcatenatedMatrix();
		float m[16];
		totalMatrix.get4DMatrix(m);
		ctxt.lsglLoadMatrixf(m);

		//Enable YUV to RGB conversion
		//width and height will not change now (the Video mutex is acquired)
		ctxt.renderTextured(netStream->getTexture(), 0, 0, width, height,
			clippedAlpha(), RenderContext::YUV_MODE);
		
		netStream->unlock();
	}
}

bool Video::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

ASFUNCTIONBODY_ATOM(Video,_constructor)
{
	Video* th=obj.as<Video>();
	assert_and_throw(argslen<=2);
	if(0 < argslen)
		th->width=args[0].toInt();
	if(1 < argslen)
		th->height=args[1].toInt();
}

ASFUNCTIONBODY_ATOM(Video,_getVideoWidth)
{
	Video* th=obj.as<Video>();
	ret.setUInt(th->videoWidth);
}

ASFUNCTIONBODY_ATOM(Video,_getVideoHeight)
{
	Video* th=obj.as<Video>();
	ret.setUInt(th->videoHeight);
}

ASFUNCTIONBODY_ATOM(Video,_getWidth)
{
	Video* th=obj.as<Video>();
	ret.setUInt(th->width);
}

ASFUNCTIONBODY_ATOM(Video,_setWidth)
{
	Video* th=obj.as<Video>();
	Mutex::Lock l(th->mutex);
	assert_and_throw(argslen==1);
	th->width=args[0].toInt();
}

ASFUNCTIONBODY_ATOM(Video,_getHeight)
{
	Video* th=obj.as<Video>();
	ret.setUInt(th->height);
}

ASFUNCTIONBODY_ATOM(Video,_setHeight)
{
	Video* th=obj.as<Video>();
	assert_and_throw(argslen==1);
	Mutex::Lock l(th->mutex);
	th->height=args[0].toInt();
}

ASFUNCTIONBODY_ATOM(Video,attachNetStream)
{
	Video* th=obj.as<Video>();
	assert_and_throw(argslen==1);
	if(args[0].type==T_NULL || args[0].type==T_UNDEFINED) //Drop the connection
	{
		Mutex::Lock l(th->mutex);
		th->netStream=NullRef;
		return;
	}

	//Validate the parameter
	if(!args[0].is<NetStream>())
		throw RunTimeException("Type mismatch in Video::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Mutex::Lock l(th->mutex);
	th->netStream=_MR(args[0].as<NetStream>());
}
ASFUNCTIONBODY_ATOM(Video,clear)
{
	LOG(LOG_NOT_IMPLEMENTED,"clear is not implemented");
}

_NR<DisplayObject> Video::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	//TODO: support masks
	if(x>=0 && x<=width && y>=0 && y<=height)
		return last;
	else
		return NullRef;
}

Sound::Sound(Class_base* c)
	:EventDispatcher(c),downloader(NULL),soundData(new MemoryStreamCache(c->getSystemState())),
	 container(true),format(CODEC_NONE, 0, 0),bytesLoaded(0),bytesTotal(0),length(60*1000)
{
	subtype=SUBTYPE_SOUND;
}

Sound::Sound(Class_base* c, _R<StreamCache> data, AudioFormat _format)
	:EventDispatcher(c),downloader(NULL),soundData(data),
	 container(false),format(_format),
	 bytesLoaded(soundData->getReceivedLength()),
	 bytesTotal(soundData->getReceivedLength()),length(60*1000)
{
	subtype=SUBTYPE_SOUND;
}

Sound::~Sound()
{
	if(downloader && getSys()->downloadManager)
		getSys()->downloadManager->destroy(downloader);
}

void Sound::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	REGISTER_GETTER(c,bytesLoaded);
	REGISTER_GETTER(c,bytesTotal);
	REGISTER_GETTER(c,length);
}

void Sound::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Sound,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);

	if (argslen>0)
		Sound::load(ret,sys,obj, args, argslen);
}

ASFUNCTIONBODY_ATOM(Sound,load)
{
	Sound* th=obj.as<Sound>();
	_NR<URLRequest> urlRequest;
	_NR<SoundLoaderContext> context;
	
	ARG_UNPACK_ATOM(urlRequest)(context,NullRef);
	if (!urlRequest.isNull())
	{
		th->url = urlRequest->getRequestURL();
		urlRequest->getPostData(th->postData);
	}
	_R<StreamCache> c(_MR(new MemoryStreamCache(th->getSystemState())));
	th->soundData = c;

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(th->getSystemState())));
		return;
	}

	//The URL is valid so we can start the download

	if(th->postData.empty())
	{
		//This is a GET request
		//Use disk cache our downloaded files
		th->downloader=th->getSystemState()->downloadManager->download(th->url, th->soundData, th);
	}
	else
	{
		list<tiny_string> headers=urlRequest->getHeaders();
		th->downloader=th->getSystemState()->downloadManager->downloadWithData(th->url,
				th->soundData, th->postData, headers, th);
		//Clean up the postData for the next load
		th->postData.clear();
	}
	if(th->downloader->hasFailed())
	{
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(th->getSystemState())));
	}
}

ASFUNCTIONBODY_ATOM(Sound,play)
{
	Sound* th=obj.as<Sound>();
	number_t startTime;
	ARG_UNPACK_ATOM(startTime, 0);
	//TODO: use startTime
	if(startTime!=0)
		LOG(LOG_NOT_IMPLEMENTED,"startTime not supported in Sound::play");

	th->incRef();
	if (th->container)
		ret = asAtom::fromObject(Class<SoundChannel>::getInstanceS(sys,th->soundData));
	else
		ret = asAtom::fromObject(Class<SoundChannel>::getInstanceS(sys,th->soundData, th->format));
}

ASFUNCTIONBODY_ATOM(Sound,close)
{
	Sound* th=obj.as<Sound>();
	if(th->downloader)
		th->downloader->stop();
}

void Sound::setBytesTotal(uint32_t b)
{
	bytesTotal=b;
}

void Sound::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		bytesLoaded=b;
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded,bytesTotal)));
		if(bytesLoaded==bytesTotal)
		{
			this->incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
		}
	}
}

ASFUNCTIONBODY_GETTER(Sound,bytesLoaded);
ASFUNCTIONBODY_GETTER(Sound,bytesTotal);
ASFUNCTIONBODY_GETTER(Sound,length);

void SoundMixer::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("stopAll","",Class<IFunction>::getFunction(c->getSystemState(),stopAll),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("computeSpectrum","",Class<IFunction>::getFunction(c->getSystemState(),computeSpectrum),NORMAL_METHOD,false);
	REGISTER_GETTER_SETTER(c,bufferTime);
	REGISTER_GETTER_SETTER(c,soundTransform);
}
ASFUNCTIONBODY_GETTER_SETTER(SoundMixer,bufferTime);
ASFUNCTIONBODY_GETTER_SETTER(SoundMixer,soundTransform);

ASFUNCTIONBODY_ATOM(SoundMixer,stopAll)
{
	LOG(LOG_NOT_IMPLEMENTED,"SoundMixer.stopAll does nothing");
}
ASFUNCTIONBODY_ATOM(SoundMixer,computeSpectrum)
{
	_NR<ByteArray> output;
	bool FFTMode;
	int stretchFactor;
	ARG_UNPACK_ATOM (output) (FFTMode,false) (stretchFactor,0);
	output->setLength(0);
	output->setPosition(0);
	for (int i = 0; i < 4*512; i++) // 512 floats
		output->writeByte(0);
	output->setPosition(0);
	LOG(LOG_NOT_IMPLEMENTED,"SoundMixer.computeSpectrum not implemented");
}

void SoundLoaderContext::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,bufferTime);
	REGISTER_GETTER_SETTER(c,checkPolicyFile);
}

ASFUNCTIONBODY_ATOM(SoundLoaderContext,_constructor)
{
	SoundLoaderContext* th=obj.as<SoundLoaderContext>();
	
	assert_and_throw(argslen<=2);
	th->bufferTime = 1000;
	th->checkPolicyFile = false;
	ARG_UNPACK_ATOM(th->bufferTime,1000)(th->checkPolicyFile,false);
}

ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,bufferTime);
ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,checkPolicyFile);

SoundChannel::SoundChannel(Class_base* c, _NR<StreamCache> _stream, AudioFormat _format)
	: EventDispatcher(c),stream(_stream),stopped(false),audioDecoder(NULL),audioStream(NULL),
	format(_format),oldVolume(-1.0),soundTransform(_MR(Class<SoundTransform>::getInstanceS(c->getSystemState()))),
	leftPeak(1),position(0),rightPeak(1)
{
	subtype=SUBTYPE_SOUNDCHANNEL;
	if (!stream.isNull())
	{
		// Start playback
		incRef();
		getSys()->addJob(this);
	}
}

SoundChannel::~SoundChannel()
{
	threadAbort();
}

void SoundChannel::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);

	REGISTER_GETTER(c,leftPeak);
	REGISTER_GETTER(c,position);
	REGISTER_GETTER(c,rightPeak);
	REGISTER_GETTER_SETTER(c,soundTransform);
}

ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(SoundChannel,leftPeak);
ASFUNCTIONBODY_GETTER(SoundChannel,position);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(SoundChannel,rightPeak);
ASFUNCTIONBODY_GETTER_SETTER_CB(SoundChannel,soundTransform,validateSoundTransform);

void SoundChannel::buildTraits(ASObject* o)
{
}

void SoundChannel::finalize()
{
	EventDispatcher::finalize();
	soundTransform.reset();
}

void SoundChannel::validateSoundTransform(_NR<SoundTransform> oldValue)
{
	if (soundTransform.isNull())
	{
		soundTransform = oldValue;
		throwError<TypeError>(kNullPointerError, "soundTransform");
	}
}

ASFUNCTIONBODY_ATOM(SoundChannel,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
}

ASFUNCTIONBODY_ATOM(SoundChannel, stop)
{
	SoundChannel* th=obj.as<SoundChannel>();
	th->threadAbort();
}

void SoundChannel::execute()
{
	playStream();
}

void SoundChannel::playStream()
{
	assert(!stream.isNull());
	std::streambuf *sbuf = stream->createReader();
	istream s(sbuf);
	s.exceptions ( istream::failbit | istream::badbit );

	bool waitForFlush=true;
	StreamDecoder* streamDecoder=NULL;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		streamDecoder=new FFMpegStreamDecoder(this->getSystemState()->getEngineData(),s,&format,stream->hasTerminated() ? stream->getReceivedLength() : -1);
		if(!streamDecoder->isValid())
			threadAbort();

		while(!ACQUIRE_READ(stopped))
		{
			bool decodingSuccess=streamDecoder->decodeNextFrame();
			if(decodingSuccess==false)
				break;

			if(audioDecoder==NULL && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;

			if(audioStream==NULL && audioDecoder && audioDecoder->isValid())
				audioStream=getSystemState()->audioManager->createStream(audioDecoder,false);

			// TODO: check the position only when the getter is called
			if(audioStream)
				position=audioStream->getPlayedTime();

			if(audioStream)
			{
				//TODO: use soundTransform->pan
				if(soundTransform && soundTransform->volume != oldVolume)
				{
					audioStream->setVolume(soundTransform->volume);
					oldVolume = soundTransform->volume;
				}
			}
			
			if(threadAborting)
				throw JobTerminationException();
		}
#endif //ENABLE_LIBAVCODEC
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "Exception in SoundChannel " << e.cause);
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, _("Exception in reading SoundChannel: ")<<e.what());
	}
	if(waitForFlush)
	{
		//Put the decoders in the flushing state and wait for the complete consumption of contents
		if(audioDecoder)
		{
			audioDecoder->setFlushing();
			audioDecoder->waitFlushed();
		}
	}

	{
		Locker l(mutex);
		audioDecoder=NULL;
		delete audioStream;
		audioStream=NULL;
	}
	delete streamDecoder;
	delete sbuf;

	if (!ACQUIRE_READ(stopped))
	{
		incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"soundComplete")));
	}
}


void SoundChannel::jobFence()
{
	this->decRef();
}

void SoundChannel::threadAbort()
{
	RELEASE_WRITE(stopped,true);
	Locker l(mutex);
	if(audioDecoder)
	{
		//Clear everything we have in buffers, discard all frames
		audioDecoder->setFlushing();
		audioDecoder->skipAll();
	}
}

void StageVideo::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("videoWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("videoHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
}


void StageVideo::finalize()
{
	netStream.reset();
}

ASFUNCTIONBODY_ATOM(StageVideo,_getVideoWidth)
{
	StageVideo* th=obj.as<StageVideo>();
	ret.setUInt(th->videoWidth);
}

ASFUNCTIONBODY_ATOM(StageVideo,_getVideoHeight)
{
	StageVideo* th=obj.as<StageVideo>();
	ret.setUInt(th->videoHeight);
}

ASFUNCTIONBODY_ATOM(StageVideo,attachNetStream)
{
	StageVideo* th=obj.as<StageVideo>();
	assert_and_throw(argslen==1);
	if(args[0].type==T_NULL || args[0].type==T_UNDEFINED) //Drop the connection
	{
		Mutex::Lock l(th->mutex);
		th->netStream=NullRef;
		return;
	}

	//Validate the parameter
	if(!args[0].is<NetStream>())
		throw RunTimeException("Type mismatch in StageVideo::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Mutex::Lock l(th->mutex);
	th->netStream=_MR(args[0].as<NetStream>());
}

void StageVideoAvailability::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("AVAILABLE","",abstract_s(c->getSystemState(),"available"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",abstract_s(c->getSystemState(),"unavailable"),DECLARED_TRAIT);
}

void VideoStatus::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("ACCELERATED","",abstract_s(c->getSystemState(),"accelerated"),DECLARED_TRAIT);
	c->setVariableByQName("SOFTWARE","",abstract_s(c->getSystemState(),"software"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",abstract_s(c->getSystemState(),"unavailable"),DECLARED_TRAIT);
}
void Microphone::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER(c,isSupported);
	c->setDeclaredMethodByQName("getMicrophone","",Class<IFunction>::getFunction(c->getSystemState(),getMicrophone),NORMAL_METHOD,false);

}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Microphone,isSupported)

ASFUNCTIONBODY_ATOM(Microphone,getMicrophone)
{
	LOG(LOG_NOT_IMPLEMENTED,"Microphone.getMicrophone always returns null");
	ret.setNull();
}

void Camera::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER(c,isSupported);

}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Camera,isSupported)

void VideoStreamSettings::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("setKeyFrameInterval","",Class<IFunction>::getFunction(c->getSystemState(),setKeyFrameInterval),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMode","",Class<IFunction>::getFunction(c->getSystemState(),setMode),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings is a stub");
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,setKeyFrameInterval)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings.setKeyFrameInterval");
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,setMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings.setMode");
}

void H264VideoStreamSettings::sinit(Class_base* c)
{
	CLASS_SETUP(c, VideoStreamSettings, _constructor, CLASS_SEALED);
}
ASFUNCTIONBODY_ATOM(H264VideoStreamSettings,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"H264VideoStreamSettings is a stub");
}
