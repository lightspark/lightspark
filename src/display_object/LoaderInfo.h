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

#ifndef DISPLAY_OBJECT_LOADERINFO_H
#define DISPLAY_OBJECT_LOADERINFO_H 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <unordered_set>
#include <vector>

#include "backends/geometry.h"
#include "interfaces/backends/netutils.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "threading.h"
#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"

namespace lightspark
{

class ApplicationDomain;
class DisplayObject;
class Event;
class EventDispatcher;
class Loader;
class ParseThread;
class ProgressEvent;
class SecurityDomain;
class SystemState;
class UncaughtErrorEvents;

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
	_NGC<AVM1Object> broadcaster;
public:
	AVM1LoaderData(_NGC<AVM1Object> _broadcaster) :
	LoaderData(Type::AVM1),
	broadcaster(_broadcaster) {}

	_NGC<AVM1Object> getBroadcaster() const { return broadcaster; }
};

class ASLoaderData : public LoaderData
{
private:
	_R<ASLoaderInfo> loaderInfo;
	_NR<ASObject> context;
	_R<ApplicationDomain> defaultDomain;
public:
	ASLoaderData
	(
		_R<ASLoaderInfo> _loaderInfo,
		_NR<ASObject> _context,
		_R<ApplicationDomain> domain
	) :
	LoaderData(Type::AVM2),
	loaderInfo(_loaderInfo),
	context(_context),
	defaultDomain(domain) {}

	_R<ASLoaderInfo> getLoaderInfo() const { return loaderInfo; }
	_NR<ASObject> getContext() const { return context; }
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

class LoaderInfo : public ILoadable
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
	SystemState* sys;
	_NR<ApplicationDomain> appDomain;
	_NR<SecurityDomain> secDomain;
	LoaderData& loaderData;
	ParseThread* parseThread;
	std::streambuf* streamBuf;
	size_t bytesLoaded;
	size_t bytesLoadedPublic; // bytes loaded synchronized with ProgressEvent
	size_t bytesTotal;
	tiny_string url;
	tiny_string loaderURL;
	_NR<EventDispatcher> sharedEvents;
	Loader* loader;
	DisplayObject* content;
	std::vector<uint8_t> bytesData;
	ProgressEvent* progressEvent;
	Mutex mutex;
	LoadStatus loadStatus;
	bool fromByteArray;
	bool _hasAVM1Target;
	// set of events that need refcounting of loader
	std::unordered_set<std::reference_wrapper<Event>> loaderEvents;

	_NR<ASObject> parameters;
	tiny_string contentType;
	size_t asVersion;
	size_t swfVersion;
	bool childAllowsParent;
	_NR<UncaughtErrorEvents> uncaughtErrorEvents;
	bool parentAllowsChild;
	number_t frameRate;

	/*
	 * sendInit should be called with the spinlock held
	 */
	void sendInit();
	void checkSendComplete();
public:
	LoaderInfo
	(
		SystemState* _sys,
		LoaderData& _loaderData,
		Loader* _loader = nullptr
	);

	~LoaderInfo();
	void parseData(std::streambuf* _streamBuf);
	void beforeHandleEvent(Event* ev) override;
	void afterHandleEvent(Event* ev) override;
	void setOpened(bool fromBytes);
	void onStart();
	void onProgress(size_t bytesLoaded, size_t bytesTotal);
	void onComplete
	(
		DisplayObject* obj,
		uint16_t _status,
		bool redirected
	);

	void onError
	(
		const tiny_string& msg,
		uint16_t _status,
		bool redirected,
		const tiny_string& url
	);

	bool clipLoaded();
	void setStarted() { loadStatus = LoadStatus::Started; }
	const LoadStatus& getLoadStatus() const { return loadStatus; }
	DisplayObject* getParsedObject() const { return content; }
	const tiny_string& getLoaderURL() const { return loaderURL; }
	const tiny_string& getURL() const { return url; }
	size_t getBytesLoadedPublic() const { return bytesLoadedPublic; }
	Span<uint8_t> getBytes();
	Span<uint8_t> getBytesData() const { return makeSpan(bytesData); }
	_NR<ApplicationDomain> getAppDomain() const { return appDomain; }
	_NR<SecurityDomain> getSecDomain() const { return secDomain; }
	_NR<EventDispatcher> getSharedEvents() const { return sharedEvents; }
	Vector2u getSize() const;
	size_t getWidth() const { return getSize().x; }
	size_t getHeight() const { return getSize().y; }
	_NR<ASObject> getParameters() const { return parameters; }
	size_t getASVersion() const { return asVersion; }
	bool getChildAllowsParent() const { return childAllowsParent; }
	const tiny_string& getContentType() const { return contentType; }
	size_t getSwfVersion() const { return swfVersion; }
	bool getParentAllowsChild() const { return parentAllowsChild; }
	number_t getFrameRate() const { return frameRate; }
	_NR<UncaughtErrorEvents> getUncaughtErrorEvents() const
	{
		return uncaughtErrorEvents;
	}

	//ILoadable interface
	void setBytesTotal(uint32_t b) override { bytesTotal = b; }
	void setBytesLoaded(uint32_t b) override;

	void setBytesLoadedPublic(size_t b) { bytesLoadedPublic = b; }
	size_t getBytesLoaded() const { return bytesLoaded; }
	size_t getBytesTotal() const { return bytesTotal; }
	void setURL(const tiny_string& _url, bool setParameters = true);
	void setLoaderURL(const tiny_string& _url) { loaderURL = _url; }
	void setParameters(_NR<ASObject> p) { parameters = p; }
	void resetState();
	void setFrameRate(number_t f) { frameRate = f; }
	void setComplete();
	void setContent(DisplayObject* c);
	void setAVM1Target(bool avm1Target) { _hasAVM1Target = avm1Target; }
	bool fillBytesData(Optional<const std::vector<uint8_t>&> data);
	Loader* getLoader() const { return loader; }
	DisplayObject* getContent() const { return content; }
	bool isFromByteArray() const { return fromByteArray; }
	bool hasAVM1Target() const { return _hasAVM1Target; }
};

}
#endif /* DISPLAY_OBJECT_LOADERINFO_H */
