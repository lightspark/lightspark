/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "ExtensionContext.h"
#include "class.h"
#include "argconv.h"

using namespace lightspark;

ExtensionContext::ExtensionContext(ASWorker* wrk, Class_base* c) : EventDispatcher(wrk,c)
{
	subtype = SUBTYPE_EXTENSIONCONTEXT;
	LOG(LOG_NOT_IMPLEMENTED,"class ExtensionContext is a stub");
}
void ExtensionContext::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);
	c->setDeclaredMethodByQName("createExtensionContext","",Class<IFunction>::getFunction(c->getSystemState(),createExtensionContext,2,Class<ExtensionContext>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(c->getSystemState(),_call,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(ExtensionContext,createExtensionContext)
{
	ExtensionContext* ctxt = Class<ExtensionContext>::getInstanceSNoArgs(wrk);
	ARG_CHECK(ARG_UNPACK(ctxt->extensionID)(ctxt->contextType));
	ret = asAtomHandler::fromObjectNoPrimitive(ctxt);
}
ASFUNCTIONBODY_ATOM(ExtensionContext,_call)
{
	LOG(LOG_NOT_IMPLEMENTED,"ExtensionContext.call always returns null");
	ret = asAtomHandler::nullAtom;
}

