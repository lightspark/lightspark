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
	ARG_UNPACK_ATOM (lockcenter)(x1)(y1)(x2)(y2);

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
	Stage::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
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
	int depth =0;
	if (asAtomHandler::isNumeric(target))
	{
		depth = asAtomHandler::toInt(target);
	}
	else if (asAtomHandler::is<DisplayObject>(target))
	{
		DisplayObject* t = asAtomHandler::getObject(target)->as<DisplayObject>();
		if (t->getParent())
		{
			parent = t->getParent();
			depth = t->getParent()->findLegacyChildDepth(t);
			parent->deleteLegacyChildAt(depth);
		}
	}
	else
		throwError<ArgumentError>(kInvalidArgumentError,"target");
	
	th->loaderInfo = th->getContentLoaderInfo();
	parent->insertLegacyChildAt(depth+1,th);
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
void AVM1MovieClipLoader::AVM1HandleEvent(EventDispatcher *dispatcher, _R<Event> e)
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
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
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
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
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
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
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
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
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
