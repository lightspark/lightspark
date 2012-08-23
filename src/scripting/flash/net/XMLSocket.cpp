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

#include "XMLSocket.h"
#include "abc.h"
#include "toplevel/Error.h"
#include "class.h"
#include "argconv.h"
#include "swf.h"
#include "flash/errors/flasherrors.h"
#include "backends/security.h"
#include <sys/types.h>
#ifdef _WIN32
#	define _WIN32_WINNT 0x0501
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

XMLSocket::~XMLSocket()
{
	finalize();
}

void XMLSocket::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(_close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(_connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(_send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(_connected),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,timeout);
}

void XMLSocket::buildTraits(ASObject* o)
{
}

void XMLSocket::finalize()
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
}

ASFUNCTIONBODY_GETTER_SETTER(XMLSocket, timeout);

ASFUNCTIONBODY(XMLSocket, _constructor)
{
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_UNPACK (host, "") (port, 0);

	EventDispatcher::_constructor(obj,NULL,0);

	XMLSocket* th=obj->as<XMLSocket>();
	host_is_null = argslen > 0 && args[0]->is<Null>();
	if (port != 0)
	{
		if (host_is_null)
			th->connect("", port);
		else if (!host.empty())
			th->connect(host, port);
	}

	return NULL;
}

ASFUNCTIONBODY(XMLSocket, _close)
{
	XMLSocket* th=obj->as<XMLSocket>();
	SpinlockLocker l(th->joblock);

	if (th->job)
	{
		th->job->requestClose();
	}

	return NULL;
}

void XMLSocket::connect(tiny_string host, int port)
{
	if (port <= 0 || port > 65535)
		throw Class<SecurityError>::getInstanceS("Invalid port");

	if (host.empty())
		host = getSys()->mainClip->getOrigin().getHostname();

	if (isConnected())
		throw Class<IOError>::getInstanceS("Already connected");

	// Host shouldn't contain scheme or port
	if (host.strchr(':') != NULL)
		throw Class<SecurityError>::getInstanceS("Invalid hostname");

	// Check sandbox and policy file
	size_t buflen = host.numBytes() + 22;
	char *urlbuf = g_newa(char, buflen);
	snprintf(urlbuf, buflen, "xmlsocket://%s:%d", host.raw_buf(), port);
	URLInfo url(urlbuf);

	getSys()->securityManager->checkURLStaticAndThrow(url,
		~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
		true);

	SecurityManager::EVALUATIONRESULT evaluationResult;
	evaluationResult = getSys()->securityManager->evaluateSocketConnection(url, true);
	if(evaluationResult != SecurityManager::ALLOWED)
	{
		incRef();
		getVm()->addEvent(_MR(this), _MR(Class<SecurityErrorEvent>::getInstanceS("No policy file allows socket connection")));
		return;
	}

	incRef();
	XMLSocketThread *thread = new XMLSocketThread(_MR(this), host, port, timeout);
	getSys()->addJob(thread);
	job = thread;
}

ASFUNCTIONBODY(XMLSocket, _connect)
{
	XMLSocket* th=obj->as<XMLSocket>();
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_UNPACK (host) (port);
	host_is_null = argslen > 0 && args[0]->is<Null>();

	if (host_is_null)
		th->connect("", port);
	else
		th->connect(host, port);

	return NULL;
}

ASFUNCTIONBODY(XMLSocket, _send)
{
	XMLSocket* th=obj->as<XMLSocket>();
	tiny_string data;
	ARG_UNPACK (data);

	SpinlockLocker l(th->joblock);
	if (th->job)
	{
		th->job->sendData(data);
	}
	else
	{
		throw Class<IOError>::getInstanceS("Socket is not connected");
	}

	return NULL;
}

bool XMLSocket::isConnected()
{
	SpinlockLocker l(joblock);
	return job && job->isConnected();
}

ASFUNCTIONBODY(XMLSocket, _connected)
{
	XMLSocket* th=obj->as<XMLSocket>();
	return abstract_b(th->isConnected());
}

void XMLSocket::threadFinished()
{
	SpinlockLocker l(joblock);
	job = NULL;
}

XMLSocketThread::XMLSocketThread(_R<XMLSocket> _owner, const tiny_string& _hostname, int _port, int _timeout)
: owner(_owner), hostname(_hostname), port(_port), timeout(_timeout)
{
	sendQueue = g_async_queue_new();

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

XMLSocketThread::~XMLSocketThread()
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

void XMLSocketThread::execute()
{
	if (!sock.connect(hostname, port))
	{
		getVm()->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS()));
		return;
	}

	getVm()->addEvent(owner, _MR(Class<Event>::getInstanceS("connect")));

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
			getVm()->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS()));
			return;
		}

		if (FD_ISSET(signalListener, &readfds))
		{
			char cmd;

			ssize_t nbytes = read(signalListener, &cmd, 1);
			if (nbytes < 0)
			{
				getVm()->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS()));
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
		}
		else if (FD_ISSET(sock.fileDescriptor(), &readfds))
		{
			readSocket(sock);
		}
	}
}

void XMLSocketThread::readSocket(const SocketIO& sock)
{
	char buf[1024];
	size_t nbytes = sock.receive(buf, sizeof buf - 1);

	if (nbytes > 0)
	{
		buf[nbytes] = '\0';
		tiny_string data(buf, true);
		getVm()->addEvent(owner, _MR(Class<DataEvent>::getInstanceS(data)));
	}
	else if (nbytes == 0)
	{
		// The server has closed the socket
		getVm()->addEvent(owner, _MR(Class<Event>::getInstanceS("close")));
		threadAborting = true;
	}
	else
	{
		// Error
		getVm()->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS()));
		threadAborting = true;
	}
}

void XMLSocketThread::executeCommand(char cmd, SocketIO& sock)
{
	switch (cmd)
	{
		case SOCKET_COMMAND_SEND:
		{
			void *data;
			while ((data = g_async_queue_try_pop(sendQueue)) != NULL)
			{
				tiny_string *s = (tiny_string *)data;
				sock.sendAll(s->raw_buf(), s->numBytes());
				delete s;
			}
			break;
		}
		case SOCKET_COMMAND_CLOSE:
		{
			sock.close();
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

bool XMLSocketThread::isConnected()
{
	return sock.connected();
}

void XMLSocketThread::jobFence()
{
	owner->threadFinished();
	delete this;
}

void XMLSocketThread::sendData(const tiny_string& data)
{
	if (threadAborting)
		return;

	tiny_string *packet = new tiny_string(data);
	g_async_queue_push(sendQueue, packet);
	write(signalEmitter, &SOCKET_COMMAND_SEND, 1);
}

void XMLSocketThread::requestClose()
{
	write(signalEmitter, &SOCKET_COMMAND_CLOSE, 1);
}
