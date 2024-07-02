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

#ifndef SCRIPTING_ABCUTILS_H
#define SCRIPTING_ABCUTILS_H 1

#include "asobject.h"
#include "smartrefs.h"
#include "errorconstants.h"

namespace lightspark
{

class method_info;
class ABCContext;
class ASObject;
class Class_base;
union asAtom;
class SyntheticFunction;

struct scope_entry
{
	asAtom object;
	bool considerDynamic;
	scope_entry(asAtom o, bool c):object(o),considerDynamic(c)
	{
	}
};
class scope_entry_list: public RefCountable
{
public:
	std::vector<scope_entry> scope;
};
struct variable;
struct call_context
{
	asAtom* locals;
	asAtom* stack;
	asAtom* stackp;
	struct preloadedcodedata* exec_pos;
	asAtom* max_stackp;
	asAtom* lastlocal;
	scope_entry_list* parent_scope_stack;
	uint32_t curr_scope_stack;
	int32_t argarrayposition; // position of argument array in locals ( -1 if no argument array needed)
	asAtom* scope_stack;
	bool* scope_stack_dynamic;
	asAtom** localslots;
	method_info* mi;
	/* This is the function that is currently executing.
	 * */
	SyntheticFunction* function;
	SystemState* sys;
	class ASWorker* worker;
	bool explicitConstruction;
	/* Current namespace set by 'default xml namespace = ...'.
	 * Defaults to empty string according to ECMA-357 13.1.1.1
	 */
	uint32_t defaultNamespaceUri;
	ASObject* exceptionthrown;
	call_context(method_info* _mi):
		locals(nullptr),stack(nullptr),
		stackp(nullptr),exec_pos(nullptr),
		max_stackp(nullptr),
		parent_scope_stack(nullptr),curr_scope_stack(0),argarrayposition(-1),
		scope_stack(nullptr),scope_stack_dynamic(nullptr),localslots(nullptr),mi(_mi),
		function(nullptr),sys(nullptr),worker(nullptr),explicitConstruction(false),defaultNamespaceUri(0),exceptionthrown(nullptr)
	{
	}
	static void handleError(int errorcode);
	inline void runtime_stack_clear()
	{
		while(stackp != stack)
		{
			--stackp;
			ASATOM_DECREF_POINTER(stackp);
		}
	}
};
typedef ASObject* (*synt_function)(call_context* cc);
typedef void (*as_atom_function)(asAtom&, ASWorker*, asAtom&, asAtom*, const unsigned int);


class AVM1context
{
friend class AVM1Function;
private:
	std::vector<uint32_t> avm1strings;
public:
	AVM1context():keepLocals(true) {}
	void AVM1ClearConstants()
	{
		avm1strings.clear();
	}
	void AVM1AddConstant(uint32_t nameID)
	{
		avm1strings.push_back(nameID);
	}
	asAtom AVM1GetConstant(uint16_t index)
	{
		if (index < avm1strings.size())
			return asAtomHandler::fromStringID(avm1strings[index]);
		LOG(LOG_ERROR,"AVM1:constant not found in pool:"<<index<<" "<<avm1strings.size());
		return asAtomHandler::undefinedAtom;
	}
	bool keepLocals;
};


#define CONTEXT_GETLOCAL(context,pos) \
	(*(context->localslots[pos]))

#define RUNTIME_STACK_PUSH(context,val) \
if(USUALLY_FALSE(context->stackp==context->max_stackp)) \
	context->handleError(kStackOverflowError); \
else asAtomHandler::set(*(context->stackp++),val)

#define RUNTIME_STACK_POP(context,ret) \
	if(USUALLY_TRUE(context->stackp!=context->stack)) asAtomHandler::set(ret,*(--context->stackp)); \
	else context->handleError(kStackUnderflowError);


// this creates a pointer to the element on top of the stack
// don't use the pointer after something was pushed on the stack
#define RUNTIME_STACK_POP_CREATE(context,ret) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
	  context->handleError(kStackUnderflowError); \
	asAtom* ret= --context->stackp; 

#define RUNTIME_STACK_POP_N_CREATE(context,n,ret) \
	if(USUALLY_FALSE(context->stackp<context->stack+n)) \
	  context->handleError(kStackUnderflowError); \
	context->stackp-=n; \
	asAtom* ret= context->stackp;

#define RUNTIME_STACK_POP_ASOBJECT(context,ret) \
	if(USUALLY_TRUE(context->stackp!=context->stack)) ret=asAtomHandler::toObject(*(--context->stackp),context->worker); \
	else context->handleError(kStackUnderflowError);

#define RUNTIME_STACK_POP_CREATE_ASOBJECT(context,ret) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
		context->handleError(kStackUnderflowError); \
	ASObject* ret = asAtomHandler::toObject(*(--context->stackp),context->worker);

#define RUNTIME_STACK_PEEK(context,ret) \
	ret= USUALLY_TRUE(context->stackp != context->stack) ? (context->stackp-1) : nullptr; 

#define RUNTIME_STACK_PEEK_ASOBJECT(context,ret) \
	ret= USUALLY_TRUE(context->stackp != context->stack) ? asAtomHandler::toObject(*(context->stackp-1),context->worker) : nullptr; 

#define RUNTIME_STACK_PEEK_CREATE(context,ret) \
	asAtom* ret = USUALLY_TRUE(context->stackp != context->stack) ? (context->stackp-1) : nullptr; 

#define RUNTIME_STACK_POINTER_CREATE(context,ret) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
		context->handleError(kStackUnderflowError); \
	asAtom* ret = context->stackp-1;

}
#endif /* SCRIPTING_ABCUTILS_H */
