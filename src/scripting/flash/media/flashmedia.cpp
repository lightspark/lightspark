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
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	ARG_UNPACK_ATOM(th->volume, 1.0)(th->pan, 0.0);
}

void Video::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
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

bool Video::destruct()
{
	videotag=nullptr;
	resetDecoder();
	netStream.reset();
	return DisplayObject::destruct();
}

void Video::finalize()
{
	resetDecoder();
	netStream.reset();
	DisplayObject::finalize();
}

void Video::resetDecoder()
{
	Locker l(mutex);
	lastuploadedframe=UINT32_MAX;
	if (embeddedVideoDecoder)
	{
		if (embeddedVideoDecoder->isUploading())
			embeddedVideoDecoder->markForDestruction();
		else
			delete embeddedVideoDecoder;
		embeddedVideoDecoder=nullptr;
	}
}

Video::Video(Class_base* c, uint32_t w, uint32_t h, DefineVideoStreamTag *v)
	: DisplayObject(c),width(w),height(h),videoWidth(0),videoHeight(0),
	  netStream(NullRef),deblocking(v ? v->VideoFlagsDeblocking:0),smoothing(v ? v->VideoFlagsSmoothing : false),videotag(v),embeddedVideoDecoder(nullptr),lastuploadedframe(UINT32_MAX)
{
	subtype=SUBTYPE_VIDEO;
}

void Video::checkRatio(uint32_t ratio, bool inskipping)
{
	Locker l(mutex);
	if (videotag)
	{
#ifdef ENABLE_LIBAVCODEC
		if (embeddedVideoDecoder==nullptr)
		{
			LS_VIDEO_CODEC lscodec;
			bool ok=false;
			switch (videotag->VideoCodecID)
			{
				case 2:
					lscodec = LS_VIDEO_CODEC::H263;
					ok=true;
					break;
				case 3:
					LOG(LOG_ERROR,"video codec SCREEN not implemented for embedded video");
					break;
				case 4:
					lscodec = LS_VIDEO_CODEC::VP6;
					ok=true;
					break;
				case 5:
					lscodec = LS_VIDEO_CODEC::VP6A;
					ok=true;
					break;
				default:
					LOG(LOG_ERROR,"invalid video codec id for embedded video:"<<int(videotag->VideoCodecID));
					break;
			}
			if (ok)
				embeddedVideoDecoder = new FFMpegVideoDecoder(lscodec,nullptr,0,videotag->loadedFrom->getFrameRate(),videotag);
			lastuploadedframe=UINT32_MAX;
		}
#endif
		if (embeddedVideoDecoder && !embeddedVideoDecoder->isUploading() && ratio > 0 && ratio <= videotag->NumFrames && videotag->frames[ratio-1])
		{
			if (lastuploadedframe == UINT32_MAX)
			{
				embeddedVideoDecoder->decodeData(videotag->frames[ratio-1]->getData(),videotag->frames[ratio-1]->getNumBytes(),ratio);
				embeddedVideoDecoder->waitForFencing();
				lastuploadedframe = ratio;
				getSystemState()->getRenderThread()->addUploadJob(embeddedVideoDecoder);
			}
			if (ratio != lastuploadedframe && !embeddedVideoDecoder->isUploading())
			{
				embeddedVideoDecoder->waitForFencing();
				embeddedVideoDecoder->setVideoFrameToDecode(ratio-1);
				lastuploadedframe = ratio;
				getSystemState()->getRenderThread()->addUploadJob(embeddedVideoDecoder);
			}
		}
		if (lastuploadedframe==uint32_t(videotag->NumFrames-1))
		{
			resetDecoder();
		}
	}
}

void Video::afterLegacyDelete(DisplayObjectContainer *par)
{
	Locker l(mutex);
	resetDecoder();
}

void Video::setOnStage(bool staged, bool force)
{
	if(staged!=onStage||force)
	{
		if (!staged)
		{
			Locker l(mutex);
			resetDecoder();
		}
	}
	DisplayObject::setOnStage(staged,force);
}

uint32_t Video::getTagID() const
{
	return videotag ? uint32_t(videotag->CharacterID) : UINT32_MAX;
}

Video::~Video()
{
}

bool Video::renderImpl(RenderContext& ctxt) const
{
	Locker l(mutex);
	if(skipRender())
		return false;

	//Video is especially optimized for GL rendering
	//It needs special treatment for SOFTWARE contextes
	if(ctxt.contextType != RenderContext::GL)
	{
		LOG(LOG_NOT_IMPLEMENTED, "Video::renderImpl on SOFTWARE context is not yet supported");
		return false;
	}

	bool valid=false;
	if(!netStream.isNull() && netStream->lockIfReady())
	{
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();
		valid=true;
	}
	if (videotag)
	{
		videoWidth=videotag->Width;
		videoHeight=videotag->Height;
		valid=embeddedVideoDecoder!= nullptr && embeddedVideoDecoder->getTexture().isValid();
	}
	if (valid)
	{
		//All operations here should be non blocking
		ctxt.setProperties(this->getBlendMode());
		const MATRIX totalMatrix=getConcatenatedMatrix();
		float m[16];
		totalMatrix.get4DMatrix(m);
		ctxt.lsglLoadMatrixf(m);
	
		float scalex;
		float scaley;
		int offx,offy;
		getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth,getSystemState()->getRenderThread()->windowHeight,offx,offy, scalex,scaley);

		//Enable YUV to RGB conversion
		//width and height will not change now (the Video mutex is acquired)
		ctxt.renderTextured(embeddedVideoDecoder ? embeddedVideoDecoder->getTexture() : netStream->getTexture(),
			0, 0, width*scalex, height*scaley,
			clippedAlpha(), RenderContext::YUV_MODE,totalMatrix.getRotation(),
			0, 0, // transformed position is already set through lsglLoadMatrixf above
			width*scalex,height*scaley,
			1.0f,1.0f,
			1.0f,1.0f,1.0f,1.0f,
			0.0f,0.0f,0.0f,0.0f,
			false,false);
		if (!videotag)
			netStream->unlock();
		return false;
	}
	return true;
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
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen<=2);
	if(0 < argslen)
		th->width=asAtomHandler::toInt(args[0]);
	if(1 < argslen)
		th->height=asAtomHandler::toInt(args[1]);
}

ASFUNCTIONBODY_ATOM(Video,_getVideoWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,sys,th->videoWidth);
}

ASFUNCTIONBODY_ATOM(Video,_getVideoHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,sys,th->videoHeight);
}

ASFUNCTIONBODY_ATOM(Video,_getWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,sys,th->width);
}

ASFUNCTIONBODY_ATOM(Video,_setWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	Locker l(th->mutex);
	assert_and_throw(argslen==1);
	th->width=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(Video,_getHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,sys,th->height);
}

ASFUNCTIONBODY_ATOM(Video,_setHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen==1);
	Locker l(th->mutex);
	th->height=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(Video,attachNetStream)
{
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0])) //Drop the connection
	{
		Locker l(th->mutex);
		th->netStream=NullRef;
		return;
	}

	//Validate the parameter
	if(!asAtomHandler::is<NetStream>(args[0]))
		throw RunTimeException("Type mismatch in Video::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Locker l(th->mutex);
	th->netStream=_MR(asAtomHandler::as<NetStream>(args[0]));
}
ASFUNCTIONBODY_ATOM(Video,clear)
{
	LOG(LOG_NOT_IMPLEMENTED,"clear is not implemented");
}

_NR<DisplayObject> Video::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//TODO: support masks
	if(x>=0 && x<=width && y>=0 && y<=height)
		return last;
	else
		return NullRef;
}

Sound::Sound(Class_base* c)
	:EventDispatcher(c),downloader(NULL),soundData(new MemoryStreamCache(c->getSystemState())),
	 container(true),format(CODEC_NONE, 0, 0),bytesLoaded(0),bytesTotal(0),length(-1)
{
	subtype=SUBTYPE_SOUND;
}

Sound::Sound(Class_base* c, _R<StreamCache> data, AudioFormat _format, number_t duration_in_ms)
	:EventDispatcher(c),downloader(NULL),soundData(data),
	 container(false),format(_format),
	 bytesLoaded(soundData->getReceivedLength()),
	 bytesTotal(soundData->getReceivedLength()),length(duration_in_ms)
{
	subtype=SUBTYPE_SOUND;
}

Sound::~Sound()
{
	if(downloader && getSystemState()->downloadManager)
		getSystemState()->downloadManager->destroy(downloader);
}

void Sound::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadCompressedDataFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),loadCompressedDataFromByteArray),NORMAL_METHOD,true);
	REGISTER_GETTER(c,bytesLoaded);
	REGISTER_GETTER(c,bytesTotal);
	REGISTER_GETTER(c,length);
}

ASFUNCTIONBODY_ATOM(Sound,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);

	if (argslen>0)
		Sound::load(ret,sys,obj, args, argslen);
}

ASFUNCTIONBODY_ATOM(Sound,load)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
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
		th->incRef();
		th->downloader=th->getSystemState()->downloadManager->download(th->url, th->soundData, th);
	}
	else
	{
		list<tiny_string> headers=urlRequest->getHeaders();
		th->incRef();
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
	Sound* th=asAtomHandler::as<Sound>(obj);
	number_t startTime;
	ARG_UNPACK_ATOM(startTime, 0);
	if (!sys->mainClip->usesActionScript3) // actionscript2 expects the starttime in seconds, actionscript3 in milliseconds
		startTime *= 1000;
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED,"Sound.play with more than one argument");

	th->incRef();
	if (th->container)
	{
		_NR<ByteArray> data = _MR(Class<ByteArray>::getInstanceS(th->getSystemState()));
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<SampleDataEvent>::getInstanceS(th->getSystemState(),data,0)));
		th->soundChannel = _MR(Class<SoundChannel>::getInstanceS(sys,th->soundData, AudioFormat(LINEAR_PCM_FLOAT_BE,44100,2),false));
		th->soundChannel->setStartTime(startTime);
		th->soundChannel->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(th->soundChannel.getPtr());
	}
	else
	{
		if (th->soundChannel)
		{
			ret = asAtomHandler::fromObjectNoPrimitive(th->soundChannel.getPtr());
 			th->soundChannel->play(startTime);
			return;
		}
		SoundChannel* s = Class<SoundChannel>::getInstanceS(sys,th->soundData, th->format);
		s->setStartTime(startTime);
		ret = asAtomHandler::fromObjectNoPrimitive(s);
	}
}

ASFUNCTIONBODY_ATOM(Sound,close)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	if(th->downloader)
	{
		th->downloader->stop();
		th->decRef();
	}
}


ASFUNCTIONBODY_ATOM(Sound,loadCompressedDataFromByteArray)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	_NR<ByteArray> bytes;
	uint32_t bytesLength;

	ARG_UNPACK_ATOM(bytes)(bytesLength);
	_R<StreamCache> c(_MR(new MemoryStreamCache(th->getSystemState())));
	th->soundData = c;
	if (bytes)
	{
		uint8_t* buf = new uint8_t[bytesLength];
		if (bytes->readBytes(bytes->getPosition(),bytesLength,buf))
		{
			th->soundData->append(buf,bytesLength);
		}
		delete[] buf;
	}
}
void Sound::afterExecution(_R<Event> e)
{
	if (e->type == "sampleData")
	{
		_NR<ByteArray> data = e->as<SampleDataEvent>()->data;
		if (data.isNull())
		{
			this->soundData->markFinished(false);
			return;
		}
		uint32_t len = data->getLength();
		uint32_t reallen = len;
		this->soundData->append(data->getBuffer(len,false),len);
		if (reallen)
			soundChannel->resume();
		if (len < 2048*sizeof(float))
		{
			// less than 2048 samples, stop requesting data
			this->soundData->markFinished(true);
		}
		else
		{
			// request more data
			_NR<ByteArray> data = _MR(Class<ByteArray>::getInstanceS(getSystemState()));
			incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<SampleDataEvent>::getInstanceS(getSystemState(),data,this->soundData->getReceivedLength())));
		}
	}
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
		// make sure that the event queue is not flooded with progressEvents
		if (progressEvent.isNull())
		{
			this->incRef();
			progressEvent = _MR(Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded,bytesTotal));
			progressEvent->incRef();
			getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
		}
		else
		{
			// event already exists, we only update the values
			Locker l(progressEvent->accesmutex);
			progressEvent->bytesLoaded = bytesLoaded;
			progressEvent->bytesTotal = bytesTotal;
			// if event is already in event queue, we don't need to add it again
			if (!progressEvent->queued)
			{
				this->incRef();
				progressEvent->incRef();
				getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
			}
		}
		if(bytesLoaded==bytesTotal)
		{
			this->incRef();
			getVm(getSystemState())->addIdleEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
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
	REGISTER_GETTER_SETTER_STATIC(c,bufferTime);
	REGISTER_GETTER_SETTER_STATIC(c,soundTransform);
}
ASFUNCTIONBODY_GETTER_SETTER_STATIC(SoundMixer,bufferTime);
ASFUNCTIONBODY_GETTER_SETTER_STATIC(SoundMixer,soundTransform);

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
	SoundLoaderContext* th=asAtomHandler::as<SoundLoaderContext>(obj);
	
	assert_and_throw(argslen<=2);
	th->bufferTime = 1000;
	th->checkPolicyFile = false;
	ARG_UNPACK_ATOM(th->bufferTime,1000)(th->checkPolicyFile,false);
}

ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,bufferTime);
ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,checkPolicyFile);

SoundChannel::SoundChannel(Class_base* c, _NR<StreamCache> _stream, AudioFormat _format, bool autoplay)
	: EventDispatcher(c),stream(_stream),stopped(true),terminated(true),audioDecoder(NULL),audioStream(NULL),
	format(_format),oldVolume(-1.0),startTime(0),restartafterabort(false),soundTransform(_MR(Class<SoundTransform>::getInstanceS(c->getSystemState()))),
	leftPeak(1),rightPeak(1)
{
	subtype=SUBTYPE_SOUNDCHANNEL;
	if (autoplay)
		play();
}

SoundChannel::~SoundChannel()
{
}

void SoundChannel::appendStreamBlock(unsigned char *buf, int len)
{
	if (stream)
		SoundStreamBlockTag::decodeSoundBlock(stream.getPtr(),format.codec,buf,len);
}

void SoundChannel::play(number_t starttime)
{
	mutex.lock();
	if (!ACQUIRE_READ(stopped))
	{
		RELEASE_WRITE(stopped,true);
		if (stream)
		{
			if (audioStream)
				startTime = audioStream->getPlayedTime();
			stream->markFinished(false);
		}
		if(audioDecoder)
		{
			//Clear everything we have in buffers, discard all frames
			audioDecoder->setFlushing();
			audioDecoder->skipAll();
			audioDecoder=nullptr;
		}
		restartafterabort=true;
		startTime = starttime;
	}
	else
	{
		if (restartafterabort)
		{
			mutex.unlock();
			return;
		}
		mutex.unlock();
		while (!ACQUIRE_READ(terminated))
			compat_msleep(10);
		mutex.lock();
		restartafterabort=false;
		startTime = starttime;
		if (!stream.isNull() && ACQUIRE_READ(stopped))
		{
			// Start playback
			incRef();
			getSystemState()->addJob(this);
			RELEASE_WRITE(stopped,false);
			RELEASE_WRITE(terminated,false);
		}
	}
	mutex.unlock();
}
void SoundChannel::resume()
{
	if (!stream.isNull() && ACQUIRE_READ(stopped))
	{
		// Start playback
		incRef();
		getSystemState()->addJob(this);
		RELEASE_WRITE(stopped,false);
		RELEASE_WRITE(terminated,false);
	}
}

void SoundChannel::markFinished()
{
	if (stream)
		stream->markFinished();
}

void SoundChannel::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),getPosition),GETTER_METHOD,true);

	REGISTER_GETTER(c,leftPeak);
	REGISTER_GETTER(c,rightPeak);
	REGISTER_GETTER_SETTER(c,soundTransform);
}

ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(SoundChannel,leftPeak);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(SoundChannel,rightPeak);
ASFUNCTIONBODY_GETTER_SETTER_CB(SoundChannel,soundTransform,validateSoundTransform);

void SoundChannel::buildTraits(ASObject* o)
{
}

void SoundChannel::finalize()
{
	EventDispatcher::finalize();
	soundTransform.reset();
	threadAbort();
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
	SoundChannel* th=asAtomHandler::as<SoundChannel>(obj);
	th->threadAbort();
	while (!ACQUIRE_READ(th->terminated))
		compat_msleep(10);
}
ASFUNCTIONBODY_ATOM(SoundChannel,getPosition)
{
	if(!asAtomHandler::is<SoundChannel>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	SoundChannel* th = asAtomHandler::as<SoundChannel>(obj);
	// TODO adobe seems to add some buffering time to the position, but the mechanism behind that is unclear
	// so for now we just add 500ms
	asAtomHandler::setUInt(ret,sys,th->audioStream ? th->audioStream->getPlayedTime()+500 : th->startTime);
}
void SoundChannel::execute()
{
	playStream();
}

void SoundChannel::playStream()
{
	// ensure audio manager is initialized
	getSystemState()->waitInitialized();
	assert(!stream.isNull());
	std::streambuf *sbuf = stream->createReader();
	istream s(sbuf);
	s.exceptions ( istream::failbit | istream::badbit );

	bool waitForFlush=true;
	StreamDecoder* streamDecoder=nullptr;
	{
		mutex.lock();
		if (audioStream)
		{
			delete audioStream;
			audioStream=nullptr;
		}
		mutex.unlock();
	}
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		streamDecoder=new FFMpegStreamDecoder(nullptr,this->getSystemState()->getEngineData(),s,&format,stream->hasTerminated() ? stream->getReceivedLength() : -1);
		if(!streamDecoder->isValid())
		{
			LOG(LOG_ERROR,"invalid streamDecoder");
			threadAbort();
			restartafterabort=false;
		}
		else
		{
			streamDecoder->jumpToPosition(this->startTime);
			if (audioStream)
				audioStream->setPlayedTime(this->startTime);
		}
		while(!ACQUIRE_READ(stopped))
		{
			bool decodingSuccess=streamDecoder->decodeNextFrame();
			if(decodingSuccess==false)
				break;
			if(audioDecoder==nullptr && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;

			if(audioStream==nullptr && audioDecoder && audioDecoder->isValid())
				audioStream=getSystemState()->audioManager->createStream(audioDecoder,false,this,startTime);

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
		if(streamDecoder && streamDecoder->audioDecoder)
		{
			streamDecoder->audioDecoder->setFlushing();
			streamDecoder->audioDecoder->waitFlushed();
		}
	}

	{
		mutex.lock();
		audioDecoder=nullptr;
		delete audioStream;
		audioStream=nullptr;
		mutex.unlock();
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
	mutex.lock();
	RELEASE_WRITE(terminated,true);
	if (restartafterabort && !getSystemState()->isShuttingDown())
	{
		restartafterabort=false;
		incRef();
		getSystemState()->addJob(this);
		RELEASE_WRITE(stopped,false);
		RELEASE_WRITE(terminated,false);
	}
	// ensure that this is moved to freelist in vm thread
	if (getVm(getSystemState()))
	{
		getVm(getSystemState())->addDeletableObject(this);
	}
	else
		this->decRef();
	mutex.unlock();
}

void SoundChannel::threadAbort()
{
	mutex.lock();
	if (ACQUIRE_READ(stopped))
	{
		mutex.unlock();
		return;
	}
	RELEASE_WRITE(stopped,true);
	if (stream)
	{
		if (audioStream)
			startTime = audioStream->getPlayedTime();
		stream->markFinished(false);
	}
	if(audioDecoder)
	{
		//Clear everything we have in buffers, discard all frames
		audioDecoder->setFlushing();
		audioDecoder->skipAll();
		audioDecoder=nullptr;
	}
	mutex.unlock();
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
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	asAtomHandler::setUInt(ret,sys,th->videoWidth);
}

ASFUNCTIONBODY_ATOM(StageVideo,_getVideoHeight)
{
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	asAtomHandler::setUInt(ret,sys,th->videoHeight);
}

ASFUNCTIONBODY_ATOM(StageVideo,attachNetStream)
{
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0])) //Drop the connection
	{
		Locker l(th->mutex);
		th->netStream=NullRef;
		return;
	}

	//Validate the parameter
	if(!asAtomHandler::is<NetStream>(args[0]))
		throw RunTimeException("Type mismatch in StageVideo::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Locker l(th->mutex);
	th->netStream=_MR(asAtomHandler::as<NetStream>(args[0]));
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
	asAtomHandler::setNull(ret);
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
