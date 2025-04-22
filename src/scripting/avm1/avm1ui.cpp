#include "avm1ui.h"
#include "avm1array.h"
#include "scripting/class.h"

using namespace lightspark;

AVM1ContextMenu::AVM1ContextMenu(ASWorker* wrk, Class_base* c):ContextMenu(wrk,c),avm1builtInItems(asAtomHandler::invalidAtom)
{
}

void AVM1ContextMenu::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, AVM1_constructor, CLASS_FINAL);
	c->prototype->setDeclaredMethodByQName("builtInItems","",c->getSystemState()->getBuiltinFunction(AVM1_get_builtInItems),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("copy","",c->getSystemState()->getBuiltinFunction(AVM1_copy),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("customItems","",c->getSystemState()->getBuiltinFunction(_getter_customItems),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("customItems","",c->getSystemState()->getBuiltinFunction(_setter_customItems),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("hideBuiltInItems","",c->getSystemState()->getBuiltinFunction(hideBuiltInItems),NORMAL_METHOD,false,false);
}
void AVM1ContextMenu::finalize()
{
	ASATOM_REMOVESTOREDMEMBER(avm1builtInItems);
	avm1builtInItems = asAtomHandler::invalidAtom;
	ContextMenu::finalize();
}

bool AVM1ContextMenu::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(avm1builtInItems);
	avm1builtInItems = asAtomHandler::invalidAtom;
	return ContextMenu::destruct();
}

void AVM1ContextMenu::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ContextMenu::prepareShutdown();
	ASObject* o = asAtomHandler::getObject(avm1builtInItems);
	if(o)
		o->prepareShutdown();
}

bool AVM1ContextMenu::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ContextMenu::countCylicMemberReferences(gcstate);
	ASObject* o = asAtomHandler::getObject(avm1builtInItems);
	if(o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void AVM1ContextMenu::AVM1enumerate(std::stack<asAtom>& stack)
{
	ContextMenu::AVM1enumerate(stack);
	asAtom name;
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = BUILTIN_STRINGS::STRING_ONSELECT;
	m.isAttribute = false;
	if (!this->hasPropertyByMultiname(m,true,false,getInstanceWorker()))
	{
		name = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_ONSELECT);
		ACTIONRECORD::PushStack(stack, name);
	}
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("builtInItems"));
	ACTIONRECORD::PushStack(stack, name);
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("customItems"));
	ACTIONRECORD::PushStack(stack, name);
}
ASFUNCTIONBODY_ATOM(AVM1ContextMenu,AVM1_constructor)
{
	ContextMenu::_constructor(ret,wrk,obj, nullptr, 0);
	AVM1ContextMenu* th=asAtomHandler::as<AVM1ContextMenu>(obj);
	th->customItems = _MR(Class<AVM1Array>::getInstanceSNoArgs(wrk));
	ASObject* o = new_asobject(wrk);
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.isAttribute = false;

	m.name_s_id = wrk->getSystemState()->getUniqueStringId("print");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("forward_back");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("rewind");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("loop");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("play");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("quality");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("zoom");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	m.name_s_id = wrk->getSystemState()->getUniqueStringId("save");
	o->setVariableByMultiname(m,asAtomHandler::trueAtom,CONST_ALLOWED,nullptr,wrk);
	th->avm1builtInItems = asAtomHandler::fromObjectNoPrimitive(o);
	o->addStoredMember();
}

ASFUNCTIONBODY_ATOM(AVM1ContextMenu,AVM1_get_builtInItems)
{
	AVM1ContextMenu* th=asAtomHandler::as<AVM1ContextMenu>(obj);
	ASATOM_INCREF(th->avm1builtInItems);
	ret = th->avm1builtInItems;
}
ASFUNCTIONBODY_ATOM(AVM1ContextMenu,AVM1_copy)
{
	AVM1ContextMenu* th=asAtomHandler::as<AVM1ContextMenu>(obj);
	AVM1ContextMenu* res = Class<AVM1ContextMenu>::getInstanceSNoArgs(wrk);
	ASObject* src = asAtomHandler::getObject(th->avm1builtInItems);
	if (src)
	{
		ASObject* o = new_asobject(wrk);
		src->copyValues(o,wrk);
		res->avm1builtInItems=asAtomHandler::fromObjectNoPrimitive(o);
		o->addStoredMember();
	}
	res->customItems = _MR(Class<AVM1Array>::getInstanceSNoArgs(wrk));
	if (th->customItems)
	{
		for (uint32_t i=0; i < th->customItems->size(); i++)
		{
			asAtom a=asAtomHandler::invalidAtom;
			th->customItems->at_nocheck(a,i);
			ASATOM_INCREF(a);
			res->customItems->push(a);
		}
	}
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}

bool AVM1ContextMenu::builtInItemEnabled(const tiny_string& name)
{
	ASObject* o = asAtomHandler::getObject(this->avm1builtInItems);
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = getSystemState()->getUniqueStringId(name);
	asAtom ret = asAtomHandler::invalidAtom;
	o->getVariableByMultiname(ret,m,
						GET_VARIABLE_OPTION(GET_VARIABLE_OPTION::NO_INCREF|GET_VARIABLE_OPTION::DONT_CALL_GETTER|GET_VARIABLE_OPTION::DONT_CHECK_CLASS|GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE),
						getInstanceWorker());
	return asAtomHandler::AVM1toBool(ret,getInstanceWorker(),getInstanceWorker()->AVM1getSwfVersion());
}

void AVM1ContextMenuItem::sinit(Class_base* c)
{
	CLASS_SETUP(c, NativeMenuItem, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->prototype->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_getter_label),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_setter_label),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("separatorBefore","",c->getSystemState()->getBuiltinFunction(_getter_separatorBefore),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("separatorBefore","",c->getSystemState()->getBuiltinFunction(_setter_separatorBefore),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("visible","",c->getSystemState()->getBuiltinFunction(_getter_visible),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("visible","",c->getSystemState()->getBuiltinFunction(_setter_visible),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(_getter_enabled),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(_setter_enabled),SETTER_METHOD,false,false);
}
multiname* AVM1ContextMenuItem::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	multiname* res = ContextMenuItem::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONSELECT)
	{
		if (asAtomHandler::isFunction(o))
		{
			ASATOM_ADDSTOREDMEMBER(o);
			ASATOM_REMOVESTOREDMEMBER(this->callbackfunction);
			this->callbackfunction = o;
		}
		else
			LOG(LOG_ERROR,"ContextMenuItem.onSelect value is no function:"<<asAtomHandler::toDebugString(o));
	}
	return res;
}

GET_VARIABLE_RESULT AVM1ContextMenuItem::AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk, bool isSlashPath)
{
	if (name.name_type==multiname::NAME_STRING && name.name_s_id == BUILTIN_STRINGS::STRING_ONSELECT)
	{
		if ((opt & GET_VARIABLE_OPTION::NO_INCREF) == 0)
			ASATOM_INCREF(this->callbackfunction);
		ret = this->callbackfunction;
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	else
		return ContextMenuItem::AVM1getVariableByMultiname(ret,name,opt,wrk,isSlashPath);
}

void AVM1ContextMenuItem::AVM1enumerate(std::stack<asAtom>& stack)
{
	ContextMenuItem::AVM1enumerate(stack);
	asAtom name;
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("caption"));
	ACTIONRECORD::PushStack(stack, name);
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = BUILTIN_STRINGS::STRING_ONSELECT;
	m.isAttribute = false;
	if (!this->hasPropertyByMultiname(m,true,false,getInstanceWorker()))
	{
		asAtom name = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_ONSELECT);
		ACTIONRECORD::PushStack(stack, name);
	}
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("separatorBefore"));
	ACTIONRECORD::PushStack(stack, name);
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("enabled"));
	ACTIONRECORD::PushStack(stack, name);
	name = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId("visible"));
	ACTIONRECORD::PushStack(stack, name);
}
