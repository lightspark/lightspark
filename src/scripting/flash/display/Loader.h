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

#ifndef SCRIPTING_FLASH_DISPLAY_LOADER_H
#define SCRIPTING_FLASH_DISPLAY_LOADER_H 1

#include "backends/netutils.h"
#include "scripting/flash/display/flashdisplay.h"

namespace lightspark
{
class URLRequest;

class LoaderThread : public DownloaderThreadBase
{
private:
	enum SOURCE { URL, BYTES };
	_NR<ByteArray> bytes;
	_R<Loader> loader;
	_NR<LoaderInfo> loaderInfo;
	SOURCE source;
	void execute() override;
public:
	LoaderThread(_R<URLRequest> request, _R<Loader> loader);
	LoaderThread(_R<ByteArray> bytes, _R<Loader> loader);
};

class Loader: public DisplayObjectContainer, public IDownloaderThreadListener
{
private:
	mutable Mutex spinlock;
	DisplayObject* content;
	// There can be multiple jobs, one active and aborted ones
	// that have not yet terminated
	std::list<IThreadJob *> jobs;
	URLInfo url;
	LoaderInfo* contentLoaderInfo;
	void unload();
	bool loaded;
	bool allowCodeImport;
protected:
	_NR<DisplayObject> avm1target;
public:
	Loader(ASWorker* wrk, Class_base* c);
	~Loader();
	void finalize() override;
	bool destruct() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void prepareShutdown() override;
	void threadFinished(IThreadJob* job) override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(loadBytes);
	ASFUNCTION_ATOM(_unload);
	ASFUNCTION_ATOM(_unloadAndStop);
	ASFUNCTION_ATOM(_getContentLoaderInfo);
	ASFUNCTION_ATOM(_getContent);
	ASPROPERTY_GETTER(_NR<UncaughtErrorEvents>,uncaughtErrorEvents);
	int getDepth() const
	{
		return 0;
	}
	void setContent(DisplayObject* o);
	DisplayObject* getContent() const { return content; }
	_NR<LoaderInfo> getContentLoaderInfo();
	bool allowLoadingSWF() { return allowCodeImport; }
	bool hasAVM1Target() const { return !avm1target.isNull(); }
	void loadIntern(URLRequest* r, LoaderContext* context, DisplayObject* _avm1target=nullptr);
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_LOADER_H */
