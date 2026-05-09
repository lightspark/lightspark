/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "scripting/abc.h"
#include "scripting/abc_interpreter_helper.h"
#include "scripting/class.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

namespace lightspark
{
bool preload_setlocal(preloadstate& state,memorystream& code,uint32_t value,int32_t p, uint32_t opcode,std::vector<typestackentry>& typestack)
{
#ifdef ENABLE_OPTIMIZATION
	assert_and_throw(value < state.mi->body->getReturnValuePos());
	auto it = state.setlocal_handled.find(p);
	if (it!=state.setlocal_handled.end()
		|| checkInitializeLocalToConstant(state,value))
	{
		if (it!=state.setlocal_handled.end())
		{
			if (state.operandlist.size() > (*it).second)
			{
				state.operandlist[(*it).second].removeArg(state);
				state.operandlist.erase(state.operandlist.begin()+(*it).second);
			}
			state.setlocal_handled.erase(it);
		}
		removetypestack(typestack,1);
		return true;
	}
	removeInitializeLocalToConstant(state,value);
	if (state.operandlist.size() && state.operandlist.back().type==OP_LOCAL && state.operandlist.back().index==(int32_t)value)
	{
		// getlocal followed by setlocal on same index, can be skipped
		state.operandlist.back().removeArg(state);
		state.operandlist.pop_back();
		removetypestack(typestack,1);
		removeInitializeLocalToConstant(state,value);
		return true;
	}
	setOperandModified(state,OP_LOCAL,value);
#endif
	setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_SETLOCAL,opcode,code,p);
	state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint = value;
#ifdef ENABLE_OPTIMIZATION
	if (typestack.back().obj && typestack.back().obj->is<Class_base>())
		state.localtypes[value]=typestack.back().obj->as<Class_base>();
	else
		state.localtypes[value]=nullptr;
#endif
	removetypestack(typestack,1);
	return false;
}

}
