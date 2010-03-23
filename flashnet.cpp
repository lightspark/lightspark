/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "abc.h"
#include "flashnet.h"
#include "class.h"
#include "flv.h"
#include "fastpaths.h"

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
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(URLRequest,_constructor)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
	if(argslen>0 && args[0]->getObjectType()==T_STRING)
		th->url=args[0]->toString();

	obj->setSetterByQName("url","",new Function(_setUrl));
	obj->setGetterByQName("url","",new Function(_getUrl));
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_setUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
	th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->url)->obj;
}

URLLoader::URLLoader():dataFormat("text"),data(NULL)
{
}

void URLLoader::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void URLLoader::buildTraits(ASObject* o)
{
	o->setGetterByQName("dataFormat","",new Function(_getDataFormat));
	o->setGetterByQName("data","",new Function(_getData));
	o->setSetterByQName("dataFormat","",new Function(_setDataFormat));
	o->setVariableByQName("load","",new Function(load));
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	ASObject* arg=args[0];
	assert(arg->prototype==Class<URLRequest>::getClass());
	URLRequest* urlRequest=static_cast<URLRequest*>(arg->implementation);
	th->url=urlRequest->url;
	ASObject* data=arg->getVariableByQName("data","").obj;
	if(data)
	{
		if(data->prototype==Class<URLVariables>::getClass())
			::abort();
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
	assert(th->dataFormat=="binary" || th->dataFormat=="text");
	sys->addJob(th);
	return NULL;
}

void URLLoader::execute()
{
	CurlDownloader curlDownloader(url);

	bool done=curlDownloader.download();
	if(done)
	{
		if(dataFormat=="binary")
		{
			ByteArray* byteArray=Class<ByteArray>::getInstanceS(true);
			byteArray->acquireBuffer(curlDownloader.getBuffer(),curlDownloader.getLen());
			data=byteArray->obj;
		}
		else if(dataFormat=="text")
		{
			if(curlDownloader.getLen())
				abort();
			data=Class<ASString>::getInstanceS(true)->obj;
		}
		//Send a complete event for this object
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS(true,"complete",obj));
	}
	else
	{
		//Notify an error during loading
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS(true,"ioError",obj));
	}
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->dataFormat)->obj;
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	if(th->data==NULL)
		return new Undefined;
	
	th->data->incRef();
	return th->data;
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	assert(args[0]);
	th->dataFormat=args[0]->toString();
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS(true,"variables")->obj);
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS(true,"text")->obj);
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS(true,"binary")->obj);
}

void SharedObject::sinit(Class_base* c)
{
};

void ObjectEncoding::sinit(Class_base* c)
{
	c->setVariableByQName("AMF0","",new Integer(0));
	c->setVariableByQName("AMF3","",new Integer(3));
	c->setVariableByQName("DEFAULT","",new Integer(3));
};

NetConnection::NetConnection():isFMS(false)
{
}

void NetConnection::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	//c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void NetConnection::buildTraits(ASObject* o)
{
	o->setVariableByQName("connect","",new Function(connect));
}

ASFUNCTIONBODY(NetConnection,connect)
{
	NetConnection* th=Class<NetConnection>::cast(obj->implementation);
	assert(argslen==1);
	if(args[0]->getObjectType()!=T_UNDEFINED)
	{
		th->isFMS=true;
		abort();
	}

	//When the URI is undefined the connect is successful (tested on Adobe player)
	Event* status=Class<NetStatusEvent>::getInstanceS(true, "status", "NetConnection.Connect.Success");
	getVm()->addEvent(th, status);
	return NULL;
}

NetStream::NetStream():codecContext(NULL),bufferHead(0),bufferTail(0),bufferSize(0),frameCount(0),frameRate(0)
{
	sem_init(&mutex,0,1);
	sem_init(&freeBuffers,0,10);
	sem_init(&usedBuffers,0,0);
	for(int i=0;i<10;i++)
	{
		buffers[i][0]=NULL;
		buffers[i][1]=NULL;
		buffers[i][2]=NULL;
	}
}

NetStream::~NetStream()
{
	assert(!executing);
	for(int i=0;i<10;i++)
	{
		if(buffers[i][0])
		{
			free(buffers[i][0]);
			free(buffers[i][1]);
			free(buffers[i][2]);
		}
	}
	assert(codecContext);
	av_free(codecContext);
	sem_destroy(&mutex);
	sem_destroy(&freeBuffers);
	sem_destroy(&usedBuffers);
}

void NetStream::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void NetStream::buildTraits(ASObject* o)
{
	o->setVariableByQName("play","",new Function(play));
	o->setGetterByQName("bytesLoaded","",new Function(getBytesLoaded));
	o->setGetterByQName("bytesTotal","",new Function(getBytesTotal));
	o->setGetterByQName("time","",new Function(getTime));
}

ASFUNCTIONBODY(NetStream,_constructor)
{
	LOG(LOG_CALLS,"NetStream constructor");
	assert(argslen==1);
	assert(args[0]->prototype==Class<NetConnection>::getClass());

	NetConnection* netConnection = Class<NetConnection>::cast(args[0]->implementation);
	assert(netConnection->isFMS==false);
	return NULL;
}

ASFUNCTIONBODY(NetStream,play)
{
	NetStream* th=Class<NetStream>::cast(obj->implementation);
	assert(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	//cout << "__URL " << arg0 << endl;
	th->url = arg0;

	sys->addJob(th);
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
		abort();

	s.seekg(0);
	return ret;
}

void NetStream::copyFrameToBuffers(const AVFrame* frameIn, uint32_t width, uint32_t height)
{
	sem_wait(&freeBuffers);
	if(aborting)
		throw "Aborting";
	//Only one thread may access the tail
	if(buffers[bufferTail][0]==NULL)
	{
		posix_memalign((void**)&buffers[bufferTail][0], 16, bufferSize);
		posix_memalign((void**)&buffers[bufferTail][1], 16, bufferSize/4);
		posix_memalign((void**)&buffers[bufferTail][2], 16, bufferSize/4);
	}
	int offset[3]={0,0,0};
	for(uint32_t y=0;y<height;y++)
	{
		memcpy(buffers[bufferTail][0]+offset[0],frameIn->data[0]+(y*frameIn->linesize[0]),width);
		offset[0]+=width;
	}
	for(uint32_t y=0;y<height/2;y++)
	{
		memcpy(buffers[bufferTail][1]+offset[1],frameIn->data[1]+(y*frameIn->linesize[1]),width/2);
		memcpy(buffers[bufferTail][2]+offset[2],frameIn->data[2]+(y*frameIn->linesize[2]),width/2);
		offset[1]+=width/2;
		offset[2]+=width/2;
	}

	bufferTail=(bufferTail+1)%10;
	sem_post(&usedBuffers);
}

void NetStream::execute()
{
	Downloader* downloader=sys->downloadManager->download(url);
	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	AVFrame* frameIn=avcodec_alloc_frame();

	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
		STREAM_TYPE t=classifyStream(s);
		if(t==FLV_STREAM)
		{
			FLV_HEADER h(s);
			if(!h.isValid())
				abort();

			unsigned int prevSize=0;
			bool done=false;
			do
			{
				UI32 PreviousTagSize;
				s >> PreviousTagSize;
				PreviousTagSize.bswap();
				assert(PreviousTagSize==prevSize);

				//Check tag type and read it
				UI8 TagType;
				s >> TagType;
				switch(TagType)
				{
					case 8:
					{
						AudioDataTag tag(s);
						prevSize=tag.getTotalLen();
						break;
					}
					case 9:
					{
						VideoDataTag tag(s);
						prevSize=tag.getTotalLen();
						assert(tag.codecId==7);

						if(tag.isHeader())
						{
							sem_wait(&mutex);
							//The tag is the header, initialize decoding
							assert(codecContext==NULL); //The context can be set only once
							//TODO: serialize access to avcodec_open
							AVCodec* codec=avcodec_find_decoder(CODEC_ID_H264);
							assert(codec);

							codecContext=avcodec_alloc_context();
							//If this tag is the header, fill the extradata for the codec
							codecContext->extradata=tag.packetData;
							codecContext->extradata_size=tag.packetLen;
							tag.releaseBuffer();

							if(avcodec_open(codecContext, codec)<0)
								abort();
							sem_post(&mutex);
						}
						else
						{
							//The tag is a data packet, decode it
							int frameOk=0;
							avcodec_decode_video(codecContext, frameIn, &frameOk, tag.packetData, tag.packetLen);
							frameCount++;
							if(frameOk==0)
								abort();
							assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

							const uint32_t height=codecContext->height;
							const uint32_t width=codecContext->width;
							assert(frameIn->pts==0);

							const uint32_t size=width*height*4;
							if(size!=bufferSize)
							{
								assert(bufferSize==0);
								bufferSize=size;
							}
							copyFrameToBuffers(frameIn,width,height);
						}
						break;
					}
					case 18:
					{
						ScriptDataTag tag(s);
						prevSize=tag.getTotalLen();

						//HACK: initialize frameRate from the container
						frameRate=tag.frameRate;
						break;
					}
					default:
						cout << (int)TagType << endl;
						abort();
				}
			}
			while(!done);
		}
		else
			abort();

	}
	catch(exception& e)
	{
		cout << e.what() << endl;
		abort();
	}
	catch(const char*)
	{
	}

	av_free(frameIn);
/*	Event* status=Class<NetStatusEvent>::getInstanceS(true, "status", "NetStream.Play.Start");
	getVm()->addEvent(this, status);
	status=Class<NetStatusEvent>::getInstanceS(true, "status", "NetStream.Buffer.Full");
	getVm()->addEvent(this, status);*/
}

void NetStream::abort()
{
	sem_post(&freeBuffers);
	sem_post(&usedBuffers);
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


uint32_t NetStream::getVideoWidth()
{
	sem_wait(&mutex);
	uint32_t ret=0;
	if(codecContext)
		ret=codecContext->width;
	sem_post(&mutex);
	return ret;
}

uint32_t NetStream::getVideoHeight()
{
	sem_wait(&mutex);
	uint32_t ret=0;
	if(codecContext)
		ret=codecContext->height;
	sem_post(&mutex);
	return ret;
}

double NetStream::getFrameRate()
{
	return frameRate;
}

void NetStream::copyBuffer(uint8_t* dest)
{
	sem_wait(&mutex);
	uint32_t width=0,height=0;
	if(codecContext)
	{
		width=codecContext->width;
		height=codecContext->height;
	}
	sem_post(&mutex);

	sem_wait(&usedBuffers);
//	if(aborting)
//		throw "Aborting"
	//At least a frame is available
	int offset=0;
	for(uint32_t y=0;y<height;y++)
	{
		fastYUV420ChannelsToBuffer(buffers[bufferHead][0]+(y*width),
				buffers[bufferHead][1]+((y>>1)*(width>>1)),
				buffers[bufferHead][2]+((y>>1)*(width>>1)),
				dest+offset,width);
		offset+=width*4;
	}
	bufferHead=(bufferHead+1)%10;
	sem_post(&freeBuffers);
}

void URLVariables::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(URLVariables,_constructor)
{
	assert(argslen==0);
	return NULL;
}

bool URLVariables::toString(tiny_string& ret)
{
	//Should urlencode
	abort();
	int size=obj->numVariables();
	for(int i=0;i<size;i++)
	{
		const tiny_string& tmp=obj->getNameAt(i);
		if(tmp=="")
			abort();
		ret+=tmp;
		ret+="=";
		ret+=obj->getValueAt(i)->toString();
		if(i!=size-1)
			ret+="&";
	}
	return true;
}

