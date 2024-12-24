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
#include "threading.h"
#include "flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
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

struct socketbuf
{
	uint8_t* buf;
	size_t len;
	socketbuf(uint8_t* data, size_t l)
	{
		buf = new uint8_t[l];
		len = l;
		memcpy(buf,data,len);
	}
	~socketbuf()
	{
		delete[] buf;
	}
};


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

bool SocketIO::connect(const tiny_string& hostname, int port, int timeoutseconds)
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

	for(p = servinfo; p != nullptr; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (timeoutseconds != 0)
		{
#ifdef _WIN32
			::setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,(const char *)&timeoutseconds,sizeof(timeoutseconds));
#else
			timeval tv;
			tv.tv_sec=timeoutseconds;
			tv.tv_usec=0;
			::setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
#endif
		}

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
	c->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(_close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",c->getSystemState()->getBuiltinFunction(_connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",c->getSystemState()->getBuiltinFunction(_connected,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("flush","",c->getSystemState()->getBuiltinFunction(_flush),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",c->getSystemState()->getBuiltinFunction(bytesAvailable,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",c->getSystemState()->getBuiltinFunction(_getEndian),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",c->getSystemState()->getBuiltinFunction(_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",c->getSystemState()->getBuiltinFunction(_getObjectEncoding,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",c->getSystemState()->getBuiltinFunction(_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("readBoolean","",c->getSystemState()->getBuiltinFunction(readBoolean,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",c->getSystemState()->getBuiltinFunction(readByte,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",c->getSystemState()->getBuiltinFunction(readBytes),NORMAL_METHOD,true);

	c->setDeclaredMethodByQName("readDouble","",c->getSystemState()->getBuiltinFunction(readDouble,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",c->getSystemState()->getBuiltinFunction(readFloat,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",c->getSystemState()->getBuiltinFunction(readInt,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readMultiByte","",c->getSystemState()->getBuiltinFunction(readMultiByte,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readShort","",c->getSystemState()->getBuiltinFunction(readShort,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedByte","",c->getSystemState()->getBuiltinFunction(readUnsignedByte,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",c->getSystemState()->getBuiltinFunction(readUnsignedInt,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedShort","",c->getSystemState()->getBuiltinFunction(readUnsignedShort,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",c->getSystemState()->getBuiltinFunction(readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",c->getSystemState()->getBuiltinFunction(readUTF,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",c->getSystemState()->getBuiltinFunction(readUTFBytes),NORMAL_METHOD,true);

	c->setDeclaredMethodByQName("writeBoolean","",c->getSystemState()->getBuiltinFunction(writeBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTF","",c->getSystemState()->getBuiltinFunction(writeUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTFBytes","",c->getSystemState()->getBuiltinFunction(writeUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBytes","",c->getSystemState()->getBuiltinFunction(writeBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeByte","",c->getSystemState()->getBuiltinFunction(writeByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeDouble","",c->getSystemState()->getBuiltinFunction(writeDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeFloat","",c->getSystemState()->getBuiltinFunction(writeFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeInt","",c->getSystemState()->getBuiltinFunction(writeInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeMultiByte","",c->getSystemState()->getBuiltinFunction(writeMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUnsignedInt","",c->getSystemState()->getBuiltinFunction(writeUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeObject","",c->getSystemState()->getBuiltinFunction(writeObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeShort","",c->getSystemState()->getBuiltinFunction(writeShort),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,timeout);

	c->addImplementedInterface(InterfaceClass<IDataInput>::getClass(c->getSystemState()));
	IDataInput::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IDataOutput>::getClass(c->getSystemState()));
	IDataOutput::linkTraits(c);
}

void ASSocket::finalize()
{
	EventDispatcher::finalize();

	Locker l(joblock);
	if (job)
	{
		job->threadAborting = true;
		job->requestClose();
		job->threadAbort();
		job = nullptr;
	}
	timeout = 20000;
}

ASFUNCTIONBODY_GETTER_SETTER(ASSocket, timeout);

ASFUNCTIONBODY_ATOM(ASSocket,_constructor)
{
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_CHECK(ARG_UNPACK (host, "") (port, 0));

	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);

	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	host_is_null = argslen > 0 && asAtomHandler::is<Null>(args[0]);
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
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);

	if (th->job)
	{
		th->job->requestClose();
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,_getObjectEncoding)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	asAtomHandler::setUInt(ret,wrk,th->objectEncoding);
}

ASFUNCTIONBODY_ATOM(ASSocket,_setObjectEncoding)
{
	LOG(LOG_NOT_IMPLEMENTED,"setting Socket.objectEncoding has no effect");
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	uint32_t value;
	ARG_CHECK(ARG_UNPACK(value));
	if(value!=OBJECT_ENCODING::AMF0 && value!=OBJECT_ENCODING::AMF3)
	{
		createError<ArgumentError>(wrk,kInvalidEnumError, "objectEncoding");
		return;
	}

	th->objectEncoding=value;
}

void ASSocket::connect(tiny_string host, int port)
{
	if ((port <= 0) || (port > 65535))
	{
		createError<SecurityError>(getInstanceWorker(),0,"Invalid port");
		return;
	}

	if (host.empty())
		host = getSys()->mainClip->getOrigin().getHostname();

	if (isConnected())
	{
		createError<IOError>(getInstanceWorker(),0,"Already connected");
		return;
	}

	// Host shouldn't contain scheme or port
	if (host.strchr(':') != nullptr)
	{
		createError<SecurityError>(getInstanceWorker(),0,"Invalid hostname");
		return;
	}

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
	if (getInstanceWorker()->currentCallContext && getInstanceWorker()->currentCallContext->exceptionthrown)
		return;

	SecurityManager::EVALUATIONRESULT evaluationResult;
	evaluationResult = getSys()->securityManager->evaluateSocketConnection(url, true);
	if(evaluationResult != SecurityManager::ALLOWED)
	{
		incRef();
		getVm(getSystemState())->addEvent(_MR(this), _MR(Class<SecurityErrorEvent>::getInstanceS(getInstanceWorker(),"No policy file allows socket connection")));
		return;
	}

	incRef();
	ASSocketThread *thread = new ASSocketThread(_MR(this), host, port, timeout);
	getSys()->addJob(thread);
	job = thread;
}

ASFUNCTIONBODY_ATOM(ASSocket, _connect)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_CHECK(ARG_UNPACK (host) (port));
	host_is_null = argslen > 0 && asAtomHandler::is<Null>(args[0]);

	if (host_is_null)
		th->connect("", port);
	else
		th->connect(host, port);
}
ASFUNCTIONBODY_ATOM(ASSocket, bytesAvailable)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	asAtomHandler::setUInt(ret,wrk, th->job != nullptr ? th->_bytesAvailable : 0);
}

ASFUNCTIONBODY_ATOM(ASSocket,_getEndian)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		if(th->job->datasend->getLittleEndian())
			ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::littleEndian);
		else
			ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::bigEndian);
	}
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::bigEndian);
}

ASFUNCTIONBODY_ATOM(ASSocket,_setEndian)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	bool v = false;
	if(asAtomHandler::toString(args[0],wrk) == Endian::littleEndian)
		v = true;
	else if(asAtomHandler::toString(args[0],wrk) == Endian::bigEndian)
		v = false;
	else
	{
		createError<ArgumentError>(wrk,kInvalidEnumError, "endian");
		return;
	}
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datasend->setLittleEndian(v);
		th->job->datareceive->setLittleEndian(v);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readBoolean)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);

	uint8_t res=0;
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		th->job->datareceive->readByte(res);
		th->job->datareceive->unlock();
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
	asAtomHandler::setBool(ret,res!=0);
}

ASFUNCTIONBODY_ATOM(ASSocket,readByte)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);

	uint8_t res=0;
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		th->job->datareceive->readByte(res);
		th->job->datareceive->unlock();
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
	asAtomHandler::setInt(ret,wrk,int32_t(res));
}

ASFUNCTIONBODY_ATOM(ASSocket,readBytes)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	_NR<ByteArray> data;
	uint32_t offset;
	uint32_t length;
	ARG_CHECK(ARG_UNPACK (data)(offset,0)(length,0));
	if (data.isNull())
		return;
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		if (length == 0)
			length = th->_bytesAvailable;
		uint8_t buf[length];
		th->job->datareceive->readBytes(0,length,buf);
		th->job->datareceive->setPosition(length);
		th->job->datareceive->removeFrontBytes(length);
		th->job->datareceive->unlock();
		uint32_t pos = data->getPosition();
		data->setPosition(offset);
		data->writeBytes(buf,length);
		data->setPosition(pos);
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,readDouble)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readDouble");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readFloat)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readFloat");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readInt)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readInt");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readMultiByte)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readMultiByte");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readObject)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readObject");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readShort)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readShort");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readUnsignedByte)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readUnsignedByte");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readUnsignedInt)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readUnsignedInt");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readUnsignedShort)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readUnsignedShort");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,readUTF)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.readUTF");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,writeBoolean)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeBoolean");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeByte)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeByte");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeDouble)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeDouble");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeFloat)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeFloat");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeInt)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeInt");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeUnsignedInt)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeUnsignedInt");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeMultiByte)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeMultiByte");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeObject)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeObject");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeShort)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeShort");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeUTF)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Socket.writeUTF");
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,writeUTFBytes)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	tiny_string data;
	ARG_CHECK(ARG_UNPACK (data));
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datasend->lock();
		th->job->datasend->writeUTF(data);
		th->job->datasend->unlock();
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,writeBytes)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		_NR<ByteArray> data;
		uint32_t offset;
		uint32_t length;
		ARG_CHECK(ARG_UNPACK (data)(offset,0)(length,0));
		if (data.isNull())
			return;
		if (offset >= data->getLength())
		{
			createError<RangeError>(wrk,kParamRangeError);
			return;
		}
		if (offset+length > data->getLength())
		{
			createError<RangeError>(wrk,kParamRangeError);
			return;
		}
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
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
}

ASFUNCTIONBODY_ATOM(ASSocket,readUTFBytes)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	uint32_t length;
	ARG_CHECK(ARG_UNPACK (length));
	tiny_string data;
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->datareceive->lock();
		th->job->datareceive->readUTFBytes(length,data);
		th->job->datareceive->removeFrontBytes(length);
		th->job->datareceive->unlock();
		asAtomHandler::set(ret,asAtomHandler::fromString(wrk->getSystemState(),data));
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
		return;
	}
}
ASFUNCTIONBODY_ATOM(ASSocket,_flush)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	Locker l(th->joblock);
	if (th->job)
	{
		th->job->flushData();
	}
	else
	{
		createError<IOError>(wrk,kInvalidSocket);
	}
}

bool ASSocket::isConnected()
{
	Locker l(joblock);
	return job && job->isConnected();
}

ASSocket::ASSocket(ASWorker* wrk, Class_base* c) : EventDispatcher(wrk,c), job(nullptr), objectEncoding(OBJECT_ENCODING::AMF3), _bytesAvailable(0), timeout(20000)
{
}

ASFUNCTIONBODY_ATOM(ASSocket, _connected)
{
	ASSocket* th=asAtomHandler::as<ASSocket>(obj);
	asAtomHandler::setBool(ret,th->isConnected());
}

void ASSocket::threadFinished()
{
	Locker l(joblock);
	job = nullptr;
}

ASSocketThread::ASSocketThread(_R<ASSocket> _owner, const tiny_string& _hostname, int _port, int _timeout)
: owner(_owner), hostname(_hostname), port(_port), timeout(_timeout)
{
	sendQueue = g_async_queue_new();
	datasend = _MR(Class<ByteArray>::getInstanceS(owner->getInstanceWorker()));
	datareceive = _MR(Class<ByteArray>::getInstanceS(owner->getInstanceWorker()));
#ifdef _WIN32
	HANDLE readPipe, writePipe;
	if (!CreatePipe(&readPipe,&writePipe,nullptr,0))
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
	while ((data = g_async_queue_try_pop(sendQueue)) != nullptr)
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
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getInstanceWorker())));
		return;
	}
	if (!threadAborting)
	{
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"connect")));
	}

//	bool first=true;
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

		int status = select(maxfd+1, &readfds, nullptr, nullptr, &timeout);
		if (status  < 0)
		{
			owner->incRef();
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getInstanceWorker())));
			return;
		}

		if (FD_ISSET(signalListener, &readfds))
		{
			char cmd;

			ssize_t nbytes = read(signalListener, &cmd, 1);
			if (nbytes < 0)
			{
				owner->incRef();
				getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getInstanceWorker())));
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
//			if (first && !threadAborting)
//			{
//				// send connect event only after first succesful communication
//				first = false;
//				owner->incRef();
//				getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"connect")));
//			}
		}
		else if (FD_ISSET(sock.fileDescriptor(), &readfds))
		{
			readSocket(sock);
		}
	}
}

ssize_t ASSocketThread::receive()
{
	uint8_t buf[4096];
	ssize_t size;
	ssize_t total = 0;
	if (datareceive->getPosition())
		datareceive->removeFrontBytes(datareceive->getPosition());
	do
	{
		size = sock.receive(buf, sizeof buf);
		if (size > 0)
			datareceive->append(buf,size);
		total += size;
	}
	while (size == sizeof(buf));
	return total;
}

void ASSocketThread::readSocket(const SocketIO& sock)
{
	ssize_t nbytes = receive();
	if (nbytes > 0)
	{
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<ProgressEvent>::getInstanceS(owner->getInstanceWorker(),nbytes,0,"socketData")));
	}
	else if (nbytes == 0)
	{
		// The server has closed the socket
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"close")));
		threadAborting = true;
	}
	else
	{
		// Error
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getInstanceWorker())));
		threadAborting = true;
	}
}

void ASSocketThread::executeCommand(char cmd, SocketIO& sock)
{
	switch (cmd)
	{
		case SOCKET_COMMAND_SEND:
		{
			void *data;
			while ((data = g_async_queue_try_pop(sendQueue)) != nullptr)
			{
				socketbuf *s = (socketbuf *)data;
				sock.sendAll(s->buf, s->len);
				delete s;
			}
			break;
		}
		case SOCKET_COMMAND_CLOSE:
		{
			sock.close();
			owner->incRef();
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"close")));
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

	datasend->lock();
	socketbuf* packet = new socketbuf(datasend->getBuffer(datasend->getLength(),false),datasend->getLength());
	datasend->setLength(0);
	datasend->unlock();
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
