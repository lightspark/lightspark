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
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "parsing/tags.h"
#include "scripting/abc.h"
#include "backends/security.h"

using namespace std;
using namespace lightspark;

void AVM1MovieClip::afterConstruction(bool _explicit)
{
	MovieClip::afterConstruction(_explicit);
	getSystemState()->stage->AVM1AddEventListener(this);
}

bool AVM1MovieClip::destruct()
{
	avm1loader.reset();
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
}

void AVM1MovieClip::sinit(Class_base* c)
{
	MovieClip::sinit(c);
	MovieClip::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("_totalframes","",c->getSystemState()->getBuiltinFunction(_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_currentframe","",c->getSystemState()->getBuiltinFunction(_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_framesloaded","",c->getSystemState()->getBuiltinFunction(_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_getter_name),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_setter_name),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",c->getSystemState()->getBuiltinFunction(startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",c->getSystemState()->getBuiltinFunction(stopDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachAudio","",c->getSystemState()->getBuiltinFunction(attachAudio),NORMAL_METHOD,true);
}

void AVM1MovieClip::setColor(AVM1Color* c)
{
	c->incRef();
	this->color = _MNR(c);
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,startDrag)
{
	bool lockcenter;
	number_t x1, y1, x2, y2;
	ARG_CHECK(ARG_UNPACK(lockcenter,false)(x1,0)(y1,0)(x2,0)(y2,0));

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
	Sprite::_stopDrag(ret,wrk,obj,nullptr,0);
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

void AVM1SimpleButton::sinit(Class_base* c)
{
	SimpleButton::sinit(c);
	c->isSealed = false;
	DisplayObject::AVM1SetupMethods(c);
}

void AVM1Stage::sinit(Class_base* c)
{
	// in AVM1 Stage is no DisplayObject and all methods/properties are static
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getStageWidth),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_setStageWidth),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getStageHeight),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_setStageHeight),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_getDisplayState),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_setDisplayState),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_getScaleMode),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_setScaleMode),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_getShowMenu),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_setShowMenu),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(getAlign),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(setAlign),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addResizeListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeResizeListener),NORMAL_METHOD,false);
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


void AVM1MovieClipLoader::addLoader(URLRequest* r, DisplayObject* target)
{
	Loader* ldr = Class<Loader>::getInstanceSNoArgs(getInstanceWorker());
	ldr->addStoredMember();
	ldr->loadedFrom=target->loadedFrom;
	loadermutex.lock();
	loaderlist.insert(ldr);
	loadermutex.unlock();
	ldr->loadIntern(r,nullptr,target);
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
	URLRequest* r = Class<URLRequest>::getInstanceS(wrk,strurl,"GET",NullRef,t->loadedFrom);
	th->addLoader(r,t);
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
		if ((*itldr)->getContentLoaderInfo().getPtr() == dispatcher)
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
					{
						ldr->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					}
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
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					if (ldr->getContent())
					{
						ldr->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
					asAtomHandler::as<AVM1Function>(func)->decRef();
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
					{
						ldr->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					}
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
					{
						ldr->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					}
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

bool AVM1MovieClipLoader::destruct()
{
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
	if (gcstate.checkAncestors(this))
		return false;
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


void AVM1MovieClipLoader::load(const tiny_string& url, const tiny_string& method, AVM1MovieClip* target)
{
	URLRequest* r = Class<URLRequest>::getInstanceS(getInstanceWorker(),url,method,NullRef,target->loadedFrom);
	addLoader(r, target);
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
	ASObject* o = Class<ASObject>::getInstanceS(wrk);
	if (th->target && th->target->colorTransform)
	{
		asAtom a= asAtomHandler::undefinedAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ra");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->redMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->redOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ga");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->greenMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("gb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->greenOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ba");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->blueMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("bb");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->blueOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("aa");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->alphaMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ab");
		a = asAtomHandler::fromNumber(wrk,th->target->colorTransform->alphaOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED,nullptr,wrk);

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
	c->setDeclaredMethodByQName("initialize","",c->getSystemState()->getBuiltinFunction(initialize),NORMAL_METHOD,false);
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
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NONE,wrk);
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
				asAtom res;
				if (asAtomHandler::is<Function>(f))
				{
					asAtomHandler::as<Function>(f)->call(res,wrk,o,nullptr,0);
					asAtomHandler::as<Function>(f)->decRef();
				}
				else if (asAtomHandler::is<SyntheticFunction>(f))
				{
					asAtomHandler::as<SyntheticFunction>(f)->call(wrk,res,o,nullptr,0,false,false);
				}
				else if (asAtomHandler::is<AVM1Function>(f))
				{
					asAtomHandler::as<AVM1Function>(f)->call(&res,&o,nullptr,0);
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
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NONE,wrk);
	if (asAtomHandler::isArray(l))
	{
		// TODO spec is not clear if listener can be added multiple times
		Array* listeners = asAtomHandler::as<Array>(l);
		listener->incRef();
		listeners->push(asAtomHandler::fromObjectNoPrimitive(listener.getPtr()));
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
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NONE,wrk);
	if (asAtomHandler::isArray(l))
	{
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o=asAtomHandler::invalidAtom;
			listeners->at_nocheck(o,i);
			if (o.uintval==l.uintval)
			{
				asAtom res;
				asAtom index = asAtomHandler::fromUInt(i);
				listeners->removeAt(res,wrk,obj,&index,1);
				ret=asAtomHandler::trueAtom;
				break;
			}
		}
	}
}

void AVM1BitmapData::sinit(Class_base *c)
{
	BitmapData::sinit(c);
	c->setDeclaredMethodByQName("loadBitmap","",c->getSystemState()->getBuiltinFunction(loadBitmap),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("rectangle","",c->getSystemState()->getBuiltinFunction(getRect,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1BitmapData,loadBitmap)
{
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	BitmapTag* tag = dynamic_cast<BitmapTag*>( wrk->rootClip->dictionaryLookupByName(wrk->getSystemState()->getUniqueStringId(name)));
	if (tag)
		ret = asAtomHandler::fromObjectNoPrimitive(tag->instance());
	else
		LOG(LOG_ERROR,"BitmapData.loadBitmap tag not found:"<<name);
}

AVM1Bitmap::AVM1Bitmap(ASWorker* wrk, Class_base* c, _NR<LoaderInfo> li, istream* s, FILE_TYPE type):Bitmap(wrk,c,li,s,type)
{
}

void AVM1Bitmap::sinit(Class_base *c)
{
	Bitmap::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_setter_smoothing),SETTER_METHOD,true);
}
