/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "backends/security.h"
#include "parsing/streams.h"
#include "scripting/flash/display_object/Loader.h"
#include "scripting/flash/display_object/RootMovieClip.h"

using namespace std;
using namespace lightspark;

LoaderThread::LoaderThread
(
	const URLRequest& request,
	Loader& _loader
) : DownloaderThreadBase(&request, _loader),
loader(_loader),
loaderInfo(_loader.getContentLoaderInfo()),
source(URL)
{
}

LoaderThread::LoaderThread
(
	Span<uint8_t> _bytes,
	Loader& _loader
) : DownloaderThreadBase(nullptr, _loader),
bytes(_bytes),
loader(_loader),
loaderInfo(_loader.getContentLoaderInfo()),
source(BYTES)
{
}

void LoaderThread::execute()
{
	assert(source == URL || source == BYTES);

	std::streambuf* sbuf = nullptr;
	if (source == URL)
	{
		auto cache = _MR(new MemoryStreamCache(loader.getSys()));
		loaderInfo->incRef();
		if(!createDownloader(cache, _MR(loaderInfo), loaderInfo, false))
			return;

		sbuf = cache->createReader();

		// Wait for some data, making sure our check for failure is working
		sbuf->sgetc(); // peek one byte
		if (downloader->hasEmptyAnswer())
		{
			LOG(LOG_INFO,"empty answer:" << url);
			return;
		}

		if (cache->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG
			(
				LOG_ERROR,
				"Loader::execute(): "
				"Download of URL failed: " << url
			);
			auto vm = getVm(loader->getSys());
			auto ev = Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker());
			loaderInfo->incRef();
			vm->addEvent(_MR(loaderInfo), _MR(ev));
			vm->addEvent
			(
				_MR(loader),
				_MR(Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker()))
			);
			delete sbuf;
			// downloader will be deleted in jobFence
			return;
		}
		loaderInfo->setBytesTotal(downloader->getLength());
		loaderInfo->setBytesLoaded(downloader->getReceivedLength());
		loaderInfo->setOpened(false);
	}
	else if (source == BYTES)
	{
		assert_and_throw(bytes.getData() != nullptr);
		loaderInfo->setBytesTotal(bytes.getSize());
		loaderInfo->setOpened(true);
		sbuf = new bytes_buf(bytes);

// extract embedded swf to separate file
//		char* name_used=nullptr;
//		int fd = g_file_open_tmp("lightsparkXXXXXX.swf",&name_used,nullptr);
//		write(fd,bytes->bytes,bytes->getLength());
//		close(fd);
//		g_free(name_used);
	}
	loaderInfo->parseData(sbuf);

	if (source == URL)
	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		if(downloader)
			loaderInfo->getSystemState()->downloadManager->destroy(downloader);
		downloader=nullptr;
	}

	auto ret = loaderInfo->getParsedObject();

	// The stream did not contain RootMovieClip or Bitmap
	if (ret == nullptr && !threadAborting)
	{
		auto ev = Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker());
		loaderInfo->incRef();
		getVm(loader.getSys()())->addEvent(_MR(loaderInfo),_MR(ev));
		return;
	}
	else if (ret == nullptr)
		return;

	auto _root = ret->as<RootMovieClip>();
	if (_root == nullptr || !_root->hasFinishedLoading())
		return;

	if (_root->isAS3() && !_root->hasMainClass)
		loaderInfo->setComplete();

	_root->AVM1setLevel(loader.AVM1getLevel());
}

void LoaderThread::jobFence()
{
	auto vm = getVm(loader.getSys());
	if (vm != nullptr)
	{
		vm->addDeletableObject(loader);
		vm->addDeletableObject(loaderInfo);
	}
	DownloaderThreadBase::jobFence();
}

void Loader::close()
{
	Locker l(spinlock);
	for (auto job : jobs)
		job->threadAbort();
}

void Loader::load(const URLRequest& req, LoaderContext* ctx)
{
	unload();
	loadIntern(req, ctx);
}

void Loader::loadIntern
(
	const URLRequest& req,
	LoaderContext* ctx,
	DisplayObject* _avm1Target
)
{
	if (_avm1Target != nullptr)
		avm1Target = _avm1Target;

	checkContentLoaderInfo();
	url = req.getRequestURL();
	contentLoaderInfo->setURL(url.getParsedURL());
	contentLoaderInfo->setAVM1Target(avm1Rarget);
	contentLoaderInfo->resetState();
	//Check if a security domain has been manually set
	SecurityDomain* secDomain = nullptr;
	auto curSecDomain = getAVM2Root()->securityDomain.getPtr();
	if (ctx != nullptr)
	{
		auto ctxDomain = ctx->securityDomain;
		//The passed domain must be the current one. See Loader::load specs.
		if (ctxDomain != nullptr && ctxDomain != curSecDomain)
		{
			createError<SecurityError>
			(
				this->getInstanceWorker(),
				0,
				"SecurityError: "
				"securityDomain must be current one"
			);
			return;
		}
		else if (ctxDomain != nullptr)
			secDomain = curSecDomain;

		bool sameDomain = secDomain == curSecDomain;
		allowCodeImport = !sameDomain || ctx->getAllowCodeImport();

		if (!ctx->parameters.isNull())
			contentLoaderInfo->setParameters(ctx->parameters);
	}
	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	auto appDomain =
	(
		loadedFrom != nullptr ?
		loadedFrom :
		getAVM2Root()->applicationDomain.getPtr()
	);

	// empty origin is possible if swf is loaded by loadBytes()
	auto origin = appDomain->getOrigin();
	if
	(
		origin.isEmpty() ||
		origin.getHostname() == url.getHostname() ||
		secDomain != nullptr
	)
	{
		//Same domain
		auto parentDomain =
		(
			loadedFrom != nullptr ?
			loadedFrom :
			nullptr
		);
		if
		(
			parentDomain == nullptr &&
			getInstanceWorker()->currentCallContext != nullptr
		)
		{
			parentDomain = ABCVm::getCurrentApplicationDomain
			(
				getInstanceWorker()->currentCallContext
			);
		}

		if (parentDomain != nullptr)
			parentDomain->incRef();
		//Support for LoaderContext
		if (ctx == nullptr || ctx->applicationDomain.isNull())
		{
			contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS
			(
				getInstanceWorker(),
				_MNR(parentDomain)
			));
		}
		else
			contentLoaderInfo->applicationDomain = ctx->applicationDomain;
		curSecDomain->incRef();
		contentLoaderInfo->securityDomain = _MNR(curSecDomain);
	}
	else
	{
		//Different domain
		auto parentDomain = _MR(getSys()->systemDomain);
		contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS
		(
			getInstanceWorker(),
			parentDomain
		));
		contentLoaderInfo->securityDomain = _MR(Class<SecurityDomain>::getInstanceS
		(
			getInstanceWorker()
		));
	}

	if(!this->url.isValid())
	{
		//Notify an error during loading
		getSys()->currentVm->addEvent
		(
			_MR(this),
			_MR(Class<IOErrorEvent>::getInstanceS
			(
				getInstanceWorker()
			))
		);
		return;
	}

	SecurityManager::checkURLStaticAndThrow
	(
		url,
		~SecurityManager::LOCAL_WITH_FILE,
		(
			SecurityManager::LOCAL_WITH_FILE |
			SecurityManager::LOCAL_TRUSTED
		),
		true
	);

	auto callCtx = getInstanceWorker()->currentCallContext;
	if (callCtx != nullptr && callCtx->exceptionthrown != nullptr)
		return;

	if (ctx != nullptr && ctx->getCheckPolicyFile())
	{
		auto secMgr = getSys()->securityManager;
		//TODO: this should be async as it could block if invoked from ExternalInterface
		auto evalRet = secMgr->evaluatePoliciesURL(url, true);
		if (evalRet == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			// should this dispatch SecurityErrorEvent instead of throwing?
			createError<SecurityError>
			(
				getInstanceWorker(),
				0,
				"SecurityError: "
				"connection to domain not allowed by "
				"securityManager"
			);
			return;
		}
	}
	contentLoaderInfo->setStarted();
	auto thread = new LoaderThread(req, *this);
	auto unaccountedMem = getSys()->unaccountedMemory;
	getVm(getSys())->addEvent(NullRef, _MR
	(
		new (unaccountedMem) StartJobEvent(thread)
	));
	jobs.push_back(thread);
}

void Loader::loadBytes(Span<uint8_t> bytes, LoaderContext* ctx)
{
	unload();

	auto parentDomain = ABCVm::getCurrentApplicationDomain(wrk->currentCallContext);
	if (parentDomain != nullptr)
		parentDomain->incRef();
	contentLoaderInfo->applicationDomain =
	(
		ctx != nullptr &&
		!ctx->applicationDomain.isNull()
	) ? ctx->applicationDomain : _MR(Class<ApplicationDomain>::getInstanceS
	(
		wrk,
		_MNR(parentDomain)
	));

	//Always loaded in the current security domain
	auto curSecDomain = ABCVm::getCurrentSecurityDomain(wrk->currentCallContext);
	if (curSecDomain != nullptr)
		curSecDomain->incRef();
	contentLoaderInfo->securityDomain = _MNR(curSecDomain);

	allowCodeImport = ctx == nullptr || ctx->getAllowCodeImport();

	if (ctx != nullptr && !ctx->parameters.isNull())
		contentLoaderInfo->setParameters(ctx->parameters);

	if (bytes.empty())
	{
		LOG
		(
			LOG_INFO,
			"Empty `Span` passed to `Loader::loadBytes()`"
		);
		return;
	}

	// better work on a copy of the source data as it may be modified by actionscript before loading is completed
	std::vector<uint8_t> data(bytes.begin, bytes.end());

	auto thread = new LoaderThread(makeSpan(data), *this);
	if (isVmThread())
	{
		thread->execute();
		thread->jobFence();
		return;
	}

	auto vm = getVm(getSys());
	Locker l(spinlock);
	jobs.push_back(thread);
	vm->addEvent(NullRef, _MR(new
	(
		getSys()->unaccountedMemory
	) StartJobEvent(thread)));
}

void Loader::unload()
{
	close();

	auto contentCopy = content;
	content = nullptr;

	if (loaded)
	{
		auto li = getContentLoaderInfo();
		li->incRef();

		getVm(getSys())->addEvent
		(
			_MR(li),
			_MR(Class<Event>::getInstanceS
			(
				getInstanceWorker(),
				"unload"
			))
		);
		loaded = false;
	}

	// removeChild may execute AS code, release the lock before
	// calling
	if(contentCopy != nullptr)
		removeChild(contentCopy);

	if (contentLoaderInfo != nullptr)
		contentLoaderInfo->resetState();
}

Loader::Loader(SystemState* sys, SWFMovie& _movie) : InteractiveObject
(
	Type::Loader,
	sys
),
DisplayObjectContainer(_movie),
content(nullptr),
loaded(false),
allowCodeImport(true)
avm1level(-1),
avm1Target(nullptr),
avm1container(nullptr)
{
}

void Loader::threadFinished(IThreadJob* finishedJob)
{
	Locker l(spinlock);
	jobs.remove(finishedJob);
	delete finishedJob;
}

#define LOADER_CONTENT_LEGACY_DEPTH -16384-0xF000
void Loader::setContent(DisplayObject& obj)
{
	if
	(
		obj.isOnStage() ||
		content == &obj ||
		(content != nullptr && obj.getParent() == content)
	)
		return;

	{
		Locker l(mutexDisplayList);
		clearDisplayList();
	}

	content = [&]
	{
		Locker l(spinlock);
		bool addAVM1Movie =
		(
			isAS3() &&
			obj.is<RootMovieClip>() &&
			&obj != getSys()->mainClip &&
			!obj.isAS3()
		);

		loaded = true;
		if (!addAVM1Movie)
		{
			obj.isLoadedRoot = true;
			return &obj;
		}

		auto m = Class<AVM1Movie>::getInstanceS(getInstanceWorker());
		m->setIsInitialized();
		m->setConstructIndicator();
		m->setLoaderInfo(loaderInfo);
		m->insertChildAt
		(
			16384 + 0xf000,
			obj,
			false,
			false
		);
		m->isLoadedRoot = true;
		return m;
	}();

	if (avm1Target == nullptr)
	{
		addChildAt(*content, 0);
		goto end;
	}

	// _addChild may cause AS code to run, release locks beforehand.
	obj.tx = avm1Target->tx;
	obj.ty = avm1Target->ty;
	obj.tz = avm1Target->tz;
	obj.rotation = avm1Target->rotation;
	obj.sx = avm1Target->sx;
	obj.sy = avm1Target->sy;
	obj.sz = avm1Target->sz;
	obj.name = avm1Target->name;

	if (avm1Target->getParent() != nullptr)
	{
		auto parent = avm1Target->getParent();
		auto depth =
		(
			avm1level < 0 ?
			avm1Target->getDepth() :
			avm1level
		);

		auto container = avm1Target->as<DisplayObjectContainer>();
		if (container != nullptr)
			container->removeAllChildren(true, true);

		if (parent->is<Stage>() && avm1level < 0)
			parent->removeChild(*avm1Target);
		else
			parent->deleteChildAt(depth, false);
		parent->insertChildAt(depth, obj, false, false);
	}

	avm1target = nullptr;
end:
	if (obj.loaderInfo != nullptr)
		obj.loaderInfo->setComplete();
}

LoaderInfo* Loader::getContentLoaderInfo()
{
	return contentLoaderInfo;
}

void Loader::checkContentLoaderInfo()
{
	if (contentLoaderInfo != nullptr)
		return;
	contentLoaderInfo = Class<LoaderInfo>::getInstanceS
	(
		getInstanceWorker(),
		this
	);
}

void Loader::AVM1setup(int level, ASObject* container)
{
	avm1level = level;
	assert(avm1container == nullptr);
	if (container == nullptr)
		return;

	// the container (AVM1MovieClipLoader) is set here to ensure it is kept alive until this Loader is destroyed,
	// as it may have event handlers that need to be executed
	avm1container = container;
}
