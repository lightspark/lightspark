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
	DisplayObject* t = nullptr;
	if (asAtomHandler::isNumeric(target))
	{
		int depth = asAtomHandler::toInt(target);
		if (sys->mainClip->hasLegacyChildAt(depth))
			t = sys->mainClip->getLegacyChildAt(depth);
	}
	else
		t = asAtomHandler::getObject(target)->as<DisplayObject>();
	if (t)
		t->loaderInfo = th->getContentLoaderInfo();
	th->loadIntern(r,nullptr);
}
