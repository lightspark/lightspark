/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

namespace lightspark
{

class method_info;
class ABCContext;
class ASObject;
class Class_base;

struct scope_entry
{
	_R<ASObject> object;
	bool considerDynamic;
	scope_entry(_R<ASObject> o, bool c):object(o),considerDynamic(c)
	{
	}
};

struct call_context
{
#include "packed_begin.h"
	struct
	{
		ASObject** locals;
		ASObject** stack;
		uint32_t stack_index;
		uint32_t exec_pos;
	} PACKED;
#include "packed_end.h"
	ABCContext* context;
	uint32_t locals_size;
	uint32_t max_stack;
	std::vector<scope_entry> scope_stack;
	method_info* mi;
	/* This is the function's inClass that is currently executing. It is used
	 * by {construct,call,get,set}Super
	 * */
	Class_base* inClass;
	/* Current namespace set by 'default xml namespace = ...'.
	 * Defaults to empty string according to ECMA-357 13.1.1.1
	 */
	tiny_string defaultNamespaceUri;
	int initialScopeStack;
	~call_context();
	void runtime_stack_clear();
	void runtime_stack_push(ASObject* s)
	{
		if(stack_index>=max_stack)
			throw RunTimeException("Stack overflow");
		stack[stack_index++]=s;
	}
	ASObject* runtime_stack_pop()
	{
		if(stack_index==0)
			throw RunTimeException("Empty stack");

		ASObject* ret=stack[--stack_index];
		return ret;
	}
	ASObject* runtime_stack_peek()
	{
		if(stack_index==0)
		{
			LOG(LOG_ERROR,_("Empty stack"));
			return NULL;
		}
		return stack[stack_index-1];
	}
};

};
#endif /* SCRIPTING_ABCUTILS_H */
