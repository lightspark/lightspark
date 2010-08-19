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

#ifndef _FLASH_NET_H
#define _FLASH_NET_H

#include "compat.h"
#include "asobject.h"
#include "flashevents.h"
#include "thread_pool.h"
#include "backends/netutils.h"
#include "timer.h"
#include "backends/decoder.h"

namespace lightspark
{

class URLRequest: public ASObject
{
friend class Loader;
friend class URLLoader;
private:
	tiny_string url;
public:
	URLRequest();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getUrl);
	ASFUNCTION(_setUrl);
};

class URLVariables: public ASObject
{
public:
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	tiny_string toString(bool debugMsg=false);
};

class URLLoaderDataFormat: public ASObject
{
public:
	static void sinit(Class_base*);
};

class SharedObject: public ASObject
{
public:
	static void sinit(Class_base*);
};

class ObjectEncoding: public ASObject
{
public:
	enum ENCODING { AMF0=0, AMF3=3, DEFAULT=3 };
	static void sinit(Class_base*);
};

class URLLoader: public EventDispatcher, public IThreadJob
{
private:
	tiny_string dataFormat;
	tiny_string url;
	bool isLocal;
	ASObject* data;
	Downloader* downloader;
	volatile bool executingAbort;
	void execute();
	void threadAbort();
public:
	URLLoader();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(_getDataFormat);
	ASFUNCTION(_getData);
	ASFUNCTION(_setDataFormat);
};

class NetConnection: public EventDispatcher
{
friend class NetStream;
private:
	//Indicates whether the application is connected to a server through a persistent RMTP connection/HTTP server with Flash Remoting
	bool _connected;
	//The connection is to a flash media server
	bool isFMS;
	ObjectEncoding::ENCODING objectEncoding;
	tiny_string protocol;
	tiny_string uri;
public:
	NetConnection();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(connect);
	ASFUNCTION(_getConnected);
	ASFUNCTION(_getDefaultObjectEncoding);
	ASFUNCTION(_setDefaultObjectEncoding);
	ASFUNCTION(_getObjectEncoding);
	ASFUNCTION(_setObjectEncoding);
	ASFUNCTION(_getProtocol);
	ASFUNCTION(_getUri);
};

class NetStream: public EventDispatcher, public IThreadJob, public ITickJob
{
private:
	enum STREAM_TYPE { FLV_STREAM=0 };
	tiny_string url;
	STREAM_TYPE classifyStream(std::istream& s);
	double frameRate;
	bool tickStarted;
	Downloader* downloader;
	VideoDecoder* videoDecoder;
	AudioDecoder* audioDecoder;
	LS_AUDIO_CODEC audioCodec;
	uint32_t soundStreamId;
	uint32_t streamTime;
	sem_t mutex;
	//IThreadJob interface for long jobs
	void execute();
	void threadAbort();
	//ITickJob interface to frame advance
	void tick();
	bool isReady() const;
	bool paused;
	enum CONNECTION_TYPE { CONNECT_TO_FMS=0, DIRECT_CONNECTIONS };
	CONNECTION_TYPE peerID;
	bool isLocal;
public:
	NetStream();
	~NetStream();
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
	  	copy the current frame to a texture

		@pre lock on the object should be acquired and object should be ready
		@param tex the TextureBuffer to copy in
		@return true if a new frame has been copied
	*/
	bool copyFrameToTexture(TextureBuffer& tex);
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

};

#endif
