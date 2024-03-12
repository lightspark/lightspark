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

#ifndef FLASH_NET_SOCKET_H_
#define FLASH_NET_SOCKET_H_

#include "forwards/threading.h"
#include "interfaces/threading.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/utils/flashutils.h"
#include "tiny_string.h"
#include "asobject.h"
#include <glib.h>

namespace lightspark
{
class ByteArray;
class SocketIO
{
private:
	int fd;
public:
	SocketIO();
	~SocketIO();

	bool connect(const tiny_string& hostname, int port, int timeoutseconds=0);
	bool connected() const;
	void close();
	ssize_t receive(void *buf, size_t count) const;
	ssize_t sendAll(const void *buf, size_t count) const;
	int fileDescriptor() const { return fd; }
};

class ASSocketThread;

class ASSocket : public EventDispatcher, IDataInput, IDataOutput
{
protected:
	ASSocketThread *job;
	Mutex joblock; // protect access to job
	uint8_t objectEncoding;
	size_t _bytesAvailable;

	ASPROPERTY_GETTER_SETTER(int,timeout);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_close);
	ASFUNCTION_ATOM(_connect);
	ASFUNCTION_ATOM(_connected);
	ASFUNCTION_ATOM(_flush);
	ASFUNCTION_ATOM(bytesAvailable);
	ASFUNCTION_ATOM(_getEndian);
	ASFUNCTION_ATOM(_setEndian);
	ASFUNCTION_ATOM(_getObjectEncoding);
	ASFUNCTION_ATOM(_setObjectEncoding);
	ASFUNCTION_ATOM(readBoolean);
	ASFUNCTION_ATOM(readByte);
	ASFUNCTION_ATOM(readBytes);
	ASFUNCTION_ATOM(readDouble);
	ASFUNCTION_ATOM(readFloat);
	ASFUNCTION_ATOM(readInt);
	ASFUNCTION_ATOM(readMultiByte);
	ASFUNCTION_ATOM(readObject);
	ASFUNCTION_ATOM(readShort);
	ASFUNCTION_ATOM(readUnsignedByte);
	ASFUNCTION_ATOM(readUnsignedInt);
	ASFUNCTION_ATOM(readUnsignedShort);
	ASFUNCTION_ATOM(readUTF);
	ASFUNCTION_ATOM(readUTFBytes);
	ASFUNCTION_ATOM(writeBoolean);
	ASFUNCTION_ATOM(writeByte);
	ASFUNCTION_ATOM(writeBytes);
	ASFUNCTION_ATOM(writeDouble);
	ASFUNCTION_ATOM(writeFloat);
	ASFUNCTION_ATOM(writeInt);
	ASFUNCTION_ATOM(writeUnsignedInt);
	ASFUNCTION_ATOM(writeMultiByte);
	ASFUNCTION_ATOM(writeObject);
	ASFUNCTION_ATOM(writeShort);
	ASFUNCTION_ATOM(writeUTF);
	ASFUNCTION_ATOM(writeUTFBytes);

	void connect(tiny_string host, int port);
	bool isConnected();
public:
	ASSocket(ASWorker* wrk,Class_base* c);
	~ASSocket();
	static void sinit(Class_base*);
	void finalize() override;
	void threadFinished();
	void setBytesAvailable(size_t size) { _bytesAvailable = size; }
	size_t getBytesAvailable() const { return _bytesAvailable; }
};

class ASSocketThread : public IThreadJob
{
friend class ASSocket;
private:
	SocketIO sock;
	_R<ASSocket> owner;
	tiny_string hostname;
	int port;
	int timeout;
	GAsyncQueue *sendQueue;
	int signalEmitter;
	int signalListener;

	int connectSocket();
	ssize_t receive();
	void readSocket(const SocketIO& sock);
	void executeCommand(char cmd, SocketIO& sock);
protected:
	_NR<ByteArray> datasend;
	_NR<ByteArray> datareceive;
public:
	ASSocketThread(_R<ASSocket> owner, const tiny_string& hostname, int port, int timeout);
	~ASSocketThread();
	void execute() override;
	void jobFence() override;
	void flushData();
	void requestClose();
	bool isConnected();
};

}

#endif
