#include "avm1ui.h"
#include "scripting/class.h"

using namespace lightspark;

multiname* AVM1ContextMenuItem::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	multiname* res = ContextMenuItem::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONSELECT)
	{
		if (asAtomHandler::isFunction(o))
		{
			this->callbackfunction = _MR(asAtomHandler::getObjectNoCheck(o));
		}
		else
			LOG(LOG_ERROR,"ContextMenuItem.onSelect value is no function:"<<asAtomHandler::toDebugString(o));
	}
	return res;
}

void AVM1ContextMenuItem::sinit(Class_base* c)
{
	CLASS_SETUP(c, NativeMenuItem, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_getter_label),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_setter_label),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,separatorBefore);
	REGISTER_GETTER_SETTER(c,visible);
	REGISTER_GETTER_SETTER(c,enabled);
}
