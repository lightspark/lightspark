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

#include "abc.h"
#include "flashnet.h"
#include "class.h"
#include "parsing/flv.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(URLLoader);
REGISTER_CLASS_NAME(URLLoaderDataFormat);
REGISTER_CLASS_NAME(URLRequest);
REGISTER_CLASS_NAME(URLVariables);
REGISTER_CLASS_NAME(SharedObject);
REGISTER_CLASS_NAME(ObjectEncoding);
REGISTER_CLASS_NAME(NetConnection);
REGISTER_CLASS_NAME(NetStream);

URLRequest::URLRequest()
{
}

void URLRequest::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSetterByQName("url","",Class<IFunction>::getFunction(_setUrl));
	c->setGetterByQName("url","",Class<IFunction>::getFunction(_getUrl));
}

void URLRequest::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URLRequest,_constructor)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	if(argslen>0 && args[0]->getObjectType()==T_STRING)
		th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_setUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	return Class<ASString>::getInstanceS(th->url);
}

URLLoader::URLLoader():dataFormat("text"),data(NULL),downloader(NULL),executingAbort(false)
{
}

void URLLoader::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("dataFormat","",Class<IFunction>::getFunction(_getDataFormat));
	c->setGetterByQName("data","",Class<IFunction>::getFunction(_getData));
	c->setSetterByQName("dataFormat","",Class<IFunction>::getFunction(_setDataFormat));
	c->setVariableByQName("load","",Class<IFunction>::getFunction(load));
}

void URLLoader::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	ASObject* arg=args[0];
	assert_and_throw(arg->getPrototype()==Class<URLRequest>::getClass());
	URLRequest* urlRequest=static_cast<URLRequest*>(arg);
	th->url=urlRequest->url;
	ASObject* data=arg->getVariableByQName("data","");
	if(data)
	{
		if(data->getPrototype()==Class<URLVariables>::getClass())
			throw RunTimeException("Type mismatch in URLLoader::load");
		else
		{
			const tiny_string& tmp=data->toString();
			//TODO: Url encode the string
			string tmp2;
			tmp2.reserve(tmp.len()*2);
			for(int i=0;i<tmp.len();i++)
			{
				if(tmp[i]==' ')
				{
					char buf[4];
					sprintf(buf,"%%%x",(unsigned char)tmp[i]);
					tmp2+=buf;
				}
				else
					tmp2.push_back(tmp[i]);
			}
			th->url+=tmp2.c_str();
		}
	}
	assert_and_throw(th->dataFormat=="binary" || th->dataFormat=="text");
	th->incRef();
	sys->addJob(th);
	return NULL;
}

void URLLoader::execute()
{
	downloader=new CurlDownloader(url);

	downloader->run();
	if(!downloader->hasFailed())
	{
		if(dataFormat=="binary")
		{
			ByteArray* byteArray=Class<ByteArray>::getInstanceS();
			byteArray->acquireBuffer(downloader->getBuffer(),downloader->getLen());
			data=byteArray;
		}
		else if(dataFormat=="text")
		{
			data=Class<ASString>::getInstanceS((const char *)downloader->getBuffer(),
								downloader->getLen());
		}
		//Send a complete event for this object
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS("complete"));
	}
	else
	{
		//Notify an error during loading
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS("ioError"));
	}

	//Save the pointer locally
	CurlDownloader* tmp=downloader;
	downloader=NULL;
	while(executingAbort); //If threadAbort has been executed it may have stopped the downloader or not.
				//If it has not been executed the downloader is now NULL
	delete tmp;
}

void URLLoader::threadAbort()
{
	executingAbort=true;
	if(downloader)
		downloader->Downloader::stop();
	executingAbort=false;
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	return Class<ASString>::getInstanceS(th->dataFormat);
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	if(th->data==NULL)
		return new Undefined;
	
	th->data->incRef();
	return th->data;
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	assert_and_throw(args[0]);
	th->dataFormat=args[0]->toString();
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS("variables"));
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS("text"));
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS("binary"));
}

void SharedObject::sinit(Class_base* c)
{
};

void ObjectEncoding::sinit(Class_base* c)
{
	c->setVariableByQName("AMF0","",abstract_i(0));
	c->setVariableByQName("AMF3","",abstract_i(3));
	c->setVariableByQName("DEFAULT","",abstract_i(3));
};

NetConnection::NetConnection():isFMS(false)
{
}

void NetConnection::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setVariableByQName("connect","",Class<IFunction>::getFunction(connect));
}

void NetConnection::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(NetConnection,connect)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()!=T_UNDEFINED)
	{
		th->isFMS=true;
		throw UnsupportedException("NetConnection::connect to FMS");
	}

	//When the URI is undefined the connect is successful (tested on Adobe player)
	Event* status=Class<NetStatusEvent>::getInstanceS("status", "NetConnection.Connect.Success");
	getVm()->addEvent(th, status);
	status->decRef();
	return NULL;
}

NetStream::NetStream():frameRate(0),tickStarted(false),downloader(NULL),videoDecoder(NULL),audioDecoder(NULL),soundStreamId(0),streamTime(0)
{
	sem_init(&mutex,0,1);
}

NetStream::~NetStream()
{
	assert(!executing);
	if(tickStarted)
		sys->removeJob(this);
	delete videoDecoder; 
	delete audioDecoder; 
	sem_destroy(&mutex);
}

void NetStream::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setVariableByQName("play","",Class<IFunction>::getFunction(play));
	c->setVariableByQName("close","",Class<IFunction>::getFunction(close));
	c->setGetterByQName("bytesLoaded","",Class<IFunction>::getFunction(getBytesLoaded));
	c->setGetterByQName("bytesTotal","",Class<IFunction>::getFunction(getBytesTotal));
	c->setGetterByQName("time","",Class<IFunction>::getFunction(getTime));
}

void NetStream::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(NetStream,_constructor)
{
	LOG(LOG_CALLS,"NetStream constructor");
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getPrototype()==Class<NetConnection>::getClass());

	NetConnection* netConnection = Class<NetConnection>::cast(args[0]);
	assert_and_throw(netConnection->isFMS==false);
	return NULL;
}

ASFUNCTIONBODY(NetStream,play)
{
	NetStream* th=Class<NetStream>::cast(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	th->url = arg0;
	assert_and_throw(th->downloader==NULL);
	th->downloader=sys->downloadManager->download(th->url);
	th->streamTime=0;
	th->incRef();
	sys->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(NetStream,close)
{
	NetStream* th=Class<NetStream>::cast(obj);
	//The downloader is stopped in threadAbort
	th->threadAbort();
	return NULL;
}

NetStream::STREAM_TYPE NetStream::classifyStream(istream& s)
{
	char buf[3];
	s.read(buf,3);
	STREAM_TYPE ret;
	if(strncmp(buf,"FLV",3)==0)
		ret=FLV_STREAM;
	else
		threadAbort();

	s.seekg(0);
	return ret;
}

//Tick is called from the timer thread, this happens only if a decoder is available
void NetStream::tick()
{
	//Advance video and audio to current time, follow the audio stream time
	//No mutex needed, ticking can happen only when stream is completely ready
#ifdef ENABLE_SOUND
	if(soundStreamId && sys->soundManager->isTimingAvailable())
	{
		assert(audioDecoder);
		streamTime=sys->soundManager->getPlayedTime(soundStreamId);
	}
	else
#endif
	{
		streamTime+=1000/frameRate;
		audioDecoder->skipAll();
	}
	videoDecoder->skipUntil(streamTime);
}

bool NetStream::isReady() const
{
	if(videoDecoder==NULL || audioDecoder==NULL)
		return false;

	bool ret=videoDecoder->isValid() && audioDecoder->isValid();
	return ret;
}

bool NetStream::lockIfReady()
{
	sem_wait(&mutex);
	bool ret=isReady();
	if(!ret) //If the data is not valid so not release the lock to keep the condition
		sem_post(&mutex);
	return ret;
}

void NetStream::unlock()
{
	sem_post(&mutex);
}

void NetStream::execute()
{
	//mutex access to downloader
	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

	ThreadProfile* profile=sys->allocateProfiler(RGB(0,0,200));
	profile->setTag("NetStream");
	//We need to catch possible EOF and other error condition in the non reliable stream
	uint32_t decodedAudioBytes=0;
	uint32_t decodedVideoFrames=0;
	//The decoded time is computed from the decodedAudioBytes to avoid drifts
	uint32_t decodedTime=0;
	bool waitForFlush=true;
	try
	{
		Chronometer chronometer;
		STREAM_TYPE t=classifyStream(s);
		if(t==FLV_STREAM)
		{
			FLV_HEADER h(s);
			if(!h.isValid())
				threadAbort();

			unsigned int prevSize=0;
			bool done=false;
			do
			{
				UI32 PreviousTagSize;
				s >> PreviousTagSize;
				PreviousTagSize.bswap();
				assert_and_throw(PreviousTagSize==prevSize);

				//Check tag type and read it
				UI8 TagType;
				s >> TagType;
				switch(TagType)
				{
					case 8:
					{
						AudioDataTag tag(s);
						prevSize=tag.getTotalLen();
#ifdef ENABLE_SOUND
						if(audioDecoder==NULL)
						{
							audioCodec=tag.SoundFormat;
							switch(tag.SoundFormat)
							{
								case AAC:
									assert_and_throw(tag.isHeader())
#ifdef ENABLE_LIBAVCODEC
									audioDecoder=new FFMpegAudioDecoder(tag.SoundFormat,
											tag.packetData, tag.packetLen);
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
							if(audioDecoder->isValid())
								soundStreamId=sys->soundManager->createStream(audioDecoder);
						}
						else
						{
							assert_and_throw(audioCodec==tag.SoundFormat);
							decodedAudioBytes+=audioDecoder->decodeData(tag.packetData,tag.packetLen,decodedTime);
							if(soundStreamId==0 && audioDecoder->isValid())
								soundStreamId=sys->soundManager->createStream(audioDecoder);
							//Adjust timing
							decodedTime=decodedAudioBytes/audioDecoder->getBytesPerMSec();
						}
#endif
						break;
					}
					case 9:
					{
						VideoDataTag tag(s);
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
							else if(videoDecoder==NULL)
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
							Event* status=Class<NetStatusEvent>::getInstanceS("status", "NetStream.Play.Start");
							getVm()->addEvent(this, status);
							status->decRef();
							status=Class<NetStatusEvent>::getInstanceS("status", "NetStream.Buffer.Full");
							getVm()->addEvent(this, status);
							status->decRef();
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
						ScriptDataTag tag(s);
						prevSize=tag.getTotalLen();

						//The frameRate of the container overrides the stream
						if(tag.frameRate)
							frameRate=tag.frameRate;
						break;
					}
					default:
						LOG(LOG_ERROR,"Unexpected tag type " << (int)TagType << " in FLV");
						threadAbort();
				}
				if(!tickStarted && isReady())
				{
					tickStarted=true;
					if(frameRate==0)
					{
						assert(videoDecoder->frameRate);
						frameRate=videoDecoder->frameRate;
					}
					sys->addTick(1000/frameRate,this);
					//Also ask for a render rate equal to the video one (capped at 24)
					float localRenderRate=dmin(frameRate,24);
					sys->setRenderRate(localRenderRate);
				}
				profile->accountTime(chronometer.checkpoint());
				if(aborting)
					throw JobTerminationException();
			}
			while(!done);
		}
		else
			threadAbort();

	}
	catch(LightsparkException& e)
	{
		cout << e.cause << endl;
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, "Exception in reading: "<<e.what());
	}

	if(waitForFlush)
	{
		//Put the decoders in the flushing state and wait for the complete consumption of contents
		audioDecoder->setFlushing();
		videoDecoder->setFlushing();
		
		audioDecoder->waitFlushed();
		videoDecoder->waitFlushed();
	}

	sem_wait(&mutex);
	sys->downloadManager->destroy(downloader);
	downloader=NULL;
	//This transition is critical, so the mutex is needed
	//Before deleting stops ticking, removeJobs also spin waits for termination
	sys->removeJob(this);
	tickStarted=false;
	delete videoDecoder;
	videoDecoder=NULL;
#if ENABLE_SOUND
	if(soundStreamId)
		sys->soundManager->freeStream(soundStreamId);
	delete audioDecoder;
	audioDecoder=NULL;
#endif
	sem_post(&mutex);
}

void NetStream::threadAbort()
{
	sem_wait(&mutex);
	if(downloader)
		downloader->stop();
	//Discard a frame, the decoder may be blocked on a full buffer
	if(videoDecoder)
		videoDecoder->discardFrame();
	if(audioDecoder)
		audioDecoder->discardFrame();
	sem_post(&mutex);
}

ASFUNCTIONBODY(NetStream,getBytesLoaded)
{
	return abstract_i(0);
}

ASFUNCTIONBODY(NetStream,getBytesTotal)
{
	return abstract_i(100);
}

ASFUNCTIONBODY(NetStream,getTime)
{
	return abstract_d(0);
}

uint32_t NetStream::getVideoWidth() const
{
	assert(isReady());
	return videoDecoder->getWidth();
}

uint32_t NetStream::getVideoHeight() const
{
	assert(isReady());
	return videoDecoder->getHeight();
}

double NetStream::getFrameRate()
{
	assert(isReady());
	return frameRate;
}

bool NetStream::copyFrameToTexture(TextureBuffer& tex)
{
	assert(isReady());
	return videoDecoder->copyFrameToTexture(tex);
}

void URLVariables::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

ASFUNCTIONBODY(URLVariables,_constructor)
{
	assert_and_throw(argslen==0);
	return NULL;
}

tiny_string URLVariables::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	//Should urlencode
	throw UnsupportedException("URLVariables::toString");
	if(debugMsg)
		return ASObject::toString(debugMsg);
	int size=numVariables();
	tiny_string ret;
	for(int i=0;i<size;i++)
	{
		const tiny_string& tmp=getNameAt(i);
		if(tmp=="")
			throw UnsupportedException("URLVariables::toString");
		ret+=tmp;
		ret+="=";
		ret+=getValueAt(i)->toString();
		if(i!=size-1)
			ret+="&";
	}
	return ret;
}

