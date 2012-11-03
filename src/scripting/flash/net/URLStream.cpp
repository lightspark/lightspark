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

#include <map>
#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/argconv.h"

/**
 * TODO: This whole class shares a lot of code with URLLoader - unify!
 * TODO: This whole class is quite untested
 */

using namespace std;
using namespace lightspark;

URLStreamThread::URLStreamThread(_R<URLRequest> request, _R<URLStream> ldr, _R<ByteArray> bytes)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr), data(bytes)
{
}

void URLStreamThread::execute()
{
	assert(!downloader);

	//TODO: support httpStatus, progress events

	if(!createDownloader(false, loader))
		return;

	bool success=false;
	if(!downloader->hasFailed())
	{
		getVm()->addEvent(loader,_MR(Class<Event>::getInstanceS("open")));
		downloader->waitForTermination();
		if(!downloader->hasFailed() && !threadAborting)
		{
			istream s(downloader);
			uint8_t* buf=new uint8_t[downloader->getLength()];
			//TODO: avoid this useless copy
			s.read((char*)buf,downloader->getLength());
			//TODO: test binary data format
			data->acquireBuffer(buf,downloader->getLength());
			//The buffers must not be deleted, it's now handled by the ByteArray instance
			success=true;
		}
	}

	// Don't send any events if the thread is aborting
	if(success && !threadAborting)
	{
		//Send a complete event for this object
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

void URLStream::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(bytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(_getEndian),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("readBoolean","",Class<IFunction>::getFunction(readBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(readByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(readDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(readFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readMultiByte","",Class<IFunction>::getFunction(readMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readShort","",Class<IFunction>::getFunction(readShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedByte","",Class<IFunction>::getFunction(readUnsignedByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(readUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedShort","",Class<IFunction>::getFunction(readUnsignedShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(readUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(readUTFBytes),NORMAL_METHOD,true);

	c->addImplementedInterface(InterfaceClass<IDataInput>::getClass());
	IDataInput::linkTraits(c);
}

void URLStream::buildTraits(ASObject* o)
{
}

void URLStream::threadFinished(IThreadJob* finishedJob)
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

	{
		SpinlockLocker l(th->spinlock);
		if(th->job)
			th->job->threadAbort();
	}

	th->url=urlRequest->getRequestURL();
	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getSys()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::checkURLStaticAndThrow(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	th->incRef();
	URLStreamThread *job=new URLStreamThread(urlRequest, _MR(th), th->data);
	getSys()->addJob(job);
	th->job=job;
	return NULL;
}

ASFUNCTIONBODY(URLStream,close)
{
	URLStream* th=static_cast<URLStream*>(obj);
 	SpinlockLocker l(th->spinlock);
	if(th->job)
		th->job->threadAbort();

	return NULL;
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

ASFUNCTIONBODY(URLStream,_getEndian) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::_getEndian(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,_setEndian) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::_setEndian(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,_getObjectEncoding) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::_getObjectEncoding(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,_setObjectEncoding) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::_setObjectEncoding(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readBoolean) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readBoolean(th->data.getPtr(), args, argslen);
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

ASFUNCTIONBODY(URLStream,readMultiByte) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readMultiByte(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readObject) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readObject(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readShort) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readShort(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUnsignedByte) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUnsignedByte(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUnsignedInt) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUnsignedInt(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUnsignedShort) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUnsignedShort(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUTF) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUTF(th->data.getPtr(), args, argslen);
}

ASFUNCTIONBODY(URLStream,readUTFBytes) {
	URLStream* th=static_cast<URLStream*>(obj);
	return ByteArray::readUTFBytes(th->data.getPtr(), args, argslen);
}
