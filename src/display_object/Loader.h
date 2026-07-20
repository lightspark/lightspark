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
#include "utils/span.h"

namespace lightspark
{

class SWFMovie;
class URLRequest;

class LoaderThread : public DownloaderThreadBase
{
private:
	enum SOURCE { URL, BYTES };
	Span<uint8_t> bytes;
	Loader& loader;
	LoaderInfo* loaderInfo;
	SOURCE source;
public:
	void jobFence() override;
	void execute() override;
	LoaderThread(const URLRequest& request, Loader& _loader);
	LoaderThread(Span<uint8_t> _bytes, Loader& _loader);
	const Loader& getLoader() const { return loader; }
};

class Loader :
public InteractiveObject,
public DisplayObjectContainer,
public IDownloaderThreadListener
{
private:
	SWFMovie& movie;
	mutable Mutex spinlock;
	DisplayObject* content;
	// There can be multiple jobs, one active and aborted ones
	// that have not yet terminated
	std::list<IThreadJob*> jobs;
	URLInfo url;
	LoaderInfo* contentLoaderInfo;
	bool loaded;
	bool allowCodeImport;
	int avm1level;
protected:
	DisplayObject* avm1Target;
	ASObject* avm1container;
public:
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

	Loader(SystemState* sys, SWFMovie& _movie);
	void threadFinished(IThreadJob* job) override;
	void close();
	void load(const URLRequest& req, LoaderContext* ctx);
	void loadBytes(Span<uint8_t> bytes, LoaderContext* ctx);
	void setContent(DisplayObject& obj);
	DisplayObject* getContent() const { return content; }
	LoaderInfo* getContentLoaderInfo();
	void checkContentLoaderInfo();
	bool allowLoadingSWF() { return allowCodeImport; }
	void AVM1setup(int level, ASObject* container);
	int AVM1getLevel() const { return avm1level; }
	void loadIntern
	(
		const URLRequest& req,
		LoaderContext* ctx,
		DisplayObject* avm1Target
	);

	void unload();
};

}
#endif /* DISPLAY_OBJECT_LOADER_H */
