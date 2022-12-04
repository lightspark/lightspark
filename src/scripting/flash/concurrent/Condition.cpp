/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "scripting/flash/concurrent/Condition.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ASCondition::ASCondition(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_CONDITION)
{
	
}
void ASCondition::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->setVariableByQName("isSupported","",abstract_b(c->getSystemState(),false),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("notify","",Class<IFunction>::getFunction(c->getSystemState(),_notify),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("notifyAll","",Class<IFunction>::getFunction(c->getSystemState(),_notifyAll),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("wait","",Class<IFunction>::getFunction(c->getSystemState(),_wait),NORMAL_METHOD,true);
	REGISTER_GETTER(c,mutex);
}

ASFUNCTIONBODY_GETTER(ASCondition,mutex)

ASFUNCTIONBODY_ATOM(ASCondition,_constructor)
{
	ASCondition* th=asAtomHandler::as<ASCondition>(obj);
	_NR<ASObject> arg;
	ARG_CHECK(ARG_UNPACK(arg));
	if (arg->is<Null>())
	{
		createError<ArgumentError>(wrk,kNullPointerError);
		return;
	}
	
	if (!arg->is<ASMutex>())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError) ;
		return;
	}
	arg->incRef();
	th->mutex = _NR<ASMutex>(arg->as<ASMutex>());
}
ASFUNCTIONBODY_ATOM(ASCondition,_notify)
{
	LOG(LOG_NOT_IMPLEMENTED,"condition notify not implemented");
	ASCondition* th=asAtomHandler::as<ASCondition>(obj);
	if (!th->mutex->getLockCount())
		createError<ASError>(wrk,kConditionCannotNotify);
	asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(ASCondition,_notifyAll)
{
	LOG(LOG_NOT_IMPLEMENTED,"condition notifyAll not implemented");
	ASCondition* th=asAtomHandler::as<ASCondition>(obj);
	if (!th->mutex->getLockCount())
		createError<ASError>(wrk,kConditionCannotNotifyAll) ;
	asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(ASCondition,_wait)
{
	LOG(LOG_NOT_IMPLEMENTED,"condition wait not implemented");
	ASCondition* th=asAtomHandler::as<ASCondition>(obj);
	if (!th->mutex->getLockCount())
		createError<ASError>(wrk,kConditionCannotWait) ;
	asAtomHandler::setBool(ret,true);
}

