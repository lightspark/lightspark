/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _ABCUTILS_H
#define _ABCUTILS_H

#include "smartrefs.h"

namespace lightspark
{

class method_info;
class ABCContext;
class ASObject;

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
	} PACKED;
#include "packed_end.h"
	ABCContext* context;
	int locals_size;
	std::vector<scope_entry> scope_stack;
	int initialScopeStack;
	void runtime_stack_push(ASObject* s);
	void runtime_stack_clear();
	ASObject* runtime_stack_pop();
	ASObject* runtime_stack_peek();
	method_info* mi;
	std::istringstream* code;
	call_context(method_info* th, int l, ASObject* const* args, const unsigned int numArgs);
	~call_context();
};

};
#endif
