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

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "abc.h"

using namespace lightspark;

Global::Global(ASWorker* wrk, Class_base* cb, ABCContext* c, int s, bool avm1):ASObject(wrk,cb,T_OBJECT,SUBTYPE_GLOBAL),scriptId(s),context(c),isavm1(avm1)
{
}

void Global::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef(c->getSystemState()));
}

void Global::getVariableByMultinameOpportunistic(asAtom& ret, const multiname& name,ASWorker* wrk)
{
	ASObject::getVariableByMultinameIntern(ret,name,this->getClass(),NO_INCREF,wrk);
	//Do not attempt to define the variable now in any case
}

GET_VARIABLE_RESULT Global::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	/*
	 * All properties are registered by now, even if the script init has
	 * not been run. Thus if ret == NULL, we don't have to run the script init.
	 */
	if(asAtomHandler::isInvalid(ret) || !context || context->hasRunScriptInit[scriptId])
		return res;
	LOG_CALL("Access to " << name << ", running script init");
	asAtom v = asAtomHandler::fromObject(this);
	context->runScriptInit(scriptId,v);
	return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
}

multiname *Global::setVariableByMultiname(multiname &name, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool *alreadyset,ASWorker* wrk)
{
	if (context && !context->hasRunScriptInit[scriptId])
	{
		LOG_CALL("Access to " << name << ", running script init");
		asAtom v = asAtomHandler::fromObject(this);
		context->runScriptInit(scriptId,v);
	}
	return setVariableByMultiname_intern(name,o,allowConst,this->getClass(),alreadyset,wrk);
}

void Global::registerBuiltin(const char* name, const char* ns, _R<ASObject> o, NS_KIND nskind)
{
	o->incRef();
	o->setRefConstant();
	if (o->is<Class_base>())
		o->as<Class_base>()->setGlobalScope(this);
	setVariableByQName(name,nsNameAndKind(getSystemState(),ns,nskind),o.getPtr(),CONSTANT_TRAIT);
}

void Global::checkScriptInit()
{
	if(context->hasRunScriptInit[scriptId])
		return;
	LOG_CALL("running script init "<<scriptId);
	asAtom v = asAtomHandler::fromObject(this);
	context->runScriptInit(scriptId,v);
}
