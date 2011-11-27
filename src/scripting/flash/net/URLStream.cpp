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
#include "URLStream.h"
#include "flash/net/flashnet.h"
#include "backends/security.h"
#include "argconv.h"

/**
 * TODO: This whole class shares a lot of code with URLLoader - unify!
 * TODO: This whole class is quite untested
 */

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.net");

REGISTER_CLASS_NAME(URLStream);

void URLStream::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(bytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(readByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(readDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(readFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(readUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(readUTFBytes),NORMAL_METHOD,true);
}

void URLStream::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URLStream,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(URLStream,load)
{
	URLStream* th=obj->as<URLStream>();
	_NR<URLRequest> urlRequest;
	ARG_UNPACK (urlRequest);

	th->url=urlRequest->getRequestURL();

	if(th->downloader)
	{
		LOG(LOG_NOT_IMPLEMENTED,"URLStream::load called with download already running - ignored");
		return NULL;
	}

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getSys()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::EVALUATIONRESULT evaluationResult =
		getSys()->securityManager->evaluateURLStatic(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
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

	urlRequest->getPostData(th->postData);

	//To be decreffed in jobFence
	th->incRef();
	getSys()->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(URLStream,close)
{
	obj->as<URLStream>()->threadAbort();
	return NULL;
}

void URLStream::jobFence()
{
	decRef();
}

void URLStream::execute()
{
	//TODO: support httpStatus, progress, open events

	//Check for URL policies and send SecurityErrorEvent if needed
	SecurityManager::EVALUATIONRESULT evaluationResult = getSys()->securityManager->evaluatePoliciesURL(url, true);
	if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
	{
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<SecurityErrorEvent>::getInstanceS("SecurityError: URLStream::load: "
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
			downloader=getSys()->downloadManager->download(url, false, NULL);
		}
		else
		{
			downloader=getSys()->downloadManager->downloadWithData(url, postData, NULL);
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
			data = _MR(Class<ByteArray>::getInstanceS());
			data->acquireBuffer(buf,downloader->getLength());
			//The buffers must not be deleted, it's now handled by the ByteArray instance
			//Send a complete event for this object
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
		}
		else
		{
			//Notify an error during loading
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
		}
	}
	else
	{
		//Notify an error during loading
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS()));
	}

	{
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
}

void URLStream::threadAbort()
{
	SpinlockLocker l(downloaderLock);
	if(downloader != NULL)
		downloader->stop();
}

void URLStream::finalize()
{
	EventDispatcher::finalize();
	data.reset();
}

ASFUNCTIONBODY(URLStream,bytesAvailable) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::_getBytesAvailable(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readByte) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readByte(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readBytes) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readBytes(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readDouble) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readDouble(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readFloat) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readFloat(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readInt) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readInt(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUnsignedInt) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUnsignedInt(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readObject) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readObject(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUTF) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUTF(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUTFBytes) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUTFBytes(th->data.getPtr(), args, argslen);
}
