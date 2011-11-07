/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "abc.h"
#include "flashnet.h"
#include "class.h"
#include "flash/system/flashsystem.h"
#include "compat.h"
#include "backends/audio.h"
#include "backends/builtindecoder.h"
#include "backends/rendering.h"
#include "backends/security.h"
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.net");

REGISTER_CLASS_NAME(URLLoader);
REGISTER_CLASS_NAME(URLLoaderDataFormat);
REGISTER_CLASS_NAME(URLRequest);
REGISTER_CLASS_NAME(URLRequestMethod);
REGISTER_CLASS_NAME(URLVariables);
REGISTER_CLASS_NAME(SharedObject);
REGISTER_CLASS_NAME(ObjectEncoding);
REGISTER_CLASS_NAME(NetConnection);
REGISTER_CLASS_NAME(NetStream);

URLRequest::URLRequest():method(GET)
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
}

void URLRequest::buildTraits(ASObject* o)
{
}

URLInfo URLRequest::getRequestURL() const
{
	URLInfo ret=sys->getOrigin().goToURL(url);
	if(method!=GET)
		return ret;

	if(data.isNull())
		return ret;

	if(data->getClass()==Class<ByteArray>::getClass())
		throw RunTimeException("ByteArray data not supported in URLRequest");
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

void URLRequest::getPostData(vector<uint8_t>& outData) const
{
	if(method!=POST)
		return;

	if(data.isNull())
		return;

	if(data->getClass()==Class<ByteArray>::getClass())
		throw RunTimeException("ByteArray not support in URLRequest");
	else if(data->getClass()==Class<URLVariables>::getClass())
	{
		//Prepend the Content-Type header
		tiny_string strData="Content-type: application/x-www-form-urlencoded\r\nContent-length: ";
		const tiny_string& tmpStr=data->toString();
		char buf[20];
		snprintf(buf,20,"%u\r\n\r\n",tmpStr.numBytes());
		strData+=buf;
		strData+=tmpStr;
		outData.insert(outData.end(),strData.raw_buf(),strData.raw_buf()+strData.numBytes());
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
		return new Undefined;

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

void URLRequestMethod::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("GET","",Class<ASString>::getInstanceS("GET"),DECLARED_TRAIT);
	c->setVariableByQName("POST","",Class<ASString>::getInstanceS("POST"),DECLARED_TRAIT);
}

URLLoader::URLLoader():dataFormat("text"),data(NULL),downloader(NULL)
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
	c->setDeclaredMethodByQName("dataFormat","",Class<IFunction>::getFunction(_setDataFormat),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
}

void URLLoader::buildTraits(ASObject* o)
{
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

	assert_and_throw(th->downloader==NULL);
	th->url=urlRequest->getRequestURL();

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		sys->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::EVALUATIONRESULT evaluationResult = 
		sys->securityManager->evaluateURLStatic(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
			SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);
	//Network sandboxes can't access local files (this should be a SecurityErrorEvent)
	if(evaluationResult == SecurityManager::NA_REMOTE_SANDBOX)
		throw Class<SecurityError>::getInstanceS("SecurityError: URLLoader::load: "
				"connect to network");
	//Local-with-filesystem sandbox can't access network
	else if(evaluationResult == SecurityManager::NA_LOCAL_SANDBOX)
		throw Class<SecurityError>::getInstanceS("SecurityError: URLLoader::load: "
				"connect to local file");
	else if(evaluationResult == SecurityManager::NA_PORT)
		throw Class<SecurityError>::getInstanceS("SecurityError: URLLoader::load: "
				"connect to restricted port");
	else if(evaluationResult == SecurityManager::NA_RESTRICT_LOCAL_DIRECTORY)
		throw Class<SecurityError>::getInstanceS("SecurityError: URLLoader::load: "
				"not allowed to navigate up for local files");

	//TODO: should we disallow accessing local files in a directory above 
	//the current one like we do with NetStream.play?

	urlRequest->getPostData(th->postData);

	//To be decreffed in jobFence
	th->incRef();
	sys->addJob(th);
	return NULL;
}

void URLLoader::jobFence()
{
	decRef();
}

void URLLoader::execute()
{
	//TODO: support httpStatus, progress, open events

	//Check for URL policies and send SecurityErrorEvent if needed
	SecurityManager::EVALUATIONRESULT evaluationResult = sys->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
	{
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<SecurityErrorEvent>::getInstanceS("SecurityError: URLLoader::load: "
					"connection to domain not allowed by securityManager")));
		return;
	}

	{
		SpinlockLocker l(downloaderLock);
		//All the checks passed, create the downloader
		if(postData.empty())
		{
			//This is a GET request
			//Don't cache our downloaded files
			downloader=sys->downloadManager->download(url, false, NULL);
		}
		else
		{
			downloader=sys->downloadManager->downloadWithData(url, postData, NULL);
			//Clean up the postData for the next load
			postData.clear();
		}
	}

	if(!downloader->hasFailed())
	{
		downloader->waitForTermination();
		//HACK: the downloader may have been cleared in the mean time
		assert(downloader);
		if(!downloader->hasFailed())
		{
			istream s(downloader);
			uint8_t* buf=new uint8_t[downloader->getLength()];
			//TODO: avoid this useless copy
			s.read((char*)buf,downloader->getLength());
			//TODO: test binary data format
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
			//Send a complete event for this object
			this->incRef();
			sys->currentVm->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
		}
		else
		{
			//Notify an error during loading
			this->incRef();
			sys->currentVm->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
		}
	}
	else
	{
		//Notify an error during loading
		this->incRef();
		sys->currentVm->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
	}

	{
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		sys->downloadManager->destroy(downloader);
		downloader = NULL;
	}
}

void URLLoader::threadAbort()
{
	SpinlockLocker l(downloaderLock);
	if(downloader != NULL)
		downloader->stop();
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	return Class<ASString>::getInstanceS(th->dataFormat);
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	if(th->data.isNull())
		return new Undefined;
	
	th->data->incRef();
	return th->data.getPtr();
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	assert_and_throw(args[0]);
	th->dataFormat=args[0]->toString();
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS("variables"),DECLARED_TRAIT);
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS("text"),DECLARED_TRAIT);
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS("binary"),DECLARED_TRAIT);
}

void SharedObject::sinit(Class_base* c)
{
	c->setSuper(Class<EventDispatcher>::getRef());
};

void ObjectEncoding::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("AMF0","",abstract_i(AMF0),DECLARED_TRAIT);
	c->setVariableByQName("AMF3","",abstract_i(AMF3),DECLARED_TRAIT);
	c->setVariableByQName("DEFAULT","",abstract_i(DEFAULT),DECLARED_TRAIT);
};

NetConnection::NetConnection():_connected(false)
{
}

void NetConnection::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(_getConnected),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_setDefaultObjectEncoding),SETTER_METHOD,false);
	sys->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::DEFAULT;
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("protocol","",Class<IFunction>::getFunction(_getProtocol),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
}

void NetConnection::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(NetConnection, _constructor)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	th->objectEncoding = sys->staticNetConnectionDefaultObjectEncoding;
	return NULL;
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
	&& sys->securityManager->evaluateSandbox(SecurityManager::LOCAL_WITH_FILE))
		throw Class<SecurityError>::getInstanceS("SecurityError: NetConnection::connect "
				"from LOCAL_WITH_FILE sandbox");
	//Null argument means local file or web server, the spec only mentions NULL, but youtube uses UNDEFINED, so supporting that too.
	if(args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED)
	{
		th->_connected = false;
	}
	//String argument means Flash Remoting/Flash Media Server
	else
	{
		th->_connected = false;
		ASString* command = static_cast<ASString*>(args[0]);
		th->uri = URLInfo(command->toString());

		if(sys->securityManager->evaluatePoliciesURL(th->uri, true) != SecurityManager::ALLOWED)
		{
			//TODO: find correct way of handling this case
			throw Class<SecurityError>::getInstanceS("SecurityError: connection to domain not allowed by securityManager");
		}
		
		if(!(th->uri.getProtocol() == "rtmp" ||
		     th->uri.getProtocol() == "rtmpe" ||
		     th->uri.getProtocol() == "rtmps"))
		{
			throw UnsupportedException("NetConnection::connect: only RTMP is supported");
		}

		// We actually create the connection later in
		// NetStream::play(). For now, we support only
		// streaming, not remoting (NetConnection.call() is
		// not implemented).
	}

	//When the URI is undefined the connect is successful (tested on Adobe player)
	th->incRef();
	getVm()->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS("status", "NetConnection.Connect.Success")));
	return NULL;
}

ASFUNCTIONBODY(NetConnection,_getConnected)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	return abstract_b(th->_connected);
}

ASFUNCTIONBODY(NetConnection,_getDefaultObjectEncoding)
{
	return abstract_i(sys->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY(NetConnection,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = args[0]->toInt();
	if(value == 0)
		sys->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
		sys->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF3;
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

NetStream::NetStream():frameRate(0),tickStarted(false),connection(NULL),downloader(NULL),
	videoDecoder(NULL),audioDecoder(NULL),audioStream(NULL),streamTime(0),paused(false),
	closed(true),client(NullRef),checkPolicyFile(false),rawAccessAllowed(false),
	oldVolume(-1.0)
{
	sem_init(&mutex,0,1);
}

void NetStream::finalize()
{
	EventDispatcher::finalize();
	connection.reset();
	client.reset();
}

NetStream::~NetStream()
{
	assert(!executing);
	if(tickStarted)
		sys->removeJob(this);
	delete videoDecoder; 
	delete audioDecoder; 
	sem_destroy(&mutex);
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
		return new Undefined();

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
		th->url = sys->getOrigin().goToURL(args[0]->toString());

		SecurityManager::EVALUATIONRESULT evaluationResult = 
			sys->securityManager->evaluateURLStatic(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
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
		sys->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
	}
	else //The URL is valid so we can start the download and add ourself as a job
	{
		//Cache our downloaded files
		th->downloader=sys->downloadManager->download(th->url, true, NULL);
		th->streamTime=0;
		//To be decreffed in jobFence
		th->incRef();
		sys->addJob(th);
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
		sem_wait(&th->mutex);
		if(th->audioStream)
			sys->audioManager->resumeStreamPlugin(th->audioStream);
		sem_post(&th->mutex);
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
		sem_wait(&th->mutex);
		if(th->audioStream)
			sys->audioManager->pauseStreamPlugin(th->audioStream);
		sem_post(&th->mutex);
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
	if(audioStream && sys->audioManager->isTimingAvailablePlugin())
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
	sys->getRenderThread()->addUploadJob(videoDecoder);
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
	sem_wait(&mutex);
	bool ret=isReady();
	if(!ret) //If the data is not valid so not release the lock to keep the condition
		sem_post(&mutex);
	return ret;
}

void NetStream::unlock()
{
	sem_post(&mutex);
}

void NetStream::execute()
{
	//checkPolicyFile only applies to per-pixel access, loading and playing is always allowed.
	//So there is no need to disallow playing if policy files disallow it.
	//We do need to check if per-pixel access is allowed.
	SecurityManager::EVALUATIONRESULT evaluationResult = sys->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		rawAccessAllowed = true;

	if(downloader->hasFailed())
	{
		this->incRef();
		sys->currentVm->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
		sys->downloadManager->destroy(downloader);
		return;
	}

	//The downloader hasn't failed yet at this point

	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

	ThreadProfile* profile=sys->allocateProfiler(RGB(0,0,200));
	profile->setTag("NetStream");
	bool waitForFlush=true;
	StreamDecoder* streamDecoder=NULL;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
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

			if(audioStream==NULL && audioDecoder && audioDecoder->isValid() && sys->audioManager->pluginLoaded())
				audioStream=sys->audioManager->createStreamPlugin(audioDecoder);

			if(audioStream && audioStream->paused() && !paused)
			{
				//The audio stream is paused but should not!
				//As we have new data fill the stream
				audioStream->fill();
			}

			if(!tickStarted && isReady())
			{
				multiname onMetaDataName;
				onMetaDataName.name_type=multiname::NAME_STRING;
				onMetaDataName.name_s="onMetaData";
				onMetaDataName.ns.push_back(nsNameAndKind("",NAMESPACE));
				_NR<ASObject> callback = client->getVariableByMultiname(onMetaDataName);
				if(!callback.isNull() && callback->getObjectType() == T_FUNCTION)
				{
					ASObject* callbackArgs[1];
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
					client->incRef();
					metadata->incRef();
					callbackArgs[0] = metadata;
					callback->incRef();
					_R<FunctionEvent> event(new FunctionEvent(_MR(static_cast<IFunction*>(callback.getPtr())),
							_MR(client), callbackArgs, 1));
					getVm()->addEvent(NullRef,event);
				}

				tickStarted=true;
				if(frameRate==0)
				{
					assert(videoDecoder->frameRate);
					frameRate=videoDecoder->frameRate;
				}
				sys->addTick(1000/frameRate,this);
				//Also ask for a render rate equal to the video one (capped at 24)
				float localRenderRate=dmin(frameRate,24);
				sys->setRenderRate(localRenderRate);
			}
			profile->accountTime(chronometer.checkpoint());
			if(aborting)
				throw JobTerminationException();
		}

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
	}
	//Before deleting stops ticking, removeJobs also spin waits for termination
	sys->removeJob(this);
	tickStarted=false;
	sem_wait(&mutex);
	//Change the state to invalid to avoid locking
	videoDecoder=NULL;
	audioDecoder=NULL;
	//Clean up everything for a possible re-run
	sys->downloadManager->destroy(downloader);
	//This transition is critical, so the mutex is needed
	downloader=NULL;
	if(audioStream)
		sys->audioManager->freeStreamPlugin(audioStream);
	audioStream=NULL;
	sem_post(&mutex);
	delete streamDecoder;
}

void NetStream::threadAbort()
{
	//This will stop the rendering loop
	closed = true;

	sem_wait(&mutex);
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
	sem_post(&mutex);
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
			//Check if the variable already exists
			multiname propName;
			propName.name_type=multiname::NAME_STRING;
			propName.name_s=name;
			propName.ns.push_back(nsNameAndKind("",NAMESPACE));
			_NR<ASObject> curValue=getVariableByMultiname(propName);
			if(!curValue.isNull())
			{
				//If the variable already exists we have to create an Array of values
				Array* arr=NULL;
				if(curValue->getObjectType()!=T_ARRAY)
				{
					arr=Class<Array>::getInstanceS();
					curValue->incRef();
					arr->push(curValue.getPtr());
					setVariableByMultiname(propName,arr);
				}
				else
					arr=Class<Array>::cast(curValue.getPtr());

				arr->push(Class<ASString>::getInstanceS(value));
			}
			else
				setVariableByMultiname(propName,Class<ASString>::getInstanceS(value));

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

URLVariables::URLVariables(const tiny_string& s)
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

		ASObject* val=getValueAt(i);
		if(val->getObjectType()==T_ARRAY)
		{
			//Print using multiple properties
			//Ex. ["foo","bar"] -> prop1=foo&prop1=bar
			Array* arr=Class<Array>::cast(val);
			for(int32_t j=0;j<arr->size();j++)
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
		sys->securityManager->evaluateURLStatic(url, ~(SecurityManager::LOCAL_WITH_FILE),
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
	evaluationResult = sys->securityManager->evaluatePoliciesURL(url, true);
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
	Downloader* downloader=sys->downloadManager->download(url, false, NULL);
	//TODO: make the download asynchronous instead of waiting for an unused response
	downloader->waitForTermination();
	sys->downloadManager->destroy(downloader);
	return NULL;
}
