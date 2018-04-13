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

#include "smartrefs.h"
#include "errorconstants.h"

namespace lightspark
{

class method_info;
class ABCContext;
class ASObject;
class Class_base;
class asAtom;

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
struct call_context
{
#include "packed_begin.h"
	struct
	{
		asAtom* locals;
		asAtom* stack;
		asAtom* stackp;
		struct preloadedcodedata* exec_pos;
	} PACKED;
#include "packed_end.h"
	uint32_t locals_size;
	asAtom* max_stackp;
	int32_t argarrayposition; // position of argument array in locals ( -1 if no argument array needed)
	_NR<scope_entry_list> parent_scope_stack;
	uint32_t max_scope_stack;
	uint32_t curr_scope_stack;
	asAtom* scope_stack;
	bool* scope_stack_dynamic;
	method_info* mi;
	/* This is the function's inClass that is currently executing. It is used
	 * by {construct,call,get,set}Super
	 * */
	Class_base* inClass;
	/* Current namespace set by 'default xml namespace = ...'.
	 * Defaults to empty string according to ECMA-357 13.1.1.1
	 */
	uint32_t defaultNamespaceUri;
	asAtom& returnvalue;
	bool returning;
	call_context(method_info* _mi,Class_base* _inClass, asAtom& ret);
	static void handleError(int errorcode);
	inline void runtime_stack_clear()
	{
		while(stackp != stack)
		{
			ASATOM_DECREF_POINTER((--stackp));
		}
	}
};
#define RUNTIME_STACK_PUSH(context,val) \
if(USUALLY_FALSE(context->stackp==context->max_stackp)) \
	context->handleError(kStackOverflowError); \
else (context->stackp++)->set(val)

#define RUNTIME_STACK_POP(context,ret) \
	if(USUALLY_TRUE(context->stackp!=context->stack)) ret.set(*(--context->stackp)); \
	else context->handleError(kStackUnderflowError);


// this creates a pointer to the element on top of the stack
// don't use the pointer after something was pushed on the stack
#define RUNTIME_STACK_POP_CREATE(context,ret) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
	  context->handleError(kStackUnderflowError); \
	asAtom* ret= --context->stackp; 

#define RUNTIME_STACK_POP_ASOBJECT(context,ret, sys) \
	if(USUALLY_TRUE(context->stackp!=context->stack)) ret=(--context->stackp)->toObject(sys); \
	else context->handleError(kStackUnderflowError);

#define RUNTIME_STACK_POP_CREATE_ASOBJECT(context,ret, sys) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
		context->handleError(kStackUnderflowError); \
	ASObject* ret = (--context->stackp)->toObject(sys);

#define RUNTIME_STACK_PEEK(context,ret) \
	ret= USUALLY_TRUE(context->stackp != context->stack) ? (context->stackp-1) : NULL; 

#define RUNTIME_STACK_PEEK_ASOBJECT(context,ret, sys) \
	ret= USUALLY_TRUE(context->stackp != context->stack) ? (context->stackp-1)->toObject(sys) : NULL; 

#define RUNTIME_STACK_PEEK_CREATE(context,ret) \
	asAtom* ret = USUALLY_TRUE(context->stackp != context->stack) ? (context->stackp-1) : NULL; 

#define RUNTIME_STACK_POINTER_CREATE(context,ret) \
	if(USUALLY_FALSE(context->stackp==context->stack)) \
		context->handleError(kStackUnderflowError); \
	asAtom* ret = context->stackp-1;

}
#endif /* SCRIPTING_ABCUTILS_H */
