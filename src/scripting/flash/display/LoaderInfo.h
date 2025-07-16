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

#ifndef SCRIPTING_FLASH_DISPLAY_LOADERINFO_H
#define SCRIPTING_FLASH_DISPLAY_LOADERINFO_H 1

#include "scripting/flash/events/flashevents.h"
#include "interfaces/backends/netutils.h"

namespace lightspark
{
class Loader;
class SecurityDomain;
class ParseThread;

class LoaderInfo: public EventDispatcher, public ILoadable
{
public:
	_NR<ApplicationDomain> applicationDomain;
	_NR<SecurityDomain> securityDomain;
	ASPROPERTY_GETTER(_NR<ASObject>,parameters);
	ASPROPERTY_GETTER(tiny_string, contentType);
private:
	ParseThread* local_pt;
	streambuf *sbuf;
	uint32_t bytesLoaded;
	uint32_t bytesLoadedPublic; // bytes loaded synchronized with ProgressEvent
	uint32_t bytesTotal;
	tiny_string url;
	tiny_string loaderURL;
	_NR<EventDispatcher> sharedEvents;
	Loader* loader;
	DisplayObject* content;
	_NR<ByteArray> bytesData;
	ProgressEvent* progressEvent;
	Mutex spinlock;
	enum LOAD_STATUS { LOAD_START=0, LOAD_OPENED, LOAD_PROGRESSING, LOAD_DOWNLOAD_DONE, LOAD_INIT_SENT, LOAD_COMPLETE };
	LOAD_STATUS loadStatus;
	bool fromByteArray;
	bool hasavm1target;
	/*
	 * sendInit should be called with the spinlock held
	 */
	void sendInit();
	void checkSendComplete();
	// set of events that need refcounting of loader
	std::unordered_set<Event*> loaderevents;
public:
	ASPROPERTY_GETTER(uint32_t,actionScriptVersion);
	ASPROPERTY_GETTER(uint32_t,swfVersion);
	ASPROPERTY_GETTER(bool, childAllowsParent);
	ASPROPERTY_GETTER(_NR<UncaughtErrorEvents>,uncaughtErrorEvents);
	ASPROPERTY_GETTER(bool,parentAllowsChild);
	ASPROPERTY_GETTER(number_t,frameRate);
	LoaderInfo(ASWorker*,Class_base* c);
	LoaderInfo(ASWorker*, Class_base* c, Loader* l);
	void parseData(streambuf* _sbuf);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void beforeHandleEvent(Event* ev) override;
	void afterHandleEvent(Event* ev) override;
	void addLoaderEvent(Event* ev);
	void setOpened(bool fromBytes);
	DisplayObject* getParsedObject() const;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLoaderURL);
	ASFUNCTION_ATOM(_getURL);
	ASFUNCTION_ATOM(_getBytesLoaded);
	ASFUNCTION_ATOM(_getBytesTotal);
	ASFUNCTION_ATOM(_getBytes);
	ASFUNCTION_ATOM(_getApplicationDomain);
	ASFUNCTION_ATOM(_getLoader);
	ASFUNCTION_ATOM(_getContent);
	ASFUNCTION_ATOM(_getSharedEvents);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_getHeight);
	//ILoadable interface
	void setBytesTotal(uint32_t b) override
	{
		bytesTotal=b;
	}
	void setBytesLoaded(uint32_t b) override;
	void setBytesLoadedPublic(uint32_t b) { bytesLoadedPublic = b; }
	uint32_t getBytesLoaded() { return bytesLoaded; }
	uint32_t getBytesTotal() { return bytesTotal; }
	void setURL(const tiny_string& _url, bool setParameters=true);
	void setLoaderURL(const tiny_string& _url) { loaderURL=_url; }
	void setParameters(_NR<ASObject> p) { parameters = p; }
	void resetState();
	void setFrameRate(number_t f) { frameRate=f; }
	void setComplete();
	void setContent(DisplayObject* c);
	void setAVM1Target(bool avm1target) { hasavm1target = avm1target; }
	bool fillBytesData(ByteArray* data);
	Loader* getLoader() const { return loader; }
	DisplayObject* getContent() const { return content; }
	bool isFromByteArray() const { return fromByteArray; }
	bool hasAVM1Target() const { return hasavm1target; }
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_LOADERINFO_H */
