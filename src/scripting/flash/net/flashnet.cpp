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

#include <map>
#include "scripting/abc.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/class.h"
#include "scripting/flash/system/flashsystem.h"
#include "compat.h"
#include "backends/audio.h"
#include "backends/builtindecoder.h"
#include "backends/rendering.h"
#include "backends/security.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

URLRequest::URLRequest(Class_base* c):ASObject(c),method(GET),contentType("application/x-www-form-urlencoded"),
	requestHeaders(Class<Array>::getInstanceS())
{
}

void URLRequest::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(_setURL),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(_getURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("method","",Class<IFunction>::getFunction(_setMethod),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("method","",Class<IFunction>::getFunction(_getMethod),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(_setData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(_getData),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,contentType);
	REGISTER_GETTER_SETTER(c,requestHeaders);
}

void URLRequest::buildTraits(ASObject* o)
{
}

URLInfo URLRequest::getRequestURL() const
{
	URLInfo ret=getSys()->mainClip->getBaseURL().goToURL(url);
	if(method!=GET)
		return ret;

	if(data.isNull())
		return ret;

	if(data->getClass()==Class<ByteArray>::getClass())
		ret=ret.getParsedURL();
	else
	{
		tiny_string newURL = ret.getParsedURL();
		if(ret.getQuery() == "")
			newURL += "?";
		else
			newURL += "&amp;";
		newURL += data->toString();
		ret=ret.goToURL(newURL);
	}
	return ret;
}

/* Return contentType if it is a valid value for Content-Type header,
 * otherwise raise ArgumentError.
 */
tiny_string URLRequest::validatedContentType() const
{
	if(contentType.find("\r")!=contentType.npos || 
	   contentType.find("\n")!=contentType.npos)
	{
		throw Class<ArgumentError>::getInstanceS(tiny_string("Error #2096: The HTTP request header ") + contentType + tiny_string(" cannot be set via ActionScript."));
	}

	return contentType;
}

/* Throws ArgumentError if headerName is not an allowed HTTP header
 * name.
 */
void URLRequest::validateHeaderName(const tiny_string& headerName) const
{
	const char *illegalHeaders[] = 
		{"accept-charset", "accept_charset", "accept-encoding",
		 "accept_encoding", "accept-ranges", "accept_ranges",
		 "age", "allow", "allowed", "authorization", "charge-to",
		 "charge_to", "connect", "connection", "content-length",
		 "content_length", "content-location", "content_location",
		 "content-range", "content_range", "cookie", "date", "delete",
		 "etag", "expect", "get", "head", "host", "if-modified-since",
		 "if_modified-since", "if-modified_since", "if_modified_since",
		 "keep-alive", "keep_alive", "last-modified", "last_modified",
		 "location", "max-forwards", "max_forwards", "options",
		 "origin", "post", "proxy-authenticate", "proxy_authenticate",
		 "proxy-authorization", "proxy_authorization",
		 "proxy-connection", "proxy_connection", "public", "put",
		 "range", "referer", "request-range", "request_range",
		 "retry-after", "retry_after", "server", "te", "trace",
		 "trailer", "transfer-encoding", "transfer_encoding",
		 "upgrade", "uri", "user-agent", "user_agent", "vary", "via",
		 "warning", "www-authenticate", "www_authenticate",
		 "x-flash-version", "x_flash-version", "x-flash_version",
		 "x_flash_version"};

	if ((headerName.strchr('\r') != NULL) ||
	     headerName.strchr('\n') != NULL)
		throw Class<ArgumentError>::getInstanceS("Error #2096: The HTTP request header cannot be set via ActionScript");

	for (unsigned i=0; i<(sizeof illegalHeaders)/(sizeof illegalHeaders[0]); i++)
	{
		if (headerName.lowercase() == illegalHeaders[i])
		{
			tiny_string msg("Error #2096: The HTTP request header ");
			msg += headerName;
			msg += " cannot be set via ActionScript";
			throw Class<ArgumentError>::getInstanceS(msg);
		}
	}
}

/* Validate requestHeaders and return them as list. Throws
 * ArgumentError if requestHeaders contains illegal headers or the
 * cumulative length is too long.
 */
std::list<tiny_string> URLRequest::getHeaders() const
{
	list<tiny_string> headers;
	int headerTotalLen = 0;
	for (unsigned i=0; i<requestHeaders->size(); i++)
	{
		_R<ASObject> headerObject = requestHeaders->at(i);

		// Validate
		if (!headerObject->is<URLRequestHeader>())
			throw Class<TypeError>::getInstanceS("Error #1034: Object is not URLRequestHeader");
		URLRequestHeader *header = headerObject->as<URLRequestHeader>();
		tiny_string headerName = header->name;
		validateHeaderName(headerName);
		if ((header->value.strchr('\r') != NULL) ||
		     header->value.strchr('\n') != NULL)
			throw Class<ArgumentError>::getInstanceS("Illegal HTTP header value");
		
		// Should this include the separators?
		headerTotalLen += header->name.numBytes();
		headerTotalLen += header->value.numBytes();
		if (headerTotalLen >= 8192)
			throw Class<ArgumentError>::getInstanceS("Error #2145: Cumulative length of requestHeaders must be less than 8192 characters.");

		// Append header to results
		headers.push_back(headerName + ": " + header->value);
	}

	tiny_string contentType = getContentTypeHeader();
	if (!contentType.empty())
		headers.push_back(contentType);

	return headers;
}

tiny_string URLRequest::getContentTypeHeader() const
{
	if(method!=POST)
		return "";

	if(!data.isNull() && data->getClass()==Class<URLVariables>::getClass())
		return "Content-type: application/x-www-form-urlencoded";
	else
		return tiny_string("Content-Type: ") + validatedContentType();
}

void URLRequest::getPostData(vector<uint8_t>& outData) const
{
	if(method!=POST)
		return;

	if(data.isNull())
		return;

	if(data->getClass()==Class<ByteArray>::getClass())
	{
		ByteArray *ba=data->as<ByteArray>();
		const uint8_t *buf=ba->getBuffer(ba->getLength(), false);
		outData.insert(outData.end(),buf,buf+ba->getLength());
	}
	else
	{
		const tiny_string& strData=data->toString();
		outData.insert(outData.end(),strData.raw_buf(),strData.raw_buf()+strData.numBytes());
	}
}

void URLRequest::finalize()
{
	ASObject::finalize();
	data.reset();
}

ASFUNCTIONBODY(URLRequest,_constructor)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	if(argslen==1 && args[0]->getObjectType()==T_STRING)
	{
		th->url=args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_setURL)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	assert_and_throw(argslen==1);
	th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getURL)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	return Class<ASString>::getInstanceS(th->url);
}

ASFUNCTIONBODY(URLRequest,_setMethod)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& tmp=args[0]->toString();
	if(tmp=="GET")
		th->method=GET;
	else if(tmp=="POST")
		th->method=POST;
	else
		throw UnsupportedException("Unsupported method in URLLoader");
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getMethod)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	switch(th->method)
	{
		case GET:
			return Class<ASString>::getInstanceS("GET");
		case POST:
			return Class<ASString>::getInstanceS("POST");
	}
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getData)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	if(th->data.isNull())
		return getSys()->getUndefinedRef();

	th->data->incRef();
	return th->data.getPtr();
}

ASFUNCTIONBODY(URLRequest,_setData)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	assert_and_throw(argslen==1);

	args[0]->incRef();
	th->data=_MR(args[0]);

	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(URLRequest,contentType);
ASFUNCTIONBODY_GETTER_SETTER(URLRequest,requestHeaders);

void URLRequestMethod::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("GET","",Class<ASString>::getInstanceS("GET"),DECLARED_TRAIT);
	c->setVariableByQName("POST","",Class<ASString>::getInstanceS("POST"),DECLARED_TRAIT);
}

URLLoaderThread::URLLoaderThread(_R<URLRequest> request, _R<URLLoader> ldr)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr)
{
}

void URLLoaderThread::execute()
{
	assert(!downloader);

	//TODO: support httpStatus, progress events

	if(!createDownloader(false, loader, loader.getPtr()))
		return;

	_NR<ASObject> data;
	bool success=false;
	if(!downloader->hasFailed())
	{
		getVm()->addEvent(loader,_MR(Class<Event>::getInstanceS("open")));
		downloader->waitForTermination();
		if(!downloader->hasFailed() && !threadAborting)
		{
			istream s(downloader);
			uint8_t* buf=new uint8_t[downloader->getLength()+1];
			//TODO: avoid this useless copy
			s.read((char*)buf,downloader->getLength());
			buf[downloader->getLength()] = '\0';
			//TODO: test binary data format
			tiny_string dataFormat=loader->getDataFormat();
			if(dataFormat=="binary")
			{
				_R<ByteArray> byteArray=_MR(Class<ByteArray>::getInstanceS());
				byteArray->acquireBuffer(buf,downloader->getLength());
				data=byteArray;
				//The buffers must not be deleted, it's now handled by the ByteArray instance
			}
			else if(dataFormat=="text")
			{
				data=_MR(Class<ASString>::getInstanceS((char*)buf,downloader->getLength()));
				delete[] buf;
			}
			else if(dataFormat=="variables")
			{
				data=_MR(Class<URLVariables>::getInstanceS((char*)buf));
				delete[] buf;
			}
			else
			{
				assert(false && "invalid dataFormat");
			}

			success=true;
		}
	}

	// Don't send any events if the thread is aborting
	if(success && !threadAborting)
	{
		//Send a complete event for this object
		loader->setData(data);
		getVm()->addEvent(loader,_MR(Class<Event>::getInstanceS("complete")));
	}
	else if(!success && !threadAborting)
	{
		//Notify an error during loading
		getVm()->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS()));
	}

	{
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
}

URLLoader::URLLoader(Class_base* c):EventDispatcher(c),dataFormat("text"),data(),job(NULL)
{
}

void URLLoader::finalize()
{
	EventDispatcher::finalize();
	data.reset();
}

void URLLoader::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("dataFormat","",Class<IFunction>::getFunction(_getDataFormat),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(_getData),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(_setData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("dataFormat","",Class<IFunction>::getFunction(_setDataFormat),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,bytesLoaded);
	REGISTER_GETTER_SETTER(c,bytesTotal);
}

ASFUNCTIONBODY_GETTER_SETTER(URLLoader, bytesLoaded);
ASFUNCTIONBODY_GETTER_SETTER(URLLoader, bytesTotal);

void URLLoader::buildTraits(ASObject* o)
{
}

void URLLoader::threadFinished(IThreadJob *finishedJob)
{
	// If this is the current job, we are done. If these are not
	// equal, finishedJob is a job that was cancelled when load()
	// was called again, and we have to still wait for the correct
	// job.
	SpinlockLocker l(spinlock);
	if(finishedJob==job)
		job=NULL;

	delete finishedJob;
}

void URLLoader::setData(_NR<ASObject> newData)
{
	SpinlockLocker l(spinlock);
	data=newData;
}

void URLLoader::setBytesTotal(uint32_t b)
{
	bytesTotal = b;
}

void URLLoader::setBytesLoaded(uint32_t b)
{
	bytesLoaded = b;
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	if(argslen==1 && args[0]->getClass() == Class<URLRequest>::getClass())
	{
		//URLRequest* urlRequest=Class<URLRequest>::dyncast(args[0]);
		load(obj, args, argslen);
	}
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	ASObject* arg=args[0];
	URLRequest* urlRequest=Class<URLRequest>::dyncast(arg);
	assert_and_throw(urlRequest);

	{
		SpinlockLocker l(th->spinlock);
		if(th->job)
			th->job->threadAbort();
	}

	URLInfo url=urlRequest->getRequestURL();
	if(!url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getSys()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::checkURLStaticAndThrow(url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	//TODO: should we disallow accessing local files in a directory above 
	//the current one like we do with NetStream.play?

	th->incRef();
	urlRequest->incRef();
	URLLoaderThread *job=new URLLoaderThread(_MR(urlRequest), _MR(th));
	getSys()->addJob(job);
	th->job=job;
	return NULL;
}

ASFUNCTIONBODY(URLLoader,close)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	SpinlockLocker l(th->spinlock);
	if(th->job)
		th->job->threadAbort();

	return NULL;
}

tiny_string URLLoader::getDataFormat()
{
	SpinlockLocker l(spinlock);
	return dataFormat;
}

void URLLoader::setDataFormat(const tiny_string& newFormat)
{
	SpinlockLocker l(spinlock);
	dataFormat=newFormat.lowercase();
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	return Class<ASString>::getInstanceS(th->getDataFormat());
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	SpinlockLocker l(th->spinlock);
	if(th->data.isNull())
		return getSys()->getUndefinedRef();
	
	th->data->incRef();
	return th->data.getPtr();
}

ASFUNCTIONBODY(URLLoader,_setData)
{
	if(!obj->is<URLLoader>())
		throw Class<ArgumentError>::getInstanceS("Function applied to wrong object");
	URLLoader* th = obj->as<URLLoader>();
	if(argslen != 1)
		throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter");
	args[0]->incRef();
	th->setData(_MR(args[0]));
	return NULL;
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	assert_and_throw(args[0]);
	th->setDataFormat(args[0]->toString());
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS("variables"),DECLARED_TRAIT);
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS("text"),DECLARED_TRAIT);
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS("binary"),DECLARED_TRAIT);
}

SharedObject::SharedObject(Class_base* c):EventDispatcher(c)
{
	data=_MR(new_asobject());
}

void SharedObject::sinit(Class_base* c)
{
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("getLocal","",Class<IFunction>::getFunction(getLocal),NORMAL_METHOD,false);
	REGISTER_GETTER(c,data);
};

ASFUNCTIONBODY_GETTER(SharedObject,data);

ASFUNCTIONBODY(SharedObject,getLocal)
{
	//TODO: Implement this
	return Class<SharedObject>::getInstanceS();
}

void ObjectEncoding::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("AMF0","",abstract_i(AMF0),DECLARED_TRAIT);
	c->setVariableByQName("AMF3","",abstract_i(AMF3),DECLARED_TRAIT);
	c->setVariableByQName("DEFAULT","",abstract_i(DEFAULT),DECLARED_TRAIT);
};

NetConnection::NetConnection(Class_base* c):EventDispatcher(c),_connected(false),downloader(NULL),messageCount(0)
{
}

void NetConnection::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(call),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(_getConnected),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_setDefaultObjectEncoding),SETTER_METHOD,false);
	getSys()->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::DEFAULT;
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("protocol","",Class<IFunction>::getFunction(_getProtocol),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,client);
}

void NetConnection::buildTraits(ASObject* o)
{
}

void NetConnection::finalize()
{
	EventDispatcher::finalize();
	responder.reset();
	client.reset();
}

ASFUNCTIONBODY(NetConnection, _constructor)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	th->objectEncoding = getSys()->staticNetConnectionDefaultObjectEncoding;
	return NULL;
}

ASFUNCTIONBODY(NetConnection,call)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	//Arguments are:
	//1) A string for the command
	//2) A Responder instance (optional)
	//And other arguments to be passed to the server
	tiny_string command;
	ARG_UNPACK (command) (th->responder, NullRef);

	th->messageCount++;

	if(!th->uri.isValid())
		return NULL;

	if(th->uri.isRTMP())
	{
		LOG(LOG_NOT_IMPLEMENTED, "RTMP not yet supported in NetConnection.call()");
		return NULL;
	}

	//This function is supposed to be passed a array for the rest
	//of the arguments. Since that is not supported for native methods
	//just create it here
	_R<Array> rest=_MR(Class<Array>::getInstanceS());
	for(uint32_t i=2;i<argslen;i++)
	{
		args[i]->incRef();
		rest->push(_MR(args[i]));
	}

	_R<ByteArray> message=_MR(Class<ByteArray>::getInstanceS());
	//Version?
	message->writeByte(0x00);
	message->writeByte(0x03);
	//Number of headers: 0
	message->writeShort(0);
	//Number of messages: 1
	message->writeShort(1);
	//Write the command
	message->writeUTF(command);
	//Write a "response URI". Use an increasing index
	//NOTE: this assumes that the tiny_string is constant and not modified
	char responseBuf[20];
	snprintf(responseBuf,20,"/%u", th->messageCount);
	message->writeUTF(tiny_string(responseBuf));
	uint32_t messageLenPosition=message->getPosition();
	message->writeUnsignedInt(0x0);
	//HACK: Write the escape code for AMF3 data, it's the only supported mode
	message->writeByte(0x11);
	uint32_t messageLen=message->writeObject(rest.getPtr());
	message->setPosition(messageLenPosition);
	message->writeUnsignedInt(messageLen+1);

	uint32_t len=message->getLength();
	uint8_t* buf=message->getBuffer(len, false);
	th->messageData.clear();
	th->messageData.insert(th->messageData.end(), buf, buf+len);

	//To be decreffed in jobFence
	th->incRef();
	getSys()->addJob(th);
	return NULL;
}

void NetConnection::execute()
{
	LOG(LOG_CALLS,_("NetConnection async execution ") << uri);
	assert(!messageData.empty());
	std::list<tiny_string> headers;
	headers.push_back("Content-Type: application/x-amf");
	downloader=getSys()->downloadManager->downloadWithData(uri, messageData,
			headers, NULL);
	//Get the whole answer
	downloader->waitForTermination();
	if(downloader->hasFailed()) //Check to see if the download failed for some reason
	{
		LOG(LOG_ERROR, "NetConnection::execute(): Download of URL failed: " << uri);
//		getVm()->addEvent(contentLoaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
		getSys()->downloadManager->destroy(downloader);
		return;
	}
	istream s(downloader);
	_R<ByteArray> message=_MR(Class<ByteArray>::getInstanceS());
	uint8_t* buf=message->getBuffer(downloader->getLength(), true);
	s.read((char*)buf,downloader->getLength());
	//Download is done, destroy it
	{
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
	_R<ParseRPCMessageEvent> event=_MR(new (getSys()->unaccountedMemory) ParseRPCMessageEvent(message, client, responder));
	getVm()->addEvent(NullRef,event);
	responder.reset();
}

void NetConnection::threadAbort()
{
	//We have to stop the downloader
	SpinlockLocker l(downloaderLock);
	if(downloader != NULL)
		downloader->stop();
}

void NetConnection::jobFence()
{
	decRef();
}

ASFUNCTIONBODY(NetConnection,connect)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	//This takes 1 required parameter and an unspecified number of optional parameters
	assert_and_throw(argslen>0);

	//This seems strange:
	//LOCAL_WITH_FILE may not use connect(), even if it tries to connect to a local file.
	//I'm following the specification to the letter. Testing showed
	//that the official player allows connect(null) in localWithFile.
	if(args[0]->getObjectType() != T_NULL
	&& getSys()->securityManager->evaluateSandbox(SecurityManager::LOCAL_WITH_FILE))
		throw Class<SecurityError>::getInstanceS("SecurityError: NetConnection::connect "
				"from LOCAL_WITH_FILE sandbox");

	bool isNull = false;
	bool isRTMP = false;
	//bool isRPC = false;

	//Null argument means local file or web server, the spec only mentions NULL, but youtube uses UNDEFINED, so supporting that too.
	if(args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED)
	{
		th->_connected = false;
		isNull = true;
	}
	//String argument means Flash Remoting/Flash Media Server
	else
	{
		th->_connected = false;
		ASString* command = static_cast<ASString*>(args[0]);
		th->uri = URLInfo(command->toString());

		if(getSys()->securityManager->evaluatePoliciesURL(th->uri, true) != SecurityManager::ALLOWED)
		{
			//TODO: find correct way of handling this case
			throw Class<SecurityError>::getInstanceS("SecurityError: connection to domain not allowed by securityManager");
		}
		
		//By spec NetConnection::connect is true for RTMP and remoting and false otherwise
		if(th->uri.isRTMP())
		{
			isRTMP = true;
			th->_connected = true;
		}
		else if(th->uri.getProtocol() == "http" ||
		     th->uri.getProtocol() == "https")
		{
			th->_connected = true;
			//isRPC = true;
		}
		else
		{
			LOG(LOG_ERROR, "Unsupported protocol " << th->uri.getProtocol() << " in NetConnection::connect");
			throw UnsupportedException("NetConnection::connect: protocol not supported");
		}

		// We actually create the connection later in
		// NetStream::play() or NetConnection.call()
	}

	//When the URI is undefined the connect is successful (tested on Adobe player)
	if(isNull || isRTMP)
	{
		th->incRef();
		getVm()->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetConnection.Connect.Success")));
	}
	return NULL;
}

ASFUNCTIONBODY(NetConnection,_getConnected)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	return abstract_b(th->_connected);
}

ASFUNCTIONBODY(NetConnection,_getDefaultObjectEncoding)
{
	return abstract_i(getSys()->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY(NetConnection,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = args[0]->toInt();
	if(value == 0)
		getSys()->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
		getSys()->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF3;
	else
		throw RunTimeException("Invalid object encoding");
	return NULL;
}

ASFUNCTIONBODY(NetConnection,_getObjectEncoding)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	return abstract_i(th->objectEncoding);
}

ASFUNCTIONBODY(NetConnection,_setObjectEncoding)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	assert_and_throw(argslen == 1);
	if(th->_connected)
	{
		throw Class<ReferenceError>::getInstanceS("set NetConnection.objectEncoding after connect");
	}
	int32_t value = args[0]->toInt();
	if(value == 0)
		th->objectEncoding = ObjectEncoding::AMF0; 
	else
		th->objectEncoding = ObjectEncoding::AMF3; 
	return NULL;
}

ASFUNCTIONBODY(NetConnection,_getProtocol)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	if(th->_connected)
		return Class<ASString>::getInstanceS(th->protocol);
	else
		throw Class<ArgumentError>::getInstanceS("get NetConnection.protocol before connect");
}

ASFUNCTIONBODY(NetConnection,_getURI)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	if(th->_connected && th->uri.isValid())
		return Class<ASString>::getInstanceS(th->uri.getURL());
	else
	{
		//Reference says the return should be undefined. The right thing is "null" as a string
		return Class<ASString>::getInstanceS("null");
	}
}

ASFUNCTIONBODY_GETTER_SETTER(NetConnection, client);

NetStream::NetStream(Class_base* c):EventDispatcher(c),tickStarted(false),paused(false),closed(true),
	streamTime(0),frameRate(0),connection(),downloader(NULL),videoDecoder(NULL),
	audioDecoder(NULL),audioStream(NULL),
	client(NullRef),oldVolume(-1.0),checkPolicyFile(false),rawAccessAllowed(false)
{
	soundTransform = _MNR(Class<SoundTransform>::getInstanceS());
}

void NetStream::finalize()
{
	EventDispatcher::finalize();
	connection.reset();
	client.reset();
}

NetStream::~NetStream()
{
	if(tickStarted)
		getSys()->removeJob(this);
	delete videoDecoder;
	delete audioDecoder;
}

void NetStream::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setVariableByQName("CONNECT_TO_FMS","",Class<ASString>::getInstanceS("connectToFMS"),DECLARED_TRAIT);
	c->setVariableByQName("DIRECT_CONNECTIONS","",Class<ASString>::getInstanceS("directConnections"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("resume","",Class<IFunction>::getFunction(resume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pause","",Class<IFunction>::getFunction(pause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("togglePause","",Class<IFunction>::getFunction(togglePause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("seek","",Class<IFunction>::getFunction(seek),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",Class<IFunction>::getFunction(_getTime),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFPS","",Class<IFunction>::getFunction(_getCurrentFPS),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("client","",Class<IFunction>::getFunction(_getClient),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("client","",Class<IFunction>::getFunction(_setClient),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("checkPolicyFile","",Class<IFunction>::getFunction(_getCheckPolicyFile),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("checkPolicyFile","",Class<IFunction>::getFunction(_setCheckPolicyFile),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,soundTransform);
}

void NetStream::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_GETTER_SETTER(NetStream,soundTransform);

ASFUNCTIONBODY(NetStream,_getClient)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->client.isNull())
		return getSys()->getUndefinedRef();

	th->client->incRef();
	return th->client.getPtr();
}

ASFUNCTIONBODY(NetStream,_setClient)
{
	assert_and_throw(argslen == 1);
	if(args[0]->getObjectType() == T_NULL)
		throw Class<TypeError>::getInstanceS();

	NetStream* th=Class<NetStream>::cast(obj);

	args[0]->incRef();
	th->client = _MR(args[0]);
	return NULL;
}

ASFUNCTIONBODY(NetStream,_getCheckPolicyFile)
{
	NetStream* th=Class<NetStream>::cast(obj);

	return abstract_b(th->checkPolicyFile);
}

ASFUNCTIONBODY(NetStream,_setCheckPolicyFile)
{
	assert_and_throw(argslen == 1);
	
	NetStream* th=Class<NetStream>::cast(obj);

	th->checkPolicyFile = Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(NetStream,_constructor)
{
	obj->incRef();
	_R<NetStream> th=_MR(Class<NetStream>::cast(obj));

	LOG(LOG_CALLS,_("NetStream constructor"));
	assert_and_throw(argslen>=1 && argslen <=2);
	assert_and_throw(args[0]->getClass()==Class<NetConnection>::getClass());

	args[0]->incRef();
	_R<NetConnection> netConnection = _MR(Class<NetConnection>::cast(args[0]));
	if(argslen == 2)
	{
		if(args[1]->getObjectType() == T_STRING)
		{
			tiny_string value = Class<ASString>::cast(args[1])->toString();
			if(value == "directConnections")
				th->peerID = DIRECT_CONNECTIONS;
			else
				th->peerID = CONNECT_TO_FMS;
		}
		else if(args[1]->getObjectType() == T_NULL)
			th->peerID = CONNECT_TO_FMS;
		else
			throw Class<ArgumentError>::getInstanceS("NetStream constructor: peerID");
	}

	th->client = th;
	th->connection=netConnection;

	return NULL;
}

ASFUNCTIONBODY(NetStream,play)
{
	NetStream* th=Class<NetStream>::cast(obj);

	//Make sure the stream is restarted properly
	if(th->closed)
		th->closed = false;
	else
		return NULL;

	//Reset the paused states
	th->paused = false;
//	th->audioPaused = false;
	assert(!th->connection.isNull());

	if(th->connection->uri.getProtocol()=="http")
	{
		//Remoting connection used, this should not happen
		throw RunTimeException("Remoting NetConnection used in NetStream::play");
	}
	
	if(th->connection->uri.isValid())
	{
		//We should connect to FMS
		assert_and_throw(argslen>=1 && argslen<=4);
		//Args: name, start, len, reset
		th->url=th->connection->uri;
		th->url.setStream(args[0]->toString());
	}
	else
	{
		//HTTP download
		assert_and_throw(argslen>=1);
		//args[0] is the url
		//what is the meaning of the other arguments
		th->url = getSys()->mainClip->getOrigin().goToURL(args[0]->toString());

		SecurityManager::EVALUATIONRESULT evaluationResult =
			getSys()->securityManager->evaluateURLStatic(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
				SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
				true); //Check for navigating up in local directories (not allowed)
		if(evaluationResult == SecurityManager::NA_REMOTE_SANDBOX)
			throw Class<SecurityError>::getInstanceS("SecurityError: NetStream::play: "
					"connect to network");
		//Local-with-filesystem sandbox can't access network
		else if(evaluationResult == SecurityManager::NA_LOCAL_SANDBOX)
			throw Class<SecurityError>::getInstanceS("SecurityError: NetStream::play: "
					"connect to local file");
		else if(evaluationResult == SecurityManager::NA_PORT)
			throw Class<SecurityError>::getInstanceS("SecurityError: NetStream::play: "
					"connect to restricted port");
		else if(evaluationResult == SecurityManager::NA_RESTRICT_LOCAL_DIRECTORY)
			throw Class<SecurityError>::getInstanceS("SecurityError: NetStream::play: "
					"not allowed to navigate up for local files");
	}

	assert_and_throw(th->downloader==NULL);

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm()->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
	}
	else //The URL is valid so we can start the download and add ourself as a job
	{
		//Cahe the download only if it is not RTMP based
		bool cached=!th->url.isRTMP();
		th->downloader=getSys()->downloadManager->download(th->url, cached, NULL);
		th->streamTime=0;
		//To be decreffed in jobFence
		th->incRef();
		getSys()->addJob(th);
	}
	return NULL;
}

void NetStream::jobFence()
{
	decRef();
}

ASFUNCTIONBODY(NetStream,resume)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->paused)
	{
		th->paused = false;
		{
			Mutex::Lock l(th->mutex);
			if(th->audioStream)
				th->audioStream->resume();
		}
		th->incRef();
		getVm()->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Unpause.Notify")));
	}
	return NULL;
}

ASFUNCTIONBODY(NetStream,pause)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(!th->paused)
	{
		th->paused = true;
		{
			Mutex::Lock l(th->mutex);
			if(th->audioStream)
				th->audioStream->pause();
		}
		th->incRef();
		getVm()->addEvent(_MR(th),_MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Pause.Notify")));
	}
	return NULL;
}

ASFUNCTIONBODY(NetStream,togglePause)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->paused)
		th->resume(obj, NULL, 0);
	else
		th->pause(obj, NULL, 0);
	return NULL;
}

ASFUNCTIONBODY(NetStream,close)
{
	NetStream* th=Class<NetStream>::cast(obj);
	//TODO: set the time property to 0
	
	//Everything is stopped in threadAbort
	if(!th->closed)
	{
		th->threadAbort();
		th->incRef();
		getVm()->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Play.Stop")));
	}
	LOG(LOG_CALLS, _("NetStream::close called"));
	return NULL;
}

ASFUNCTIONBODY(NetStream,seek)
{
	//NetStream* th=Class<NetStream>::cast(obj);
	assert_and_throw(argslen == 1);
	return NULL;
}

//Tick is called from the timer thread, this happens only if a decoder is available
void NetStream::tick()
{
	//Check if the stream is paused
	if(audioStream && audioStream->isValid())
	{
		//TODO: use soundTransform->pan
		if(soundTransform && soundTransform->volume != oldVolume)
		{
			audioStream->setVolume(soundTransform->volume);
			oldVolume = soundTransform->volume;
		}
	}
	if(paused)
		return;
	//Advance video and audio to current time, follow the audio stream time
	//No mutex needed, ticking can happen only when stream is completely ready
	if(audioStream && getSys()->audioManager->isTimingAvailablePlugin())
	{
		assert(audioDecoder);
		streamTime=audioStream->getPlayedTime()+audioDecoder->initialTime;
	}
	else
	{
		streamTime+=1000/frameRate;
		audioDecoder->skipAll();
	}
	videoDecoder->skipUntil(streamTime);
	//The next line ensures that the downloader will not be destroyed before the upload jobs are fenced
	videoDecoder->waitForFencing();
	getSys()->getRenderThread()->addUploadJob(videoDecoder);
}

void NetStream::tickFence()
{
}

bool NetStream::isReady() const
{
	if(videoDecoder==NULL || audioDecoder==NULL)
		return false;

	bool ret=videoDecoder->isValid() && audioDecoder->isValid();
	return ret;
}

bool NetStream::lockIfReady()
{
	mutex.lock();
	bool ret=isReady();
	if(!ret) //If the data is not valid so not release the lock to keep the condition
		mutex.unlock();
	return ret;
}

void NetStream::unlock()
{
	mutex.unlock();
}

void NetStream::execute()
{
	//checkPolicyFile only applies to per-pixel access, loading and playing is always allowed.
	//So there is no need to disallow playing if policy files disallow it.
	//We do need to check if per-pixel access is allowed.
	SecurityManager::EVALUATIONRESULT evaluationResult = getSys()->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		rawAccessAllowed = true;

	if(downloader->hasFailed())
	{
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
		getSys()->downloadManager->destroy(downloader);
		return;
	}

	//The downloader hasn't failed yet at this point

	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

	ThreadProfile* profile=getSys()->allocateProfiler(RGB(0,0,200));
	profile->setTag("NetStream");
	bool waitForFlush=true;
	StreamDecoder* streamDecoder=NULL;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		Chronometer chronometer;
		streamDecoder=new FFMpegStreamDecoder(s);
		if(!streamDecoder->isValid())
			threadAbort();

		bool done=false;
		while(!done)
		{
			//Check if threadAbort has been called, if so, stop this loop
			if(closed)
				done = true;
			bool decodingSuccess=streamDecoder->decodeNextFrame();
			if(decodingSuccess==false)
				done = true;

			if(videoDecoder==NULL && streamDecoder->videoDecoder)
			{
				videoDecoder=streamDecoder->videoDecoder;
				this->incRef();
				getVm()->addEvent(_MR(this),
						_MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Play.Start")));
				this->incRef();
				getVm()->addEvent(_MR(this),
						_MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Buffer.Full")));
			}

			if(audioDecoder==NULL && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;

			if(audioStream==NULL && audioDecoder && audioDecoder->isValid() && getSys()->audioManager->pluginLoaded())
				audioStream=getSys()->audioManager->createStreamPlugin(audioDecoder);

			if(!tickStarted && isReady())
			{
				sendClientNotification("onMetaData", createMetaDataObject(streamDecoder));

				tickStarted=true;
				if(frameRate==0)
				{
					assert(videoDecoder->frameRate);
					frameRate=videoDecoder->frameRate;
				}
				getSys()->addTick(1000/frameRate,this);
				//Also ask for a render rate equal to the video one (capped at 24)
				float localRenderRate=dmin(frameRate,24);
				getSys()->setRenderRate(localRenderRate);
			}
			profile->accountTime(chronometer.checkpoint());
			if(threadAborting)
				throw JobTerminationException();
		}
#endif //ENABLE_LIBAVCODEC
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "Exception in NetStream " << e.cause);
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, _("Exception in reading: ")<<e.what());
	}
	if(waitForFlush)
	{
		//Put the decoders in the flushing state and wait for the complete consumption of contents
		if(audioDecoder)
			audioDecoder->setFlushing();
		if(videoDecoder)
			videoDecoder->setFlushing();
		
		if(audioDecoder)
			audioDecoder->waitFlushed();
		if(videoDecoder)
			videoDecoder->waitFlushed();

		this->incRef();
		getVm()->addEvent(_MR(this), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Play.Stop")));
		this->incRef();
		getVm()->addEvent(_MR(this), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetStream.Buffer.Flush")));
		sendClientNotification("onPlayStatus", createPlayStatusObject("NetStream.Play.Complete"));
	}
	//Before deleting stops ticking, removeJobs also spin waits for termination
	getSys()->removeJob(this);
	tickStarted=false;

	{
		Mutex::Lock l(mutex);
		//Change the state to invalid to avoid locking
		videoDecoder=NULL;
		audioDecoder=NULL;
		//Clean up everything for a possible re-run
		getSys()->downloadManager->destroy(downloader);
		//This transition is critical, so the mutex is needed
		downloader=NULL;
		delete audioStream;
		audioStream=NULL;
	}
	delete streamDecoder;
}

void NetStream::threadAbort()
{
	Mutex::Lock l(mutex);
	//This will stop the rendering loop
	closed = true;

	if(downloader)
		downloader->stop();

	//Clear everything we have in buffers, discard all frames
	if(videoDecoder)
	{
		videoDecoder->setFlushing();
		videoDecoder->skipAll();
	}
	if(audioDecoder)
	{
		//Clear everything we have in buffers, discard all frames
		audioDecoder->setFlushing();
		audioDecoder->skipAll();
	}
}

ASObject *NetStream::createMetaDataObject(StreamDecoder* streamDecoder)
{
	if(!streamDecoder)
		return NULL;

	ASObject* metadata = Class<ASObject>::getInstanceS();
	double d;
	uint32_t i;
	if(streamDecoder->getMetadataDouble("width",d))
		metadata->setVariableByQName("width", "",abstract_d(d),DYNAMIC_TRAIT);
	else
		metadata->setVariableByQName("width", "", abstract_d(getVideoWidth()),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataDouble("height",d))
		metadata->setVariableByQName("height", "",abstract_d(d),DYNAMIC_TRAIT);
	else
		metadata->setVariableByQName("height", "", abstract_d(getVideoHeight()),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataDouble("framerate",d))
		metadata->setVariableByQName("framerate", "",abstract_d(d),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataDouble("duration",d))
		metadata->setVariableByQName("duration", "",abstract_d(d),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataInteger("canseekontime",i))
		metadata->setVariableByQName("canSeekToEnd", "",abstract_b(i == 1),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataDouble("audiodatarate",d))
		metadata->setVariableByQName("audiodatarate", "",abstract_d(d),DYNAMIC_TRAIT);
	if(streamDecoder->getMetadataDouble("videodatarate",d))
		metadata->setVariableByQName("videodatarate", "",abstract_d(d),DYNAMIC_TRAIT);

	//TODO: missing: audiocodecid (Number), cuePoints (Object[]),
	//videocodecid (Number), custommetadata's

	return metadata;
}

ASObject *NetStream::createPlayStatusObject(const tiny_string& code)
{
	ASObject* info=Class<ASObject>::getInstanceS();
	info->setVariableByQName("level", "",Class<ASString>::getInstanceS("status"),DYNAMIC_TRAIT);
	info->setVariableByQName("code", "",Class<ASString>::getInstanceS(code),DYNAMIC_TRAIT);
	return info;
}

void NetStream::sendClientNotification(const tiny_string& name, ASObject *arg)
{
	if (client.isNull() || !arg)
		return;

	multiname callbackName(NULL);
	callbackName.name_type=multiname::NAME_STRING;
	callbackName.name_s_id=getSys()->getUniqueStringId(name);
	callbackName.ns.push_back(nsNameAndKind("",NAMESPACE));
	_NR<ASObject> callback = client->getVariableByMultiname(callbackName);
	if(!callback.isNull() && callback->is<Function>())
	{
		ASObject* callbackArgs[1];

		client->incRef();
		arg->incRef();
		callbackArgs[0] = arg;
		callback->incRef();
		_R<FunctionEvent> event(new (getSys()->unaccountedMemory) FunctionEvent(_MR(
				static_cast<IFunction*>(callback.getPtr())),
				_MR(client), callbackArgs, 1));
		getVm()->addEvent(NullRef,event);
	}
}

ASFUNCTIONBODY(NetStream,_getBytesLoaded)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->isReady())
		return abstract_i(th->getReceivedLength());
	else
		return abstract_i(0);
}

ASFUNCTIONBODY(NetStream,_getBytesTotal)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->isReady())
		return abstract_i(th->getTotalLength());
	else
		return abstract_d(0);
}

ASFUNCTIONBODY(NetStream,_getTime)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->isReady())
		return abstract_d(th->getStreamTime()/1000.);
	else
		return abstract_d(0);
}

ASFUNCTIONBODY(NetStream,_getCurrentFPS)
{
	//TODO: provide real FPS (what really is displayed)
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->isReady() && !th->paused)
		return abstract_d(th->getFrameRate());
	else
		return abstract_d(0);
}

uint32_t NetStream::getVideoWidth() const
{
	assert(isReady());
	return videoDecoder->getWidth();
}

uint32_t NetStream::getVideoHeight() const
{
	assert(isReady());
	return videoDecoder->getHeight();
}

double NetStream::getFrameRate()
{
	assert(isReady());
	return frameRate;
}

const TextureChunk& NetStream::getTexture() const
{
	assert(isReady());
	return videoDecoder->getTexture();
}

uint32_t NetStream::getStreamTime()
{
	assert(isReady());
	return streamTime;
}

uint32_t NetStream::getReceivedLength()
{
	assert(isReady());
	return downloader->getReceivedLength();
}

uint32_t NetStream::getTotalLength()
{
	assert(isReady());
	return downloader->getLength();
}

void URLVariables::decode(const tiny_string& s)
{
	const char* nameStart=NULL;
	const char* nameEnd=NULL;
	const char* valueStart=NULL;
	const char* valueEnd=NULL;
	const char* cur=s.raw_buf();
	while(1)
	{
		if(nameStart==NULL)
			nameStart=cur;
		if(*cur == '=')
		{
			if(nameStart==NULL || valueStart!=NULL) //Skip this
			{
				nameStart=NULL;
				nameEnd=NULL;
				valueStart=NULL;
				valueEnd=NULL;
				cur++;
				continue;
			}
			nameEnd=cur;
			valueStart=cur+1;
		}
		else if(*cur == '&' || *cur==0)
		{
			if(nameStart==NULL || nameEnd==NULL || valueStart==NULL || valueEnd!=NULL)
			{
				nameStart=NULL;
				nameEnd=NULL;
				valueStart=NULL;
				valueEnd=NULL;
				cur++;
				continue;
			}
			valueEnd=cur;
			char* name=g_uri_unescape_segment(nameStart,nameEnd,NULL);
			char* value=g_uri_unescape_segment(valueStart,valueEnd,NULL);
			nameStart=NULL;
			nameEnd=NULL;
			valueStart=NULL;
			valueEnd=NULL;
			if(name==NULL || value==NULL)
			{
				g_free(name);
				g_free(value);
				cur++;
				continue;
			}

			//Check if the variable already exists
			multiname propName(NULL);
			propName.name_type=multiname::NAME_STRING;
			propName.name_s_id=getSys()->getUniqueStringId(tiny_string(name,true));
			propName.ns.push_back(nsNameAndKind("",NAMESPACE));
			_NR<ASObject> curValue=getVariableByMultiname(propName);
			if(!curValue.isNull())
			{
				//If the variable already exists we have to create an Array of values
				Array* arr=NULL;
				if(curValue->getObjectType()!=T_ARRAY)
				{
					arr=Class<Array>::getInstanceS();
					arr->push(curValue);
					setVariableByMultiname(propName,arr,ASObject::CONST_NOT_ALLOWED);
				}
				else
					arr=Class<Array>::cast(curValue.getPtr());

				arr->push(_MR(Class<ASString>::getInstanceS(value)));
			}
			else
				setVariableByMultiname(propName,Class<ASString>::getInstanceS(value),ASObject::CONST_NOT_ALLOWED);

			g_free(name);
			g_free(value);
			if(*cur==0)
				break;
		}
		cur++;
	}
}

void URLVariables::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("decode","",Class<IFunction>::getFunction(decode),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

void URLVariables::buildTraits(ASObject* o)
{
}

URLVariables::URLVariables(Class_base* c, const tiny_string& s):ASObject(c)
{
	decode(s);
}

ASFUNCTIONBODY(URLVariables,decode)
{
	URLVariables* th=Class<URLVariables>::cast(obj);
	assert_and_throw(argslen==1);
	th->decode(args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(URLVariables,_toString)
{
	URLVariables* th=Class<URLVariables>::cast(obj);
	assert_and_throw(argslen==0);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

ASFUNCTIONBODY(URLVariables,_constructor)
{
	URLVariables* th=Class<URLVariables>::cast(obj);
	assert_and_throw(argslen<=1);
	if(argslen==1)
		th->decode(args[0]->toString());
	return NULL;
}

tiny_string URLVariables::toString_priv()
{
	int size=numVariables();
	tiny_string tmp;
	for(int i=0;i<size;i++)
	{
		const tiny_string& name=getNameAt(i);
		//TODO: check if the allow_unicode flag should be true or false in g_uri_escape_string

		_R<ASObject> val=getValueAt(i);
		if(val->getObjectType()==T_ARRAY)
		{
			//Print using multiple properties
			//Ex. ["foo","bar"] -> prop1=foo&prop1=bar
			Array* arr=Class<Array>::cast(val.getPtr());
			for(uint32_t j=0;j<arr->size();j++)
			{
				//Escape the name
				char* escapedName=g_uri_escape_string(name.raw_buf(),NULL, false);
				tmp+=escapedName;
				g_free(escapedName);
				tmp+="=";

				//Escape the value
				const tiny_string& value=arr->at(j)->toString();
				char* escapedValue=g_uri_escape_string(value.raw_buf(),NULL, false);
				tmp+=escapedValue;
				g_free(escapedValue);

				if(j!=arr->size()-1)
					tmp+="&";
			}
		}
		else
		{
			//Escape the name
			char* escapedName=g_uri_escape_string(name.raw_buf(),NULL, false);
			tmp+=escapedName;
			g_free(escapedName);
			tmp+="=";

			//Escape the value
			const tiny_string& value=val->toString();
			char* escapedValue=g_uri_escape_string(value.raw_buf(),NULL, false);
			tmp+=escapedValue;
			g_free(escapedValue);
		}
		if(i!=size-1)
			tmp+="&";
	}
	return tmp;
}

tiny_string URLVariables::toString()
{
	assert_and_throw(implEnable);
	return toString_priv();
}

ASFUNCTIONBODY(lightspark,sendToURL)
{
	assert_and_throw(argslen == 1);
	ASObject* arg=args[0];
	URLRequest* urlRequest=Class<URLRequest>::dyncast(arg);
	assert_and_throw(urlRequest);

	URLInfo url=urlRequest->getRequestURL();

	if(!url.isValid())
		return NULL;

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::EVALUATIONRESULT evaluationResult =
		getSys()->securityManager->evaluateURLStatic(url, ~(SecurityManager::LOCAL_WITH_FILE),
			SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
			true);
	//Network sandboxes can't access local files (this should be a SecurityErrorEvent)
	if(evaluationResult == SecurityManager::NA_REMOTE_SANDBOX)
		throw Class<SecurityError>::getInstanceS("SecurityError: sendToURL: "
				"connect to network");
	//Local-with-filesystem sandbox can't access network
	else if(evaluationResult == SecurityManager::NA_LOCAL_SANDBOX)
		throw Class<SecurityError>::getInstanceS("SecurityError: sendToURL: "
				"connect to local file");
	else if(evaluationResult == SecurityManager::NA_PORT)
		throw Class<SecurityError>::getInstanceS("SecurityError: sendToURL: "
				"connect to restricted port");
	else if(evaluationResult == SecurityManager::NA_RESTRICT_LOCAL_DIRECTORY)
		throw Class<SecurityError>::getInstanceS("SecurityError: sendToURL: "
				"not allowed to navigate up for local files");

	//Also check cross domain policies. TODO: this should be async as it could block if invoked from ExternalInterface
	evaluationResult = getSys()->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
	{
		//TODO: find correct way of handling this case (SecurityErrorEvent in this case)
		throw Class<SecurityError>::getInstanceS("SecurityError: sendToURL: "
				"connection to domain not allowed by securityManager");
	}

	//TODO: should we disallow accessing local files in a directory above 
	//the current one like we do with NetStream.play?

	vector<uint8_t> postData;
	urlRequest->getPostData(postData);
	assert_and_throw(postData.empty());

	//Don't cache our downloaded files
	Downloader* downloader=getSys()->downloadManager->download(url, false, NULL);
	//TODO: make the download asynchronous instead of waiting for an unused response
	downloader->waitForTermination();
	getSys()->downloadManager->destroy(downloader);
	return NULL;
}

void Responder::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("onResult","",Class<IFunction>::getFunction(onResult),NORMAL_METHOD,true);
}

void Responder::finalize()
{
	ASObject::finalize();
	result.reset();
	status.reset();
}

ASFUNCTIONBODY(Responder, _constructor)
{
	Responder* th=Class<Responder>::cast(obj);
	assert_and_throw(argslen==1 || argslen==2);
	assert_and_throw(args[0]->getObjectType()==T_FUNCTION);
	args[0]->incRef();
	th->result = _MR(static_cast<IFunction*>(args[0]));
	if(argslen==2 && args[1]->getObjectType()==T_FUNCTION)
	{
		args[1]->incRef();
		th->status = _MR(static_cast<IFunction*>(args[1]));
	}
	return NULL;
}

ASFUNCTIONBODY(Responder, onResult)
{
	Responder* th=Class<Responder>::cast(obj);
	assert_and_throw(argslen==1);
	args[0]->incRef();
	ASObject* ret=th->result->call(getSys()->getNullRef(), args, argslen);
	ret->decRef();
	return NULL;
}

ASFUNCTIONBODY(lightspark,registerClassAlias)
{
	assert_and_throw(argslen==2 && args[0]->getObjectType()==T_STRING && args[1]->getObjectType()==T_CLASS);
	const tiny_string& arg0 = args[0]->toString();
	args[1]->incRef();
	_R<Class_base> c=_MR(static_cast<Class_base*>(args[1]));
	getSys()->aliasMap.insert(make_pair(arg0, c));
	return NULL;
}

ASFUNCTIONBODY(lightspark,getClassByAlias)
{
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_STRING);
	const tiny_string& arg0 = args[0]->toString();
	auto it=getSys()->aliasMap.find(arg0);
	if(it==getSys()->aliasMap.end())
		throw Class<ReferenceError>::getInstanceS("Alias not set");

	it->second->incRef();
	return it->second.getPtr();
}
