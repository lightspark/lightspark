/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "backends/input.h"
#include "backends/rendering.h"
#include "scripting/argconv.h"

using namespace lightspark;
using namespace std;

void SoundTransform::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c,volume);
	REGISTER_GETTER_SETTER(c,pan);
}

ASFUNCTIONBODY_GETTER_SETTER(SoundTransform,volume);
ASFUNCTIONBODY_GETTER_SETTER(SoundTransform,pan);

ASFUNCTIONBODY(SoundTransform,_constructor)
{
	SoundTransform* th=Class<SoundTransform>::cast(obj);
	assert_and_throw(argslen<=2);
	th->volume = 1.0;
	th->pan = 0.0;
	if(0 < argslen)
		th->volume = ArgumentConversion<number_t>::toConcrete(args[0]);
	if(1 < argslen)
		th->pan = ArgumentConversion<number_t>::toConcrete(args[1]);
	return NULL;
}

void Video::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
	c->setDeclaredMethodByQName("videoWidth","",Class<IFunction>::getFunction(_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("videoHeight","",Class<IFunction>::getFunction(_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(Video::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(Video::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(Video::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(Video::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(attachNetStream),NORMAL_METHOD,true);
}

void Video::buildTraits(ASObject* o)
{
}

void Video::finalize()
{
	DisplayObject::finalize();
	netStream.reset();
}

Video::Video(Class_base* c, uint32_t w, uint32_t h)
	: DisplayObject(c),width(w),height(h),videoWidth(0),videoHeight(0),initialized(false),netStream(NullRef)
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

ASFUNCTIONBODY(Video,_constructor)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen<=2);
	if(0 < argslen)
		th->width=args[0]->toInt();
	if(1 < argslen)
		th->height=args[1]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Video,_getVideoWidth)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->videoWidth);
}

ASFUNCTIONBODY(Video,_getVideoHeight)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->videoHeight);
}

ASFUNCTIONBODY(Video,_getWidth)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(Video,_setWidth)
{
	Video* th=Class<Video>::cast(obj);
	Mutex::Lock l(th->mutex);
	assert_and_throw(argslen==1);
	th->width=args[0]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Video,_getHeight)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(Video,_setHeight)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen==1);
	Mutex::Lock l(th->mutex);
	th->height=args[0]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Video,attachNetStream)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED) //Drop the connection
	{
		Mutex::Lock l(th->mutex);
		th->netStream=NullRef;
		return NULL;
	}

	//Validate the parameter
	if(!args[0]->getClass()->isSubClass(Class<NetStream>::getClass()))
		throw RunTimeException("Type mismatch in Video::attachNetStream");

	//Acquire the netStream
	args[0]->incRef();

	Mutex::Lock l(th->mutex);
	th->netStream=_MR(Class<NetStream>::cast(args[0]));
	return NULL;
}

_NR<InteractiveObject> Video::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	//TODO: support masks
	if(x>=0 && x<=width && y>=0 && y<=height)
		return last;
	else
		return NullRef;
}

Sound::Sound(Class_base* c):EventDispatcher(c),downloader(NULL),stopped(false),audioDecoder(NULL),audioStream(NULL),
	bytesLoaded(0),bytesTotal(0),length(60*1000)
{
}

Sound::~Sound()
{
	if(downloader && getSys()->downloadManager)
		getSys()->downloadManager->destroy(downloader);
}

void Sound::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	REGISTER_GETTER(c,bytesLoaded);
	REGISTER_GETTER(c,bytesTotal);
	REGISTER_GETTER(c,length);
}

void Sound::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Sound,_constructor)
{
	EventDispatcher::_constructor(obj, NULL, 0);

	if (argslen>0)
		Sound::load(obj, args, argslen);

	return NULL;
}

ASFUNCTIONBODY(Sound,load)
{
	Sound* th=Class<Sound>::cast(obj);
	assert_and_throw(argslen==1 || argslen==2);
	URLRequest* urlRequest=Class<URLRequest>::dyncast(args[0]);
	assert_and_throw(urlRequest);
	//TODO: args[1] is the SoundLoaderContext
	th->url = urlRequest->getRequestURL();
	urlRequest->getPostData(th->postData);

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm()->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	//The URL is valid so we can start the download

	if(th->postData.empty())
	{
		//This is a GET request
		//Use disk cache our downloaded files
		th->downloader=getSys()->downloadManager->download(th->url, true, th);
	}
	else
	{
		list<tiny_string> headers=urlRequest->getHeaders();
		th->downloader=getSys()->downloadManager->downloadWithData(th->url, th->postData,
				headers, th);
		//Clean up the postData for the next load
		th->postData.clear();
	}
	if(th->downloader->hasFailed())
	{
		th->incRef();
		getVm()->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
	}
	return NULL;
}

ASFUNCTIONBODY(Sound,play)
{
	Sound* th=Class<Sound>::cast(obj);
	number_t startTime;
	ARG_UNPACK(startTime, 0);
	//TODO: use startTime
	if(startTime!=0)
		LOG(LOG_NOT_IMPLEMENTED,"startTime not supported in Sound::play");

	if(th->downloader && !th->downloader->hasFailed())
	{
		//Add the object as a job so that playback starts
		th->incRef();
		getSys()->addJob(th);
	}

	return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(Sound,close)
{
	Sound* th=Class<Sound>::cast(obj);
	if(!ACQUIRE_READ(th->stopped))
		th->threadAbort();

	return NULL;
}

void Sound::execute()
{
	downloader->waitForData();
	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

	bool waitForFlush=true;
	StreamDecoder* streamDecoder=NULL;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		streamDecoder=new FFMpegStreamDecoder(s);
		if(!streamDecoder->isValid())
			threadAbort();

		while(!ACQUIRE_READ(stopped))
		{
			bool decodingSuccess=streamDecoder->decodeNextFrame();
			if(decodingSuccess==false)
				break;

			if(audioDecoder==NULL && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;

			//TODO: Move the audio plugin check before
			if(audioStream==NULL && audioDecoder && audioDecoder->isValid() && getSys()->audioManager->pluginLoaded())
				audioStream=getSys()->audioManager->createStreamPlugin(audioDecoder);

			if(threadAborting)
				throw JobTerminationException();
		}
#endif //ENABLE_LIBAVCODEC
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "Exception in Sound " << e.cause);
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, _("Exception in reading: ")<<e.what());
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
}

void Sound::jobFence()
{
	this->decRef();
}

void Sound::threadAbort()
{
	RELEASE_WRITE(stopped,true);
	Locker l(mutex);
	if(audioDecoder)
	{
		//Clear everything we have in buffers, discard all frames
		audioDecoder->setFlushing();
		audioDecoder->skipAll();
	}
	/*if(downloader)
		downloader->stop();*/
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
		getVm()->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(bytesLoaded,bytesTotal)));
		if(bytesLoaded==bytesTotal)
		{
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
		}
	}
}

ASFUNCTIONBODY_GETTER(Sound,bytesLoaded);
ASFUNCTIONBODY_GETTER(Sound,bytesTotal);
ASFUNCTIONBODY_GETTER(Sound,length);

void SoundLoaderContext::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c,bufferTime);
	REGISTER_GETTER_SETTER(c,checkPolicyFile);
}

ASFUNCTIONBODY(SoundLoaderContext,_constructor)
{
	SoundLoaderContext* th=Class<SoundLoaderContext>::cast(obj);
	assert_and_throw(argslen<=2);
	th->bufferTime = 1000;
	th->checkPolicyFile = false;
	if(0 < argslen)
		th->bufferTime = ArgumentConversion<number_t>::toConcrete(args[0]);
	if(1 < argslen)
		th->checkPolicyFile = ArgumentConversion<bool>::toConcrete(args[1]);
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,bufferTime);
ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,checkPolicyFile);

void SoundChannel::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
}

void SoundChannel::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(SoundChannel, _constructor)
{
	return NULL;
}

void StageVideo::sinit(Class_base *c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
}

ASFUNCTIONBODY(StageVideo,_constructor)
{
	EventDispatcher::_constructor(obj, NULL, 0);
	return NULL;
}

void StageVideoAvailability::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("AVAILABLE","",Class<ASString>::getInstanceS("available"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",Class<ASString>::getInstanceS("unavailable"),DECLARED_TRAIT);
}

void VideoStatus::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("ACCELERATED","",Class<ASString>::getInstanceS("accelerated"),DECLARED_TRAIT);
	c->setVariableByQName("SOFTWARE","",Class<ASString>::getInstanceS("software"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",Class<ASString>::getInstanceS("unavailable"),DECLARED_TRAIT);
}
