/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef BACKENDS_NET_STREAM_H
#define BACKENDS_NET_STREAM_H 1

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <vector>

#include "backends/graphics.h"
#include "backends/netutils.h"
#include "compat.h"
#include "forwards/threading.h"
#include "forwards/timer.h"
#include "gc/ptr.h"
#include "interfaces/backends/netutils.h"
#include "interfaces/threading.h"
#include "interfaces/timer.h"
#include "smartrefs.h"
#include "utils/optional.h"
#include "utils/span.h"
#include "utils/timespec.h"

namespace lightspark
{

class ASNetStream;
class ASObject;
class AVM1NetStream;
class AudioDecoder;
class AudioStream;
class Downloader;
class NetConnection;
class NetStreamInfo;
class SoundTransform;
class StreamCache;
class StreamDecoder;
class VideoDecoder;
class tiny_string;

class NetStreamObject
{
public:
	enum class Type
	{
		None,
		AVM1,
		AVM2,
	};
private:
	Type type;
	union
	{
		_GC<AVM1NetStream> avm1Obj;
		_R<ASNetStream> asObj;
	};
public:
	NetStreamObject() : type(Type::None) {}
	NetStreamObject(_GC<AVM1NetStream> _avm1Obj);
	NetStreamObject(_R<ASNetStream> _asObj);
	~NetStreamObject();

	bool isNull() const { return type == Type::None; }
	bool isAVM1() const { return type == Type::AVM1; }
	bool isAVM2() const { return type == Type::AVM2; }

	_NGC<AVM1NetStream> getAVM1Obj() const
	{
		return isAVM1() ? avm1Obj : NullGc;
	}

	_NGC<AVM1NetStream> getAVM1Obj()
	{
		return isAVM1() ? avm1Obj : NullGc;
	}

	_NR<ASNetStream> getASObj() const
	{
		return isAVM2() ? asObj : NullRef;
	}

	_NR<ASNetStream> getASObj()
	{
		return isAVM2() ? asObj : NullRef;
	}
};

enum class NetStreamAppendBytesAction
{
	EndSequence,
	ResetBegin,
	ResetSeek,
};

class NetStream : public IThreadJob, public ITickJob
{
	using AppendBytesAction = NetStreamAppendBytesAction;
private:
	bool tickStarted;
	//Indicates whether the NetStream is paused
	bool paused;
	//Indicates whether the NetStream has been closed/threadAborted. This is reset at every play() call.
	//We initialize this value to true, so we can check that play() hasn't been called without being closed first.
	volatile bool closed;

	uint32_t streamTime;
	URLInfo url;
	number_t frameRate;
	//The NetConnection used by this NetStream
	NetConnection* connection;
	Downloader* downloader;
	VideoDecoder* videoDecoder;
	AudioDecoder* audioDecoder;
	AudioStream* audioStream;
	// only used when in DataGenerationMode
	StreamCache* dataGenerationFile;
	bool dataGenerationThreadStarted;
	Mutex mutex;
	Mutex counterMutex;
	//IThreadJob interface for long jobs
	void execute() override;
	void threadAbort() override;
	void jobFence() override;
	//ITickJob interface to frame advance
	void tick() override;
	void tickFence() override;
	bool isReady() const;

	NetStreamObject obj;
	_NR<ASObject> client;

	Optional<SoundTransform> soundTransform;
	number_t oldVolume;

	enum CONNECTION_TYPE
	{
		CONNECT_TO_FMS=0,
		DIRECT_CONNECTIONS
	};

	CONNECTION_TYPE peerID;

	bool checkPolicyFile;
	bool rawAccessAllowed;

	uint32_t framesDecoded;
	uint32_t prevStreamTime;
	number_t playbackBytesPerSecond;
	number_t maxBytesPerSecond;

	struct BytesPerTime
	{
		TimeSpec timestamp;
		size_t bytesRead;
	};

	std::deque<BytesPerTime> currentBytesPerSecond;

	enum DATAGENERATION_EXPECT_TYPE
	{
		DATAGENERATION_HEADER=0,
		DATAGENERATION_PREVTAG,
		DATAGENERATION_FLVTAG
	};

	DATAGENERATION_EXPECT_TYPE dataGenerationExpectType;
	std::vector<uint8_t> dataGenerationBuffer;
	StreamDecoder* streamDecoder;

	size_t backBufferLength;
	number_t backBufferTime;
	size_t bufferLength;
	number_t bufferTime;
	number_t bufferTimeMax;
	number_t maxPauseBufferTime;
	bool useHardwareDecoder;
public:
	NetStream(const NetStreamObject& _obj = NetStreamObject());
	~NetStream();

	void play(Optional<const tiny_string&> name);
	void resume();
	void pause();
	void togglePause();
	void close();
	void seek(ssize_t offset, bool notify);
	size_t getBytesLoaded() const;
	size_t getBytesTotal() const;
	number_t getTime() const;
	number_t getCurrentFPS() const;
	_NR<ASObject> getClient() const { return client; }
	void setClient(_NR<ASObject> _client) { client = _client; }
	bool getCheckPolicyFile() const { return checkPolicyFile; }
	void setCheckPolicyFile(bool flag) { checkPolicyFile = flag; }
	void attach(NetConnection& _connection)
	{
		connection = &_connection;
	}

	void appendBytes(Span<uint8_t> data);
	void appendBytesAction(const AppendBytesAction& action);
	NetStreamInfo getInfo();
	size_t getBackBufferLength() const { return backBufferLength; }
	number_t getBackBufferTime() const { return backBufferTime; }
	void setBackBufferTime(number_t val) { backBufferTime = val; }
	size_t getBufferLength() const { return bufferLength; }
	number_t getBufferTime() const { return bufferTime; }
	void setBufferTime(number_t val) { bufferTime = val; }
	number_t getBufferTimeMax() const { return bufferTimeMax; }
	void setBufferTimeMax(number_t val) { bufferTimeMax = val; }
	number_t getMaxPauseBufferTime() const { return maxPauseBufferTime; }
	void setMaxPauseBufferTime(number_t val) { maxPauseBufferTime = val; }
	bool getUseHardwareDecoder() const { return useHardwareDecoder; }
	void setUseHardwareDecoder(bool flag) { useHardwareDecoder = flag; }
	const CONNECTION_TYPE& getPeerID() const { return peerID; }
	void setPeerID(const CONNECTION_TYPE& val) { peerID = val; }

	bool hasObj() const { return !obj.isNull(); }
	_NR<ASNetStream> getASObj() const { return obj.getASObj(); }
	_NGC<AVM1NetStream> getAVM1Obj() const { return obj.getAVM1Obj(); }

	void sendClientNotification(const tiny_string& name, std::list<asAtom>& arglist);

	//Interface for video
	/**
		Get the frame size

		@pre lock on the object should be acquired and object should be ready
		@return the frame size
	*/
	const Vector2u& getVideoSize() const;
	/**
		Get the frame width

		@pre lock on the object should be acquired and object should be ready
		@return the frame width
	*/
	uint32_t getVideoWidth() const { return getFrameSize().x; }
	/**
		Get the frame height

		@pre lock on the object should be acquired and object should be ready
		@return the frame height
	*/
	uint32_t getVideoHeight() const { return getFrameSize().y; }
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
	TextureChunk& getTexture() const;
	/**
		Get the stream time

		@pre lock on the object should be acquired and object should be ready
		@return the stream time
	*/
	size_t getStreamTime();
	/**
		Get the length of loaded data

		@pre lock on the object should be acquired and object should be ready
		@return the length of loaded data
	*/
	size_t getReceivedLength();
	/**
		Get the length of loaded data

		@pre lock on the object should be acquired and object should be ready
		@return the total length of the data
	*/
	size_t getTotalLength();
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

	void clearFrameBuffer();
};

}
#endif /* BACKENDS_NET_STREAM_H */
