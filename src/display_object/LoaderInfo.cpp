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

#include <streambuf>

#include "display_object/DisplayObject.h"
#include "display_object/Loader.h"
#include "display_object/LoaderInfo.h"
#include "display_object/RootMovieClip.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/flash/system/flashsystem.h"
#include "swf.h"

using namespace lightspark;

LoaderInfo::LoaderInfo(SystemState* _sys, Loader* _loader) :
sys(_sys),
parseThread(nullptr),
streamBuf(nullptr),
bytesLoaded(0),
bytesLoadedPublic(0),
bytesTotal(0),
sharedEvents(Class<EventDispatcher>::getInstanceS(_sys->worker)),
loader(_loader),
content(nullptr),
progressEvent(nullptr),
loadStatus(LoadStatus::Start),
fromByteArray(false),
_hasAVM1Target(false),
parameters(_MR(new_asobject(_sys->worker))),
contentType("application/x-shockwave-flash"),
asVersion(3),
swfVersion(0),
childAllowsParent(true),
uncaughtErrorEvents(Class<UncaughtErrorEvents>::getInstanceS(_sys->worker)),
parentAllowsChild(true),
frameRate(0)
{
}

void LoaderInfo::beforeHandleEvent(Event* ev)
{
	if (ev->is<ProgressEvent>() && loadStatus == LOAD_OPENED)
		loadStatus = LOAD_PROGRESSING;
	else if (ev->is<ProgressEvent>() && ev->as<ProgressEvent>()->bytesLoaded == ev->as<ProgressEvent>()->bytesTotal)
		loadStatus = LOAD_DOWNLOAD_DONE;
	else if (ev->type=="unload" && this->content && !this->content->needsActionScript3())
		this->content->AVM1HandleEvent(this,ev);
}

void LoaderInfo::afterHandleEvent(Event* ev)
{
	Locker l(spinlock);
	if (ev == progressEvent)
	{
		progressEvent->decRef();
		progressEvent=nullptr;
	}
}

void LoaderInfo::resetState()
{
	Locker l(mutex);
	bytesLoaded = 0;
	bytesLoadedPublic = 0;
	bytesTotal = 0;
	bytesData.clear();
	loadStatus = LoadStatus::Start;
}

void LoaderInfo::setContent(DisplayObject* c)
{
	if (content == c)
		return;
	content = c;
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if (b == bytesLoaded)
		return;

	auto vm = getVm(sys);
	Locker l(mutex);
	bytesLoaded = b;

	if (vm == nullptr || loadStatus < LoadStatus::Opened)
		return;

	onProgress(bytesLoaded, bytesTotal);
	checkSendComplete();
}

void LoaderInfo::setURL(const tiny_string& _url, bool setParameters)
{
	url = _url;

	if (!setParameters)
		return;
	//Specs says that parameters should be set from the *main* SWF
	//URL query string, but testing shows that it should be the
	//loaded URL.
	//
	//TODO: the parameters should only be set if the loaded clip
	//uses AS3. See specs.
	parameters = _MR(new_asobject(sys->worker));
	SystemState::parseParametersFromURLIntoObject(url, parameters);
}

bool LoaderInfo::fillBytesData(Optional<const std::vector<uint8_t>&> data)
{
	if (loadStatus < LoadStatus::Started)
		return false;
	if (loader != nullptr && loadStatus < LoadStatus::DownloadDone)
		return true;
	if (data.hasValue())
	{
		bytesData = *data;
		return true;
	}

	if (loader == nullptr) //th is the LoaderInfo of the main clip
	{
		auto _parseThread = sys->mainClip->parsethread;
		if (!bytesData.empty() || _parseThread == nullptr)
			return true;
		bytesData = _parseThread->getSWFByteArray();
		return true;
	}

	if (parseThread != nullptr)
	{
		if (loadStatus >= LoadStatus::Opened)
			bytesData = parseThread->getSWFByteArray();
		return true;
	}

	if (loader->getContent() != nullptr)
	{
		auto obj = loader->getContent()->toASObject();
		if (obj.isNull())
			return true;
		AMFBytesWriter writer(bytesData);
		Amf3Serializer().writeValue(writer, obj->toAMF3Value());
		return true;
	}
	return !bytesData.empty();
}

Span<uint8_t> LoaderInfo::getBytes()
{
	if (!fillBytesData(nullptr))
		return {};

	return getBytesData();
}

Vector2u LoaderInfo::getSize() const
{
	if (loader == nullptr && this == sys->mainClip->loaderInfo)
		return sys->mainClip->getNominalSize();
	else if (loader != nullptr && loader->getContent() != nullptr)
		return loader->getContent()->getNominalSize();
	return Vector2u();
}
