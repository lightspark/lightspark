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

#include "scripting/avm1/avm1display.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/net/flashnet.h"
#include "parsing/tags.h"
#include "scripting/abc.h"
#include "backends/security.h"

using namespace std;
using namespace lightspark;

void AVM1MovieClip::afterConstruction()
{
	MovieClip::afterConstruction();
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
	c->setDeclaredMethodByQName("_totalframes","",Class<IFunction>::getFunction(c->getSystemState(),_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_currentframe","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_framesloaded","",Class<IFunction>::getFunction(c->getSystemState(),_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_name","",Class<IFunction>::getFunction(c->getSystemState(),_getter_name),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_name","",Class<IFunction>::getFunction(c->getSystemState(),_setter_name),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(c->getSystemState(),startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(c->getSystemState(),stopDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachAudio","",Class<IFunction>::getFunction(c->getSystemState(),attachAudio),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_setStageWidth),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_setStageHeight),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayState","",Class<IFunction>::getFunction(c->getSystemState(),_getDisplayState),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayState","",Class<IFunction>::getFunction(c->getSystemState(),_setDisplayState),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleMode),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleMode),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("showMenu","",Class<IFunction>::getFunction(c->getSystemState(),_getShowMenu),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("showMenu","",Class<IFunction>::getFunction(c->getSystemState(),_setShowMenu),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("align","",Class<IFunction>::getFunction(c->getSystemState(),getAlign),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("align","",Class<IFunction>::getFunction(c->getSystemState(),setAlign),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("addListener","",Class<IFunction>::getFunction(c->getSystemState(),addResizeListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("removeListener","",Class<IFunction>::getFunction(c->getSystemState(),removeResizeListener),NORMAL_METHOD,false);
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


void AVM1MovieClipLoader::sinit(Class_base* c)
{
	CLASS_SETUP(c, Loader, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loadClip","",Class<IFunction>::getFunction(c->getSystemState(),loadClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addListener","",Class<IFunction>::getFunction(c->getSystemState(),addListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeListener","",Class<IFunction>::getFunction(c->getSystemState(),removeListener),NORMAL_METHOD,true);
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
	URLRequest* r = Class<URLRequest>::getInstanceS(wrk,strurl);
	DisplayObjectContainer* parent = th->getParent();
	if (!parent)
		parent = wrk->rootClip.getPtr();
	DisplayObject* t =nullptr;
	if (asAtomHandler::isNumeric(target))
	{
		t = parent->getLegacyChildAt(asAtomHandler::toInt(target));
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
	
	th->loaderInfo = th->getContentLoaderInfo();
	t->incRef();
	th->avm1target = _MR(t);
	th->loadIntern(r,nullptr);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,addListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);

	wrk->getSystemState()->stage->AVM1AddEventListener(th);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	o->incRef();
	th->listeners.insert(_MR(o));
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,removeListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);

	
	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	th->listeners.erase(_MR(o));
}
void AVM1MovieClipLoader::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == this->getContentLoaderInfo().getPtr())
	{
		ASWorker* wrk = getInstanceWorker();
		auto it = listeners.begin();
		while (it != listeners.end())
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
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
			else if (e->type == "init")
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
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent());
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
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent());
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
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent());
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
	}
}

bool AVM1MovieClipLoader::destruct()
{
	listeners.clear();
	jobs.clear();
	return Loader::destruct();
}

void AVM1MovieClipLoader::load(const tiny_string& url, const tiny_string& method, AVM1MovieClip* target)
{
	URLRequest* r = Class<URLRequest>::getInstanceS(getInstanceWorker(),url,method);
	target->incRef();
	avm1target = _MR(target);
	loadIntern(r,nullptr);
}

void AVM1Color::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("getRGB","",Class<IFunction>::getFunction(c->getSystemState(),getRGB),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRGB","",Class<IFunction>::getFunction(c->getSystemState(),setRGB),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTransform","",Class<IFunction>::getFunction(c->getSystemState(),getTransform),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTransform","",Class<IFunction>::getFunction(c->getSystemState(),setTransform),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("initialize","",Class<IFunction>::getFunction(c->getSystemState(),initialize),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,initialize)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (!listener.isNull())
	{
		Array* listeners = Class<Array>::getInstanceSNoArgs(wrk);
		listener->setVariableAtomByQName("broadcastMessage",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(wrk->getSystemState(),broadcastMessage)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("addListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(wrk->getSystemState(),addListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("removeListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(wrk->getSystemState(),removeListener)),DYNAMIC_TRAIT);
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
	c->setDeclaredMethodByQName("loadBitmap","",Class<IFunction>::getFunction(c->getSystemState(),loadBitmap),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("rectangle","",Class<IFunction>::getFunction(c->getSystemState(),getRect,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
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

void AVM1Bitmap::sinit(Class_base *c)
{
	Bitmap::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("forceSmoothing","",Class<IFunction>::getFunction(c->getSystemState(),_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("forceSmoothing","",Class<IFunction>::getFunction(c->getSystemState(),_setter_smoothing),SETTER_METHOD,true);
}
