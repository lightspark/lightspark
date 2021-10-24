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
#include "parsing/tags.h"
#include "scripting/abc.h"
#include "backends/security.h"

using namespace std;
using namespace lightspark;

void AVM1MovieClip::afterConstruction()
{
	MovieClip::afterConstruction();
	if (getSystemState()->getSwfVersion() >= 6)
	{
		this->incRef();
		getVm(getSystemState())->prependEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"load")));
	}
}

void AVM1MovieClip::sinit(Class_base* c)
{
	MovieClip::sinit(c);
	MovieClip::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("_totalframes","",Class<IFunction>::getFunction(c->getSystemState(),_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_currentframe","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_framesloaded","",Class<IFunction>::getFunction(c->getSystemState(),_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_name","",Class<IFunction>::getFunction(c->getSystemState(),_getter_name),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(c->getSystemState(),startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(c->getSystemState(),stopDrag),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,startDrag)
{
	bool lockcenter;
	number_t x1, y1, x2, y2;
	ARG_UNPACK_ATOM (lockcenter,false)(x1,0)(y1,0)(x2,0)(y2,0);

	if (argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::getInstanceS(sys);
		asAtom fret = asAtomHandler::invalidAtom;
		asAtom fobj = asAtomHandler::fromObject(rect);
		asAtom fx1 = asAtomHandler::fromNumber(sys, x1, false);
		asAtom fy1 = asAtomHandler::fromNumber(sys, y1, false);
		asAtom fx2 = asAtomHandler::fromNumber(sys, x2, false);
		asAtom fy2 = asAtomHandler::fromNumber(sys, y2, false);
		Rectangle::_setLeft(fret,sys,fobj,&fx1,1);
		Rectangle::_setTop(fret,sys,fobj,&fy1,1);
		Rectangle::_setRight(fret,sys,fobj,&fx2,1);
		Rectangle::_setBottom(fret,sys,fobj,&fy2,1);
	
		asAtom fargs[2];
		fargs[0] = asAtomHandler::fromBool(lockcenter);
		fargs[1] = asAtomHandler::fromObject(rect);
		Sprite::_startDrag(ret,sys,obj,fargs,2);
	}
	else
	{
		asAtom fargs[1];
		fargs[0] = asAtomHandler::fromBool(lockcenter);
		Sprite::_startDrag(ret,sys,obj,fargs,1);
	}
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,stopDrag)
{
	Sprite::_stopDrag(ret,sys,obj,nullptr,0);
}

void AVM1Shape::sinit(Class_base* c)
{
	Shape::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
}

void AVM1SimpleButton::sinit(Class_base* c)
{
	SimpleButton::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
}

void AVM1Stage::sinit(Class_base* c)
{
	// in AVM1 Stage is no DisplayObject and all methods/properties are static
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,false);
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
	ret = asAtomHandler::fromString(sys,sys->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setDisplayState)
{
	ARG_UNPACK_ATOM(sys->stage->displayState);
	sys->stage->onDisplayState(sys->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_getShowMenu)
{
	ret = asAtomHandler::fromBool(sys->stage->showDefaultContextMenu);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setShowMenu)
{
	ARG_UNPACK_ATOM(sys->stage->showDefaultContextMenu);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,getAlign)
{
	ret = asAtomHandler::fromString(sys,sys->stage->align);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,setAlign)
{
	tiny_string oldalign=sys->stage->align;
	ARG_UNPACK_ATOM(sys->stage->align);
	sys->stage->onAlign(oldalign);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,addResizeListener)
{
	_NR<ASObject> listener;
	ARG_UNPACK_ATOM(listener,NullRef);
	sys->stage->AVM1AddResizeListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Stage,removeResizeListener)
{
	_NR<ASObject> listener;
	ARG_UNPACK_ATOM(listener,NullRef);
	ret = asAtomHandler::fromBool(sys->stage->AVM1RemoveResizeListener(listener.getPtr()));
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
	ARG_UNPACK_ATOM (strurl)(target);
	URLRequest* r = Class<URLRequest>::getInstanceS(sys,strurl);
	DisplayObjectContainer* parent = th->getParent();
	if (!parent)
		parent = sys->mainClip;
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
		throwError<ArgumentError>(kInvalidArgumentError,"target");
	if (!t)
		throwError<ArgumentError>(kInvalidArgumentError,"target");
	
	th->loaderInfo = th->getContentLoaderInfo();
	t->incRef();
	th->avm1target = _MR(t);
	th->loadIntern(r,nullptr);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,addListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);

	sys->stage->AVM1AddEventListener(th);
	ASObject* o = asAtomHandler::toObject(args[0],sys);

	o->incRef();
	th->listeners.insert(_MR(o));
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,removeListener)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);

	
	ASObject* o = asAtomHandler::toObject(args[0],sys);

	th->listeners.erase(_MR(o));
}
void AVM1MovieClipLoader::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == this->getContentLoaderInfo().getPtr())
	{
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
				(*it)->getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					
					asAtom args[1];
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent().getPtr());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
			}
			else if (e->type == "init")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadInit");
				(*it)->getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent().getPtr());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
			}
			else if (e->type == "progress")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadProgress");
				(*it)->getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					ProgressEvent* ev = e->as<ProgressEvent>();
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[3];
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent().getPtr());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					args[1] = asAtomHandler::fromInt(ev->bytesLoaded);
					args[2] = asAtomHandler::fromInt(ev->bytesTotal);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,3);
				}
			}
			else if (e->type == "complete")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadComplete");
				(*it)->getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					if (this->getContent())
					{
						this->getContent()->incRef();
						args[0] = asAtomHandler::fromObject(this->getContent().getPtr());
					}
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
			}
			else if (e->type == "ioError")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadError");
				(*it)->getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
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
	ARG_UNPACK_ATOM(th->target);
}
ASFUNCTIONBODY_ATOM(AVM1Color,getRGB)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	if (th->target && th->target->colorTransform)
	{
		asAtom t = asAtomHandler::fromObject(th->target->colorTransform.getPtr());
		ColorTransform::getColor(ret,sys,t,nullptr,0);
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
		ColorTransform::setColor(ret,sys,t,args,1);
		th->target->hasChanged=true;
		th->target->requestInvalidation(sys);
	}
}

ASFUNCTIONBODY_ATOM(AVM1Color,getTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	ASObject* o = Class<ASObject>::getInstanceS(sys);
	if (th->target && th->target->colorTransform)
	{
		asAtom a= asAtomHandler::undefinedAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=sys->getUniqueStringId("ra");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->redMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("rb");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->redOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("ga");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->greenMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("gb");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->greenOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("ba");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->blueMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("bb");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->blueOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("aa");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->alphaMultiplier*100.0,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

		m.name_s_id=sys->getUniqueStringId("ab");
		a = asAtomHandler::fromNumber(sys,th->target->colorTransform->alphaOffset,false);
		o->setVariableByMultiname(m,a,ASObject::CONST_ALLOWED);

	}
	ret = asAtomHandler::fromObject(o);
}

ASFUNCTIONBODY_ATOM(AVM1Color,setTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	assert_and_throw(argslen==1);
	ASObject* o = asAtomHandler::toObject(args[0],sys);
	if (th->target)
	{
		if (!th->target->colorTransform)
			th->target->colorTransform = _R<ColorTransform>(Class<ColorTransform>::getInstanceSNoArgs(sys));
		asAtom a = asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=sys->getUniqueStringId("ra");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->redMultiplier = asAtomHandler::toNumber(a)/100.0;

		m.name_s_id=sys->getUniqueStringId("rb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->redOffset = asAtomHandler::toNumber(a);

		m.name_s_id=sys->getUniqueStringId("ga");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->greenMultiplier = asAtomHandler::toNumber(a)/100.0;

		m.name_s_id=sys->getUniqueStringId("gb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->greenOffset = asAtomHandler::toNumber(a);

		m.name_s_id=sys->getUniqueStringId("ba");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->blueMultiplier = asAtomHandler::toNumber(a)/100.0;

		m.name_s_id=sys->getUniqueStringId("bb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->blueOffset = asAtomHandler::toNumber(a);

		m.name_s_id=sys->getUniqueStringId("aa");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->alphaMultiplier = asAtomHandler::toNumber(a)/100.0;

		m.name_s_id=sys->getUniqueStringId("ab");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform->alphaOffset = asAtomHandler::toNumber(a);
		th->target->hasChanged=true;
		th->target->requestInvalidation(sys);
	}
}
bool AVM1Color::destruct()
{
	target.reset();
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
	ARG_UNPACK_ATOM(listener);
	if (!listener.isNull())
	{
		Array* listeners = Class<Array>::getInstanceSNoArgs(sys);
		listener->setVariableAtomByQName("broadcastMessage",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(sys,broadcastMessage)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("addListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(sys,addListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("removeListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<IFunction>::getFunction(sys,removeListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("_listeners",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(listeners),DYNAMIC_TRAIT);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,broadcastMessage)
{
	ASObject* th = asAtomHandler::getObject(obj);
	tiny_string msg;
	ARG_UNPACK_ATOM(msg);
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=sys->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m);
	if (asAtomHandler::isArray(l))
	{
		multiname mmsg(nullptr);
		mmsg.name_type=multiname::NAME_STRING;
		mmsg.name_s_id=sys->getUniqueStringId(msg);
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o;
			listeners->at_nocheck(o,i);
			if (asAtomHandler::isObject(o))
			{
				ASObject* listener = asAtomHandler::getObjectNoCheck(o);
				asAtom f = asAtomHandler::invalidAtom;
				listener->getVariableByMultiname(f,mmsg);
				asAtom res;
				if (asAtomHandler::is<Function>(f))
				{
					asAtomHandler::as<Function>(f)->call(res,o,nullptr,0);
				}
				else if (asAtomHandler::is<SyntheticFunction>(f))
				{
					asAtomHandler::as<SyntheticFunction>(f)->call(res,o,nullptr,0,false,false);
				}
				else if (asAtomHandler::is<AVM1Function>(f))
				{
					asAtomHandler::as<AVM1Function>(f)->call(&res,&o,nullptr,0);
				}
			}
		}
	}
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,addListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	_NR<ASObject> listener;
	ARG_UNPACK_ATOM(listener);
	if (listener.isNull())
	{
		ret = asAtomHandler::falseAtom;
		return;
	}
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=sys->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m);
	if (asAtomHandler::isArray(l))
	{
		// TODO spec is not clear if listener can be added multiple times
		Array* listeners = asAtomHandler::as<Array>(l);
		listeners->push(asAtomHandler::fromObjectNoPrimitive(listener.getPtr()));
	}
	ret = asAtomHandler::trueAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,removeListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	_NR<ASObject> listener;
	ARG_UNPACK_ATOM(listener);
	ret = asAtomHandler::falseAtom;
	if (listener.isNull())
		return;
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=sys->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m);
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
				listeners->removeAt(res,sys,obj,&index,1);
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
	ARG_UNPACK_ATOM(name);
	BitmapTag* tag = dynamic_cast<BitmapTag*>( sys->mainClip->dictionaryLookupByName(sys->getUniqueStringId(name)));
	if (tag)
		ret = asAtomHandler::fromObjectNoPrimitive(tag->instance());
	else
		LOG(LOG_ERROR,"BitmapData.loadBitmap tag not found:"<<name);
}

void AVM1Bitmap::sinit(Class_base *c)
{
	Bitmap::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
}
