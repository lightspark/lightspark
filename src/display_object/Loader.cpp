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
#include "display_object/Loader.h"
#include "display_object/RootMovieClip.h"
#include "scripting/avm1/movieclip_ref.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

LoaderThread::LoaderThread
(
	const URLRequest& request,
	Loader& _loader,
	LoaderData& _data
) :
DownloaderThreadBase(&request, _loader),
loader(_loader),
data(_data),
isBytes(false)
{
}

LoaderThread::LoaderThread
(
	Span<uint8_t> _bytes,
	Loader& _loader
	LoaderData& _data
) :
DownloaderThreadBase(nullptr, _loader),
bytes(_bytes),
loader(_loader),
data(_data),
isBytes(false)
{
}

std::streambuf* LoaderThread::getStreamBuf()
{
	if (isBytes)
	{
		assert_and_throw(bytes.getData() != nullptr);
		loader.onProgress(data, bytes.getSize(), bytes.getSize());
		return new bytes_buf(bytes);

// extract embedded swf to separate file
//		char* name_used=nullptr;
//		int fd = g_file_open_tmp("lightsparkXXXXXX.swf",&name_used,nullptr);
//		write(fd,bytes->bytes,bytes->getLength());
//		close(fd);
//		g_free(name_used);
	}

	auto cache = _MR(new MemoryStreamCache(loader.getSys()));
	if (!createDownloader(cache, loader, data, false))
		return nullptr;

	auto ret = cache->createReader();

	// Wait for some data, making sure our check for failure is working
	ret->sgetc(); // peek one byte
	if (downloader->hasEmptyAnswer())
	{
		LOG(LOG_INFO, "empty answer:" << url);
		return nullptr;
	}

	if (!cache->hasFailed())
	{
		loader.onProgress
		(
			data,
			downloader->getLength(),
			downloader->getReceivedLength()
		);
		loader.onStart(data);
		return ret;
	}

	// The download failed for some reason.
	LOG
	(
		LOG_ERROR,
		"Loader::getStreamBuf(): Download of URL failed: " << url
	);

	loader.onError
	(
		data,
		"Movie loader error",
		"LoadNeverCompleted",
		downloader->getRequestStatus(),
		downloader->isRedirected(),
		downloader->getURL()
	);

	delete ret;
	// downloader will be deleted in jobFence
	return nullptr;
}

void LoaderThread::execute()
{
	auto streamBuf = getStreamBuf();
	if (streamBuf == nullptr)
		return;

	loader.parseData(data, streamBuf);

	if (!isBytes)
	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		if (downloader != nullptr)
			loaderInfo.getSys()->downloadManager->destroy(downloader);
		downloader = nullptr;
	}

	auto ret = loader.getContent();

	// The stream did not contain RootMovieClip or Bitmap
	if (ret == nullptr && !threadAborting)
	{
		loader.onError
		(
			data,
			"The stream doesn't contain a `RootMovieClip`, or `Bitmap`",
			"LoadNeverCompleted",
			status,
			redirected,
			_url
		);
		return;
	}
	else if (ret == nullptr)
		return;

	auto _root = ret->as<RootMovieClip>();
	if (_root == nullptr || !_root->hasFinishedLoading())
		return;

	if (_root->isAS3() && !_root->hasMainClass)
		loader.setComplete(data);
}

void LoaderThread::jobFence()
{
	auto vm = getVm(loader.getSys());
	if (vm != nullptr && data.isAVM2())
	{
		auto& data = static_cast<ASLoaderData&>(data);
		auto obj = loader.toASObject();
		if (!obj.isNull())
			vm->addDeletableObject(obj);
		vm->addDeletableObject(data.getLoaderInfo());
	}
	DownloaderThreadBase::jobFence();
}

template<typename... Args>
static void sendBroadcastMsg
(
	SystemState* sys,
	AVM1Activation& act,
	_NGC<AVM1Object> broadcaster,
	_GC<AVM1MovieClipRef> target,
	const tiny_string& name,
	Args&&... args
)
{
	if (broadcaster.isNull())
		return;

	sys->queueActionBack(act, target, MethodAction
	(
		broadcaster,
		"broadcastMessage",
		makeSpan
		({
			AVM1Value(name),
			AVM1Value(target),
			AVM1Value(args)...
		});
	));
}

template<typename T = Event, typename... Args>
static void sendEvent(SystemState* sys, _R<ASObject> obj, Args&&... args)
{
	auto wrk = obj->getInstanceWorker();
	auto vm = getVm(sys);
	vm->tryAddEvent(obj, _MR(Class<T>::getInstanceS(wrk, args...)));
}

void Loader::parseData(LoaderData& data, std::streambuf* _streamBuf)
{
	streamBuf = _streamBuf;
	std::istream s(streamBuf);

	parseThread = new ParseThread(s, *this, data, url);
	parseThread->execute();
}

void Loader::setOpened(LoaderData& loaderData, bool fromBytes)
{
	if (loadStatus >= LoadStatus::Opened)
		return;

	loadStatus =
	(
		fromBytes ?
		LoadStatus::DownloadDone :
		LoadStatus::Opened
	);

	if (!fromBytes)
		onStart(loaderData);

	// it seems an additional ProgressEvent is always added at the start of loading (see ruffle test avm2/large_preload_from_*)
	if (isAS3() || loaderData.isAVM2())
		onProgress(loaderData, 0, bytesTotal);
}

void Loader::setComplete(LoaderData& loaderData)
{
	Locker l(mutex);
	if (!isAS3() || loaderData.isAVM1())
		clipLoaded(loaderData);

	if (loadStatus >= LoadStatus::InitSent)
		sendInit(loaderData);
}

void Loader::sendInit(LoaderData& loaderData)
{
	auto vm = getVm(sys);
	// loader.content has to be set before "init" event is dispatched
	if (content != nullptr)
	{
		auto mem = sys->unaccountedMemory;
		// we have a loader, so it is not the main clip
		vm->tryAddEvent(NullRef, _MR(new (mem) SetLoaderContentEvent
		(
			*content,
			*this,
			loaderData
		)));
	}

	if (loaderData.isAVM2())
	{
		auto& data = static_cast<ASLoaderData&>(loaderData);
		sendEvent(sys, data.getLoaderInfo(), "init");
	}
	assert(loadStatus < LoadStatus::InitSent);
	loadStatus = LoadStatus::InitSent;
	checkSendComplete(loaderData);
}

void LoaderInfo::checkSendComplete(LoaderData& loaderData)
{
	if
	(
		loadStatus != LoadStatus::InitSent ||
		!movie.getBytesTotal() ||
		movie.getBytesLoaded() != movie.getBytesTotal()
	)
		return;

	//The clip is also complete now
	onComplete(loaderData, 0, false);
	loadStatus = LoadStatus::Complete;
}

void Loader::onStart(LoaderData& loaderData)
{
	loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data)
		{
			sendBroadcastMsg
			(
				sys,
				data.getAct(),
				data.getBroadcaster(),
				data.getTarget(),
				"onLoadStart"
			);
		},
		[&](const ASLoaderData& data)
		{
			sendEvent(sys, data.getLoaderInfo(), "open");
		}
	));
}

void Loader::onProgress
(
	LoaderData& loaderData,
	size_t bytesLoaded,
	size_t bytesTotal
)
{
	loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data)
		{
			sendBroadcastMsg
			(
				sys,
				data.getAct(),
				data.getBroadcaster(),
				data.getTarget(),
				"onLoadProgress"
				bytesLoaded,
				bytesTotal
			);
		},
		[&](const ASLoaderData& data)
		{
			sendEvent<ProgressEvent>
			(
				sys,
				data.getLoaderInfo(),
				bytesLoaded,
				bytesTotal
			);
		}
	));
}

void Loader::onComplete
(
	LoaderData& loaderData,
	uint16_t status,
	bool redirected
)
{
	loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data)
		{
			sendBroadcastMsg
			(
				sys,
				data.getAct(),
				data.getBroadcaster(),
				data.getTarget(),
				"onLoadComplete",
				status
			);
		},
		[&](const ASLoaderData& data)
		{
			auto obj = data.getLoaderInfo();
			if (!url.empty())
				sendEvent<HTTPStatusEvent>(sys, obj);
			sendEvent(sys, obj, "complete");
		}
	));
}

void Loader::onError
(
	LoaderData& loaderData,
	const tiny_string& msg,
	const tiny_string& errorCode,
	uint16_t _status,
	bool redirected
)
{
	loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data)
		{
			sendBroadcastMsg
			(
				sys,
				data.getAct(),
				data.getBroadcaster(),
				data.getTarget(),
				"onLoadError",
				errorCode
			);
		},
		[&](const ASLoaderData& data)
		{
			sendEvent<IOErrorEvent>
			(
				sys,
				data.getLoaderInfo(),
				"ioError",
				msg
			);
		}
	));
}

void Loader::onSecError(LoaderData& loaderData, const tiny_string& msg)
{
	if (loaderData.isAVM1())
	{
		onError(loaderData);
		return;
	}

	auto data = static_cast<ASLoaderData&>(loaderData);
	sendEvent<SecurityErrorEvent>(sys, data.getLoaderInfo(), msg);
}

bool Loader::clipLoaded(LoaderData& loaderData)
{

	if (!loaderData.isAVM1())
		return false;

	if (loadStatus < LoadStatus::InitSent)
	{
		onProgress(bytesTotal, bytesTotal);
		return true;
	}

	auto& data = static_cast<AVM1LoaderData&>(loaderData);
	auto& act = data.getAct();
	auto bcast = data.getBroadcaster();
	auto target = data.getTarget();
	sendBroadcastMsg(sys, act, bcast, target, "onLoadInit");
	return true;
}

void Loader::close()
{
	Locker l(mutex);
	for (auto job : jobs)
		job->threadAbort();
}

bool Loader::checkSecurityDomain(LoaderData& loaderData)
{
	if (!_data.isAVM2())
		return false;

	SecurityDomain* secDomain = nullptr;
	SecurityDomain* curSecDomain = nullptr;
	bool ret = loaderData.visit(makeVisitor
	(
		[&](const ASLoaderData& data)
		{
			if (ctx.isNull())
				return true;
			auto ctx = data.getContext();
			auto loaderInfo = data.getLoaderInfo();
			auto wrk = loaderInfo->getInstanceWorker();
			//Check if a security domain has been manually set
			curSecDomain = getAVM2Root()->securityDomain.getPtr();
			auto ctxDomain = ctx->securityDomain;
			//The passed domain must be the current one. See Loader::load specs.
			if (ctxDomain != nullptr && ctxDomain != curSecDomain)
			{
				createError<SecurityError>
				(
					wrk,
					0,
					"SecurityError: "
					"securityDomain must be current one"
				);
				return false;
			}
			else if (ctxDomain != nullptr)
				secDomain = _MR(curSecDomain);

			bool sameDomain = secDomain == curSecDomain;
			allowCodeImport = !sameDomain || ctx->getAllowCodeImport();

			if (!ctx->parameters.isNull())
				loaderInfo->setParameters(ctx->parameters);
			return true;
		},
		[](const auto&) { return true; }
	));

	if (!ret)
		return false;

	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	if (!loaderData.isAVM2())
		return true;

	auto& data = static_cast<ASLoaderData&>(loaderData);
	auto loaderInfo = data.getLoaderInfo();
	auto wrk = loaderInfo->getInstanceWorker();
	auto defDomain = data.getDefaultDomain();
	auto ctx = data.getContext();
	auto& domain =
	(
		!ctx.isNull() &&
		!ctx->applicationDomain.isNull()
	) ? *ctx->applicationDomain : *data.getDefaultDomain();

	// empty origin is possible if swf is loaded by loadBytes()
	auto origin = domain.getOrigin();
	if
	(
		!origin.isEmpty() &&
		origin.getHostname() != url.getHostname() &&
		secDomain.isNull()
	)
	{
		//Different domain
		loaderInfo->appDomain = _MR(Class<ApplicationDomain>::getInstanceS
		(
			wrk,
			_MR(sys->systemDomain)
		));
		secDomain = _MR(Class<SecurityDomain>::getInstanceS(wrk));
		return true;
	}

	//Same domain
	auto callCtx = wrk->currentCallContext;
	auto parentDomain =
	(
		callCtx != nullptr ?
		ABCVm::getCurrentApplicationDomain(callCtx) :
		defDomain.getPtr()
	);

	if (parentDomain != nullptr)
		parentDomain->incRef();
	//Support for LoaderContext
	loaderInfo->appDomain =
	(
		ctx.isNull() ||
		ctx->applicationDomain.isNull()
	) ? _MR(Class<ApplicationDomain>::getInstanceS
	(
		wrk,
		_MNR(parentDomain)
	)) : ctx->applicationDomain;

	curSecDomain->incRef();
	loaderInfo->securityDomain = _MNR(curSecDomain);
	return true;
}

void Loader::load(const URLRequest& req, LoaderData& data)
{

	url = req.getRequestURL();
	if (!checkSecurityDomain(data))
		return;

	if (!url.isValid())
	{
		//Notify an error during loading
		onError(data, "", "URLNotFound");
		return;
	}

	constexpr auto remoteAllowed = ~SecurityManager::LOCAL_WITH_FILE;
	constexpr auto localAllowed =
	(
		SecurityManager::LOCAL_WITH_FILE |
		SecurityManager::LOCAL_TRUSTED
	);

	auto checkPolicyFile = loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data) -> Optional<bool>
		{
			auto secMgr = sys->securityManager;
			auto evalRet = secMgr->evaluateURLStatic
			(
				url,
				remoteAllowed,
				localAllowed
				true
			);

			auto loader = data.getBroadcaster();
			if (evalRet != ALLOWED)
			{
				onError(sys, loader);
				return {};
			}

			return
			(
				!loader.isNull() &&
				loader->getCheckPolicyFile()
			);
		},
		[&](const ASLoaderData& data) -> Optional<bool>
		{
			SecurityManager::checkURLStaticAndThrow
			(
				url,
				remoteAllowed,
				localAllowed
				true
			);

			auto ctx = data.getContext();
			auto obj = data.getLoaderInfo();
			auto wrk = obj->getInstanceWorker();
			auto callCtx = wrk->currentCallContext;
			if (callCtx != nullptr && callCtx->exceptionthrown != nullptr)
				return {};
			return !ctx.isNull() && ctx->getCheckPolicyFile();
		}
	));

	if (!checkPolicyFile.hasValue())
		return;
	else if (*checkPolicyFile)
	{
		auto secMgr = getSys()->securityManager;
		//TODO: this should be async as it could block if invoked from ExternalInterface
		auto evalRet = secMgr->evaluatePoliciesURL(url, true);
		if (evalRet != SecurityManager::ALLOWED)
		{
			onSecError
			(
				loaderData,
				"SecurityError: "
				"connection to domain not allowed by "
				"securityManager"
			);
			return;
		}
	}

	loadStatus = LoadStatus::Started;
	auto thread = new LoaderThread(req, *this, data);
	auto unaccountedMem = getSys()->unaccountedMemory;
	getVm(getSys())->addEvent(NullRef, _MR
	(
		new (unaccountedMem) StartJobEvent(thread)
	));
	jobs.push_back(thread);
}

void Loader::loadBytes(Span<uint8_t> bytes, ASLoaderData& data)
{
	unload(data);

	auto ctx = data.getContext();
	auto loaderInfo = data.getLoaderInfo();
	auto wrk = loaderInfo->getInstanceWorker();
	auto parentDomain = data.getDefaultDomain();
	parentDomain->incRef();
	loaderInfo->appDomain =
	(
		!ctx.isNull() &&
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
	loaderInfo->secDomain = _MNR(curSecDomain);

	allowCodeImport = ctx == nullptr || ctx->getAllowCodeImport();
	if (ctx != nullptr && !ctx->parameters.isNull())
		loaderInfo->setParameters(ctx->parameters);

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

	auto thread = new LoaderThread(makeSpan(data), *this, data);
	if (isVmThread())
	{
		thread->execute();
		thread->jobFence();
		return;
	}

	auto vm = getVm(getSys());
	Locker l(mutex);
	jobs.push_back(thread);
	vm->addEvent(NullRef, _MR(new
	(
		getSys()->unaccountedMemory
	) StartJobEvent(thread)));
}

void Loader::unload(LoaderData& loaderData)
{
	close();

	auto contentCopy = content;
	content = nullptr;

	if (loaded && loaderData.isAVM2())
	{
		auto data = static_cast<ASLoaderData&>(loaderData);
		sendEvent(sys, data.getLoaderInfo(), "unload");
	}

	loaded = false;

	// removeChild may execute AS code, release the lock before
	// calling
	if(contentCopy != nullptr)
		removeChild(contentCopy);

	if (contentLoaderInfo != nullptr)
		contentLoaderInfo->resetState();
}

InteractiveObject* Loader::AVM1getMouseTarget
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	bool requiresButtonMode
)
{
	// Don't bother, if we're running in an AVM2 context.
	if (isAS3())
		return nullptr;

	Locker l(mutexDisplayList);
	auto& list = dynamicDisplayList;
	for (auto it = list.rbegin(); it != list.rend(); ++it)
	{
		auto child = it->as<InteractiveObject>();
		if (child == nullptr)
			continue;
		auto point = localPoint * child->getMatrix();
		auto ret = !child->isAS3() ? child->AVM1getMouseTarget
		(
			globalPoint,
			point,
			requiresButtonMode
		) : child->AVM2getMouseTarget
		(
			globalPoint,
			point,
			requiresButtonMode
		).getObj();
		if (ret != nullptr)
			return ret;
	}
	return nullptr;
}

AVM2MouseTarget Loader::AVM2getMouseTarget
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	bool requiresButtonMode
)
{
	using SkipInvis = HitTestFlags::SkipInvisible;
	using SkipMask = HitTestFlags::SkipMask;
	using MousePick = HitTestFlags::MousePick;
	using MouseTargetType = AVM2MouseTarget::Type;

	// Don't bother, if we're running in an AVM1 context.
	if (!isAS3())
		return MouseTargetType::Miss;

	auto flags = SkipInvis;
	if (getMaskee() == nullptr)
		flags |= SkipMask;
	
	DisplayObject* child;
	{
		Locker l(mutexDisplayList);
		if (dynamicDisplayList.empty())
			return MouseTargetType::Miss;
		child = &dynamicDisplayList.front();
	}

	auto point = localPoint * child->getMatrix();
	auto _child = child->as<InteractiveObject>();
	if (_child != nullptr && _child->isAS3())
	{
		return _child->AVM2getMouseTarget
		(
			globalPoint,
			point,
			requiresButtonMode
		);
	}
	else if (_child != nullptr)
	{
		return _child->AVM1getMouseTarget
		(
			globalPoint,
			point,
			requiresButtonMode
		);
	}

	if (!child->hitTestShape(globalPoint point, flags))
		return MouseTargetType::Miss;

	if (getMouseEnabled())
		return this;
	return MouseTargetType::PropagateToParent;
}

Loader::Loader
(
	SystemState* sys,
	SWFMovie& _movie
) :
InteractiveObject(Type::Loader, sys),
DisplayObjectContainer(_movie),
content(nullptr),
loadStatus(LoadStatus::Start),
parseThread(nullptr),
streamBuf(nullptr),
loaded(false),
allowCodeImport(true)
{
}

Loader::~Loader()
{
	if (parseThread != nullptr)
		delete parseThread;
	if (streamBuf != nullptr)
		delete streamBuf;
}

void Loader::threadFinished(IThreadJob* finishedJob)
{
	Locker l(mutex);
	jobs.remove(finishedJob);
	delete finishedJob;
}

void Loader::setContent(LoaderData& loaderData, DisplayObject& obj)
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
		Locker l(mutex);
		bool addAVM1Movie =
		(
			loaderData.isAVM2() &&
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

		auto& data = static_cast<ASLoaderData&>(data);
		auto loaderInfo = data.getLoaderInfo();
		auto wrk = loaderInfo->getInstanceWorker();
		auto m = Class<AVM1Movie>::getInstanceS(wrk);
		m->setIsInitialized();
		m->setConstructIndicator();
		m->setLoaderInfo(loaderInfo);
		m->insertChildAt
		(
			-0xf000,
			obj,
			false,
			false
		);
		m->isLoadedRoot = true;
		return m;
	}();

	loaderData.visit(makeVisitor
	(
		[&](const AVM1LoaderData& data)
		{
			auto target = data.getTarget();
			auto& act = data.getAct();
			auto& targetClip = target.resolveClip(act)->second;
			// _addChild may cause AS code to run, release locks beforehand.
			obj.tx = targetClip.tx;
			obj.ty = targetClip.ty;
			obj.tz = targetClip.tz;
			obj.rotation = targetClip.rotation;
			obj.sx = targetClip.sx;
			obj.sy = targetClip.sy;
			obj.sz = targetClip.sz;
			obj.name = targetClip.name;

			if (targetClip.getParent() == nullptr)
				return;

			auto parent = targetClip.getParent();
			auto depth = targetClip.getDepth();
			auto _targetClip = targetClip.as<DisplayObjectContainer>();

			if (_targetClip != nullptr)
				_targetClip->removeAllChildren(true, true);

			if (parent->is<Stage>() && !target.hasLevel())
				parent->removeChild(targetClip);
			else
				parent->deleteChildAt(depth, false);
			parent->insertChildAt(depth, obj, false, false);
		},
		[&](const auto&) { addChildAt(*content, 0); }
	));

	setComplete(loaderData);
}
