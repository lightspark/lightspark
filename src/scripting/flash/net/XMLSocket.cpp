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

#include "XMLSocket.h"
#include "backends/security.h"
#include "abc.h"
#include "toplevel/Error.h"
#include "class.h"
#include "argconv.h"
#include "swf.h"
#include "flash/errors/flasherrors.h"

#ifdef _WIN32
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0501
#endif
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <fcntl.h>
#else
#include <unistd.h>
#endif

const char SOCKET_COMMAND_SEND = '*';
const char SOCKET_COMMAND_CLOSE = '-';

using namespace std;
using namespace lightspark;

XMLSocket::~XMLSocket()
{
}

void XMLSocket::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),_close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),_connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),_send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(c->getSystemState(),_connected),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,timeout);
}

void XMLSocket::buildTraits(ASObject* o)
{
}

void XMLSocket::finalize()
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

ASFUNCTIONBODY_GETTER_SETTER(XMLSocket, timeout)

ASFUNCTIONBODY_ATOM(XMLSocket,_constructor)
{
	tiny_string host;
	bool host_is_null;
	int port;
	ARG_CHECK(ARG_UNPACK (host, "") (port, 0));

	EventDispatcher::_constructor(ret,wrk,obj,NULL,0);

	XMLSocket* th=asAtomHandler::as<XMLSocket>(obj);
	host_is_null = argslen > 0 && asAtomHandler::is<Null>(args[0]);
	if (port != 0)
	{
		if (host_is_null)
			th->connect("", port);
		else if (!host.empty())
			th->connect(host, port);
	}
}

ASFUNCTIONBODY_ATOM(XMLSocket, _close)
{
	XMLSocket* th=asAtomHandler::as<XMLSocket>(obj);
	Locker l(th->joblock);

	if (th->job)
	{
		th->job->requestClose();
	}
}

void XMLSocket::connect(tiny_string host, int port)
{
	if (port <= 0 || port > 65535)
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
		createError<SecurityError>(getInstanceWorker(),0,"Invalid hostname");

	// Check sandbox and policy file
	size_t buflen = host.numBytes() + 22;
	char *urlbuf = g_newa(char, buflen);
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
		getVm(getSystemState())->addEvent(_MR(this), _MR(Class<SecurityErrorEvent>::getInstanceS(getInstanceWorker(),"No policy file allows socket connection")));
		return;
	}

	incRef();
	XMLSocketThread *thread = new XMLSocketThread(_MR(this), host, port, timeout);
	getSys()->addJob(thread);
	job = thread;
}

ASFUNCTIONBODY_ATOM(XMLSocket, _connect)
{
	XMLSocket* th=asAtomHandler::as<XMLSocket>(obj);
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

ASFUNCTIONBODY_ATOM(XMLSocket, _send)
{
	XMLSocket* th=asAtomHandler::as<XMLSocket>(obj);
	tiny_string data;
	ARG_CHECK(ARG_UNPACK (data));

	Locker l(th->joblock);
	if (th->job)
	{
		th->job->sendData(data);
	}
	else
	{
		createError<IOError>(wrk,0,"Socket is not connected");
	}
}

bool XMLSocket::isConnected()
{
	Locker l(joblock);
	return job && job->isConnected();
}

ASFUNCTIONBODY_ATOM(XMLSocket, _connected)
{
	XMLSocket* th=asAtomHandler::as<XMLSocket>(obj);
	asAtomHandler::setBool(ret,th->isConnected());
}

void XMLSocket::threadFinished()
{
	Locker l(joblock);
	job = nullptr;
}
void XMLSocket::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == this)
	{
		ASWorker* wrk = getInstanceWorker();
		if (e->type == "connect")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONCONNECT;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom arg0 = asAtomHandler::fromBool(this->isConnected());
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,&arg0,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
		else if (e->type == "data")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONDATA;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom arg0 = asAtomHandler::fromString(getSystemState(),e->as<DataEvent>()->data);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,&arg0,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
		else if (e->type == "close")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONCLOSE;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
	}
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
	while ((data = g_async_queue_try_pop(sendQueue)) != nullptr)
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
		owner->incRef();
		if (owner->getSystemState()->mainClip->needsActionScript3())
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<IOErrorEvent>::getInstanceS(owner->getInstanceWorker())));
		else
			getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"connect")));
		return;
	}

	owner->incRef();
	getVm(owner->getSystemState())->addEvent(owner, _MR(Class<Event>::getInstanceS(owner->getInstanceWorker(),"connect")));

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
		owner->incRef();
		getVm(owner->getSystemState())->addEvent(owner, _MR(Class<DataEvent>::getInstanceS(owner->getInstanceWorker(),data)));
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

void XMLSocketThread::executeCommand(char cmd, SocketIO& sock)
{
	switch (cmd)
	{
		case SOCKET_COMMAND_SEND:
		{
			void *data;
			while ((data = g_async_queue_try_pop(sendQueue)) != nullptr)
			{
				tiny_string *s = (tiny_string *)data;
				sock.sendAll(s->raw_buf(), s->numBytes());
				delete s;
				// according to specs every message is terminated by a null byte
				char buf=0;
				sock.sendAll(&buf, 1);
				
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(signalEmitter, &SOCKET_COMMAND_SEND, 1);
#pragma GCC diagnostic pop
}

void XMLSocketThread::requestClose()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(signalEmitter, &SOCKET_COMMAND_CLOSE, 1);
#pragma GCC diagnostic pop
}
