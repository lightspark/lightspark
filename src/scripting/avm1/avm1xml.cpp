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

#include "avm1xml.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/toplevel/AVM1Function.h"

using namespace std;
using namespace lightspark;

void AVM1XMLDocument::finalize()
{
	XMLDocument::finalize();
	loader.reset();
}
bool AVM1XMLDocument::destruct()
{
	loader.reset();
	getSystemState()->stage->AVM1RemoveEventListener(this);
	return XMLDocument::destruct();
}

void AVM1XMLDocument::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	XMLDocument::prepareShutdown();
	if (loader)
		loader->prepareShutdown();
}

void AVM1XMLDocument::sinit(Class_base* c)
{
	XMLDocument::sinit(c);
	c->isSealed = false;
	c->setDeclaredMethodByQName("load","",c->getSystemState()->getBuiltinFunction(load),NORMAL_METHOD,true);
	REGISTER_GETTER(c, status);
}

ASFUNCTIONBODY_GETTER(AVM1XMLDocument,status)

ASFUNCTIONBODY_ATOM(AVM1XMLDocument,load)
{
	AVM1XMLDocument* th=asAtomHandler::as<AVM1XMLDocument>(obj);
	if (th->loader.isNull())
		th->loader = _MR(Class<URLLoader>::getInstanceS(wrk));
	tiny_string url;
	ARG_CHECK(ARG_UNPACK(url));
	URLRequest* req = Class<URLRequest>::getInstanceS(wrk,url);
	asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
	asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(th->loader.getPtr());
	URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
	req->decRef();
}

multiname* AVM1XMLDocument::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset,ASWorker* wrk)
{
	multiname* res = XMLDocument::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
	{
		if (loader.isNull())
			loader = _MR(Class<URLLoader>::getInstanceS(wrk));
		getSystemState()->stage->AVM1AddEventListener(this);
		setIsEnumerable(name, false);
	}
	return res;
}
void AVM1XMLDocument::AVM1HandleEvent(EventDispatcher* dispatcher, Event* e)
{
	if (dispatcher == loader.getPtr())
	{
		if (e->type == "complete" || e->type=="ioError")
		{
			if (e->type == "complete" && loader->getData())
			{
				std::string str = loader->getData()->toString().raw_buf();
				this->status = this->parseXMLImpl(str);
			}
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];
				args[0] = asAtomHandler::fromBool(e->type=="complete");
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
	}
}
