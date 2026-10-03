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

#ifndef DISPLAY_OBJECT_LOADER_H
#define DISPLAY_OBJECT_LOADER_H 1

#include <list>

#include "backends/netutils.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/InteractiveObject.h"
#include "gc/ptr.h"
#include "smartrefs.h"
#include "utils/span.h"

namespace lightspark
{

class AVM1MovieClipLoader;
class ApplicationDomain;
class LoaderContext;
class LoaderInfo;
class SWFMovie;
class URLRequest;

class LoaderData
{
public:
	enum class Type
	{
		AVM1,
		AVM2,
	};
private:
	Type type;
protected:
	LoaderData(const Type& _type) : type(_type) {}
public:
	LoaderData() = delete;
	virtual ~LoaderData() {}
	const Type& getType() const { return type; }
	bool isAVM1() const { return type == Type::AVM1; }
	bool isAVM2() const { return type == Type::AVM2; }
	template<typename V>
	auto visit(V&& visitor) const;
};

class AVM1LoaderData : public LoaderData
{
private:
	_NGC<AVM1MovieClipLoader> broadcaster;
public:
	AVM1LoaderData(_NGC<AVM1Object> _broadcaster) :
	LoaderData(Type::AVM1),
	broadcaster(_broadcaster) {}

	_NGC<AVM1MovieClipLoader> getBroadcaster() const { return broadcaster; }
};

class ASLoaderData : public LoaderData
{
private:
	_R<LoaderInfo> loaderInfo;
	_NR<LoaderContext> context;
	_R<ApplicationDomain> defaultDomain;
public:
	ASLoaderData
	(
		_R<LoaderInfo> _loaderInfo,
		_NR<LoaderContext> _context,
		_R<ApplicationDomain> domain
	) :
	LoaderData(Type::AVM2),
	loaderInfo(_loaderInfo),
	context(_context),
	defaultDomain(domain) {}

	_R<LoaderInfo> getLoaderInfo() const { return loaderInfo; }
	_NR<LoaderContext> getContext() const { return context; }
	_R<ApplicationDomain> getDefaultDomain() const { return defaultDomain; }
};

template<typename V>
auto LoaderData::visit(V&& visitor) const
{
	using AVM1Loader = AVM1LoaderData;
	using ASLoader = ASLoaderData;
	switch (getType())
	{
		case Type::AVM1: return visitor(static_cast<const AVM1Loader&>(*this));
		case Type::AVM2: return visitor(static_cast<const ASLoader&>(*this));
	}
}

class LoaderThread : public DownloaderThreadBase
{
private:
	Span<uint8_t> bytes;
	Loader& loader;
	LoaderData& data;
	bool isBytes;

	std::streambuf* getStreamBuf();
public:
	void jobFence() override;
	void execute() override;
	LoaderThread
	(
		const URLRequest& request,
		Loader& _loader,
		LoaderData& _data
	);

	LoaderThread
	(
		Span<uint8_t> _bytes,
		Loader& _loader,
		LoaderData& _data
	);

	const Loader& getLoader() const { return loader; }
};

class Loader :
public InteractiveObject,
public DisplayObjectContainer,
public IDownloaderThreadListener
{
public
	enum class LoadStatus
	{
		Start,
		Started,
		Opened,
		Progressing,
		DownloadDone,
		InitSent,
		Complete,
	};
private:
	SWFMovie& movie;
	mutable Mutex mutex;
	DisplayObject* content;
	LoadStatus loadStatus;
	ParseThread* parseThread;
	std::streambuf* streamBuf;
	URLInfo url;
	bool loaded;
	bool allowCodeImport;
	// There can be multiple jobs, one active and aborted ones
	// that have not yet terminated
	std::list<IThreadJob*> jobs;

	/*
	 * sendInit should be called with the spinlock held
	 */
	void sendInit(LoaderData& loaderData);
	void checkSendComplete();
public:
	Loader(SystemState* sys, SWFMovie& _movie);
	~Loader();
	void parseData(LoaderData& loaderData, std::streambuf* _streamBuf);
	void setOpened(LoaderData& loaderData, bool fromBytes);
	void setComplete(LoaderData& loaderData);
	void onStart(LoaderData& loaderData);
	void onProgress
	(
		LoaderData& loaderData,
		size_t bytesLoaded,
		size_t bytesTotal
	);

	void onComplete
	(
		LoaderData& loaderData,
		DisplayObject* obj,
		uint16_t _status,
		bool redirected
	);

	void onError
	(
		LoaderData& loaderData,
		const tiny_string& msg = "",
		const tiny_string& errorCode = "LoadNeverCompleted",
		uint16_t _status = 0,
		bool redirected = false
	);

	void onSecError(LoaderData& loaderData, const tiny_string& msg = "");
	bool clipLoaded(LoaderData& loaderData);
	void threadFinished(IThreadJob* job) override;
	void close();
	void load(const URLRequest& req, LoaderData& data);
	void loadBytes(Span<uint8_t> bytes, LoaderContext* ctx);
	void setContent(LoaderData& loaderData, DisplayObject& obj);
	DisplayObject* getContent() const { return content; }
	bool allowLoadingSWF() { return allowCodeImport; }
	void unload(LoaderData& loaderData);
	InteractiveObject* AVM1getMouseTarget
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		bool requiresButtonMode
	) override;

	AVM2MouseTarget AVM2getMouseTarget
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		bool requiresButtonMode
	) override;
};

}
#endif /* DISPLAY_OBJECT_LOADER_H */
