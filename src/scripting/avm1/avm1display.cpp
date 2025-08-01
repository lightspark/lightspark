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

#include "backends/cachedsurface.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Integer.h"
#include "parsing/tags.h"
#include "scripting/abc.h"
#include "backends/security.h"
#include "backends/input.h"

using namespace std;
using namespace lightspark;

void AVM1MovieClip::afterConstruction(bool _explicit)
{
	MovieClip::afterConstruction(_explicit);
	getSystemState()->stage->AVM1AddEventListener(this);
}

void AVM1MovieClip::finalize()
{
	MovieClip::finalize();
	if (droptarget)
		droptarget->removeStoredMember();
	droptarget=nullptr;
	color.reset();
	getSystemState()->stage->AVM1RemoveEventListener(this);
}

bool AVM1MovieClip::destruct()
{
	if (droptarget)
		droptarget->removeStoredMember();
	droptarget=nullptr;
	color.reset();
	getSystemState()->stage->AVM1RemoveEventListener(this);
	return MovieClip::destruct();
}

void AVM1MovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	MovieClip::prepareShutdown();
	if (color)
		color->prepareShutdown();
	if (droptarget)
		droptarget->prepareShutdown();
}
bool AVM1MovieClip::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = MovieClip::countCylicMemberReferences(gcstate);
	if (droptarget)
		ret = droptarget->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void AVM1MovieClip::sinit(Class_base* c)
{
	MovieClip::sinit(c);
	MovieClip::AVM1SetupMethods(c);
	c->prototype->setDeclaredMethodByQName("_totalframes","",c->getSystemState()->getBuiltinFunction(_getTotalFrames),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("_currentframe","",c->getSystemState()->getBuiltinFunction(_getCurrentFrame),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("_framesloaded","",c->getSystemState()->getBuiltinFunction(_getFramesLoaded),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_getter_name),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_setter_name),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("startDrag","",c->getSystemState()->getBuiltinFunction(startDrag),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("stopDrag","",c->getSystemState()->getBuiltinFunction(stopDrag),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("attachAudio","",c->getSystemState()->getBuiltinFunction(attachAudio),NORMAL_METHOD,false);
}

void AVM1MovieClip::setColor(AVM1Color* c)
{
	c->incRef();
	this->color = _MNR(c);
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,startDrag)
{
	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	bool lockcenter;
	number_t x1, y1, x2, y2;
	ARG_CHECK(ARG_UNPACK(lockcenter,false)(x1,0)(y1,0)(x2,0)(y2,0));

	if (th->droptarget)
	{
		th->droptarget->removeStoredMember();
		th->droptarget=nullptr;
	}
	if (argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::getInstanceS(wrk);
		asAtom fret = asAtomHandler::invalidAtom;
		asAtom fobj = asAtomHandler::fromObject(rect);
		asAtom fx1 = asAtomHandler::fromNumber(wrk, x1, false);
		asAtom fy1 = asAtomHandler::fromNumber(wrk, y1, false);
		asAtom fx2 = asAtomHandler::fromNumber(wrk, x2, false);
		asAtom fy2 = asAtomHandler::fromNumber(wrk, y2, false);
		Rectangle::_setLeft(fret,wrk,fobj,&fx1,1);
		Rectangle::_setTop(fret,wrk,fobj,&fy1,1);
		Rectangle::_setRight(fret,wrk,fobj,&fx2,1);
		Rectangle::_setBottom(fret,wrk,fobj,&fy2,1);
	
		asAtom fargs[2];
		fargs[0] = asAtomHandler::fromBool(lockcenter);
		fargs[1] = asAtomHandler::fromObject(rect);
		Sprite::_startDrag(ret,wrk,obj,fargs,2);
	}
	else
	{
		asAtom fargs[1];
		fargs[0] = asAtomHandler::fromBool(lockcenter);
		Sprite::_startDrag(ret,wrk,obj,fargs,1);
	}
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,stopDrag)
{
	if (!asAtomHandler::is<DisplayObject>(obj))
		return;
	DisplayObject* dt = wrk->getSystemState()->getInputThread()->stopDrag(asAtomHandler::as<DisplayObject>(obj));
	if (asAtomHandler::is<AVM1MovieClip>(obj))
	{
		AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
		if (th->droptarget)
			th->droptarget->removeStoredMember();
		th->droptarget = dt;
		if (th->droptarget)
		{
			th->droptarget->incRef();
			th->droptarget->addStoredMember();
		}
	}
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,attachAudio)
{
//	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"AVM1MovieClip.attachAudio");
}

void AVM1Shape::sinit(Class_base* c)
{
	Shape::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
}

ASObject* AVM1SimpleButton::AVM1getClassPrototypeObject() const
{
	// for some reason buttons don't allow access to its prototype on swf6 (see ruffle tests avm1/focusrect_property_swf6)
	if (this->loadedFrom->version < 7)
		return nullptr;
	return SimpleButton::AVM1getClassPrototypeObject();
}

void AVM1SimpleButton::sinit(Class_base* c)
{
	SimpleButton::sinit(c);
	c->isSealed = false;
	InteractiveObject::AVM1SetupMethods(c);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_getUseHandCursor),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_setUseHandCursor),SETTER_METHOD,false);
}

void AVM1Stage::sinit(Class_base* c)
{
	// in AVM1 Stage is no DisplayObject and all methods/properties are static
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getStageWidth),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_setStageWidth),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getStageHeight),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_setStageHeight),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_getDisplayState),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_setDisplayState),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_getScaleMode),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_setScaleMode),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_getShowMenu),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_setShowMenu),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(getAlign),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(setAlign),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addResizeListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeResizeListener),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_getDisplayState)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setDisplayState)
{
	ARG_CHECK(ARG_UNPACK(wrk->getSystemState()->stage->displayState));
	wrk->getSystemState()->stage->onDisplayState(wrk->getSystemState()->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_getShowMenu)
{
	ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->showDefaultContextMenu);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setShowMenu)
{
	ARG_CHECK(ARG_UNPACK(wrk->getSystemState()->stage->showDefaultContextMenu));
}
ASFUNCTIONBODY_ATOM(AVM1Stage,getAlign)
{
	ret = asAtomHandler::fromStringID(wrk->getSystemState()->stage->align);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,setAlign)
{
	uint32_t oldalign=wrk->getSystemState()->stage->align;
	tiny_string newalign;
	ARG_CHECK(ARG_UNPACK(newalign));
	wrk->getSystemState()->stage->align = wrk->getSystemState()->getUniqueStringId(newalign);
	wrk->getSystemState()->stage->onAlign(oldalign);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,addResizeListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener,NullRef));
	wrk->getSystemState()->stage->AVM1AddResizeListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Stage,removeResizeListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener,NullRef));
	ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1RemoveResizeListener(listener.getPtr()));
}


void AVM1MovieClipLoader::addLoader(URLRequest* r, DisplayObject* target,int level)
{
	loadermutex.lock();
	Loader* ldr = Class<Loader>::getInstanceSNoArgs(getInstanceWorker());
	ldr->addStoredMember();
	ldr->loadedFrom=target->loadedFrom;
	ldr->AVM1setup(level,this);
	loaderlist.insert(ldr);
	loadermutex.unlock();
	ldr->loadIntern(r,nullptr,target);
	r->decRef();
}

void AVM1MovieClipLoader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loadClip","",c->getSystemState()->getBuiltinFunction(loadClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,_constructor)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);
	th->getSystemState()->stage->AVM1AddEventListener(th);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,loadClip)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);
	tiny_string strurl;
	asAtom target = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(strurl)(target));
	DisplayObject* t =nullptr;
	if (asAtomHandler::isNumeric(target))
	{
		LOG(LOG_NOT_IMPLEMENTED,"AVM1MovieClipLoader,loadClip with number as target");
		return;
	}
	else if (asAtomHandler::is<DisplayObject>(target))
	{
		t = asAtomHandler::getObject(target)->as<DisplayObject>();
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"target");
		return;
	}
	if (!t)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"target");
		return;
	}
	URLRequest* r = Class<URLRequest>::getInstanceS(wrk,strurl,"GET",asAtomHandler::invalidAtom,t->loadedFrom);
	th->addLoader(r,t,-1);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,addListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);

	wrk->getSystemState()->stage->AVM1AddEventListener(th);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	o->incRef();
	o->addStoredMember();
	th->listeners.insert(o);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,removeListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);
	
	ASObject* o = asAtomHandler::toObject(args[0],wrk);
	
	auto it = th->listeners.find(o);
	if (it != th->listeners.end())
	{
		th->listeners.erase(o);
		o->removeStoredMember();
	}
}
void AVM1MovieClipLoader::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	loadermutex.lock();
	Loader* ldr = nullptr;
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		if ((*itldr)->getContentLoaderInfo() == dispatcher)
		{
			ldr = (*itldr);
			break;
		}
		itldr++;
	}
	loadermutex.unlock();
	if (ldr)
	{
		ASWorker* wrk = getInstanceWorker();
		
		if (listeners.insert(this).second)
		{
			this->incRef();
			this->addStoredMember();
		}
		std::set<ASObject*> tmplisteners = listeners;
		auto it = tmplisteners.begin();
		while (it != tmplisteners.end())
		{
			if (e->type == "open")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadStart");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
			else if (e->type == "avm1_init")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadInit");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					AVM1Function* f = asAtomHandler::as<AVM1Function>(func);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(f->getClip());
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;

					f->call(&ret,&obj,args,1);
					f->decRef();
				}
			}
			else if (e->type == "progress")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadProgress");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					ProgressEvent* ev = e->as<ProgressEvent>();
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[3];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					args[1] = asAtomHandler::fromInt(ev->bytesLoaded);
					args[2] = asAtomHandler::fromInt(ev->bytesTotal);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,3);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
			else if (e->type == "complete")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadComplete");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
			else if (e->type == "ioError")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadError");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
			it++;
		}
		if (e->type == "avm1_init" || e->type == "ioError")
		{
			// download is done, so we can remove the loader
			loadermutex.lock();
			loaderlist.erase(ldr);
			ldr->removeStoredMember();
			loadermutex.unlock();
		}
	}
}

void AVM1MovieClipLoader::finalize()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		Loader* o = (*itldr);
		itldr = loaderlist.erase(itldr);
		o->removeStoredMember();
	}
	loadermutex.unlock();

}

bool AVM1MovieClipLoader::destruct()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		Loader* o = (*itldr);
		itldr = loaderlist.erase(itldr);
		o->removeStoredMember();
	}
	loadermutex.unlock();
	return ASObject::destruct();
}

void AVM1MovieClipLoader::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		(*itlst)->prepareShutdown();
		itlst++;
	}
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		(*itldr)->prepareShutdown();
		itldr++;
	}
	loadermutex.unlock();
}
bool AVM1MovieClipLoader::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ret = (*itlst)->countAllCylicMemberReferences(gcstate) || ret;
		itlst++;
	}
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		ret = (*itldr)->countAllCylicMemberReferences(gcstate) || ret;
		itldr++;
	}
	loadermutex.unlock();
	return ret;
}


void AVM1MovieClipLoader::load(const tiny_string& url, const tiny_string& method, AVM1MovieClip* target, int level)
{
	URLRequest* r = Class<URLRequest>::getInstanceS(getInstanceWorker(),url,method,asAtomHandler::invalidAtom,target->loadedFrom);
	addLoader(r, target, level);
}

void AVM1Color::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("getRGB","",c->getSystemState()->getBuiltinFunction(getRGB),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRGB","",c->getSystemState()->getBuiltinFunction(setRGB),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTransform","",c->getSystemState()->getBuiltinFunction(getTransform),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTransform","",c->getSystemState()->getBuiltinFunction(setTransform),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1Color,_constructor)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	if(argslen == 0)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Color", "1", "0");
		return;
	}
	DisplayObject* t = nullptr;
	if (asAtomHandler::isString(args[0]))
	{
		tiny_string s = asAtomHandler::toString(args[0],wrk);
		t = wrk->rootClip->AVM1GetClipFromPath(s);
	}
	else if (asAtomHandler::is<DisplayObject>(args[0]))
		t = asAtomHandler::as<DisplayObject>(args[0]);
	if (t)
	{
		if (t->is<AVM1MovieClip>())
			t->as<AVM1MovieClip>()->setColor(th);
		else
			LOG(LOG_NOT_IMPLEMENTED,"constructing AVM1Color without MovieClip as target:"<<t->toDebugString());
		th->target=t->as<AVM1MovieClip>();
		t->addOwnedObject(th);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Color,getRGB)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	if (th->target && th->target->colorTransform)
	{
		asAtom t = asAtomHandler::fromObject(th->target->colorTransform.getPtr());
		ColorTransform::getColor(ret,wrk,t,nullptr,0);
	}
	else
		ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Color,setRGB)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	assert_and_throw(argslen==1);
	if (th->target && th->target->colorTransform)
	{
		asAtom t = asAtomHandler::fromObject(th->target->colorTransform.getPtr());
		ColorTransform::setColor(ret,wrk,t,args,1);
		th->target->hasChanged=true;
		th->target->requestInvalidation(wrk->getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(AVM1Color,getTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	ASObject* o = new_asobject(wrk);
	if (th->target && th->target->colorTransform)
	{
		asAtom a= asAtomHandler::undefinedAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ra");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->redMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->redOffset,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ga");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->greenMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("gb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->greenOffset,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ba");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->blueMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("bb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->blueOffset,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("aa");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->alphaMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ab");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->alphaOffset,false);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

	}
	ret = asAtomHandler::fromObject(o);
}

ASFUNCTIONBODY_ATOM(AVM1Color,setTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	assert_and_throw(argslen==1);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);
	if (th->target)
	{
		if (!th->target->colorTransform)
			th->target->colorTransform = _R<ColorTransform>(Class<ColorTransform>::getInstanceSNoArgs(wrk));
		asAtom a = asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ra");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->redMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->redOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ga");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->greenMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("gb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->greenOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ba");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->blueMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("bb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->blueOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("aa");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->alphaMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ab");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->alphaOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);
		th->target->hasChanged=true;
		th->target->requestInvalidation(wrk->getSystemState());
	}
}
bool AVM1Color::destruct()
{
	target=nullptr;
	return ASObject::destruct();
}

void AVM1Broadcaster::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL);
	c->isReusable = true;
	c->prototype->setDeclaredMethodByQName("initialize","",c->getSystemState()->getBuiltinFunction(initialize),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("broadcastMessage","",c->getSystemState()->getBuiltinFunction(broadcastMessage),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,initialize)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (!listener.isNull())
	{
		Array* listeners = Class<Array>::getInstanceSNoArgs(wrk);
		listener->setVariableAtomByQName("broadcastMessage",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(broadcastMessage)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("addListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(addListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("removeListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(removeListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("_listeners",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(listeners),DYNAMIC_TRAIT);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,broadcastMessage)
{
	ASObject* th = asAtomHandler::getObject(obj);
	tiny_string msg;
	ARG_CHECK(ARG_UNPACK(msg));
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		multiname mmsg(nullptr);
		mmsg.name_type=multiname::NAME_STRING;
		mmsg.name_s_id=wrk->getSystemState()->getUniqueStringId(msg);
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o;
			listeners->at_nocheck(o,i);
			if (asAtomHandler::isObject(o))
			{
				ASObject* listener = asAtomHandler::getObjectNoCheck(o);
				asAtom f = asAtomHandler::invalidAtom;
				listener->getVariableByMultiname(f,mmsg,GET_VARIABLE_OPTION::NONE,wrk);
				asAtom res = asAtomHandler::invalidAtom;
				if (asAtomHandler::is<Function>(f))
				{
					asAtomHandler::as<Function>(f)->call(res,wrk,o,argslen > 1 ? &args[1] : nullptr,argslen-1);
					asAtomHandler::as<Function>(f)->decRef();
				}
				else if (asAtomHandler::is<SyntheticFunction>(f))
				{
					asAtomHandler::as<SyntheticFunction>(f)->call(wrk,res,o,argslen > 1 ? &args[1] : nullptr,argslen-1,false,false);
				}
				else if (asAtomHandler::is<AVM1Function>(f))
				{
					asAtomHandler::as<AVM1Function>(f)->call(&res,&o,argslen > 1 ? &args[1] : nullptr,argslen-1);
					asAtomHandler::as<AVM1Function>(f)->decRef();
				}
			}
		}
	}
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,addListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener.isNull())
	{
		ret = asAtomHandler::falseAtom;
		return;
	}
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		Array* listeners = asAtomHandler::as<Array>(l);
		bool found = false;
		for (uint32_t i=0; i < listeners->size(); i++)
		{
			asAtom a = asAtomHandler::invalidAtom;
			listeners->at_nocheck(a,i);
			if (a.uintval == args[0].uintval)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			listener->incRef();
			listeners->push(asAtomHandler::fromObjectNoPrimitive(listener.getPtr()));
		}
	}
	ret = asAtomHandler::trueAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,removeListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	ret = asAtomHandler::falseAtom;
	if (listener.isNull())
		return;
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o=asAtomHandler::invalidAtom;
			listeners->at_nocheck(o,i);
			if (o.uintval==args[0].uintval)
			{
				asAtom res;
				asAtom index = asAtomHandler::fromUInt(i);
				Array::removeAt(res,wrk,l,&index,1);
				ASATOM_DECREF(res);
				ret=asAtomHandler::trueAtom;
				break;
			}
		}
	}
}

void AVM1BitmapData::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable=true;

	c->prototype->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<AVM1BitmapData>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("applyFilter","",c->getSystemState()->getBuiltinFunction(applyFilter),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<AVM1BitmapData>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(colorTransform),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("compare","",c->getSystemState()->getBuiltinFunction(compare,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("copyChannel","",c->getSystemState()->getBuiltinFunction(copyChannel),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("copyPixels","",c->getSystemState()->getBuiltinFunction(copyPixels),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("draw","",c->getSystemState()->getBuiltinFunction(draw),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("fillRect","",c->getSystemState()->getBuiltinFunction(fillRect),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("floodFill","",c->getSystemState()->getBuiltinFunction(floodFill),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("generateFilterRect","",c->getSystemState()->getBuiltinFunction(generateFilterRect),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getColorBoundsRect","",c->getSystemState()->getBuiltinFunction(getColorBoundsRect,2,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getPixel","",c->getSystemState()->getBuiltinFunction(getPixel,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getPixel32","",c->getSystemState()->getBuiltinFunction(getPixel32,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("hitTest","",c->getSystemState()->getBuiltinFunction(hitTest,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("loadBitmap","",c->getSystemState()->getBuiltinFunction(loadBitmap),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("merge","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("noise","",c->getSystemState()->getBuiltinFunction(noise),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("paletteMap","",c->getSystemState()->getBuiltinFunction(paletteMap),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("perlinNoise","",c->getSystemState()->getBuiltinFunction(perlinNoise),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("pixelDissolve","",c->getSystemState()->getBuiltinFunction(pixelDissolve,3,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("rectangle","",c->getSystemState()->getBuiltinFunction(getRect,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scroll","",c->getSystemState()->getBuiltinFunction(scroll),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setPixel","",c->getSystemState()->getBuiltinFunction(setPixel),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setPixel32","",c->getSystemState()->getBuiltinFunction(setPixel32),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("threshold","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("transparent","",c->getSystemState()->getBuiltinFunction(_getTransparent,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1BitmapData,loadBitmap)
{
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	BitmapTag* tag = dynamic_cast<BitmapTag*>( wrk->rootClip->applicationDomain->dictionaryLookupByName(wrk->getSystemState()->getUniqueStringId(name)));
	if (tag)
		ret = asAtomHandler::fromObjectNoPrimitive(tag->instance());
	else
		LOG(LOG_ERROR,"BitmapData.loadBitmap tag not found:"<<name);
}

AVM1Bitmap::AVM1Bitmap(ASWorker* wrk, Class_base* c, LoaderInfo* li, istream* s, FILE_TYPE type):Bitmap(wrk,c,li,s,type)
{
}

void AVM1Bitmap::sinit(Class_base *c)
{
	Bitmap::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_setter_smoothing),SETTER_METHOD,true);
}
