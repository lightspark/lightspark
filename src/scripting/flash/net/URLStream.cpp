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

#include <map>
#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/argconv.h"

/**
 * TODO: This whole class shares a lot of code with URLLoader - unify!
 * TODO: This whole class is quite untested
 */

using namespace std;
using namespace lightspark;

URLStreamThread::URLStreamThread(_R<URLRequest> request, _R<URLStream> ldr, _R<ByteArray> bytes)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr), data(bytes),streambuffer(NULL),timestamp_last_progress(0),bytes_total(0)
{
}

void URLStreamThread::setBytesTotal(uint32_t b)
{
	bytes_total = b;
}
void URLStreamThread::setBytesLoaded(uint32_t b)
{
	uint32_t curlen = data->getLength();
	if(b>curlen && streambuffer)
	{
		data->append(streambuffer,b - curlen);
		uint64_t cur=compat_get_thread_cputime_us();
		if (cur > timestamp_last_progress+ 40*1000)
		{
			timestamp_last_progress = cur;
			loader->incRef();
			getVm(loader->getSystemState())->addEvent(loader,_MR(Class<ProgressEvent>::getInstanceS(loader->getInstanceWorker(),b,bytes_total)));
		}
	}
}

void URLStreamThread::execute()
{
	timestamp_last_progress = compat_get_thread_cputime_us();
	assert(!downloader);

	//TODO: support httpStatus

	_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache(loader->getSystemState())));
	if(!createDownloader(cache, loader,this))
		return;
	data->setLength(0);

	bool success=false;
	if(!downloader->hasFailed())
	{
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<Event>::getInstanceS(loader->getInstanceWorker(),"open")));
		streambuffer = cache->createReader();
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<ProgressEvent>::getInstanceS(loader->getInstanceWorker(),0,bytes_total)));
		cache->waitForTermination();
		if(!downloader->hasFailed() && !threadAborting)
		{
			/*
			std::streambuf *sbuf = cache->createReader();
			istream s(sbuf);
			uint8_t* buf=new uint8_t[downloader->getLength()];
			//TODO: avoid this useless copy
			s.read((char*)buf,downloader->getLength());
			//TODO: test binary data format
			data->acquireBuffer(buf,downloader->getLength());
			//The buffers must not be deleted, it's now handled by the ByteArray instance
			delete sbuf;
			*/
			success=true;
		}
	}

	// Don't send any events if the thread is aborting
	if(success && !threadAborting)
	{
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<ProgressEvent>::getInstanceS(loader->getInstanceWorker(),downloader->getLength(),downloader->getLength())));
		//Send a complete event for this object
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<Event>::getInstanceS(loader->getInstanceWorker(),"complete")));
	}
	else if(!success && !threadAborting)
	{
		//Notify an error during loading
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker())));
	}

	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		getSys()->downloadManager->destroy(downloader);
		downloader = NULL;
	}
}

void URLStream::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(c->getSystemState(),bytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_getEndian),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("readBoolean","",Class<IFunction>::getFunction(c->getSystemState(),readBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(c->getSystemState(),readByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(c->getSystemState(),readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(c->getSystemState(),readDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(c->getSystemState(),readFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(c->getSystemState(),readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readMultiByte","",Class<IFunction>::getFunction(c->getSystemState(),readMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(c->getSystemState(),readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readShort","",Class<IFunction>::getFunction(c->getSystemState(),readShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedByte","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedShort","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(c->getSystemState(),readUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(c->getSystemState(),readUTFBytes),NORMAL_METHOD,true);
	REGISTER_GETTER(c,connected);

	c->addImplementedInterface(InterfaceClass<IDataInput>::getClass(c->getSystemState()));
	IDataInput::linkTraits(c);
}

ASFUNCTIONBODY_GETTER(URLStream,connected)

void URLStream::buildTraits(ASObject* o)
{
}

void URLStream::threadFinished(IThreadJob* finishedJob)
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

ASFUNCTIONBODY_ATOM(URLStream,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj,NULL,0);
}

ASFUNCTIONBODY_ATOM(URLStream,load)
{
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	_NR<URLRequest> urlRequest;
	ARG_CHECK(ARG_UNPACK (urlRequest));

	{
		Locker l(th->spinlock);
		if(th->job)
			th->job->threadAbort();
	}

	th->url=urlRequest->getRequestURL();
	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		wrk->getSystemState()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(wrk)));
		return;
	}

	//TODO: support the right events (like SecurityErrorEvent)
	//URLLoader ALWAYS checks for policy files, in contrast to NetStream.play().
	SecurityManager::checkURLStaticAndThrow(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	th->incRef();
	URLStreamThread *job=new URLStreamThread(urlRequest, _MR(th), th->data);
	getSys()->addJob(job);
	th->job=job;
	th->connected = true;
}

ASFUNCTIONBODY_ATOM(URLStream,close)
{
	URLStream* th=asAtomHandler::as<URLStream>(obj);
 	Locker l(th->spinlock);
	if(th->job)
		th->job->threadAbort();
	th->connected = false;
}

void URLStream::finalize()
{
	EventDispatcher::finalize();
	data.reset();
}

URLStream::URLStream(ASWorker* wrk, Class_base *c):EventDispatcher(wrk,c),data(_MNR(Class<ByteArray>::getInstanceS(wrk))),job(NULL),connected(false) 
{
}

ASFUNCTIONBODY_ATOM(URLStream,bytesAvailable) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::_getBytesAvailable(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,_getEndian) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::_getEndian(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,_setEndian) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::_setEndian(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,_getObjectEncoding) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::_getObjectEncoding(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,_setObjectEncoding) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::_setObjectEncoding(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readBoolean) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readBoolean(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readByte) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readByte(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readBytes) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readBytes(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readDouble) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readDouble(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readFloat) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readFloat(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readInt) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readInt(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readMultiByte) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readMultiByte(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readObject) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readObject(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readShort) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readShort(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readUnsignedByte) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readUnsignedByte(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readUnsignedInt) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readUnsignedInt(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readUnsignedShort) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readUnsignedShort(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readUTF) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readUTF(ret,wrk,v, args, argslen);
}

ASFUNCTIONBODY_ATOM(URLStream,readUTFBytes) {
	URLStream* th=asAtomHandler::as<URLStream>(obj);
	asAtom v = asAtomHandler::fromObject(th->data.getPtr());
	ByteArray::readUTFBytes(ret,wrk,v, args, argslen);
}
