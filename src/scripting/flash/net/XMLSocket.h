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

#ifndef XMLSOCKET_H_
#define XMLSOCKET_H_

#include "flash/events/flashevents.h"
#include "tiny_string.h"
#include "asobject.h"
#include "threading.h"
#include <glib.h>

namespace lightspark
{

class SocketIO
{
private:
	int fd;
public:
	SocketIO();
	~SocketIO();

	bool connect(const tiny_string& hostname, int port);
	bool connected() const;
	void close();
	ssize_t receive(void *buf, size_t count) const;
	ssize_t sendAll(const void *buf, size_t count) const;
	int fileDescriptor() const { return fd; }
};

class XMLSocketThread;

class XMLSocket : public EventDispatcher
{
protected:
	XMLSocketThread *job;
	Spinlock joblock; // protect access to job

	ASPROPERTY_GETTER_SETTER(int,timeout);
	ASFUNCTION(_constructor);
	ASFUNCTION(_close);
	ASFUNCTION(_connect);
	ASFUNCTION(_send);
	ASFUNCTION(_connected);

	void connect(tiny_string host, int port);
	bool isConnected();
public:
	XMLSocket(Class_base* c) : EventDispatcher(c), job(NULL), timeout(20000) {};
	~XMLSocket();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void finalize();
	void threadFinished();
};

class XMLSocketThread : public IThreadJob
{
private:
	SocketIO sock;
	_R<XMLSocket> owner;
	tiny_string hostname;
	int port;
	int timeout;
	GAsyncQueue *sendQueue;
	int signalEmitter;
	int signalListener;

	int connectSocket();
	void readSocket(const SocketIO& sock);
	void executeCommand(char cmd, SocketIO& sock);
public:
	XMLSocketThread(_R<XMLSocket> owner, const tiny_string& hostname, int port, int timeout);
	~XMLSocketThread();
	virtual void execute();
	virtual void jobFence();
	void sendData(const tiny_string& data);
	void requestClose();
	bool isConnected();
};

}

#endif
