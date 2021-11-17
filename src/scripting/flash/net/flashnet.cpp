/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <map>
#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/class.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/media/flashmedia.h"
#include "compat.h"
#include "backends/audio.h"
#include "backends/builtindecoder.h"
#include "backends/rendering.h"
#include "backends/streamcache.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

URLRequest::URLRequest(Class_base* c, const tiny_string u, const tiny_string m, _NR<ASObject> d):ASObject(c),method(m=="POST" ? POST : GET),url(u),data(d),contentType("application/x-www-form-urlencoded"),
	requestHeaders(Class<Array>::getInstanceSNoArgs(c->getSystemState()))
{
}

void URLRequest::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(c->getSystemState(),_setURL),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(c->getSystemState(),_getURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("method","",Class<IFunction>::getFunction(c->getSystemState(),_setMethod),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("method","",Class<IFunction>::getFunction(c->getSystemState(),_getMethod,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(c->getSystemState(),_setData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(c->getSystemState(),_getData,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("digest","",Class<IFunction>::getFunction(c->getSystemState(),_setDigest),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("digest","",Class<IFunction>::getFunction(c->getSystemState(),_getDigest,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,contentType,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,requestHeaders,Array);
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

	if(data->getClass()==Class<ByteArray>::getClass(data->getSystemState()))
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
		throw Class<ArgumentError>::getInstanceS(getSystemState(),tiny_string("The HTTP request header ") + contentType + tiny_string(" cannot be set via ActionScript."), 2096);
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
		 "age", "allow", "allowed", "charge-to",
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

	if ((headerName.strchr('\r') != nullptr) ||
	     headerName.strchr('\n') != nullptr)
		throw Class<ArgumentError>::getInstanceS(getSystemState(),"The HTTP request header cannot be set via ActionScript", 2096);

	for (unsigned i=0; i<(sizeof illegalHeaders)/(sizeof illegalHeaders[0]); i++)
	{
		if (headerName.lowercase() == illegalHeaders[i])
		{
			tiny_string msg("The HTTP request header ");
			msg += headerName;
			msg += " cannot be set via ActionScript";
			throw Class<ArgumentError>::getInstanceS(getSystemState(),msg, 2096);
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
		asAtom headerObject = requestHeaders->at(i);

		// Validate
		if (!asAtomHandler::is<URLRequestHeader>(headerObject))
			throwError<TypeError>(kCheckTypeFailedError, asAtomHandler::toObject(headerObject,getSystemState())->getClassName(), "URLRequestHeader");
		URLRequestHeader *header = asAtomHandler::as<URLRequestHeader>(headerObject);
		tiny_string headerName = header->name;
		validateHeaderName(headerName);
		if ((header->value.strchr('\r') != nullptr) ||
		     header->value.strchr('\n') != nullptr)
			throw Class<ArgumentError>::getInstanceS(getSystemState(),"Illegal HTTP header value");
		
		// Should this include the separators?
		headerTotalLen += header->name.numBytes();
		headerTotalLen += header->value.numBytes();
		if (headerTotalLen >= 8192)
			throw Class<ArgumentError>::getInstanceS(getSystemState(),"Cumulative length of requestHeaders must be less than 8192 characters.", 2145);

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

	if(!data.isNull() && data->getClass()==Class<URLVariables>::getClass(data->getSystemState()))
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

	if(data->getClass()==Class<ByteArray>::getClass(data->getSystemState()))
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

ASFUNCTIONBODY_ATOM(URLRequest,_constructor)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	ARG_UNPACK_ATOM(th->url, "");
}

ASFUNCTIONBODY_ATOM(URLRequest,_setURL)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	ARG_UNPACK_ATOM(th->url);
}

ASFUNCTIONBODY_ATOM(URLRequest,_getURL)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->url));
}

ASFUNCTIONBODY_ATOM(URLRequest,_setMethod)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],sys);
	if(tmp=="GET")
		th->method=GET;
	else if(tmp=="POST")
		th->method=POST;
	else
		throw UnsupportedException("Unsupported method in URLLoader");
}

ASFUNCTIONBODY_ATOM(URLRequest,_getMethod)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	switch(th->method)
	{
		case GET:
			ret = asAtomHandler::fromString(sys,"GET");
			break;
		case POST:
			ret = asAtomHandler::fromString(sys,"POST");
			break;
	}
}

ASFUNCTIONBODY_ATOM(URLRequest,_getData)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	if(th->data.isNull())
		asAtomHandler::setUndefined(ret);
	else
	{
		th->data->incRef();
		ret = asAtomHandler::fromObject(th->data.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(URLRequest,_setData)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	assert_and_throw(argslen==1);

	ASATOM_INCREF(args[0]);
	th->data=_MR(asAtomHandler::toObject(args[0],sys));
}

ASFUNCTIONBODY_ATOM(URLRequest,_getDigest)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	if (th->digest.empty())
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,th->digest));
}

ASFUNCTIONBODY_ATOM(URLRequest,_setDigest)
{
	URLRequest* th=asAtomHandler::as<URLRequest>(obj);
	tiny_string value;
	ARG_UNPACK_ATOM(value);

	int numHexChars = 0;
	bool validChars = true;
	for (CharIterator it=value.begin(); it!=value.end(); ++it)
	{
		if (((*it >= 'A') && (*it <= 'F')) ||
		    ((*it >= 'a') && (*it <= 'f')) ||
		    ((*it >= '0') && (*it <= '9')))
		{
			numHexChars++;
		}
		else
		{
			validChars = false;
			break;
		}
	}

	if (!validChars || numHexChars != 64)
		throw Class<ArgumentError>::getInstanceS(sys,"An invalid digest was supplied", 2034);

	th->digest = value;
}

ASFUNCTIONBODY_GETTER_SETTER(URLRequest,contentType);
ASFUNCTIONBODY_GETTER_SETTER(URLRequest,requestHeaders);

void URLRequestMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("GET",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"GET"),DECLARED_TRAIT);
	c->setVariableAtomByQName("POST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"POST"),DECLARED_TRAIT);
}

URLLoaderThread::URLLoaderThread(_R<URLRequest> request, _R<URLLoader> ldr)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr)
{
}

void URLLoaderThread::execute()
{
	assert(!downloader);

	//TODO: support httpStatus, progress events

	_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache(loader->getSystemState())));
	if(!createDownloader(cache, loader, loader.getPtr()))
		return;

	_NR<ASObject> data;
	bool success=false;
	if(!downloader->hasFailed())
	{
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"open")));

		cache->waitForTermination();
		if(!downloader->hasFailed() && !threadAborting)
		{
			std::streambuf *sbuf = cache->createReader();
			istream s(sbuf);
			uint8_t* buf=new uint8_t[downloader->getLength()+1];
			//TODO: avoid this useless copy
			s.read((char*)buf,downloader->getLength());
			buf[downloader->getLength()] = '\0';
			//TODO: test binary data format
			tiny_string dataFormat=loader->getDataFormat();
			if(dataFormat=="binary")
			{
				_R<ByteArray> byteArray=_MR(Class<ByteArray>::getInstanceS(loader->getSystemState()));
				byteArray->acquireBuffer(buf,downloader->getLength());
				data=byteArray;
				//The buffers must not be deleted, it's now handled by the ByteArray instance
			}
			else if(dataFormat=="text")
			{
				// don't use abstract_s here, because we are not in the main thread
				data=_MR(Class<ASString>::getInstanceS(loader->getSystemState(),(char*)buf,downloader->getLength()));
				delete[] buf;
			}
			else if(dataFormat=="variables")
			{
				data=_MR(Class<URLVariables>::getInstanceS(loader->getSystemState(),(char*)buf));
				delete[] buf;
			}
			else
			{
				assert(false && "invalid dataFormat");
			}

			delete sbuf;
			success=true;
		}
	}

	// Don't send any events if the thread is aborting
	if(success && !threadAborting)
	{
		//Send a complete event for this object
		loader->setData(data);

		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<ProgressEvent>::getInstanceS(loader->getSystemState(),downloader->getLength(),downloader->getLength())));
		//Send a complete event for this object
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"complete")));
	}
	else if(!success && !threadAborting)
	{
		//Notify an error during loading
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
	}

	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
}

URLLoader::URLLoader(Class_base* c):EventDispatcher(c),dataFormat("text"),data(),job(NULL),timestamp_last_progress(0)
{
}

void URLLoader::finalize()
{
	EventDispatcher::finalize();
	data.reset();
}

void URLLoader::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("dataFormat","",Class<IFunction>::getFunction(c->getSystemState(),_getDataFormat,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(c->getSystemState(),_getData,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("data","",Class<IFunction>::getFunction(c->getSystemState(),_setData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("dataFormat","",Class<IFunction>::getFunction(c->getSystemState(),_setDataFormat),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bytesLoaded,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bytesTotal,UInteger);
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
	Locker l(spinlock);
	if(finishedJob==job)
		job=NULL;

	delete finishedJob;
}

void URLLoader::setData(_NR<ASObject> newData)
{
	Locker l(spinlock);
	data=newData;
}

void URLLoader::setBytesTotal(uint32_t b)
{
	bytesTotal = b;
}

void URLLoader::setBytesLoaded(uint32_t b)
{
	bytesLoaded = b;
	uint64_t cur=compat_get_thread_cputime_us();
	if (cur > timestamp_last_progress+ 40*1000)
	{
		timestamp_last_progress = cur;
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(getSystemState(),b,bytesTotal)));
	}
}

ASFUNCTIONBODY_ATOM(URLLoader,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj,NULL,0);
	if(argslen==1 && asAtomHandler::is<URLRequest>(args[0]))
	{
		//URLRequest* urlRequest=Class<URLRequest>::dyncast(args[0]);
		load(ret,sys, obj, args, argslen);
	}
}

ASFUNCTIONBODY_ATOM(URLLoader,load)
{
	URLLoader* th=asAtomHandler::as<URLLoader>(obj);
	ASObject* arg=asAtomHandler::getObject(args[0]);
	URLRequest* urlRequest=Class<URLRequest>::dyncast(arg);
	assert_and_throw(urlRequest);

	{
		Locker l(th->spinlock);
		if(th->job)
			th->job->threadAbort();
	}

	URLInfo url=urlRequest->getRequestURL();
	if(!url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		th->getSystemState()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(th->getSystemState())));
		return;	
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
}

ASFUNCTIONBODY_ATOM(URLLoader,close)
{
	URLLoader* th=asAtomHandler::as<URLLoader>(obj);
	Locker l(th->spinlock);
	if(th->job)
		th->job->threadAbort();
}

tiny_string URLLoader::getDataFormat()
{
	Locker l(spinlock);
	return dataFormat;
}

void URLLoader::setDataFormat(const tiny_string& newFormat)
{
	Locker l(spinlock);
	dataFormat=newFormat.lowercase();
}

ASFUNCTIONBODY_ATOM(URLLoader,_getDataFormat)
{
	URLLoader* th=asAtomHandler::as<URLLoader>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->getDataFormat()));
}

ASFUNCTIONBODY_ATOM(URLLoader,_getData)
{
	URLLoader* th=asAtomHandler::as<URLLoader>(obj);
	Locker l(th->spinlock);
	if(th->data.isNull())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	
	th->data->incRef();
	ret = asAtomHandler::fromObject(th->data.getPtr());
}

ASFUNCTIONBODY_ATOM(URLLoader,_setData)
{
	if(!asAtomHandler::is<URLLoader>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	URLLoader* th = asAtomHandler::as<URLLoader>(obj);
	if(argslen != 1)
		throw Class<ArgumentError>::getInstanceS(sys,"Wrong number of arguments in setter");
	ASATOM_INCREF(args[0]);
	th->setData(_MR(asAtomHandler::toObject(args[0],sys)));
}

ASFUNCTIONBODY_ATOM(URLLoader,_setDataFormat)
{
	URLLoader* th=asAtomHandler::as<URLLoader>(obj);
	assert_and_throw(argslen);
	th->setDataFormat(asAtomHandler::toString(args[0],sys));
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("VARIABLES",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"variables"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TEXT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"text"),DECLARED_TRAIT);
	c->setVariableAtomByQName("BINARY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"binary"),DECLARED_TRAIT);
}

void SharedObjectFlushStatus::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL);
	c->setVariableAtomByQName("FLUSHED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"flushed"),DECLARED_TRAIT);
	c->setVariableAtomByQName("PENDING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pending"),DECLARED_TRAIT);
}

SharedObject::SharedObject(Class_base* c):EventDispatcher(c),client(this),objectEncoding(ObjectEncoding::AMF3)
{
	subtype=SUBTYPE_SHAREDOBJECT;
	data=_MR(new_asobject(c->getSystemState()));
}

bool SharedObject::destruct()
{
	if (client.getPtr()==this)
		client=data;// this is just to set client to "something else" to avoid that this SharedObject has a pointer to itself during destruction
	return EventDispatcher::destruct();
}

void SharedObject::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED);
	c->setDeclaredMethodByQName("getLocal","",Class<IFunction>::getFunction(c->getSystemState(),getLocal),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getRemote","",Class<IFunction>::getFunction(c->getSystemState(),getRemote),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("flush","",Class<IFunction>::getFunction(c->getSystemState(),flush),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProperty","",Class<IFunction>::getFunction(c->getSystemState(),setProperty),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,client);
	REGISTER_GETTER_RESULTTYPE(c,data,ASObject);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setDefaultObjectEncoding),SETTER_METHOD,false);
	REGISTER_SETTER(c,fps);
	REGISTER_GETTER_SETTER(c,objectEncoding);
	c->setDeclaredMethodByQName("preventBackup","",Class<IFunction>::getFunction(c->getSystemState(),_getPreventBackup),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("preventBackup","",Class<IFunction>::getFunction(c->getSystemState(),_setPreventBackup),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_getSize),GETTER_METHOD,true);

	getSys()->staticSharedObjectDefaultObjectEncoding = ObjectEncoding::AMF3;
	getSys()->staticSharedObjectPreventBackup = false;
}

ASFUNCTIONBODY_GETTER_SETTER(SharedObject,client);
ASFUNCTIONBODY_GETTER(SharedObject,data);
ASFUNCTIONBODY_SETTER(SharedObject,fps);
ASFUNCTIONBODY_GETTER_SETTER(SharedObject,objectEncoding);

ASFUNCTIONBODY_ATOM(SharedObject,_getDefaultObjectEncoding)
{
	ret = asAtomHandler::fromUInt(sys->staticSharedObjectDefaultObjectEncoding);
}

ASFUNCTIONBODY_ATOM(SharedObject,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	uint32_t value = asAtomHandler::toUInt(args[0]);
	if(value == 0)
	    sys->staticSharedObjectDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
	    sys->staticSharedObjectDefaultObjectEncoding = ObjectEncoding::AMF3;
	else
	    throw RunTimeException("Invalid shared object encoding");
}

ASFUNCTIONBODY_ATOM(SharedObject,getLocal)
{
	tiny_string name;
	tiny_string localPath;
	bool secure;
	if (!sys->mainClip->usesActionScript3)
	{
		// contrary to spec, Adobe allows getLocal() calls with 0 arguments on AVM1
		ARG_UNPACK_ATOM(name,"") (localPath,"") (secure,false);
	}
	else
	{
		ARG_UNPACK_ATOM(name) (localPath,"") (secure,false);
		if (name=="")
			throwError<ASError>(0,"invalid name");
	}
	if (secure)
		LOG(LOG_NOT_IMPLEMENTED,"SharedObject.getLocal: parameter 'secure' is ignored");

	tiny_string fullname = localPath + "|";
	fullname += name;
	SharedObject* res = nullptr;
	auto it = sys->sharedobjectmap.find(fullname);
	if (it == sys->sharedobjectmap.end())
	{
		res = Class<SharedObject>::getInstanceS(sys);
		res->name=localPath;
		if (sys->localStorageAllowed())
		{
			ByteArray* b = Class<ByteArray>::getInstanceS(sys);
			asAtom a = asAtomHandler::invalidAtom;
			if (sys->getEngineData()->fillSharedObject(localPath,b))
			{
				b->setPosition(0);
				a = b->readObject();
			}
			ASObject* d = nullptr;
			if (asAtomHandler::isObject(a))
				d = asAtomHandler::getObjectNoCheck(a);
			else
				d = Class<ASObject>::getInstanceS(sys);
			res->data = _MR(d);
			b->decRef();
		}
		sys->sharedobjectmap.insert(make_pair(fullname,_MR(res)));
		it = sys->sharedobjectmap.find(fullname);
	}
	res = it->second.getPtr();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(SharedObject,getRemote)
{
	LOG(LOG_NOT_IMPLEMENTED,"SharedObject.getRemote not implemented");
	asAtomHandler::setUndefined(ret);
}
bool SharedObject::doFlush()
{
	if (!data.isNull() && data->numVariables() && getSystemState()->localStorageAllowed())
	{
		ByteArray* b = Class<ByteArray>::getInstanceS(getSystemState());
		b->writeObject(data.getPtr());
		b->setPosition(0);
		bool ret = getSystemState()->getEngineData()->flushSharedObject(name,b);
		b->decRef();
		return ret;
	}
	return true;
}
ASFUNCTIONBODY_ATOM(SharedObject,flush)
{
	SharedObject* th=asAtomHandler::as<SharedObject>(obj);
	int minDiskSpace=0;
	ARG_UNPACK_ATOM(minDiskSpace,0);
	if (minDiskSpace != 0)
		LOG(LOG_NOT_IMPLEMENTED,"SharedOBject.flush: parameter minDiskSpace is ignored");
	if (!th->doFlush())
		throwError<ASError>(0,"flushing SharedObject failed");
	if (!sys->mainClip->usesActionScript3)
		ret = asAtomHandler::trueAtom;
	else
		ret = asAtomHandler::fromString(sys,"flushed");
}

ASFUNCTIONBODY_ATOM(SharedObject,clear)
{
	SharedObject* th=asAtomHandler::as<SharedObject>(obj);
	th->data->destroyContents();
	sys->getEngineData()->removeSharedObject(th->name);
}

ASFUNCTIONBODY_ATOM(SharedObject,close)
{
	SharedObject* th=asAtomHandler::as<SharedObject>(obj);
	th->doFlush();
}

ASFUNCTIONBODY_ATOM(SharedObject,connect)
{
	LOG(LOG_NOT_IMPLEMENTED, "SharedObject.connect not implemented");
}
ASFUNCTIONBODY_ATOM(SharedObject,setProperty)
{
	LOG(LOG_NOT_IMPLEMENTED, "SharedObject.setProperty not implemented");
}

ASFUNCTIONBODY_ATOM(SharedObject,_getPreventBackup)
{
	ret = asAtomHandler::fromUInt(sys->staticSharedObjectPreventBackup);
}

ASFUNCTIONBODY_ATOM(SharedObject,_setPreventBackup)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(asAtomHandler::isBool(args[0]));
	bool value = asAtomHandler::Boolean_concrete(args[0]);
	sys->staticSharedObjectPreventBackup = value;
}

ASFUNCTIONBODY_ATOM(SharedObject,_getSize)
{
	SharedObject* th=asAtomHandler::as<SharedObject>(obj);
	if (th->data && th->data->numVariables())
	{
		ByteArray* b = Class<ByteArray>::getInstanceS(sys);
		b->writeObject(th->data.getPtr());
		asAtomHandler::setInt(ret,sys,b->getLength());
		b->decRef();
	}
	else
		asAtomHandler::setInt(ret,sys,0);
}

void IDynamicPropertyWriter::linkTraits(Class_base* c)
{
	lookupAndLink(c,"writeDynamicProperties","flash.net:IDynamicPropertyWriter");
}
void IDynamicPropertyOutput::linkTraits(Class_base* c)
{
	lookupAndLink(c,"writeDynamicProperty","flash.net:IDynamicPropertyOutput");
}

void DynamicPropertyOutput::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL);
	c->setDeclaredMethodByQName("writeDynamicProperty","",Class<IFunction>::getFunction(c->getSystemState(),writeDynamicProperty),NORMAL_METHOD,true);
	c->addImplementedInterface(InterfaceClass<IDynamicPropertyOutput>::getClass(c->getSystemState()));
	IDynamicPropertyOutput::linkTraits(c);
}
ASFUNCTIONBODY_ATOM(DynamicPropertyOutput,writeDynamicProperty)
{
	DynamicPropertyOutput* th=asAtomHandler::as<DynamicPropertyOutput>(obj);
	asAtom name=asAtomHandler::invalidAtom;
	asAtom value=asAtomHandler::invalidAtom;
	ARG_UNPACK_ATOM(name)(value);
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = asAtomHandler::toStringId(name,sys);
	ASATOM_INCREF(value);
	th->setVariableByMultiname(m,value,CONST_NOT_ALLOWED);
}

void ObjectEncoding::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	
	REGISTER_GETTER_SETTER_STATIC(c,dynamicPropertyWriter);
	c->setVariableAtomByQName("AMF0",nsNameAndKind(),asAtomHandler::fromUInt(AMF0),DECLARED_TRAIT);
	c->setVariableAtomByQName("AMF3",nsNameAndKind(),asAtomHandler::fromUInt(AMF3),DECLARED_TRAIT);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtomHandler::fromUInt(DEFAULT),DECLARED_TRAIT);
}
ASFUNCTIONBODY_GETTER_SETTER_STATIC(ObjectEncoding,dynamicPropertyWriter);

// this is a global counter to produce uinque IDs for NetConnections
// TODO maybe it would be better to use some form of GUID
std::atomic<uint64_t> nearIDcounter(0);
NetConnection::NetConnection(Class_base* c):
	EventDispatcher(c),_connected(false),downloader(NULL),messageCount(0),
	proxyType(PT_NONE),maxPeerConnections(8)
{
	char buf[100];
	sprintf(buf, "nearID%" PRIu64 "", ++nearIDcounter);
	nearID = buf;
}

void NetConnection::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(c->getSystemState(),call),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connected","",Class<IFunction>::getFunction(c->getSystemState(),_getConnected),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setDefaultObjectEncoding),SETTER_METHOD,false);
	c->getSystemState()->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::DEFAULT;
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("protocol","",Class<IFunction>::getFunction(c->getSystemState(),_getProtocol),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("proxyType","",Class<IFunction>::getFunction(c->getSystemState(),_getProxyType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("proxyType","",Class<IFunction>::getFunction(c->getSystemState(),_setProxyType),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(c->getSystemState(),_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,client);
	REGISTER_GETTER_SETTER(c,maxPeerConnections);
	REGISTER_GETTER(c,nearID);
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

ASFUNCTIONBODY_ATOM(NetConnection,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	th->objectEncoding = getSys()->staticNetConnectionDefaultObjectEncoding;
}

ASFUNCTIONBODY_ATOM(NetConnection,call)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	//Arguments are:
	//1) A string for the command
	//2) A Responder instance (optional)
	//And other arguments to be passed to the server
	tiny_string command;
	ARG_UNPACK_ATOM (command) (th->responder, NullRef);

	th->messageCount++;

	if(!th->uri.isValid())
		return;

	if(th->uri.isRTMP())
	{
		LOG(LOG_NOT_IMPLEMENTED, "RTMP not yet supported in NetConnection.call()");
		return;
	}

	//This function is supposed to be passed a array for the rest
	//of the arguments. Since that is not supported for native methods
	//just create it here
	_R<Array> rest=_MR(Class<Array>::getInstanceSNoArgs(sys));
	for(uint32_t i=2;i<argslen;i++)
	{
		ASATOM_INCREF(args[i]);
		rest->push(args[i]);
	}

	_R<ByteArray> message=_MR(Class<ByteArray>::getInstanceS(sys));
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
	sys->addJob(th);
}

void NetConnection::execute()
{
	LOG(LOG_CALLS,_("NetConnection async execution ") << uri);
	assert(!messageData.empty());
	std::list<tiny_string> headers;
	headers.push_back("Content-Type: application/x-amf");
	_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache(getSys())));
	downloader=getSys()->downloadManager->downloadWithData(uri, cache,
			messageData, headers, NULL);
	//Get the whole answer
	cache->waitForTermination();
	if(cache->hasFailed()) //Check to see if the download failed for some reason
	{
		LOG(LOG_ERROR, "NetConnection::execute(): Download of URL failed: " << uri);
//		getVm()->addEvent(contentLoaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
		{
			//Acquire the lock to ensure consistency in threadAbort
			Locker l(downloaderLock);
			getSys()->downloadManager->destroy(downloader);
			downloader = NULL;
		}
		return;
	}
	std::streambuf *sbuf = cache->createReader();
	istream s(sbuf);
	_R<ByteArray> message=_MR(Class<ByteArray>::getInstanceS(getSys()));
	uint8_t* buf=message->getBuffer(downloader->getLength(), true);
	s.read((char*)buf,downloader->getLength());
	//Download is done, destroy it
	delete sbuf;
	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
	_R<ParseRPCMessageEvent> event=_MR(new (getSystemState()->unaccountedMemory) ParseRPCMessageEvent(message, client, responder));
	getVm(getSystemState())->addEvent(NullRef,event);
	responder.reset();
}

void NetConnection::threadAbort()
{
	//We have to stop the downloader
	Locker l(downloaderLock);
	if(downloader != NULL)
		downloader->stop();
}

void NetConnection::jobFence()
{
	decRef();
}

ASFUNCTIONBODY_ATOM(NetConnection,connect)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	//This takes 1 required parameter and an unspecified number of optional parameters
	assert_and_throw(argslen>0);

	//This seems strange:
	//LOCAL_WITH_FILE may not use connect(), even if it tries to connect to a local file.
	//I'm following the specification to the letter. Testing showed
	//that the official player allows connect(null) in localWithFile.
	if(!asAtomHandler::isNull(args[0])
		&& sys->securityManager->evaluateSandbox(SecurityManager::LOCAL_WITH_FILE))
		throw Class<SecurityError>::getInstanceS(sys,"SecurityError: NetConnection::connect "
				"from LOCAL_WITH_FILE sandbox");

	bool isNull = false;
	bool isRTMP = false;
	//bool isRPC = false;

	//Null argument means local file or web server, the spec only mentions NULL, but youtube uses UNDEFINED, so supporting that too.
	if(asAtomHandler::isNull(args[0]) || 
			asAtomHandler::isUndefined(args[0]))
	{
		th->_connected = false;
		isNull = true;
	}
	//String argument means Flash Remoting/Flash Media Server
	else
	{
		th->_connected = false;
		th->uri = URLInfo(asAtomHandler::toString(args[0],sys));

		if(sys->securityManager->evaluatePoliciesURL(th->uri, true) != SecurityManager::ALLOWED)
		{
			//TODO: find correct way of handling this case
			throw Class<SecurityError>::getInstanceS(sys,"SecurityError: connection to domain not allowed by securityManager");
		}
		
		//By spec NetConnection::connect is true for RTMP and remoting and false otherwise
		if(th->uri.isRTMP())
		{
			isRTMP = true;
			// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
			//th->_connected = true;
		}
		else if(th->uri.getProtocol() == "http" ||
		     th->uri.getProtocol() == "https")
		{
			// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
			//th->_connected = true;
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
		getVm(sys)->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS(sys,"status", "NetConnection.Connect.Success")));
	}
}
void NetConnection::afterExecution(_R<Event> ev)
{
	if (ev->is<NetStatusEvent>())
	{
		// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
		if (ev->as<NetStatusEvent>()->statuscode == "NetConnection.Connect.Success")
			this->_connected = true;
	}
}
ASFUNCTIONBODY_ATOM(NetConnection,_getConnected)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	ret = asAtomHandler::fromBool(th->_connected);
}

ASFUNCTIONBODY_ATOM(NetConnection,_getConnectedProxyType)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	if (!th->_connected)
		throw Class<ArgumentError>::getInstanceS(sys,"NetConnection object must be connected.", 2126);
	ret = asAtomHandler::fromString(sys,"none");
}

ASFUNCTIONBODY_ATOM(NetConnection,_getDefaultObjectEncoding)
{
	ret = asAtomHandler::fromUInt(sys->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY_ATOM(NetConnection,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = asAtomHandler::toInt(args[0]);
	if(value == 0)
		sys->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
		sys->staticNetConnectionDefaultObjectEncoding = ObjectEncoding::AMF3;
	else
		throw RunTimeException("Invalid object encoding");
}

ASFUNCTIONBODY_ATOM(NetConnection,_getObjectEncoding)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	ret = asAtomHandler::fromUInt(th->objectEncoding);
}

ASFUNCTIONBODY_ATOM(NetConnection,_setObjectEncoding)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	assert_and_throw(argslen == 1);
	if(th->_connected)
	{
		throw Class<ReferenceError>::getInstanceS(sys,"set NetConnection.objectEncoding after connect");
	}
	int32_t value = asAtomHandler::toInt(args[0]);
	if(value == 0)
		th->objectEncoding = ObjectEncoding::AMF0; 
	else
		th->objectEncoding = ObjectEncoding::AMF3; 
}

ASFUNCTIONBODY_ATOM(NetConnection,_getProtocol)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	if(th->_connected)
		ret = asAtomHandler::fromString(sys,th->protocol);
	else
		throw Class<ArgumentError>::getInstanceS(sys,"get NetConnection.protocol before connect");
}

ASFUNCTIONBODY_ATOM(NetConnection,_getProxyType)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	tiny_string name;
	switch(th->proxyType)
	{
		case PT_NONE:
			name = "NONE";
			break;
		case PT_HTTP:
			name = "HTTP";
			break;
		case PT_CONNECT_ONLY:
			name = "CONNECTOnly";
			break;
		case PT_CONNECT:
			name = "CONNECT";
			break;
		case PT_BEST:
			name = "best";
			break;
		default:
			assert(false && "Invalid proxy type");
			name = "";
			break;
	}
	ret = asAtomHandler::fromString(sys,name);
}

ASFUNCTIONBODY_ATOM(NetConnection,_setProxyType)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	tiny_string value;
	ARG_UNPACK_ATOM(value);
	if (value == "NONE")
		th->proxyType = PT_NONE;
	else if (value == "HTTP")
		th->proxyType = PT_HTTP;
	else if (value == "CONNECTOnly")
		th->proxyType = PT_CONNECT_ONLY;
	else if (value == "CONNECT")
		th->proxyType = PT_CONNECT;
	else if (value == "best")
		th->proxyType = PT_BEST;
	else
		throwError<ArgumentError>(kInvalidEnumError, "proxyType");

	if (th->proxyType != PT_NONE)
		LOG(LOG_NOT_IMPLEMENTED, "Unimplemented proxy type " << value);

}

ASFUNCTIONBODY_ATOM(NetConnection,_getURI)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	if(th->_connected && th->uri.isValid())
		ret = asAtomHandler::fromObject(abstract_s(sys,th->uri.getURL()));
	else
	{
		//Reference says the return should be undefined. The right thing is "null" as a string
		ret = asAtomHandler::fromString(sys,"null");
	}
}

ASFUNCTIONBODY_ATOM(NetConnection,close)
{
	NetConnection* th=asAtomHandler::as<NetConnection>(obj);
	if(th->_connected)
	{
		th->threadAbort();
		th->_connected = false;
	}
}

ASFUNCTIONBODY_GETTER_SETTER(NetConnection, client);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NetConnection, maxPeerConnections);
ASFUNCTIONBODY_GETTER(NetConnection, nearID);

void NetStreamAppendBytesAction::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL);
	c->setVariableAtomByQName("END_SEQUENCE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"endSequence"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RESET_BEGIN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resetBegin"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RESET_SEEK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resetSeek"),CONSTANT_TRAIT);
}


NetStream::NetStream(Class_base* c):EventDispatcher(c),tickStarted(false),paused(false),closed(true),
	streamTime(0),frameRate(0),connection(),downloader(nullptr),videoDecoder(nullptr),
	audioDecoder(nullptr),audioStream(nullptr),datagenerationfile(nullptr),datagenerationthreadstarted(false),client(NullRef),
	oldVolume(-1.0),checkPolicyFile(false),rawAccessAllowed(false),framesdecoded(0),playbackBytesPerSecond(0),maxBytesPerSecond(0),datagenerationexpecttype(DATAGENERATION_HEADER),datagenerationbuffer(Class<ByteArray>::getInstanceS(c->getSystemState())),
	streamDecoder(nullptr),
	backBufferLength(0),backBufferTime(30),bufferLength(0),bufferTime(0.1),bufferTimeMax(0),
	maxPauseBufferTime(0)
{
	subtype=SUBTYPE_NETSTREAM;
	soundTransform = _MNR(Class<SoundTransform>::getInstanceS(c->getSystemState()));
}

void NetStream::finalize()
{
	EventDispatcher::finalize();
	connection.reset();
	client.reset();
	if (streamDecoder)
		delete streamDecoder;
	streamDecoder=nullptr;
}

NetStream::~NetStream()
{
	if(tickStarted)
		getSys()->removeJob(this);
	delete videoDecoder;
	delete audioDecoder;
	if (datagenerationfile)
		delete datagenerationfile;
}

void NetStream::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("CONNECT_TO_FMS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"connectToFMS"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DIRECT_CONNECTIONS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"directConnections"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play2","",Class<IFunction>::getFunction(c->getSystemState(),play2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("resume","",Class<IFunction>::getFunction(c->getSystemState(),resume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pause","",Class<IFunction>::getFunction(c->getSystemState(),pause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("togglePause","",Class<IFunction>::getFunction(c->getSystemState(),togglePause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("seek","",Class<IFunction>::getFunction(c->getSystemState(),seek),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",Class<IFunction>::getFunction(c->getSystemState(),_getTime),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFPS","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFPS),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("client","",Class<IFunction>::getFunction(c->getSystemState(),_getClient),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("client","",Class<IFunction>::getFunction(c->getSystemState(),_setClient),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("checkPolicyFile","",Class<IFunction>::getFunction(c->getSystemState(),_getCheckPolicyFile),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("checkPolicyFile","",Class<IFunction>::getFunction(c->getSystemState(),_setCheckPolicyFile),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("attach","",Class<IFunction>::getFunction(c->getSystemState(),attach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendBytes","",Class<IFunction>::getFunction(c->getSystemState(),appendBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendBytesAction","",Class<IFunction>::getFunction(c->getSystemState(),appendBytesAction),NORMAL_METHOD,true);
	REGISTER_GETTER(c, backBufferLength);
	REGISTER_GETTER_SETTER(c, backBufferTime);
	REGISTER_GETTER(c, bufferLength);
	REGISTER_GETTER_SETTER(c, bufferTime);
	REGISTER_GETTER_SETTER(c, bufferTimeMax);
	REGISTER_GETTER_SETTER(c, maxPauseBufferTime);
	REGISTER_GETTER_SETTER(c,soundTransform);
	REGISTER_GETTER_SETTER(c,useHardwareDecoder);
	c->setDeclaredMethodByQName("info","",Class<IFunction>::getFunction(c->getSystemState(),_getInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("publish","",Class<IFunction>::getFunction(c->getSystemState(),publish),NORMAL_METHOD,true);
	if (!c->getSystemState()->mainClip->usesActionScript3)
		c->setDeclaredMethodByQName("setBufferTime","",Class<IFunction>::getFunction(c->getSystemState(),_setter_bufferTime),NORMAL_METHOD,true);
}

void NetStream::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_GETTER(NetStream, backBufferLength);
ASFUNCTIONBODY_GETTER_SETTER(NetStream, backBufferTime);
ASFUNCTIONBODY_GETTER(NetStream, bufferLength);
ASFUNCTIONBODY_GETTER_SETTER(NetStream, bufferTime);
ASFUNCTIONBODY_GETTER_SETTER(NetStream, bufferTimeMax);
ASFUNCTIONBODY_GETTER_SETTER(NetStream, maxPauseBufferTime);
ASFUNCTIONBODY_GETTER_SETTER(NetStream,soundTransform);
ASFUNCTIONBODY_GETTER_SETTER(NetStream,useHardwareDecoder);

ASFUNCTIONBODY_ATOM(NetStream,_getInfo)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	NetStreamInfo* res = Class<NetStreamInfo>::getInstanceS(sys);
	if(th->isReady())
	{
		res->byteCount = th->getReceivedLength();
		res->dataBufferLength = th->getReceivedLength();
	}
	if (th->datagenerationfile)
	{
		int curbps = 0;
		uint64_t cur=compat_msectiming();
		th->countermutex.lock();
		while (th->currentBytesPerSecond.size() > 0 && (cur - th->currentBytesPerSecond.front().timestamp > 1000))
		{
			th->currentBytesPerSecond.pop_front();
		}
		auto it = th->currentBytesPerSecond.cbegin();
		while (it != th->currentBytesPerSecond.cend())
		{
			curbps += it->bytesread;
			it++;
		}
		if (th->maxBytesPerSecond < curbps)
			th->maxBytesPerSecond = curbps;

		res->currentBytesPerSecond = curbps;
		res->dataBytesPerSecond = curbps;
		res->maxBytesPerSecond = th->maxBytesPerSecond;
		
		//TODO compute video/audio BytesPerSecond correctly
		res->videoBytesPerSecond = curbps*3/4;
		res->audioBytesPerSecond = curbps/4;
		
		th->countermutex.unlock();
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"NetStreamInfo.currentBytesPerSecond/maxBytesPerSecond/dataBytesPerSecond is only implemented for data generation mode");
	if (th->videoDecoder)
		res->droppedFrames = th->videoDecoder->framesdropped;
	res->playbackBytesPerSecond = th->playbackBytesPerSecond;
	res->audioBufferLength = th->bufferLength;
	res->videoBufferLength = th->bufferLength;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(NetStream,_getClient)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->client.isNull())
		asAtomHandler::setUndefined(ret);
	else
	{
		th->client->incRef();
		ret = asAtomHandler::fromObject(th->client.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(NetStream,_setClient)
{
	assert_and_throw(argslen == 1);
	if(asAtomHandler::isNull(args[0]))
		throw Class<TypeError>::getInstanceS(sys);

	NetStream* th=asAtomHandler::as<NetStream>(obj);

	ASATOM_INCREF(args[0]);
	th->client = _MR(asAtomHandler::toObject(args[0],sys));
}

ASFUNCTIONBODY_ATOM(NetStream,_getCheckPolicyFile)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);

	asAtomHandler::setBool(ret,th->checkPolicyFile);
}

ASFUNCTIONBODY_ATOM(NetStream,_setCheckPolicyFile)
{
	assert_and_throw(argslen == 1);
	
	NetStream* th=asAtomHandler::as<NetStream>(obj);

	th->checkPolicyFile = asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(NetStream,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	NetStream* th=asAtomHandler::as<NetStream>(obj);

	LOG(LOG_CALLS,_("NetStream constructor"));
	tiny_string value;
	_NR<NetConnection> netConnection;

	ARG_UNPACK_ATOM(netConnection)(value, "connectToFMS");

	if(value == "directConnections")
		th->peerID = DIRECT_CONNECTIONS;
	else
		th->peerID = CONNECT_TO_FMS;

	th->incRef();
	netConnection->incRef();
	th->connection=netConnection;
	th->client = _NR<ASObject>(th);
}

ASFUNCTIONBODY_ATOM(NetStream,play)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);

	//Make sure the stream is restarted properly
	if(th->closed)
		th->closed = false;
	else
		return;

	//Reset the paused states
	th->paused = false;
//	th->audioPaused = false;
	
	// Parameter Null means data is generated by calls to "appendBytes"
	if (asAtomHandler::is<Null>(args[0]))
	{
		th->datagenerationfile = sys->getEngineData()->createFileStreamCache(th->getSystemState());
		th->datagenerationfile->openForWriting();
		th->streamTime=0;
		return;
	}
	if (th->connection.isNull())
		throwError<ASError>(0,"not connected");
	
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
		th->url.setStream(asAtomHandler::toString(args[0],sys));
	}
	else
	{
		//HTTP download
		assert_and_throw(argslen>=1);
		//args[0] is the url
		//what is the meaning of the other arguments
		th->url = sys->mainClip->getOrigin().goToURL(asAtomHandler::toString(args[0],sys));

		SecurityManager::EVALUATIONRESULT evaluationResult =
			sys->securityManager->evaluateURLStatic(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
				SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
				true); //Check for navigating up in local directories (not allowed)
		if(evaluationResult == SecurityManager::NA_REMOTE_SANDBOX)
			throw Class<SecurityError>::getInstanceS(sys,"SecurityError: NetStream::play: "
					"connect to network");
		//Local-with-filesystem sandbox can't access network
		else if(evaluationResult == SecurityManager::NA_LOCAL_SANDBOX)
			throw Class<SecurityError>::getInstanceS(sys,"SecurityError: NetStream::play: "
					"connect to local file");
		else if(evaluationResult == SecurityManager::NA_PORT)
			throw Class<SecurityError>::getInstanceS(sys,"SecurityError: NetStream::play: "
					"connect to restricted port");
		else if(evaluationResult == SecurityManager::NA_RESTRICT_LOCAL_DIRECTORY)
			throw Class<SecurityError>::getInstanceS(sys,"SecurityError: NetStream::play: "
					"not allowed to navigate up for local files");
	}

	assert_and_throw(th->downloader==NULL);

	//Until buffering is implemented, set a fake value. The BBC
	//news player panics if bufferLength is smaller than 2.
	//th->bufferLength = 10;

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm(sys)->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(sys)));
	}
	else //The URL is valid so we can start the download and add ourself as a job
	{
		StreamCache *cache = sys->getEngineData()->createFileStreamCache(th->getSystemState());
		th->downloader=getSys()->downloadManager->download(th->url, _MR(cache), NULL);
		th->streamTime=0;
		//To be decreffed in jobFence
		th->incRef();
		sys->addJob(th);
	}
}

void NetStream::jobFence()
{
	decRef();
}

ASFUNCTIONBODY_ATOM(NetStream,resume)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->paused)
	{
		th->paused = false;
		{
			Locker l(th->mutex);
			if(th->audioStream)
				th->audioStream->resume();
		}
		th->incRef();
		getVm(sys)->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS(sys,"status", "NetStream.Unpause.Notify")));
	}
}

ASFUNCTIONBODY_ATOM(NetStream,pause)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(!th->paused)
	{
		th->paused = true;
		{
			Locker l(th->mutex);
			if(th->audioStream)
				th->audioStream->pause();
		}
		th->incRef();
		getVm(sys)->addEvent(_MR(th),_MR(Class<NetStatusEvent>::getInstanceS(sys,"status", "NetStream.Pause.Notify")));
	}
}

ASFUNCTIONBODY_ATOM(NetStream,togglePause)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->paused)
		th->resume(ret,sys,obj, NULL, 0);
	else
		th->pause(ret,sys,obj, NULL, 0);
}

ASFUNCTIONBODY_ATOM(NetStream,close)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	//TODO: set the time property to 0
	
	//Everything is stopped in threadAbort
	if(!th->closed)
	{
		th->threadAbort();
		th->incRef();
		getVm(sys)->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS(sys,"status", "NetStream.Play.Stop")));
	}
	LOG(LOG_CALLS, _("NetStream::close called"));
}
ASFUNCTIONBODY_ATOM(NetStream,play2)
{
	//NetStream* th=asAtomHandler::as<NetStream>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Netstream.play2 not implemented:"<< asAtomHandler::toDebugString(args[0]));
}
ASFUNCTIONBODY_ATOM(NetStream,seek)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	int pos;
	ARG_UNPACK_ATOM(pos);
	
	th->countermutex.lock();
	if (th->streamDecoder)
	{
		th->streamDecoder->jumpToPosition(pos*1000);
		th->streamTime=pos;
	}
	th->countermutex.unlock();
	if(th->paused)
	{
		th->paused = false;
		{
			Locker l(th->mutex);
			if(th->audioStream)
				th->audioStream->resume();
		}
		th->incRef();
		getVm(sys)->addEvent(_MR(th), _MR(Class<NetStatusEvent>::getInstanceS(sys,"status", "NetStream.Unpause.Notify")));
	}
}

ASFUNCTIONBODY_ATOM(NetStream,attach)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	_NR<NetConnection> netConnection;
	ARG_UNPACK_ATOM(netConnection);

	netConnection->incRef();
	th->connection=netConnection;
}
ASFUNCTIONBODY_ATOM(NetStream,appendBytes)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	_NR<ByteArray> bytearray;
	ARG_UNPACK_ATOM(bytearray);

	if(!bytearray.isNull())
	{
		if (th->datagenerationfile)
		{
			//th->datagenerationfile->append(bytearray->getBuffer(bytearray->getLength(),false),bytearray->getLength());
			th->datagenerationbuffer->setPosition(th->datagenerationbuffer->getLength());
			th->datagenerationbuffer->writeBytes(bytearray->getBuffer(bytearray->getLength(),false),bytearray->getLength());
			th->datagenerationbuffer->setPosition(0);
			uint8_t tmp_byte = 0;
			uint32_t processedlength = th->datagenerationbuffer->getPosition();
			bool done = false;
			while (!done)
			{
				switch (th->datagenerationexpecttype)
				{
					case DATAGENERATION_HEADER:
					{
						// TODO check for correct header?
						// skip flv header
						th->datagenerationbuffer->setPosition(5);
						uint32_t headerlen = 0;
						// header length is always in big endian
						if (!th->datagenerationbuffer->readByte(tmp_byte))
						{
							done = true; 
							break;
						}
						headerlen |= tmp_byte<<24;
						if (!th->datagenerationbuffer->readByte(tmp_byte))
						{
							done = true; 
							break;
						}
						headerlen |= tmp_byte<<16;
						if (!th->datagenerationbuffer->readByte(tmp_byte))
						{
							done = true; 
							break;
						}
						headerlen |= tmp_byte<<8;
						if (!th->datagenerationbuffer->readByte(tmp_byte))
						{
							done = true; 
							break;
						}
						headerlen |= tmp_byte;
						if (headerlen > 0)
						{
							th->datagenerationbuffer->setPosition(headerlen);
							th->datagenerationexpecttype = DATAGENERATION_PREVTAG;
							processedlength+= headerlen;
						}
						else
							done = true;
						break;
					}
					case DATAGENERATION_PREVTAG:
					{
						uint32_t tmp_uint32;
						if (!th->datagenerationbuffer->readUnsignedInt(tmp_uint32)) // prevtag (value may be wrong as we don't check for big endian)
						{
							done = true;
							break;
						}
						processedlength += 4;
						th->datagenerationexpecttype = DATAGENERATION_FLVTAG;
						break;
					}
					case DATAGENERATION_FLVTAG:
					{
						if (!th->datagenerationbuffer->readByte(tmp_byte)) // tag type
						{
							done = true;
							break;
						}
						uint32_t datalen = 0;
						if (!th->datagenerationbuffer->readByte(tmp_byte)) // data len 1
						{
							done = true;
							break;
						}
						datalen |= tmp_byte<<16;
						if (!th->datagenerationbuffer->readByte(tmp_byte)) // data len 2
						{
							done = true;
							break;
						}
						datalen |= tmp_byte<<8;
						if (!th->datagenerationbuffer->readByte(tmp_byte)) // data len 3
						{
							done = true;
							break;
						}
						datalen |= tmp_byte;
						datalen += 1 + 3 + 3 + 1 + 3; 
						if (datalen + processedlength< th->datagenerationbuffer->getLength())
						{
							processedlength += datalen;
							th->datagenerationbuffer->setPosition(processedlength);
							th->datagenerationexpecttype = DATAGENERATION_PREVTAG;
						}
						break;
					}
					default:
						LOG(LOG_ERROR,"invalid DATAGENERATION_EXPECT_TYPE:"<<th->datagenerationexpecttype);
						done = true;
						break;
				}
			}
			if (processedlength > 0)
			{
				th->datagenerationfile->append(th->datagenerationbuffer->getBuffer(processedlength,false),processedlength);
				if (processedlength!=th->datagenerationbuffer->getLength())
					th->datagenerationbuffer->removeFrontBytes(processedlength);
				else
					th->datagenerationbuffer->setLength(0);
				uint64_t cur=compat_msectiming();
				struct bytespertime b;
				b.timestamp = cur;
				b.bytesread = processedlength;
				th->countermutex.lock();
				th->currentBytesPerSecond.push_back(b);
				while (cur - th->currentBytesPerSecond.front().timestamp > 60000)
					th->currentBytesPerSecond.pop_front();
				uint32_t curbps = 0;
				auto it = th->currentBytesPerSecond.cbegin();
				while (it != th->currentBytesPerSecond.cend())
				{
					curbps += it->bytesread;
					it++;
				}
				if (th->maxBytesPerSecond < curbps)
					th->maxBytesPerSecond = curbps;
				
				th->countermutex.unlock();
			}
			if (!th->datagenerationthreadstarted && th->datagenerationfile->getReceivedLength() >= 8192)
			{
				th->closed = false;
				th->datagenerationthreadstarted = true;
				th->incRef();
				sys->addJob(th);
			}
		}

	}
}
ASFUNCTIONBODY_ATOM(NetStream,appendBytesAction)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	tiny_string val;
	ARG_UNPACK_ATOM(val);

	if (val == "resetBegin")
	{
		th->threadAbort();
		LOG(LOG_INFO,"resetBegin");
		if (th->datagenerationfile)
			delete th->datagenerationfile;
		th->datagenerationfile = sys->getEngineData()->createFileStreamCache(sys);
		th->datagenerationfile->openForWriting();
		th->datagenerationbuffer->setLength(0);
		th->datagenerationthreadstarted = false;
		th->datagenerationexpecttype = DATAGENERATION_HEADER;
	}
	else if (val == "resetSeek")
	{
		LOG(LOG_INFO,"resetSeek:"<<th->datagenerationbuffer->getLength());
		th->datagenerationbuffer->setLength(0);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"NetStream.appendBytesAction is not implemented yet:"<<val);
}
ASFUNCTIONBODY_ATOM(NetStream,publish)
{
	//NetStream* th=asAtomHandler::as<NetStream>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Netstream.publish not implemented");
}
//Tick is called from the timer thread, this happens only if a decoder is available
void NetStream::tick()
{
	//Check if the stream is paused
	if(audioStream)
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
	if(audioStream && !audioStream->hasStarted)
	{
		audioStream->hasStarted = true;
		audioStream->resume();
	}
	//Advance video and audio to current time, follow the audio stream time
	countermutex.lock();
	if(audioStream)
	{
		assert(audioDecoder);
		if (streamTime == 0)
			streamTime=audioStream->getPlayedTime()+audioDecoder->initialTime;
		else if (this->bufferLength > 0)
			streamTime+=1000/frameRate;
	}
	else
	{
		if (this->bufferLength > 0)
			streamTime+=1000/frameRate;
		if (audioDecoder)
			audioDecoder->skipAll();
	}
	this->bufferLength = (framesdecoded / frameRate) - (streamTime-prevstreamtime)/1000.0;
	if (this->bufferLength < 0)
		this->bufferLength = 0;
	//LOG(LOG_INFO,"tick:"<< " "<<bufferLength << " "<<streamTime<<" "<<frameRate<<" "<<framesdecoded<<" "<<bufferTime<<" "<<this->playbackBytesPerSecond<<" "<<this->getReceivedLength());
	countermutex.unlock();
	if (videoDecoder)
	{
		videoDecoder->skipUntil(streamTime);
		//The next line ensures that the downloader will not be destroyed before the upload jobs are fenced
		videoDecoder->waitForFencing();
		getSys()->getRenderThread()->addUploadJob(videoDecoder);
	}
}

void NetStream::tickFence()
{
}

bool NetStream::isReady() const
{
	//Must have videoDecoder, but audioDecoder is optional (in
	//case the video doesn't have audio)
	return videoDecoder && videoDecoder->isValid() && 
			(!audioDecoder || audioDecoder->isValid());
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
	SecurityManager::EVALUATIONRESULT evaluationResult = getSystemState()->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		rawAccessAllowed = true;

	std::streambuf *sbuf = nullptr;
	if (streamDecoder)
		delete streamDecoder;
	streamDecoder=nullptr;

	if (datagenerationfile)
	{
		sbuf = datagenerationfile->createReader();
	}
	else
	{
		if (!downloader)
			return;
		if(downloader->hasFailed())
		{
			this->incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS(getSystemState())));
			getSystemState()->downloadManager->destroy(downloader);
			downloader = nullptr;
			return;
		}
		
		//The downloader hasn't failed yet at this point
		
		sbuf = downloader->getCache()->createReader();
	}
	istream s(sbuf);
	s.exceptions(istream::goodbit);

	ThreadProfile* profile=getSystemState()->allocateProfiler(RGB(0,0,200));
	profile->setTag("NetStream");
	bool waitForFlush=true;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		Chronometer chronometer;
		if (!sbuf)
		{
			threadAbort();
		}
		else
		{
			streamDecoder=new BuiltinStreamDecoder(s,this);
			if (!streamDecoder->isValid()) // not FLV stream, so we try ffmpeg detection
			{
				s.seekg(0);
				streamDecoder=new FFMpegStreamDecoder(this,this->getSystemState()->getEngineData(),s);
			}
			if(!streamDecoder->isValid())
				threadAbort();
		}

		
		countermutex.lock();
		framesdecoded = 0;
		frameRate=0;
		videoDecoder = NULL;
		this->prevstreamtime = streamTime;
		this->bufferLength = 0;
		countermutex.unlock();
		bool done=false;
		bool bufferfull = true;
		while(!done)
		{
			//Check if threadAbort has been called, if so, stop this loop
			if(closed)
			{
				done = true;
				continue;
			}
			bool decodingSuccess= bufferfull && streamDecoder->decodeNextFrame();
			if(!decodingSuccess && bufferfull)
			{
				if (s.tellg() == -1)
				{
					done = true;
					continue;
				}

				LOG(LOG_INFO,"decoding failed:"<<s.tellg()<<" "<<this->getReceivedLength());
				bufferfull = false;
			}
			else
			{
				if (streamDecoder->videoDecoder)
				{
					if (streamDecoder->videoDecoder->framesdecoded != framesdecoded)
					{
						countermutex.lock();

						framesdecoded = streamDecoder->videoDecoder->framesdecoded;
						if(frameRate==0)
						{
							assert(streamDecoder->videoDecoder->frameRate);
							frameRate=streamDecoder->videoDecoder->frameRate;
						}
						if (frameRate)
						{
							this->playbackBytesPerSecond = s.tellg() / (framesdecoded / frameRate);
							this->bufferLength = (framesdecoded / frameRate) - (streamTime-prevstreamtime)/1000.0;
						}
						countermutex.unlock();
						if (bufferfull && this->bufferLength < 0)
						{
							bufferfull = false;
							this->bufferLength=0;
							this->incRef();
							getVm(getSystemState())->addEvent(_MR(this),_MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Buffer.Empty")));
						}
					}
				}
			}
			
			if(videoDecoder==NULL && streamDecoder->videoDecoder)
			{
				videoDecoder=streamDecoder->videoDecoder;
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),
								  _MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Play.Start")));
			}
			if(audioDecoder==NULL && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;
			
			if(audioStream==NULL && audioDecoder && audioDecoder->isValid())
				audioStream=getSys()->audioManager->createStream(audioDecoder,streamDecoder->hasVideo(),nullptr,0,soundTransform ? soundTransform->volume : 1.0);
			if(!tickStarted && isReady() && frameRate && ((framesdecoded / frameRate) >= this->bufferTime))
			{
				tickStarted=true;
				paused=false;
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),
								  _MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Buffer.Full")));
				getSystemState()->addTick(1000/frameRate,this);
				//Also ask for a render rate equal to the video one (capped at 24)
				float localRenderRate=dmin(frameRate,24);
				getSystemState()->setRenderRate(localRenderRate);
			}
			if (!bufferfull && frameRate && ((framesdecoded / frameRate) >= this->bufferTime))
			{
				bufferfull = true;
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),
								  _MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Buffer.Full")));
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
		LOG(LOG_ERROR, "JobTerminationException in NetStream ");
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
		getVm(getSystemState())->addEvent(_MR(this), _MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Play.Stop")));
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this), _MR(Class<NetStatusEvent>::getInstanceS(getSystemState(),"status", "NetStream.Buffer.Flush")));
	}
	//Before deleting stops ticking, removeJobs also spin waits for termination
	getSystemState()->removeJob(this);
	tickStarted=false;

	{
		Locker l(mutex);
		//Change the state to invalid to avoid locking
		videoDecoder=NULL;
		audioDecoder=NULL;
		//Clean up everything for a possible re-run
		if (downloader)
			getSys()->downloadManager->destroy(downloader);
		//This transition is critical, so the mutex is needed
		downloader=NULL;
		if (audioStream)
			delete audioStream;
		audioStream=NULL;
	}
	if (streamDecoder)
	{
		delete streamDecoder;
		streamDecoder = nullptr;
	}
	if (sbuf)
		delete sbuf;
}

void NetStream::threadAbort()
{
	Locker l(mutex);
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

void NetStream::sendClientNotification(const tiny_string& name, std::list<asAtom>& arglist)
{
	if (client.isNull())
		return;

	multiname callbackName(NULL);
	callbackName.name_type=multiname::NAME_STRING;
	callbackName.name_s_id=getSys()->getUniqueStringId(name);
	callbackName.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
	asAtom callback=asAtomHandler::invalidAtom;
	client->getVariableByMultiname(callback,callbackName);
	if(asAtomHandler::isFunction(callback))
	{
		asAtom callbackArgs[arglist.size()];

		ASObject* closure = client.getPtr();
		if (asAtomHandler::getClosure(callback))
			closure = asAtomHandler::getClosure(callback);
		closure->incRef();
		int i= 0;
		for (auto it = arglist.cbegin();it != arglist.cend(); it++)
		{
			asAtom arg = (*it);
			ASATOM_INCREF(arg);
			callbackArgs[i++] = arg;
		}
		ASATOM_INCREF(callback);
		_R<FunctionEvent> event(new (getSys()->unaccountedMemory) FunctionEvent(callback,
				asAtomHandler::fromObject(closure), callbackArgs, arglist.size()));
		getVm(getSystemState())->addEvent(NullRef,event);
	}
}

ASFUNCTIONBODY_ATOM(NetStream,_getBytesLoaded)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->isReady())
		asAtomHandler::setUInt(ret,sys,th->getReceivedLength());
	else
		asAtomHandler::setUInt(ret,sys,0);
}

ASFUNCTIONBODY_ATOM(NetStream,_getBytesTotal)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->isReady())
		asAtomHandler::setUInt(ret,sys,th->getTotalLength());
	else
		asAtomHandler::setUInt(ret,sys,0);
}

ASFUNCTIONBODY_ATOM(NetStream,_getTime)
{
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->isReady())
		asAtomHandler::setNumber(ret,sys,th->getStreamTime()/1000.);
	else
		asAtomHandler::setUInt(ret,sys,0);
}

ASFUNCTIONBODY_ATOM(NetStream,_getCurrentFPS)
{
	//TODO: provide real FPS (what really is displayed)
	NetStream* th=asAtomHandler::as<NetStream>(obj);
	if(th->isReady() && !th->paused)
		asAtomHandler::setNumber(ret,sys,th->getFrameRate());
	else
		asAtomHandler::setUInt(ret,sys,0);
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
	if (datagenerationfile)
		return datagenerationfile->getReceivedLength();
	return downloader->getReceivedLength();
}

uint32_t NetStream::getTotalLength()
{
	assert(isReady());
	if (datagenerationfile)
		return 0;
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
			propName.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
			asAtom curValue=asAtomHandler::invalidAtom;
			getVariableByMultiname(curValue,propName);
			if(asAtomHandler::isValid(curValue))
			{
				//If the variable already exists we have to create an Array of values
				Array* arr=NULL;
				if(!asAtomHandler::isArray(curValue))
				{
					arr=Class<Array>::getInstanceSNoArgs(getSystemState());
					arr->push(curValue);
					asAtom v = asAtomHandler::fromObject(arr);
					setVariableByMultiname(propName,v,ASObject::CONST_NOT_ALLOWED);
				}
				else
					arr=Class<Array>::cast(asAtomHandler::getObject(curValue));

				arr->push(asAtomHandler::fromObject(abstract_s(getSystemState(),value)));
			}
			else
			{
				asAtom v = asAtomHandler::fromObject(abstract_s(getSystemState(),value));
				setVariableByMultiname(propName,v,ASObject::CONST_NOT_ALLOWED);
			}

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
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("decode","",Class<IFunction>::getFunction(c->getSystemState(),decode),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

void URLVariables::buildTraits(ASObject* o)
{
}

URLVariables::URLVariables(Class_base* c, const tiny_string& s):ASObject(c)
{
	decode(s);
}

ASFUNCTIONBODY_ATOM(URLVariables,decode)
{
	URLVariables* th=asAtomHandler::as<URLVariables>(obj);
	assert_and_throw(argslen==1);
	th->decode(asAtomHandler::toString(args[0],sys));
}

ASFUNCTIONBODY_ATOM(URLVariables,_toString)
{
	URLVariables* th=asAtomHandler::as<URLVariables>(obj);
	assert_and_throw(argslen==0);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->toString_priv()));
}

ASFUNCTIONBODY_ATOM(URLVariables,_constructor)
{
	URLVariables* th=asAtomHandler::as<URLVariables>(obj);
	assert_and_throw(argslen<=1);
	if(argslen==1)
		th->decode(asAtomHandler::toString(args[0],sys));
}

tiny_string URLVariables::toString_priv()
{
	tiny_string tmp;
	uint32_t index=0;
	while ((index = nextNameIndex(index))!= 0)
	{
		if (!tmp.empty())
			tmp+="&";
		asAtom nameAtom = asAtomHandler::invalidAtom;
		nextName(nameAtom,index);
		const tiny_string& name=asAtomHandler::toString(nameAtom,getSystemState());
		//TODO: check if the allow_unicode flag should be true or false in g_uri_escape_string

		asAtom val=asAtomHandler::invalidAtom;
		nextValue(val,index);
		if(asAtomHandler::isArray(val))
		{
			//Print using multiple properties
			//Ex. ["foo","bar"] -> prop1=foo&prop1=bar
			Array* arr=Class<Array>::cast(asAtomHandler::getObject(val));
			for(uint32_t j=0;j<arr->size();j++)
			{
				//Escape the name
				char* escapedName=g_uri_escape_string(name.raw_buf(),NULL, false);
				tmp+=escapedName;
				g_free(escapedName);
				tmp+="=";

				//Escape the value
				asAtom a = arr->at(j);
				const tiny_string& value=asAtomHandler::toString(a,getSystemState());
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
			const tiny_string& value=asAtomHandler::toString(val,getSystemState());
			char* escapedValue=g_uri_escape_string(value.raw_buf(),NULL, false);
			tmp+=escapedValue;
			g_free(escapedValue);
		}
	}
	return tmp;
}

tiny_string URLVariables::toString()
{
	assert_and_throw(implEnable);
	return toString_priv();
}

ASFUNCTIONBODY_ATOM(lightspark,sendToURL)
{
	assert_and_throw(argslen == 1);
	ASObject* arg=asAtomHandler::getObject(args[0]);
	URLRequest* urlRequest=Class<URLRequest>::dyncast(arg);
	assert_and_throw(urlRequest);

	URLInfo url=urlRequest->getRequestURL();

	if(!url.isValid())
		return;

	sys->securityManager->checkURLStaticAndThrow(
		url, 
		~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
		true);

	//Also check cross domain policies. TODO: this should be async as it could block if invoked from ExternalInterface
	SecurityManager::EVALUATIONRESULT evaluationResult;
	evaluationResult = sys->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
	{
		//TODO: find correct way of handling this case (SecurityErrorEvent in this case)
		throw Class<SecurityError>::getInstanceS(sys,"SecurityError: sendToURL: "
				"connection to domain not allowed by securityManager");
	}

	//TODO: should we disallow accessing local files in a directory above 
	//the current one like we do with NetStream.play?

	Downloader* downloader=sys->downloadManager->download(url, _MR(new MemoryStreamCache(sys)), NULL);
	//TODO: make the download asynchronous instead of waiting for an unused response
	downloader->waitForTermination();
	sys->downloadManager->destroy(downloader);
}

ASFUNCTIONBODY_ATOM(lightspark,navigateToURL)
{
	_NR<URLRequest> request;
	tiny_string window;
	ARG_UNPACK_ATOM (request) (window,"");

	if (request.isNull())
		return;

	URLInfo url=request->getRequestURL();
	if(!url.isValid())
		return;

	sys->securityManager->checkURLStaticAndThrow(
		url, 
		~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED,
		true);

	if (window.empty())
		window = "_blank";

	vector<uint8_t> postData;
	request->getPostData(postData);
	if (!postData.empty())
	{
		LOG(LOG_NOT_IMPLEMENTED, "POST requests not supported in navigateToURL");
		return;
	}

	sys->openPageInBrowser(url.getURL(), window);
}

void Responder::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("onResult","",Class<IFunction>::getFunction(c->getSystemState(),onResult),NORMAL_METHOD,true);
}

void Responder::finalize()
{
	ASObject::finalize();
	ASATOM_DECREF(result);
	ASATOM_DECREF(status);
}

ASFUNCTIONBODY_ATOM(Responder,_constructor)
{
	Responder* th=Class<Responder>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen==1 || argslen==2);
	assert_and_throw(asAtomHandler::isFunction(args[0]));
	ASATOM_INCREF(args[0]);
	th->result = args[0];
	if(argslen==2 && asAtomHandler::isFunction(args[1]))
	{
		ASATOM_INCREF(args[1]);
		th->status = args[1];
	}
}

ASFUNCTIONBODY_ATOM(Responder, onResult)
{
	Responder* th=Class<Responder>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen==1);
	asAtom arg0 = args[0];
	asAtomHandler::callFunction(th->result,ret,asAtomHandler::nullAtom, &arg0, argslen,false);
	ASATOM_DECREF(ret);
}

LocalConnection::LocalConnection(Class_base* c):
	EventDispatcher(c),isSupported(false),client(NULL)
{
}

void LocalConnection::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowInsecureDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("domain","",Class<IFunction>::getFunction(c->getSystemState(),domain),GETTER_METHOD,true);
	REGISTER_GETTER(c,isSupported);
	REGISTER_GETTER_SETTER(c,client);
}
ASFUNCTIONBODY_GETTER(LocalConnection, isSupported);
ASFUNCTIONBODY_GETTER_SETTER(LocalConnection, client);

ASFUNCTIONBODY_ATOM(LocalConnection,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	LocalConnection* th=Class<LocalConnection>::cast(asAtomHandler::getObject(obj));
	th->incRef();
	th->client = _NR<LocalConnection>(th);
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, domain)
{
	tiny_string res = sys->mainClip->getOrigin().getHostname();
	if (sys->flashMode == SystemState::AIR)
		LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::domain is not implemented for AIR mode");
	
	if (res.empty())
		res = "localhost";
	ret = asAtomHandler::fromString(sys,res);
}
ASFUNCTIONBODY_ATOM(LocalConnection, allowDomain)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::allowDomain is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, allowInsecureDomain)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::allowInsecureDomain is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, send)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::send is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, connect)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::connect is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, close)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::close is not implemented");
}

NetGroup::NetGroup(Class_base* c):
	EventDispatcher(c)
{
}

void NetGroup::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(NetGroup,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	//NetGroup* th=Class<NetGroup>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"NetGroup is not implemented");
}

FileReference::FileReference(Class_base* c):
	EventDispatcher(c)
{
}

void FileReference::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(FileReference,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	//FileReference* th=Class<FileReference>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"FileReference is not implemented");
}

FileFilter::FileFilter(Class_base* c):
	ASObject(c)
{
}

void FileFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,description);
	REGISTER_GETTER_SETTER(c,extension);
	REGISTER_GETTER_SETTER(c,macType);
}
ASFUNCTIONBODY_GETTER_SETTER(FileFilter, description);
ASFUNCTIONBODY_GETTER_SETTER(FileFilter, extension);
ASFUNCTIONBODY_GETTER_SETTER(FileFilter, macType);

ASFUNCTIONBODY_ATOM(FileFilter,_constructor)
{
	FileFilter* th = asAtomHandler::as<FileFilter>(obj);
	ARG_UNPACK_ATOM(th->description)(th->extension)(th->macType,"");
}

DRMManager::DRMManager(Class_base* c):
	EventDispatcher(c),isSupported(false)
{
}

void DRMManager::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	REGISTER_GETTER(c,isSupported);
}
ASFUNCTIONBODY_GETTER(DRMManager, isSupported);

ASFUNCTIONBODY_ATOM(lightspark,registerClassAlias)
{
	assert_and_throw(argslen==2 && asAtomHandler::isString(args[0]) && asAtomHandler::isClass(args[1]));
	const tiny_string& arg0 = asAtomHandler::toString(args[0],sys);
	ASATOM_INCREF(args[1]);
	_R<Class_base> c=_MR(asAtomHandler::as<Class_base>(args[1]));
	sys->aliasMap.insert(make_pair(arg0, c));
}

ASFUNCTIONBODY_ATOM(lightspark,getClassByAlias)
{
	assert_and_throw(argslen==1 && asAtomHandler::isString(args[0]));
	const tiny_string& arg0 = asAtomHandler::toString(args[0],sys);
	auto it=sys->aliasMap.find(arg0);
	if(it==sys->aliasMap.end())
		throwError<ReferenceError>(kClassNotFoundError, arg0);
	it->second->incRef();
	ret = asAtomHandler::fromObject(it->second.getPtr());
}
