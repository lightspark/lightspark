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

#include "Socket.h"
#include "backends/security.h"
#include "abc.h"
#include "toplevel/Error.h"
#include "class.h"
#include "argconv.h"
#include "swf.h"
#include "flash/errors/flasherrors.h"
#include <sys/types.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0501
#endif
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <fcntl.h>
#else
#	include <sys/socket.h>
#	include <netdb.h>
#	include <sys/select.h>
#endif
#include <string.h>
#include <unistd.h>
#include <errno.h>

const char SOCKET_COMMAND_SEND = '*';
const char SOCKET_COMMAND_CLOSE = '-';

using namespace std;
using namespace lightspark;

SocketIO::SocketIO() : fd(-1)
{
#ifdef _WIN32
	//TODO: move WSAStartup/WSACleanup to some more global place
	WSADATA wsdata;
	if(WSAStartup(MAKEWORD(2, 2), &wsdata))
		LOG(LOG_ERROR,"WSAStartup failed");
#endif
}

SocketIO::~SocketIO()
{
	if (fd != -1)
		::close(fd);
#ifdef _WIN32
	WSACleanup();
#endif
}

bool SocketIO::connect(const tiny_string& hostname, int port)
{
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;

	if (fd != -1)
		return false;
	fd = -1;

	if (port <= 0 || port > 65535)
		return false;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	tiny_string portstr = Integer::toString(port);
	if (getaddrinfo(hostname.raw_buf(), portstr.raw_buf(), &hints, &servinfo) != 0)
	{
		return false;
	}

	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		// TODO: timeout on connect

		if (::connect(fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			::close(fd);
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (!p)
		fd = -1;

	return fd != -1;
}

bool SocketIO::connected() const
{
	return fd != -1;
}

void SocketIO::close()
{
	if (fd != -1)
	{
		::close(fd);
		fd = -1;
	}
}

ssize_t SocketIO::receive(void *buf, size_t count) const
{
	ssize_t n;

	do
	{
		n = read(fd, buf, count);
	}
	while (n < 0 && errno == EINTR);

	return n;
}

ssize_t SocketIO::sendAll(const void *buf, size_t count) const
{
	ssize_t n;
	ssize_t total;

	char *writeptr = (char *)buf;
	total = 0;
	while (count > 0) {
		n = write(fd, writeptr, count);

		if (n < 0)
		{
			if (errno == EINTR)
				continue;

			return -1;
		}

		writeptr += n;
		total += n;
		count -= n;
	}

	return total;
}

ASSocket::~ASSocket()
{
}

void ASSocket::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),_close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),_connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(c->getSystemState(),_connected),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("flush","",Class<IFunction>::getFunction(c->getSystemState(),_flush),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(c->getSystemState(),bytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_getEndian),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_setEndian),SETTER_METHOD,true);
//	c->setDeclaredMethodByQName("readBoolean","",Class<IFunction>::getFunction(c->getSystemState(),readBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(c->getSystemState(),readBytes),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(c->getSystemState(),readByte),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(c->getSystemState(),readDouble),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(c->getSystemState(),readFloat),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(c->getSystemState(),readInt),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readMultiByte","",Class<IFunction>::getFunction(c->getSystemState(),readMultiByte),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readShort","",Class<IFunction>::getFunction(c->getSystemState(),readShort),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readUnsignedByte","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedByte),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedInt),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readUnsignedShort","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedShort),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(c->getSystemState(),readObject),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(c->getSystemState(),readUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(c->getSystemState(),readUTFBytes),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeBoolean","",Class<IFunction>::getFunction(c->getSystemState(),writeBoolean),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeUTF","",Class<IFunction>::getFunction(c->getSystemState(),writeUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTFBytes","",Class<IFunction>::getFunction(c->getSystemState(),writeUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBytes","",Class<IFunction>::getFunction(c->getSystemState(),writeBytes),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeByte","",Class<IFunction>::getFunction(c->getSystemState(),writeByte),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeDouble","",Class<IFunction>::getFunction(c->getSystemState(),writeDouble),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeFloat","",Class<IFunction>::getFunction(c->getSystemState(),writeFloat),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeInt","",Class<IFunction>::getFunction(c->getSystemState(),writeInt),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeMultiByte","",Class<IFunction>::getFunction(c->getSystemState(),writeMultiByte),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeUnsignedInt","",Class<IFunction>::getFunction(c->getSystemState(),writeUnsignedInt),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeObject","",Class<IFunction>::getFunction(c->getSystemState(),writeObject),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("writeShort","",Class<IFunction>::getFunction(c->getSystemState(),writeShort),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,timeout);
}

void ASSocket::finalize()
{
	EventDispatcher::finalize();

	SpinlockLocker l(joblock);
	if (job)
	{
		job->threadAborting = true;
		job->requestClose();
		job->threadAbort();
		job = NULL;
	}
	timeout = 20000;
}

ASFUNCTIONBODY_GETTER_SETTER(ASSocket, timeout);

ASFUNCTIONBODY_ATOM(ASSocket,_constructor)
{
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_UNPACK_ATOM (host, "") (port, 0);

	EventDispatcher::_constructor(ret,sys,obj,NULL,0);

	ASSocket* th=obj.as<ASSocket>();
	host_is_null = argslen > 0 && args[0].is<Null>();
	if (port != 0)
	{
		if (host_is_null)
			th->connect("", port);
		else if (!host.empty())
			th->connect(host, port);
	}
}

ASFUNCTIONBODY_ATOM(ASSocket, _close)
{
	ASSocket* th=obj.as<ASSocket>();
	SpinlockLocker l(th->joblock);

	if (th->job)
	{
		th->job->requestClose();
	}
}

void ASSocket::connect(tiny_string host, int port)
{
	if (port <= 0 || port > 65535)
		throw Class<SecurityError>::getInstanceS(getSystemState(),"Invalid port");

	if (host.empty())
		host = getSys()->mainClip->getOrigin().getHostname();

	if (isConnected())
		throw Class<IOError>::getInstanceS(getSystemState(),"Already connected");

	// Host shouldn't contain scheme or port
	if (host.strchr(':') != NULL)
		throw Class<SecurityError>::getInstanceS(getSystemState(),"Invalid hostname");

	// Check sandbox and policy file
	size_t buflen = host.numBytes() + 22;
	char *urlbuf = g_newa(char, buflen);
	// TODO don't use "xmlsocket" as protocol for socket ?
	snprintf(urlbuf, buflen, "xmlsocket://%s:%d", host.raw_buf(), port);
	URLInfo url(urlbuf);

	getSystemState()->securityManager->checkURLStaticAndThrow(url,
		~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
		true);

	SecurityManager::EVALUATIONRESULT evaluationResult;
	evaluationResult = getSys()->securityManager->evaluateSocketConnection(url, true);
	if(evaluationResult != SecurityManager::ALLOWED)
	{
		incRef();
		getVm(getSystemState())->addEvent(_MR(this), _MR(Class<SecurityErrorEvent>::getInstanceS(getSystemState(),"No policy file allows socket connection")));
		return;
	}

	incRef();
	ASSocketThread *thread = new ASSocketThread(_MR(this), host, port, timeout);
	getSys()->addJob(thread);
	job = thread;
}

ASFUNCTIONBODY_ATOM(ASSocket, _connect)
{
	ASSocket* th=obj.as<ASSocket>();
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_UNPACK_ATOM (host) (port);
	host_is_null = argslen > 0 && args[0].is<Null>();

	if (host_is_null)
		th->connect("", port);
	else
		th->connect(host, port);
}
ASFUNCTIONBODY_ATOM(ASSocket, bytesAvailable)
{
	ASSocket* th=obj.as<ASSocket>();
	SpinlockLocker l(th->joblock);

	if (th->job)
	{
		th->job->datareceive->lock();
		ret.setUInt(th->job->datareceive->getLength());
		th->job->datareceive->unlock();
	}
	else
		ret.setUInt(0);
}

ASFUNCTIONBODY_ATOM(ASSocket,_getEndian)
{
	ASSocket* th=obj.as<ASSocket>();
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		if(th->job->datasend->getLittleEndian())
			ret = asAtom::fromString(sys,Endian::littleEndian);
		else
			ret = asAtom::fromString(sys,Endian::bigEndian);
	}
	else
		ret = asAtom::fromString(sys,Endian::bigEndian);
}

ASFUNCTIONBODY_ATOM(ASSocket,_setEndian)
{
	ASSocket* th=obj.as<ASSocket>();
	bool v = false;
	if(args[0].toString(sys) == Endian::littleEndian)
		v = true;
	else if(args[0].toString(sys) == Endian::bigEndian)
		v = false;
	else
		throwError<ArgumentError>(kInvalidEnumError, "endian");
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->datasend->setLittleEndian(v);
		th->job->datareceive->setLittleEndian(v);
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,readBytes)
{
	ASSocket* th=obj.as<ASSocket>();
	_NR<ByteArray> data;
	uint32_t offset;
	uint32_t length;
	ARG_UNPACK_ATOM (data)(offset,0)(length,0);
	if (data.isNull())
		return;
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		if (length == 0)
			length = th->job->datareceive->getLength();
		uint8_t buf[length];
		th->job->datareceive->readBytes(0,length,buf);
		th->job->datareceive->unlock();
		uint32_t pos = data->getPosition();
		data->setPosition(offset);
		data->writeBytes(buf,length);
		data->setPosition(pos);
	}
	else
	{
		throw Class<IOError>::getInstanceS(sys,"Socket is not connected");
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,writeUTFBytes)
{
	ASSocket* th=obj.as<ASSocket>();
	tiny_string data;
	ARG_UNPACK_ATOM (data);
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->datasend->lock();
		th->job->datasend->writeUTF(data);
		th->job->datasend->unlock();
	}
	else
	{
		throw Class<IOError>::getInstanceS(sys,"Socket is not connected");
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeBytes)
{
	ASSocket* th=obj.as<ASSocket>();
	_NR<ByteArray> data;
	uint32_t offset;
	uint32_t length;
	ARG_UNPACK_ATOM (data)(offset,0)(length,0);
	if (data.isNull())
		return;
	if (offset >= data->getLength())
		throwError<RangeError>(kParamRangeError);
	if (offset+length > data->getLength())
		throwError<RangeError>(kParamRangeError);
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		if (length == 0)
			length = data->getLength()-offset;
		uint8_t buf[length];
		data->readBytes(offset,length,buf);
		th->job->datasend->lock();
		th->job->datasend->writeBytes(buf,length);
		th->job->datasend->unlock();
	}
	else
	{
		throw Class<IOError>::getInstanceS(sys,"Socket is not connected");
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,readUTFBytes)
{
	ASSocket* th=obj.as<ASSocket>();
	uint32_t length;
	ARG_UNPACK_ATOM (length);
	tiny_string data;
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		th->job->datareceive->readUTFBytes(length,data);
		th->job->datareceive->removeFrontBytes(length);
		th->job->datareceive->unlock();
		ret.set(asAtom::fromString(sys,data));
	}
	else
	{
		throw Class<IOError>::getInstanceS(sys,"Socket is not connected");
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,_flush)
{
	ASSocket* th=obj.as<ASSocket>();
	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->flushData();
	}
	else
	{
		throw Class<IOError>::getInstanceS(sys,"Socket is not connected");
	}
}

bool ASSocket::isConnected()
{
	SpinlockLocker l(joblock);
	return job && job->isConnected();
}

ASFUNCTIONBODY_ATOM(ASSocket, _connected)
{
	ASSocket* th=obj.as<ASSocket>();
	ret.setBool(th->isConnected());
}

void ASSocket::threadFinished()
{
	SpinlockLocker l(joblock);
	job = NULL;
}

ASSocketThread::ASSocketThread(_R<ASSocket> _owner, const tiny_string& _hostname, int _port, int _timeout)
: owner(_owner), hostname(_hostname), port(_port), timeout(_timeout)
{
	sendQueue = g_async_queue_new();
	datasend = _MR(Class<ByteArray>::getInstanceS(owner->getSystemState()));
	datareceive = _MR(Class<ByteArray>::getInstanceS(owner->getSystemState()));
#ifdef _WIN32
	HANDLE readPipe, writePipe;
	if (!CreatePipe(&readPipe,&writePipe,NULL,0))
	{
		signalListener = -1;
		signalEmitter = -1;
		return;
	}
	signalListener = _open_osfhandle((intptr_t)readPipe, _O_RDONLY);
	signalEmitter = _open_osfhandle((intptr_t)writePipe, _O_WRONLY);
#else
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		signalListener = -1;
		signalEmitter = -1;
		return;
	}
	signalListener = pipefd[0];
	signalEmitter = pipefd[1];
#endif
}

ASSocketThread::~ASSocketThread()
{
	if (signalListener != -1)
		::close(signalListener);
	if (signalEmitter != -1)
		::close(signalEmitter);

	void *data;
	while ((data = g_async_queue_try_pop(sendQueue)) != NULL)
	{
		tiny_string *s = (tiny_string *)data;
		delete s;
	}
	g_async_queue_unref(sendQueue);
}

void ASSocketThread::execute()
{
	if (!sock.connect(hostname, port))
	{
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getSystemState())));
		return;
	}

	bool first=true;
	struct timeval timeout;
	int maxfd;
	fd_set readfds;
	       
	while (!threadAborting)
	{
		FD_ZERO(&readfds);
		FD_SET(signalListener, &readfds);
		FD_SET(sock.fileDescriptor(), &readfds);
		maxfd = max(signalListener, sock.fileDescriptor());

		// Check threadAborting periodically
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		int status = select(maxfd+1, &readfds, NULL, NULL, &timeout);
		if (status  < 0)
		{
			owner->incRef();
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getSystemState())));
			return;
		}

		if (FD_ISSET(signalListener, &readfds))
		{
			char cmd;

			ssize_t nbytes = read(signalListener, &cmd, 1);
			if (nbytes < 0)
			{
				owner->incRef();
				getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getSystemState())));
				return;
			}
			else if (nbytes == 0)
			{
				//The pipe has been closed. This
				//shouldn't happen, the pipe will be
				//closed only in the desctructor.
				return;
			}
			else
			{
				executeCommand(cmd, sock);
			}
			if (first && !threadAborting)
			{
				// send connect event only after first succesful communication
				first = false;
				owner->incRef();
				getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getSystemState(),"connect")));
			}
		}
		else if (FD_ISSET(sock.fileDescriptor(), &readfds))
		{
			readSocket(sock);
		}
	}
}

void ASSocketThread::readSocket(const SocketIO& sock)
{
	uint8_t buf[1024];
	size_t nbytes = sock.receive(buf, sizeof buf - 1);

	if (nbytes > 0)
	{
		buf[nbytes] = '\0';
		datareceive->writeBytes(buf,nbytes);
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<ProgressEvent>::getInstanceS(owner->getSystemState(),nbytes,0,"socketData")));
	}
	else if (nbytes == 0)
	{
		// The server has closed the socket
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getSystemState(),"close")));
		threadAborting = true;
	}
	else
	{
		// Error
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getSystemState())));
		threadAborting = true;
	}
}

void ASSocketThread::executeCommand(char cmd, SocketIO& sock)
{
	switch (cmd)
	{
		case SOCKET_COMMAND_SEND:
		{
			datasend->lock();
			sock.sendAll(datasend->getBuffer(datasend->getLength(),false),datasend->getLength());
			datasend->setLength(0);
			datasend->unlock();
			void *data;
			while ((data = g_async_queue_try_pop(sendQueue)) != NULL)
			{
				tiny_string *s = (tiny_string *)data;
				delete s;
			}
			break;
		}
		case SOCKET_COMMAND_CLOSE:
		{
			sock.close();
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getSystemState(),"close")));
			threadAborting = true;
			break;
		}
		default:
		{
			assert_and_throw(false && "Unexpected command");
			break;
		}
	}
}

bool ASSocketThread::isConnected()
{
	return sock.connected();
}

void ASSocketThread::jobFence()
{
	owner->threadFinished();
	delete this;
}

void ASSocketThread::flushData()
{
	if (threadAborting)
		return;

	tiny_string *packet = new tiny_string("");
	g_async_queue_push(sendQueue, packet);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(signalEmitter, &SOCKET_COMMAND_SEND, 1);
#pragma GCC diagnostic pop
}

void ASSocketThread::requestClose()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(signalEmitter, &SOCKET_COMMAND_CLOSE, 1);
#pragma GCC diagnostic pop
}
