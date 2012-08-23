/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_NET_FLASHNET_H
#define SCRIPTING_FLASH_NET_FLASHNET_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/events/flashevents.h"
#include "thread_pool.h"
#include "backends/netutils.h"
#include "timer.h"
#include "backends/decoder.h"
#include "backends/interfaces/audio/IAudioPlugin.h"

namespace lightspark
{

class URLRequest: public ASObject
{
private:
	enum METHOD { GET=0, POST };
	METHOD method;
	tiny_string url;
	_NR<ASObject> data;
	tiny_string validatedContentType() const;
	tiny_string getContentTypeHeader() const;
	void validateHeaderName(const tiny_string& headerName) const;
	ASPROPERTY_GETTER_SETTER(tiny_string,contentType);
	ASPROPERTY_GETTER_SETTER(_R<Array>,requestHeaders);
public:
	URLRequest(Class_base* c);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getURL);
	ASFUNCTION(_setURL);
	ASFUNCTION(_getMethod);
	ASFUNCTION(_setMethod);
	ASFUNCTION(_setData);
	ASFUNCTION(_getData);
	URLInfo getRequestURL() const;
	std::list<tiny_string> getHeaders() const;
	void getPostData(std::vector<uint8_t>& data) const;
};

class URLRequestMethod: public ASObject
{
public:
	URLRequestMethod(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
};

class URLVariables: public ASObject
{
private:
	void decode(const tiny_string& s);
	tiny_string toString_priv();
public:
	URLVariables(Class_base* c):ASObject(c){}
	URLVariables(Class_base* c,const tiny_string& s);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(decode);
	ASFUNCTION(_toString);
	tiny_string toString();
};

class URLLoaderDataFormat: public ASObject
{
public:
	URLLoaderDataFormat(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
};

class SharedObject: public EventDispatcher
{
public:
	SharedObject(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION(getLocal);
	ASPROPERTY_GETTER(_NR<ASObject>,data);
};

class ObjectEncoding: public ASObject
{
public:
	ObjectEncoding(Class_base* c):ASObject(c){}
	enum ENCODING { AMF0=0, AMF3=3, DEFAULT=3 };
	static void sinit(Class_base*);
};

class URLLoader;

class URLLoaderThread : public DownloaderThreadBase
{
private:
	_R<URLLoader> loader;
	void execute();
public:
	URLLoaderThread(_R<URLRequest> _request, _R<URLLoader> _loader);
};

class URLLoader: public EventDispatcher, public IDownloaderThreadListener, public ILoadable
{
private:
	tiny_string dataFormat;
	_NR<ASObject> data;
	Spinlock spinlock;
	URLLoaderThread *job;
public:
	URLLoader(Class_base* c);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void threadFinished(IThreadJob *job);
	void setData(_NR<ASObject> data);
	tiny_string getDataFormat();
	void setDataFormat(const tiny_string& newFormat);
	void setBytesTotal(uint32_t b);
	void setBytesLoaded(uint32_t b);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(close);
	ASFUNCTION(_getDataFormat);
	ASFUNCTION(_getData);
	ASFUNCTION(_setData);
	ASFUNCTION(_setDataFormat);
	ASPROPERTY_GETTER_SETTER(uint32_t, bytesLoaded);
	ASPROPERTY_GETTER_SETTER(uint32_t, bytesTotal);
};

class Responder: public ASObject
{
private:
	_NR<IFunction> result;
	_NR<IFunction> status;
public:
	Responder(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
	void finalize();
	ASFUNCTION(_constructor);
	ASFUNCTION(onResult);
};

class NetConnection: public EventDispatcher, public IThreadJob
{
friend class NetStream;
private:
	//Indicates whether the application is connected to a server through a persistent RMTP connection/HTTP server with Flash Remoting
	bool _connected;
	tiny_string protocol;
	URLInfo uri;
	//Data for remoting support (NetConnection::call)
	// The message data to be sent asynchronously
	std::vector<uint8_t> messageData;
	Spinlock downloaderLock;
	Downloader* downloader;
	_NR<Responder> responder;
	uint32_t messageCount;
	//The connection is to a flash media server
	ObjectEncoding::ENCODING objectEncoding;
	//IThreadJob interface
	void execute();
	void threadAbort();
	void jobFence();
public:
	NetConnection(Class_base* c);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(connect);
	ASFUNCTION(call);
	ASFUNCTION(_getConnected);
	ASFUNCTION(_getDefaultObjectEncoding);
	ASFUNCTION(_setDefaultObjectEncoding);
	ASFUNCTION(_getObjectEncoding);
	ASFUNCTION(_setObjectEncoding);
	ASFUNCTION(_getProtocol);
	ASFUNCTION(_getURI);
	ASPROPERTY_GETTER_SETTER(NullableRef<ASObject>,client);
};

class SoundTransform;
class NetStream: public EventDispatcher, public IThreadJob, public ITickJob
{
private:
	bool tickStarted;
	//Indicates whether the NetStream is paused
	bool paused;
	//Indicates whether the NetStream has been closed/threadAborted. This is reset at every play() call.
	//We initialize this value to true, so we can check that play() hasn't been called without being closed first.
	volatile bool closed;

	uint32_t streamTime;
	URLInfo url;
	double frameRate;
	//The NetConnection used by this NetStream
	_NR<NetConnection> connection;
	Downloader* downloader;
	VideoDecoder* videoDecoder;
	AudioDecoder* audioDecoder;
	AudioStream *audioStream;
	Mutex mutex;
	//IThreadJob interface for long jobs
	void execute();
	void threadAbort();
	void jobFence();
	//ITickJob interface to frame advance
	void tick();
	void tickFence();
	bool isReady() const;
	_NR<ASObject> client;

	ASPROPERTY_GETTER_SETTER(NullableRef<SoundTransform>,soundTransform);
	number_t oldVolume;

	enum CONNECTION_TYPE { CONNECT_TO_FMS=0, DIRECT_CONNECTIONS };
	CONNECTION_TYPE peerID;

	bool checkPolicyFile;
	bool rawAccessAllowed;

	ASObject *createMetaDataObject(StreamDecoder* streamDecoder);
	ASObject *createPlayStatusObject(const tiny_string& code);
	void sendClientNotification(const tiny_string& name, ASObject *args);
public:
	NetStream(Class_base* c);
	~NetStream();
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(play);
	ASFUNCTION(resume);
	ASFUNCTION(pause);
	ASFUNCTION(togglePause);
	ASFUNCTION(close);
	ASFUNCTION(seek);
	ASFUNCTION(_getBytesLoaded);
	ASFUNCTION(_getBytesTotal);
	ASFUNCTION(_getTime);
	ASFUNCTION(_getCurrentFPS);
	ASFUNCTION(_getClient);
	ASFUNCTION(_setClient);
	ASFUNCTION(_getCheckPolicyFile);
	ASFUNCTION(_setCheckPolicyFile);

	//Interface for video
	/**
	  	Get the frame width

		@pre lock on the object should be acquired and object should be ready
		@return the frame width
	*/
	uint32_t getVideoWidth() const;
	/**
	  	Get the frame height

		@pre lock on the object should be acquired and object should be ready
		@return the frame height
	*/
	uint32_t getVideoHeight() const;
	/**
	  	Get the frame rate

		@pre lock on the object should be acquired and object should be ready
		@return the frame rate
	*/
	double getFrameRate();
	/**
	  	Get the texture containing the current video Frame
		@pre lock on the object should be acquired and object should be ready
		@return a TextureChunk ready to be blitted
	*/
	const TextureChunk& getTexture() const;
	/**
	  	Get the stream time

		@pre lock on the object should be acquired and object should be ready
		@return the stream time
	*/
	uint32_t getStreamTime();
	/**
	  	Get the length of loaded data

		@pre lock on the object should be acquired and object should be ready
		@return the length of loaded data
	*/
	uint32_t getReceivedLength();
	/**
	  	Get the length of loaded data

		@pre lock on the object should be acquired and object should be ready
		@return the total length of the data
	*/
	uint32_t getTotalLength();
	/**
	  	Acquire the mutex to guarantee validity of data

		@return true if the lock has been acquired
	*/
	bool lockIfReady();
	/**
	  	Release the lock

		@pre the object should be locked
	*/
	void unlock();
};

ASObject* sendToURL(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* registerClassAlias(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* getClassByAlias(ASObject* obj,ASObject* const* args, const unsigned int argslen);

};

#endif /* SCRIPTING_FLASH_NET_FLASHNET_H */
