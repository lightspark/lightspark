/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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
#include "compat.h"
#include "scripting/abc_interpreter_helper.h"
#include "scripting/class.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "parsing/streams.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

namespace lightspark
{
preloadstate::preloadstate(SyntheticFunction* _f, ASWorker* _w):
	function(_f),worker(_w),
	mi(_f->getMethodInfo()),
	lastlocalresultpos(UINT32_MAX),
	duplocalresult(false),
	atexceptiontarget(false)
{

}

void preloadstate::refreshOldNewPosition(memorystream& code)
{
	if (!atexceptiontarget)
		oldnewpositions[code.tellg()] = (int32_t)preloadedcode.size();
}

void preloadstate::checkClearOperands(uint32_t p, Class_base** lastlocalresulttype)
{
	if (!atexceptiontarget && jumptargets.find(p) != jumptargets.end())
		clearOperands(*this,true,lastlocalresulttype);
}
void skipjump(preloadstate& state,uint8_t& b,memorystream& code,uint32_t& pos,bool jumpInCode)
{
	if (b == 0x10) // jump
	{
		int32_t j = code.peeks24FromPosition(pos);
		uint32_t p = pos+3;//3 bytes from s24;
		bool hastargets = j < 0;
		if (!hastargets)
		{
			// check if the code following the jump is unreachable
			for (int32_t i = 0; i < j; i++)
			{
				if (state.jumptargets.find(p+i+1) != state.jumptargets.end())
				{
					hastargets = true;
					break;
				}
			}
		}
		if (!hastargets)
		{
			// skip unreachable code
			pos = p+j;
			auto it = state.jumptargets.find(p+j+1);
			if (it != state.jumptargets.end() && it->second > 1)
				state.jumptargets[p+j+1]--;
			else
				state.jumptargets.erase(p+j+1);
			state.oldnewpositions[p+j] = (int32_t)state.preloadedcode.size();
			b = code.peekbyteFromPosition(pos);
			if (jumpInCode)
			{
				state.refreshOldNewPosition(code);
				code.seekg(pos);
				state.oldnewpositions[code.tellg()+1] = (int32_t)state.preloadedcode.size();
			}
			pos++;
		}
	}
}
void clearOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,bool checkchanged, OPERANDTYPES type, int index)
{
	if (resetlocaltypes)
	{
		for (uint32_t i = 0; i < state.mi->body->getReturnValuePos()+state.mi->body->localresultcount; i++)
		{
			assert(i < state.defaultlocaltypes.size() && "array out of bounds!");
			state.localtypes[i] = state.defaultlocaltypes[i];
		}
	}
	state.lastlocalresultpos=UINT32_MAX;
	if (lastlocalresulttype)
		*lastlocalresulttype=nullptr;
	bool clear = !checkchanged;
	for (auto it = state.operandlist.begin(); !clear && it != state.operandlist.end();it++)
	{
		if (it->modified && it->type == type && it->index == index)
		{
			clear=true;
			break;
		}
	}
	if (clear)
		state.operandlist.clear();
	state.duplocalresult=false;
}
void removeOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,uint32_t opcount)
{
	if (resetlocaltypes)
	{
		for (uint32_t i = 0; i < state.mi->body->getReturnValuePos()+state.mi->body->localresultcount; i++)
		{
			assert(i < state.defaultlocaltypes.size() && "array out of bounds!");
			state.localtypes[i] = state.defaultlocaltypes[i];
		}
	}
	if (lastlocalresulttype)
		*lastlocalresulttype=nullptr;
	for (uint32_t i = 0; i < opcount; i++)
		state.operandlist.pop_back();
	state.duplocalresult=false;
}
void setOperandModified(preloadstate& state,OPERANDTYPES type, int index)
{
	for (auto it = state.operandlist.begin(); it != state.operandlist.end();it++)
	{
		if (it->type == type && it->index == index)
		{
			it->modified=true;
		}
	}
}

bool canCallFunctionDirect(operands& op,multiname* name, bool ignoreoverridden)
{
	if (op.objtype && op.objtype->is<Class_inherit>() && !op.objtype->isConstructed())
	{
		if (op.objtype->getInstanceWorker()->rootClip && !op.objtype->getInstanceWorker()->rootClip->hasFinishedLoading())
			return false;
		if (!op.objtype->as<Class_inherit>()->checkScriptInit())
			return false;
	}
	return ((op.type == OP_LOCAL || op.type == OP_CACHED_CONSTANT || op.type == OP_CACHED_SLOT) &&
		op.objtype &&
		!op.objtype->isInterface && // it's not an interface
		(
		ignoreoverridden ||
		!op.objtype->as<Class_base>()->hasoverriddenmethod(name) // current method is not in overridden methods
		));
}
bool canCallFunctionDirect(ASObject* obj,multiname* name, bool ignoreoverridden)
{
	if (!obj || !obj->is<Class_base>())
		return false;
	Class_base* objtype = obj->as<Class_base>();
	if (objtype->is<Class_inherit>() && !objtype->isConstructed())
	{
		if (objtype->getInstanceWorker()->rootClip && !objtype->getInstanceWorker()->rootClip->hasFinishedLoading())
			return false;
		if (!objtype->as<Class_inherit>()->checkScriptInit())
			return false;
	}
	return !objtype->isInterface && // it's not an interface
		(
		!objtype->is<Class_inherit>() || // type is builtin class
		ignoreoverridden ||
		!objtype->as<Class_inherit>()->hasoverriddenmethod(name) // current method is not in overridden methods
		);
}
void setForceInt(preloadstate& state,memorystream& code,Class_base** resulttype)
{
#ifdef ENABLE_OPTIMIZATION
	switch (code.peekbyte())
	{
		case 0x73://convert_i
			state.refreshOldNewPosition(code);
			code.readbyte();
			// falls through
		case 0x35://li8
		case 0x36://li16
		case 0x37://li32
		case 0x38://lf32
		case 0x39://lf64
		case 0x3a://si8
		case 0x3b://si16
		case 0x3c://si32
			state.preloadedcode.back().pcode.local3.flags |= ABC_OP_FORCEINT;
			break;
		default:
			break;
	}
	if (state.preloadedcode.back().pcode.local3.flags & ABC_OP_FORCEINT)
		*resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
#endif
}
bool checkForLocalResult(preloadstate& state,memorystream& code,uint32_t opcode_jumpspace, Class_base* restype,int preloadpos,int preloadlocalpos, bool checkchanged,bool fromdup, uint32_t opcode_setslot)
{
#ifdef ENABLE_OPTIMIZATION
	bool res = false;
	uint32_t pos = code.tellg()+1;
	uint8_t b = code.peekbyte();
	bool keepchecking=true;
	if (preloadpos == -1)
		preloadpos = state.preloadedcode.size()-1;
	if (preloadlocalpos == -1)
		preloadlocalpos = state.preloadedcode.size()-1;
	uint32_t argsneeded=0;

	uint32_t dupskipped=UINT32_MAX;
	state.lastlocalresultpos=UINT32_MAX;
	while (state.jumptargets.find(pos) == state.jumptargets.end() && keepchecking)
	{
		skipjump(state,b,code,pos,false);
		// check if the next opcode can be skipped
		//LOG(LOG_CALLS,"checkforlocal skip:"<<argsneeded<<" "<<state.operandlist.size()<<" "<<pos<<" "<<hex<<(uint32_t)b);
		switch (b)
		{
			case 0x24://pushbyte
				pos++;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x73 ||//convert_i
						b==0x74)//convert_u
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x25://pushshort
			case 0x2d://pushint
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x73)//convert_i
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x26://pushtrue
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x27://pushfalse
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2e://pushuint
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x74)//convert_u
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2f://pushdouble
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x75)//convert_d
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2c://pushstring
			case 0x31://pushnamespace
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0x64://getglobalscope
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0x32://hasnext2
				pos = code.skipu30FromPosition(pos);
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				break;
			case 0x65://getscopeobject
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				break;
			case 0x5d://findpropstrict
			case 0x5e://findproperty
			{
				uint32_t t = code.peeku30FromPosition(pos);
				if (state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					if (state.function->inClass && state.function->inClass->isSealed && !state.function->isFromNewFunction() && !state.mi->needsActivation())
					{
						if (name->isStatic && !state.function->inClass->hasoverriddenmethod(name))
						{
							pos = code.skipu30FromPosition(pos);
							b = code.peekbyteFromPosition(pos);
							pos++;
							argsneeded++;
							break;
						}
					}
					else if (!state.function->inClass && !state.function->isFromNewFunction() && !state.mi->needsActivation())
					{
						if (name->isStatic)
						{
							pos = code.skipu30FromPosition(pos);
							b = code.peekbyteFromPosition(pos);
							pos++;
							argsneeded++;
							break;
						}
					}
				}
				keepchecking=false;
				break;
			}
			case 0x61://setproperty
			case 0x68://initproperty
			{
				uint32_t t = code.peeku30FromPosition(pos);
				if (argsneeded>=2 && !fromdup &&
					(uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded -= 2;
				}
				else
					keepchecking=false;
				break;
			}
			case 0x6c://getslot
				if (argsneeded || state.operandlist.size()>0)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x62://getlocal
			{
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				pos++;
				argsneeded++;
				break;
			}
			case 0x60://getlex
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				break;
			case 0x73://convert_i
				if (argsneeded
						|| restype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x74://convert_u
				if (argsneeded
						|| restype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x75://convert_d
				if (argsneeded
						|| restype == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| restype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| restype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x70://convert_s
			case 0x85://coerce_s
				if (argsneeded
						|| restype == Class<ASString>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x76://convert_b
				if (argsneeded || restype == Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == 0x11 //iftrue
						|| code.peekbyteFromPosition(pos) == 0x12 //iffalse
						|| code.peekbyteFromPosition(pos) == 0x96 //not
						|| code.peekbyteFromPosition(pos) == b
					)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x20://pushnull
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x80 //coerce
					&& (state.jumptargets.find(pos) == state.jumptargets.end()))
				{
					uint32_t t = code.peeku30FromPosition(pos);
					multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					Type* tp = Type::getTypeFromMultiname(name, state.mi->context);
					Class_base* cls =dynamic_cast<Class_base*>(tp);
					if (cls != Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() &&
						cls != Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() &&
						cls != Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() &&
						cls != Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
					{
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
					}
				}
				break;
			case 0x21://pushundefined
			case 0x28://pushnan
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0x2a://dup
				if (!argsneeded)
				{
					keepchecking=false;
					break;
				}
				if (dupskipped==UINT32_MAX)
					dupskipped=argsneeded;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0x80://coerce
			{
				uint32_t t = code.peeku30FromPosition(pos);
				multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (code.peekbyteFromPosition(code.skipu30FromPosition(pos)) == 0x80//coerce
						&& state.jumptargets.find(pos) == state.jumptargets.end())
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				if (argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				if (name->isStatic && !argsneeded)
				{
					Type* tp = Type::getTypeFromMultiname(name,state.mi->context);
					if (tp && (state.jumptargets.find(pos) == state.jumptargets.end()))
					{
						bool skip = false;
						if (restype)
						{
							if (restype == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() ||
									 restype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() ||
									 restype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() ||
									 restype == Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
								skip = tp == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || tp == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || tp == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
							else if (restype != Class<ASString>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
								skip = restype == tp;
						}
						if (skip)
						{
							restype = (Class_base*)tp;
							pos = code.skipu30FromPosition(pos);
							b = code.peekbyteFromPosition(pos);
							pos++;
							break;
						}
					}
				}
				keepchecking=false;
				break;
			}
			case 0x35://li8
			case 0x36://li16
			case 0x37://li32
			case 0x38://lf32
			case 0x39://lf64
			case 0x50://sxi1
			case 0x51://sxi8
			case 0x52://sxi16
			case 0x90://negate
			case 0x96://not
			case 0x97://bitnot
				if (argsneeded)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x08://kill
				if (state.jumptargets.find(pos) == state.jumptargets.end())
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			case 0x09://label
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			case 0x66://getproperty
			{
				uint32_t t = code.peeku30FromPosition(pos);
				if (argsneeded && !fromdup &&
					(uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					// getproperty without runtimeargs following e.g. getlocal may produce local result
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else if (argsneeded > 1 && !fromdup &&
					(uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 1)
				{
					// getproperty with 1 runtime arg needs 2 args and produces 1 arg
					argsneeded--;
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			}
			case 0x45://callsuper
			case 0x4e://callsupervoid
			case 0x4a://constructprop
			case 0x4f://callpropvoid
			case 0x46://callproperty
			{
				uint32_t t = code.peeku30FromPosition(pos);
				uint32_t pos2 = code.skipu30FromPosition(pos);
				uint32_t argcount = code.peeku30FromPosition(pos2);
				if (state.jumptargets.find(pos) == state.jumptargets.end()
						&&(argsneeded > argcount) && state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					argsneeded -= argcount;
					if (b==0x4f) //callpropvoid
						argsneeded--;
					pos = code.skipu30FromPosition(pos2);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			}
			case 0x41://call
			{
				uint32_t argcount = code.peeku30FromPosition(pos);
				if (state.jumptargets.find(pos) == state.jumptargets.end()
						&& argcount < UINT16_MAX
						&& (argsneeded > argcount+1))
				{
					argsneeded -= argcount;
					argsneeded--;
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			}
			case 0x55://newobject
			{
				uint32_t argcount = code.peeku30FromPosition(pos);
				if (state.jumptargets.find(pos) == state.jumptargets.end()
						&& argsneeded && argcount==0)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			}
			case 0xa0://add
			case 0xa1://subtract
			case 0xa2://multiply
			case 0xa3://divide
			case 0xa4://modulo
			case 0xa5://lshift
			case 0xa6://rshift
			case 0xa7://urshift
			case 0xa8://bitand
			case 0xa9://bitor
			case 0xaa://bitxor
			case 0xc5://add_i
			case 0xc6://subtract_i
			case 0xc7://multiply_i
			case 0x1e://nextname
			case 0x23://nextvalue
			case 0xab://equals
			case 0xad://lessthan
			case 0xae://lessequals
			case 0xaf://greaterthan
			case 0xb0://greaterequals
			case 0xb3://istypelate
				if (argsneeded>=2)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
				}
				else
					keepchecking=false;
				break;
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
				if (argsneeded>=2)
				{
					pos += 3;
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded-=2;
				}
				else
					keepchecking=false;
				break;
			case 0x3a://si8
			case 0x3b://si16
			case 0x3c://si32
			case 0x3d://sf32
			case 0x3e://sf64
				if (argsneeded>=2)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded-=2;
				}
				else
					keepchecking=false;
				break;
			case 0x2b://swap
				if (argsneeded>=2
						&& state.jumptargets.find(pos) == state.jumptargets.end())
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
				switch (code.peekbyteFromPosition(pos))
				{
					case 0x63: //setlocal
					case 0xd4: //setlocal_0
					case 0xd5: //setlocal_1
					case 0xd6: //setlocal_2
					case 0xd7: //setlocal_3
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
						break;
					default:
						keepchecking=false;
						break;
				}
				break;
			case 0x63://setlocal
				if (argsneeded)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
					fromdup=false;
				}
				else
					keepchecking=false;
				break;
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
				if (argsneeded)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
					fromdup=false;
				}
				else
					keepchecking=false;
				break;
			case 0x29://pop
			case 0x30://pushscope
				if (argsneeded || fromdup)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					if (!fromdup)
					{
						argsneeded--;
					}
					fromdup=false;
				}
				else
					keepchecking=false;
				break;
			default:
				keepchecking=false;
				break;
		}
	}
	skipjump(state,b,code,pos,!argsneeded);
	bool setlocal=false;
	// check if we need to store the result of the operation on stack
	switch (b)
	{
		case 0x2a://dup
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x2b://swap
			if ((argsneeded==1 || state.operandlist.size() >= 1) && state.jumptargets.find(pos) == state.jumptargets.end())
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x63://setlocal
		{
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				uint32_t num = code.peeku30FromPosition(pos);
				state.setlocal_handled.insert(pos);
				// set optimized opcode to corresponding opcode with local result
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = num;
				state.localtypes[num] = restype;
				pos = code.skipu30FromPosition(pos);
				state.lastlocalresultpos=num;
				setlocal=true;
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0xd4://setlocal_0
		case 0xd5://setlocal_1
		case 0xd6://setlocal_2
		case 0xd7://setlocal_3
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				state.setlocal_handled.insert(pos);
				// set optimized opcode to corresponding opcode with local result
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = b-0xd4;
				state.localtypes[b-0xd4] = restype;
				state.lastlocalresultpos=b-0xd4;
				setlocal=true;
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x41://call
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount < UINT16_MAX
					&& (argcount+1 >= argsneeded) && (state.operandlist.size() >= (argcount+1-argsneeded)))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x45://callsuper
		case 0x4e://callsupervoid
		case 0x4a://constructprop
		case 0x4f://callpropvoid
		case 0x46://callproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t pos2 = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos2);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
			{
				switch (argcount)
				{
					case 0:
						res = (argsneeded == 0);
						break;
					case 1:
						res = (argsneeded == 0 && state.operandlist.size()>0) || argsneeded==1;
						break;
					default:
						res = (argcount >= argsneeded) && (state.operandlist.size() >= argcount-argsneeded);
						break;
				}
				if (res)
					break;
			}
			clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x42://construct
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount == 0
					&& !argsneeded && (state.operandlist.size() >= 1))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x55://newobject
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount == 0
					&& !argsneeded && (state.operandlist.size() >= 1))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x61://setproperty
		case 0x68://initproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end() && (argsneeded<=1) &&
					(
					   (((argsneeded==1) || (state.operandlist.size() >= 1))
					    && (uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
					|| ((argsneeded==0) && (state.operandlist.size() > 1)
						&& (uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 1
						&& (state.operandlist.back().objtype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()))
					)
				)
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x66://getproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t argcount = argsneeded ? 1 : 0;
			if (state.jumptargets.find(pos) == state.jumptargets.end() && (argsneeded<=1) && !fromdup &&
					((uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == argcount
					|| (!argsneeded && (state.operandlist.size() >= 1) && state.mi->context->constant_pool.multinames[t].runtimeargs == 1)
					))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x11://iftrue
		case 0x12://iffalse
			if (!argsneeded && !fromdup  && (state.jumptargets.find(pos) == state.jumptargets.end()))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x91://increment
		case 0x93://decrement
		case 0x95://typeof
		case 0x96://not
		case 0x6c://getslot
		case 0x73://convert_i
		case 0x74://convert_u
		case 0x75://convert_d
		case 0x76://convert_b
		case 0x80://coerce
		case 0x70://convert_s
		case 0x85://coerce_s
		case 0xc0://increment_i
		case 0xc1://decrement_i
		case 0x35://li8
		case 0x36://li16
		case 0x37://li32
		case 0x38://lf32
		case 0x39://lf64
		case 0x1b://lookupswitch
		case 0x50://sxi1
		case 0x51://sxi8
		case 0x52://sxi16
		case 0x90://negate
		case 0x97://bitnot
			if (!argsneeded && (state.jumptargets.find(pos) == state.jumptargets.end()))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x0c://ifnlt
		case 0x0d://ifnle
		case 0x0e://ifngt
		case 0x0f://ifnge
		case 0x13://ifeq
		case 0x14://ifne
		case 0x15://iflt
		case 0x16://ifle
		case 0x17://ifgt
		case 0x18://ifge
		case 0x19://ifstricteq
		case 0x1a://ifstrictne
		case 0x6d://setslot
		case 0x87://astypelate
		case 0xa0://add
		case 0xa1://subtract
		case 0xa2://multiply
		case 0xa3://divide
		case 0xa4://modulo
		case 0xab://equals
		case 0xad://lessthan
		case 0xae://lessequals
		case 0xaf://greaterthan
		case 0xb0://greaterequals
		case 0xb3://istypelate
		case 0x3a://si8
		case 0x3b://si16
		case 0x3c://si32
		case 0x3d://sf32
		case 0x3e://sf64
		case 0xc5://add_i
		case 0xc6://subtract_i
		case 0xc7://multiply_i
		case 0x23://nextvalue
		case 0x1e://nextname
			if ((argsneeded==1 || (!argsneeded && state.operandlist.size() > 0)) && (state.jumptargets.find(pos) == state.jumptargets.end()))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0xa5://lshift
		case 0xa6://rshift
		case 0xa9://bitor
		case 0xa8://bitand
		case 0xaa://bitxor
		case 0xa7://urshift
			if ((argsneeded==1 || (!argsneeded && state.operandlist.size() > 0)) && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				switch (state.preloadedcode[preloadlocalpos].operator_start)
				{
					case ABC_OP_OPTIMZED_ADD:
					case ABC_OP_OPTIMZED_SUBTRACT:
					case ABC_OP_OPTIMZED_MULTIPLY:
					case ABC_OP_OPTIMZED_DIVIDE:
					case ABC_OP_OPTIMZED_MODULO:
						state.preloadedcode[preloadpos].pcode.local3.flags |= ABC_OP_FORCEINT;
						break;
				}
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x48://returnvalue
			if (!argsneeded && (state.jumptargets.find(pos) == state.jumptargets.end()))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x10://jump
			// don't clear operandlist yet, because the jump may be skipped
			break;
		case 0x29://pop
		{
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x30://pushscope
			if (!argsneeded && (state.jumptargets.find(pos) == state.jumptargets.end()))
				res = true;
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		default:
			clearOperands(state,false,nullptr,checkchanged);
			break;
	}
	if (res)
	{
		if (!setlocal)
		{
			uint32_t resultpos;
			resultpos = state.mi->body->localresultcount++;
			state.localtypes.push_back(restype);
			state.defaultlocaltypes.push_back(nullptr);
			state.defaultlocaltypescacheable.push_back(true);
			// set optimized opcode to corresponding opcode with local result
			state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
			state.preloadedcode[preloadpos].hasLocalResult=true;
			state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
			state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
			state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
			state.lastlocalresultpos=UINT32_MAX;
		}
		state.duplocalresult=true;
	}
	else
		state.lastlocalresultpos=UINT32_MAX;

	return res;
#else
	clearOperands(state,false,nullptr,checkchanged);
	return false;
#endif
}

bool setupInstructionOneArgumentNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code, uint32_t startcodepos)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 1;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		(--it)->removeArg(state);// remove arg1
		it = state.operandlist.end();
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true);
		state.refreshOldNewPosition(code);
		state.operandlist.pop_back();
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.refreshOldNewPosition(code);
	}
	return hasoperands;
}
bool setupInstructionTwoArgumentsNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.operandlist.size() >= 2;
	if (hasoperands)
	{
		auto op2 = state.operandlist.at(state.operandlist.size()-1);
		auto op1 = state.operandlist.at(state.operandlist.size()-2);
		// remove preloaded code for both operands in the correct order
		if (op1.preloadedcodepos < op2.preloadedcodepos)
		{
			op2.removeArg(state);
			op1.removeArg(state);
		}
		else
		{
			op1.removeArg(state);
			op2.removeArg(state);
		}
		auto it = state.operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		state.preloadedcode.emplace_back();
		(--it)->fillCode(state,1,state.preloadedcode.size()-1,true,&operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true,&operator_start);
		state.preloadedcode.back().pcode.func = ABCVm::abcfunctions[operator_start];
		state.refreshOldNewPosition(code);
		state.operandlist.pop_back();
		state.operandlist.pop_back();
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.refreshOldNewPosition(code);
	}
	return hasoperands;
}
bool setupInstructionOneArgument(preloadstate& state,int operator_start,int opcode,memorystream& code,bool constantsallowed, bool useargument_for_skip, Class_base* resulttype, uint32_t startcodepos, bool checkforlocalresult, bool addchanged,bool fromdup, bool checkoperands,uint32_t operator_start_setslot)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = !checkoperands || (state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 1 && (constantsallowed || state.operandlist.back().type == OP_LOCAL|| state.operandlist.back().type == OP_CACHED_SLOT));
	Class_base* skiptype = resulttype;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		(--it)->removeArg(state);// remove arg1
		if (addchanged && (it->type == OP_LOCAL || it->type == OP_CACHED_SLOT))
			setOperandModified(state,it->type,it->index);
		it = state.operandlist.end();
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,constantsallowed,nullptr,&operator_start_setslot);
		if (useargument_for_skip)
			skiptype = it->objtype;
		state.refreshOldNewPosition(code);
		state.operandlist.pop_back();
	}
	else
		state.preloadedcode.push_back((uint32_t)opcode);
	if (state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		switch (code.peekbyte())
		{
			case 0x73://convert_i
				if (!constantsallowed || skiptype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
				{
					state.refreshOldNewPosition(code);
					code.readbyte();
				}
				break;
			case 0x74://convert_u
				if (!constantsallowed || skiptype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
				{
					state.refreshOldNewPosition(code);
					code.readbyte();
				}
				break;
			case 0x75://convert_d
				if (!constantsallowed || skiptype == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()  || skiptype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || skiptype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
				{
					state.refreshOldNewPosition(code);
					code.readbyte();
				}
				break;
			case 0x70://convert_s
			case 0x85://coerce_s
				if (!constantsallowed || skiptype == Class<ASString>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()  || skiptype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || skiptype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
				{
					state.refreshOldNewPosition(code);
					code.readbyte();
				}
				break;
			case 0x76://convert_b
				if (!constantsallowed || skiptype == Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
				{
					state.refreshOldNewPosition(code);
					code.readbyte();
				}
				break;
			case 0x82://coerce_a
				state.refreshOldNewPosition(code);
				code.readbyte();
				break;
		}
	}
	if (hasoperands && checkforlocalresult)
		checkForLocalResult(state,code,constantsallowed ? 2 : 1,resulttype,-1,-1,false,fromdup,operator_start_setslot);
	else
		clearOperands(state,false,nullptr);
#else
	state.preloadedcode.push_back((uint32_t)opcode);
#endif
	return hasoperands;
}

bool setupInstructionTwoArguments(preloadstate& state,int operator_start,int opcode,memorystream& code, bool skip_conversion,bool cancollapse,bool checklocalresult, uint32_t startcodepos,Class_base* resulttype,uint32_t operator_start_setslot)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 2;
	bool op1isconstant=false;
	bool op2isconstant=false;
	if (hasoperands)
	{
		auto op2 = state.operandlist.at(state.operandlist.size()-1);
		auto op1 = state.operandlist.at(state.operandlist.size()-2);
		// remove preloaded code for both operands in the correct order
		if (op1.preloadedcodepos < op2.preloadedcodepos)
		{
			op2.removeArg(state);
			op1.removeArg(state);
		}
		else
		{
			op1.removeArg(state);
			op2.removeArg(state);
		}
		auto it = state.operandlist.end();
		if (cancollapse && (--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT && (--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT) // two constants means we can compute the result now and use it
		{
			it =state.operandlist.end();
			--it;
			asAtom* op2 = state.mi->context->getConstantAtom((it)->type,(it)->index);
			--it;
			asAtom* op1 = state.mi->context->getConstantAtom((it)->type,(it)->index);
			asAtom res = *op1;
			switch (operator_start)
			{
				case ABC_OP_OPTIMZED_SUBTRACT:
					asAtomHandler::subtract(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_MULTIPLY:
					asAtomHandler::multiply(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_DIVIDE:
					asAtomHandler::divide(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_MODULO:
					asAtomHandler::modulo(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_LSHIFT:
					asAtomHandler::lshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_RSHIFT:
					asAtomHandler::rshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_URSHIFT:
					asAtomHandler::urshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITAND:
					asAtomHandler::bit_and(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITOR:
					asAtomHandler::bit_or(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITXOR:
					asAtomHandler::bit_xor(res,state.worker,*op2);
					break;
				default:
					LOG(LOG_ERROR,"setupInstructionTwoArguments: trying to collapse invalid opcode:"<<hex<<operator_start);
					break;
			}
			state.operandlist.pop_back();
			state.operandlist.pop_back();
			if (asAtomHandler::isObject(res))
				asAtomHandler::getObjectNoCheck(res)->setRefConstant();
			uint32_t value = state.mi->context->addCachedConstantAtom(res);
			state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
			state.refreshOldNewPosition(code);
			state.preloadedcode.back().pcode.arg3_uint=value;
			state.operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(res,state.mi->context->applicationDomain->getSystemState()), value,1,state.preloadedcode.size()-1));
			return true;
		}
		it = state.operandlist.end();
		op2isconstant = ((--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT);
		op1isconstant = ((--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT);

		it = state.operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,1,state.preloadedcode.size()-1,true,nullptr,&operator_start_setslot);
		Class_base* op1type = it->objtype;
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true,nullptr,&operator_start_setslot);
		Class_base* op2type = it->objtype;
		state.refreshOldNewPosition(code);
		state.operandlist.pop_back();
		state.operandlist.pop_back();
		switch (operator_start)
		{
			case ABC_OP_OPTIMZED_ADD:
				// if both operands are numeric, the result is always a number, so we can skip convert_d opcode
				skip_conversion =
						(op1type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || op1type == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || op1type == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()) &&
						(op2type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || op2type == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() || op2type == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr());
				if (skip_conversion)
				{
					if (op1type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
						resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
					else
						resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				}
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_MODULO:
				if (op1type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr())
					resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				else
					resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_SUBTRACT:
			case ABC_OP_OPTIMZED_MULTIPLY:
			case ABC_OP_OPTIMZED_DIVIDE:
				resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_ADD_I:
			case ABC_OP_OPTIMZED_SUBTRACT_I:
			case ABC_OP_OPTIMZED_MULTIPLY_I:
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int= asAtomHandler::toInt(*state.preloadedcode.back().pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int= asAtomHandler::toInt(*state.preloadedcode.back().pcode.arg2_constant);
				setForceInt(state,code,&resulttype);
				resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_RSHIFT:
				resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_URSHIFT:
				resulttype = Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				// operators are always transformed to uint, so we can do that here if the operators are constants
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int=asAtomHandler::getUInt(state.worker,*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int=asAtomHandler::getUInt(state.worker,*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_constant);
				break;
			case ABC_OP_OPTIMZED_LSHIFT:
			case ABC_OP_OPTIMZED_BITOR:
			case ABC_OP_OPTIMZED_BITAND:
			case ABC_OP_OPTIMZED_BITXOR:
				resulttype = Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				// operators are always transformed to int, so we can do that here if the operators are constants
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int=asAtomHandler::toInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int=asAtomHandler::toInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_constant);
				break;
			default:
				break;
		}
	}
	else
		state.preloadedcode.push_back((uint32_t)opcode);
	if (checklocalresult)
	{
		if (hasoperands)
			checkForLocalResult(state,code,4, resulttype,-1,-1,false,false,operator_start_setslot);
		else
			clearOperands(state,false,nullptr);
	}
#else
	state.preloadedcode.push_back((uint32_t)opcode);
#endif
	return hasoperands;
}
bool checkmatchingLastObjtype(preloadstate& state, Type* resulttype, Class_base* requiredtype)
{
	if (requiredtype == resulttype || resulttype==Type::anyType || resulttype == Type::voidType)
		return true;
	if (requiredtype == Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() &&
			(resulttype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()
			|| resulttype == Class<UInteger>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()))
		return true;
	if (resulttype== Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr() &&
			(requiredtype == Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()))
	{
		switch(state.preloadedcode.at(state.preloadedcode.size()-1).opcode)
		{
			case ABC_OP_OPTIMZED_ADD+4:
			case ABC_OP_OPTIMZED_ADD+5:
			case ABC_OP_OPTIMZED_ADD+6:
			case ABC_OP_OPTIMZED_ADD+7:
			case ABC_OP_OPTIMZED_SUBTRACT+4:
			case ABC_OP_OPTIMZED_SUBTRACT+5:
			case ABC_OP_OPTIMZED_SUBTRACT+6:
			case ABC_OP_OPTIMZED_SUBTRACT+7:
			case ABC_OP_OPTIMZED_MULTIPLY+4:
			case ABC_OP_OPTIMZED_MULTIPLY+5:
			case ABC_OP_OPTIMZED_MULTIPLY+6:
			case ABC_OP_OPTIMZED_MULTIPLY+7:
			case ABC_OP_OPTIMZED_DIVIDE+4:
			case ABC_OP_OPTIMZED_DIVIDE+5:
			case ABC_OP_OPTIMZED_DIVIDE+6:
			case ABC_OP_OPTIMZED_DIVIDE+7:
			case ABC_OP_OPTIMZED_MODULO+4:
			case ABC_OP_OPTIMZED_MODULO+5:
			case ABC_OP_OPTIMZED_MODULO+6:
			case ABC_OP_OPTIMZED_MODULO+7:
			{
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags |= ABC_OP_FORCEINT;
				return true;
			}
			default:
				break;
		}
	}
	return false;
}
void addOperand(preloadstate& state,operands& op,memorystream& code)
{
	uint32_t opcode=0;
	switch (op.type)
	{
		case OP_LOCAL:
			opcode = 0x62;//getlocal
			break;
		case OP_CACHED_SLOT:
			opcode = ABC_OP_OPTIMZED_PUSHCACHEDSLOT;
			break;
		case OP_CACHED_CONSTANT:
			opcode = ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT;
			break;
		case OP_DOUBLE:
			opcode = 0x2f;//pushdouble
			break;
		case OP_UNDEFINED:
			opcode = 0x21;//pushundefined
			break;
		case OP_BYTE:
			opcode = 0x24;//pushbyte
			break;
		case OP_SHORT:
			opcode = 0x25;//pushshort
			break;
		case OP_TRUE:
			opcode = 0x26;//pushtrue
			break;
		case OP_FALSE:
			opcode = 0x27;//pushfalse
			break;
		case OP_NAN:
			opcode = 0x28;//pushnan
			break;
		case OP_STRING:
			opcode = 0x2c;//pushstring
			break;
		case OP_INTEGER:
			opcode = 0x2d;//pushint
			break;
		case OP_UINTEGER:
			opcode = 0x2e;//pushuint
			break;
		case OP_NAMESPACE:
			opcode = 0x31;//pushnamespace
			break;
		case OP_NULL:
			opcode = 0x20;//pushnull
			break;
	}
	state.preloadedcode.push_back(opcode);
	state.refreshOldNewPosition(code);
	state.preloadedcode.back().pcode.arg3_uint = op.index;
	state.operandlist.push_back(operands(op.type,op.objtype,op.index,1,state.preloadedcode.size()-1));
}
void addCachedConstant(preloadstate& state,method_info* mi, asAtom& val,memorystream& code)
{
	if (asAtomHandler::isObject(val))
		asAtomHandler::getObject(val)->setRefConstant();
	uint32_t value = mi->context->addCachedConstantAtom(val);
	state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
	state.refreshOldNewPosition(code);
	state.preloadedcode.back().pcode.arg3_uint=value;
	state.operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(val,mi->context->applicationDomain->getSystemState()),value,1,state.preloadedcode.size()-1));
}
void addCachedSlot(preloadstate& state, uint32_t localpos, uint32_t slotid,memorystream& code,Class_base* resulttype)
{
	uint32_t value = 0;
	for (auto it = state.mi->body->localconstantslots.begin(); it != state.mi->body->localconstantslots.end(); it++)
	{
		if (it->local_pos == localpos && it->slot_number == slotid)
			break;
		value++;
	}
	if (value == state.mi->body->localconstantslots.size())
	{
		localconstantslot sl;
		sl.local_pos= localpos;
		sl.slot_number = slotid;
		state.mi->body->localconstantslots.push_back(sl);
	}
	state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDSLOT);
	state.refreshOldNewPosition(code);
	state.preloadedcode.back().pcode.arg3_uint = value;
	state.operandlist.push_back(operands(OP_CACHED_SLOT,resulttype,value,1,state.preloadedcode.size()-1));
}
void setdefaultlocaltype(preloadstate& state,uint32_t t,Class_base* c)
{
	if (c==nullptr && t < state.defaultlocaltypescacheable.size())
	{
		state.defaultlocaltypes[t] = nullptr;
		state.defaultlocaltypescacheable[t]=false;
		return;
	}
	if (t < state.defaultlocaltypescacheable.size() && state.defaultlocaltypescacheable[t])
	{
		if (state.defaultlocaltypes[t] == nullptr)
			state.defaultlocaltypes[t] = c;
		if (state.defaultlocaltypes[t] != c)
		{
			state.defaultlocaltypescacheable[t]=false;
			state.defaultlocaltypes[t]=nullptr;
		}
	}
}
void removetypestack(std::vector<typestackentry>& typestack,int n)
{
#ifdef ENABLE_OPTIMIZATION
	assert_and_throw(uint32_t(n) <= typestack.size());
	for (int i = 0; i < n; i++)
	{
		typestack.pop_back();
	}
#endif
}
void skipunreachablecode(preloadstate& state, memorystream& code, bool updatetargets)
{
	while (!code.atend() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		uint8_t b = code.readbyte();
		switch (b)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
			case 0x40://newfunction
			case 0x41://call
			case 0x42://construct
			case 0x49://constructsuper
			case 0x53://constructgenerictype
			case 0x55://newobject
			case 0x56://newarray
			case 0x58://newclass
			case 0x59://getdescendants
			case 0x5a://newcatch
			case 0x5d://findpropstrict
			case 0x5e://findproperty
			case 0x5f://finddef
			case 0x60://getlex
			case 0x61://setproperty
			case 0x65://getscopeobject
			case 0x66://getproperty
			case 0x68://initproperty
			case 0x6a://deleteproperty
			case 0x6c://getslot
			case 0x6d://setslot
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
			case 0x86://astype
			case 0xb2://istype
			case 0x08://kill
			case 0x62://getlocal
			case 0x80://coerce
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			case 0x2c://pushstring
			case 0x2d://pushint
			case 0x2e://pushuint
			case 0x2f://pushdouble
			case 0x31://pushnamespace
			case 0x63://setlocal
			case 0x92://inclocal
			case 0x94://declocal
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
				code.readu30();
				break;
			case 0x1b://lookupswitch
			{
				code.reads24();
				uint32_t count = code.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					code.reads24();
				}
				break;
			}
			case 0x24://pushbyte
				code.readbyte();
				break;
			case 0x25://pushshort
				code.readu32();
				break;
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x46://callproperty
			case 0x4c://callproplex
			case 0x4a://constructprop
			case 0x4e://callsupervoid
			case 0x4f://callpropvoid
				code.readu30();
				code.readu30();
				break;
			case 0xef://debug
				code.readbyte();
				code.readu30();
				code.readbyte();
				code.readu30();
				break;
			case 0x10://jump
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
			case 0x11://iftrue
			case 0x12://iffalse
			{
				// make sure that unreachable jumps get erased from jumptargets
				int32_t p1 = code.reads24()+code.tellg()+1;
				if (updatetargets)
				{
					auto it = state.jumptargets.find(p1);
					if (it != state.jumptargets.end() && it->second > 1)
						state.jumptargets[p1]--;
					else
						state.jumptargets.erase(p1);
				}
				break;
			}
		}
	}
}
bool checkforpostfix(preloadstate& state,memorystream& code,uint32_t startpos,std::vector<typestackentry>& typestack, int32_t localpos, uint32_t postfix_opcode)
{
#ifdef ENABLE_OPTIMIZATION
	if (!state.operandlist.empty() && state.jumptargets.find(startpos) == state.jumptargets.end())
	{
		uint32_t pos = code.tellg();
		if (state.jumptargets.find(pos) == state.jumptargets.end())
		{
			if (code.peekbyteFromPosition(pos) ==0x73) //convert_i
				++pos;
		}
		uint32_t loc = UINT32_MAX;
		if (state.jumptargets.find(pos) == state.jumptargets.end())
		{
			uint8_t peekopcode=code.peekbyteFromPosition(pos);
			switch (peekopcode)
			{
				case 0x63: //setlocal
					loc = code.peeku30FromPosition(pos+1);
					pos = code.skipu30FromPosition(pos+1);
					break;
				case 0xd4: //setlocal_0
				case 0xd5: //setlocal_1
				case 0xd6: //setlocal_2
				case 0xd7: //setlocal_3
					loc = peekopcode-0xd4;
					++pos;
					break;
				default:
					break;
			}
			if (loc != UINT32_MAX && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				if (state.operandlist.back().type == OP_LOCAL && state.operandlist.back().index == localpos)
				{
					// code is a postfix increment/decrement where the result is used directly
					// actionscript code like:
					// y = x++;
					// is  translated sequence is of form
					// getlocal x
					// inclocal_i x
					// convert_i
					// setlocal y
					state.operandlist.pop_back();
					state.preloadedcode.pop_back(); // remove getlocal opcode
					state.preloadedcode.push_back(postfix_opcode);
					state.preloadedcode.back().pcode.arg1_uint = localpos;
					state.preloadedcode.back().pcode.local3.pos = loc;
					state.refreshOldNewPosition(code);
					setOperandModified(state,OP_LOCAL,localpos);
					code.seekg(pos);
					return true;
				}
				// current opcode is followed by setlocal, we can swap these opcodes to let setlocal be optimized
				setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_SETLOCAL,peekopcode,code,startpos);
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint =loc;
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					state.localtypes[loc]=typestack.back().obj->as<Class_base>();
				else
					state.localtypes[loc]=nullptr;
				removetypestack(typestack,1);
				code.seekg(pos);
			}
		}
	}
#endif
	return false;
}

void setupInstructionComparison(preloadstate& state,int operator_start,int opcode,memorystream& code, int operator_replace,int operator_replace_reverse,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,std::map<int32_t,int32_t>& jumppositions, std::map<int32_t,int32_t>& jumpstartpositions)
{
	if (setupInstructionTwoArguments(state,operator_start,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr()))
	{
#ifdef ENABLE_OPTIMIZATION
		if (state.preloadedcode.back().opcode >= uint32_t(operator_start+4)) // has localresult
		{
			uint32_t pos = code.tellg();
			bool ok = state.jumptargets.find(pos) == state.jumptargets.end() && state.jumptargets.find(pos+1) == state.jumptargets.end();
			bool isnot = code.peekbyteFromPosition(pos) == 0x96; //not
			bool multiconditions = false;
			if (ok && isnot)
			{
				pos++;
				ok = state.jumptargets.find(pos) == state.jumptargets.end();
			}
			if (ok && code.peekbyteFromPosition(pos) == 0x76) //convert_b
			{
				pos++;
				ok = state.jumptargets.find(pos) == state.jumptargets.end();
			}
			if (ok && code.peekbyteFromPosition(pos) == 0x2a //dup
				&& (code.peekbyteFromPosition(pos+1) == 0x11 || //iftrue
					code.peekbyteFromPosition(pos+1) == 0x12 ) //iffalse
				&& code.peeks24FromPosition(pos+2) > 0)
			{
				// common case for comparison with multiple conditions like "if (a>0 && b>0)"
				pos++;
				multiconditions = true;
				ok = state.jumptargets.find(pos) == state.jumptargets.end();
			}
			if (ok && (state.jumptargets.find(pos+1) == state.jumptargets.end()))
			{
				int j = code.peeks24FromPosition(pos+1);
				bool canoptimize = (!multiconditions || code.peekbyteFromPosition(pos+4) == 0x29); //pop

				if (multiconditions)
				{
					if (j <= 0)
						canoptimize = false;
					else
					{
						int jumppos = pos+j+3+1+1;
						auto it = state.jumptargets.find(jumppos);
						if (it != state.jumptargets.end() && (*it).second == 1)
						{
							if (code.peekbyteFromPosition(jumppos-1) ==code.peekbyteFromPosition(pos))
							{
								// remove old jump target and set new jump target
								state.jumptargets.erase(it);
								j += code.peeks24FromPosition(jumppos)+3+1;
								jumppos += code.peeks24FromPosition(jumppos)+3+1;
								it = state.jumptargets.find(jumppos+3+1);
								(*it).second++;
							}
							else
								canoptimize = false;
						}
						else
							canoptimize = false;
					}
				}
				if ((code.peekbyteFromPosition(pos) == 0x11 || //iftrue
					 code.peekbyteFromPosition(pos) == 0x12 ) //iffalse
					&& canoptimize)
				{
					// comparison operator is followed by iftrue/iffalse, can be optimized into comparison operator with jump (e.g. equals->ifeq/ifne)
					code.seekg(pos);
					state.refreshOldNewPosition(code);
					uint8_t b = code.readbyte();
					assert(b== 0x11 || b== 0x12);
					code.reads24();
					int32_t p1 = code.tellg();
					uint32_t opcodeskip = state.preloadedcode.back().opcode - (operator_start+4);
					if (isnot)
						state.preloadedcode.back().opcode = (b==0x12 ? operator_replace : operator_replace_reverse) + opcodeskip;
					else
						state.preloadedcode.back().opcode = (b==0x11 ? operator_replace : operator_replace_reverse) + opcodeskip;
					state.preloadedcode.back().pcode.func = nullptr;
					jumppositions[state.preloadedcode.size()-1] = j;
					jumpstartpositions[state.preloadedcode.size()-1] = p1;
					clearOperands(state,true,lastlocalresulttype);
					removetypestack(typestack,2);
					if (multiconditions)
					{
						// iftrue/iffalse is followed by pop, skip it
						state.refreshOldNewPosition(code);
						code.readbyte();
					}
					return;
				}
			}
		}
		else
		{
			uint32_t pos = code.tellg();
			bool ok = state.jumptargets.find(pos) == state.jumptargets.end();
			if (ok && code.peekbyteFromPosition(pos) == 0x76) //convert_b
			{
				code.readbyte();
			}
		}
#endif
	}
	removetypestack(typestack,2);
	typestack.push_back(typestackentry(Class<Boolean>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr(),false));
}


void setupInstructionIncDecInteger(preloadstate& state,memorystream& code,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,int& dup_indicator, uint8_t opcode)
{
	removetypestack(typestack,1);
	uint32_t p = code.tellg();
	// optimize common case of increment/decrement local variable
#ifdef ENABLE_OPTIMIZATION
	if (state.operandlist.size() > 0 &&
			state.operandlist.back().type == OP_LOCAL &&
			state.operandlist.back().objtype == Class<Integer>::getRef(state.function->getSystemState()).getPtr() &&
			state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		int32_t t = -1;
		if (code.peekbyte() == 0x73) //convert_i
			code.readbyte();
		switch (code.peekbyte())
		{
			case 0x63://setlocal
				t = code.peekbyteFromPosition(code.tellg()+1);
				break;
			case 0xd4://setlocal_0
				t = 0;
				break;
			case 0xd5://setlocal_1
				t = 1;
				break;
			case 0xd6://setlocal_2
				t = 2;
				break;
			case 0xd7://setlocal_3
				t = 3;
				break;
		}
		if (t == state.operandlist.back().index)
		{
			state.operandlist.back().removeArg(state);
			state.preloadedcode.push_back(opcode == 0xc0 ? ABC_OP_OPTIMZED_INCLOCAL_I : ABC_OP_OPTIMZED_DECLOCAL_I); //inclocal_i/declocal_i
			state.preloadedcode.back().pcode.arg1_uint = t;
			state.preloadedcode.back().pcode.arg2_uint = 1;
			state.refreshOldNewPosition(code);
			if (code.readbyte() == 0x63) //setlocal
				code.readbyte();
			state.operandlist.pop_back();
			setOperandModified(state,OP_LOCAL,t);
			if (dup_indicator)
				clearOperands(state,false,lastlocalresulttype);
			return;
		}
	}
#endif
	setupInstructionOneArgument(state,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I : ABC_OP_OPTIMZED_DECREMENT_I,opcode,code,false,true, Class<Integer>::getRef(state.function->getSystemState()).getPtr(),p,true,true,false,true,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I_SETSLOT : ABC_OP_OPTIMZED_DECREMENT_I_SETSLOT);
	dup_indicator=0;
	typestack.push_back(typestackentry(Class<Integer>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr(),false));
}

bool checkInitializeLocalToConstant(preloadstate& state,int32_t value)
{
	value -= state.mi->numArgs()+1;
	if (state.operandlist.size() && state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT
			&& value >= 0 && value < int(state.canlocalinitialize.size()) && state.canlocalinitialize[value])
	{
		// local is initialized to constant value
		if (!state.mi->body->localsinitialvalues)
		{
			state.mi->body->localsinitialvalues = new asAtom[state.mi->body->local_count -(state.mi->numArgs()+1)];
			memset(state.mi->body->localsinitialvalues,ATOMTYPE_UNDEFINED_BIT,(state.mi->body->local_count -(state.mi->numArgs()+1))*sizeof(asAtom));
		}
		state.mi->body->localsinitialvalues[value]= *state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
		state.canlocalinitialize[value]=false;
		state.operandlist.back().removeArg(state);
		state.operandlist.pop_back();
		return true;
	}
	if (value >= 0 && value < int(state.canlocalinitialize.size()))
		state.canlocalinitialize[value]=false;
	return false;
}
void removeInitializeLocalToConstant(preloadstate& state,int32_t value)
{
	value -= state.mi->numArgs()+1;
	if (value >= 0 && value < int(state.canlocalinitialize.size()))
	{
		state.canlocalinitialize[value]=false;
	}
}

bool getLexFindClass(preloadstate& state, multiname* name, bool checkfuncscope,std::vector<typestackentry>& typestack,memorystream& code)
{
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT r = GETVAR_NORMAL;
	if(checkfuncscope && !state.function->func_scope.isNull()) // check scope stack
	{
		auto it=state.function->func_scope->scope.rbegin();
		while(it!=state.function->func_scope->scope.rend())
		{
			GET_VARIABLE_OPTION opt= (GET_VARIABLE_OPTION)(FROM_GETLEX | DONT_CALL_GETTER | NO_INCREF);
			if(!it->considerDynamic)
				opt=(GET_VARIABLE_OPTION)(opt | SKIP_IMPL);
			else
				break;
			if (asAtomHandler::is<Class_inherit>(it->object))
				asAtomHandler::as<Class_inherit>(it->object)->checkScriptInit();
			r = asAtomHandler::toObject(it->object,state.worker)->getVariableByMultiname(o,*name, opt,state.worker);
			if(asAtomHandler::isValid(o))
				break;
			++it;
		}
	}
	if(asAtomHandler::isInvalid(o))
	{
		GET_VARIABLE_OPTION opt= (GET_VARIABLE_OPTION)(FROM_GETLEX | DONT_CALL_GETTER | NO_INCREF);
		r = state.mi->context->applicationDomain->getVariableByMultiname(o,*name,opt,state.worker);
	}
	if(asAtomHandler::isInvalid(o))
	{
		ASObject* cls = (ASObject*)dynamic_cast<const Class_base*>(name->cachedType);
		if (cls)
			o = asAtomHandler::fromObjectNoPrimitive(cls);
	}
	// fast check for builtin classes if no custom class with same name is defined
	if(asAtomHandler::isInvalid(o) &&
			state.mi->context->applicationDomain->customClasses.find(name->name_s_id) == state.mi->context->applicationDomain->customClasses.end())
	{
		ASObject* cls = state.mi->context->applicationDomain->getSystemState()->systemDomain->getVariableByMultinameOpportunistic(*name,state.worker);
		if (cls)
			o = asAtomHandler::fromObject(cls);
		if (cls && !cls->is<Class_base>() && cls->getConstant())
		{
			// global builtin method/constant
			addCachedConstant(state,state.mi, o,code);
			typestack.push_back(typestackentry(cls->getClass(),false));
			return true;
		}
	}
	if(asAtomHandler::isInvalid(o))
	{
		Type* t = Type::getTypeFromMultiname(name,state.mi->context,true);
		Class_base* cls = (Class_base*)dynamic_cast<Class_base*>(t);
		if (cls)
			o = asAtomHandler::fromObject(cls);
	}
	if (asAtomHandler::is<Template_base>(o))
	{
		addCachedConstant(state,state.mi, o,code);
		typestack.push_back(typestackentry(nullptr,false));
		return true;
	}
	if (asAtomHandler::is<Class_base>(o))
	{
		Class_base* resulttype = asAtomHandler::as<Class_base>(o);
		addCachedConstant(state,state.mi, o,code);
		typestack.push_back(typestackentry(resulttype,true));
		return true;
	}
	else if (r & GETVAR_ISCONSTANT && !asAtomHandler::isNull(o)) // class may not be constructed yet, so the result is null and we do not cache
	{
		addCachedConstant(state,state.mi, o,code);
		typestack.push_back(typestackentry(nullptr,false));
		return true;
	}
	else if (state.function->inClass && state.function->inClass->super.isNull()) //TODO slot access for derived classes
	{
		uint32_t slotid = state.function->inClass->findInstanceSlotByMultiname(name);
		if (slotid != UINT32_MAX)
		{
			state.preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX_FROMSLOT);
			state.preloadedcode.back().pcode.arg1_uint=slotid;
			state.preloadedcode.back().pcode.arg2_int=-1;
			state.refreshOldNewPosition(code);
			checkForLocalResult(state,code,1,nullptr);
			typestack.push_back(typestackentry(nullptr,false));
			if (r & GETVAR_ISNEWOBJECT)
				ASATOM_DECREF(o);
			return true;
		}
	}
	if (asAtomHandler::isInvalid(o))
	{
		ASObject* target=nullptr;
		state.mi->context->applicationDomain->getVariableAndTargetByMultiname(o,*name, target,state.worker);
		if (asAtomHandler::isValid(o))
		{
			addCachedConstant(state,state.mi, o,code);
			Class_base* cls = asAtomHandler::getClass(o,state.function->getSystemState());
			typestack.push_back(typestackentry(cls,false));
			return true;
		}
	}
	return false;
}

void preloadFunction_secondPass(preloadstate& state)
{
#ifdef ENABLE_OPTIMIZATION
	method_info* mi = state.mi;
	SyntheticFunction* function = state.function;
	Class_base* currenttype=nullptr;
	memorystream codetypes(mi->body->code.data(), mi->body->code.size());
	uint8_t opcode = 0;
	while(!codetypes.atend())
	{
		uint8_t prevopcode=opcode;
		opcode = codetypes.readbyte();
		//LOG(LOG_ERROR,"preload pass2:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< codetypes.tellg()-1<<" "<<currenttype<<" "<<hex<<(int)opcode);
		switch(opcode)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
			case 0x40://newfunction
			case 0x41://call
			case 0x42://construct
			case 0x49://constructsuper
			case 0x53://constructgenerictype
			case 0x55://newobject
			case 0x56://newarray
			case 0x58://newclass
			case 0x59://getdescendants
			case 0x5a://newcatch
			case 0x5d://findpropstrict
			case 0x5e://findproperty
			case 0x5f://finddef
			case 0x60://getlex
			case 0x61://setproperty
			case 0x65://getscopeobject
			case 0x66://getproperty
			case 0x68://initproperty
			case 0x6a://deleteproperty
			case 0x6c://getslot
			case 0x6d://setslot
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
			case 0xb2://istype
			case 0x08://kill
				codetypes.readu30();
				currenttype=nullptr;
				break;
			case 0x62://getlocal
			{
				uint32_t t = codetypes.readu30();
				if (state.defaultlocaltypes.size()>t)
					currenttype=state.defaultlocaltypes[t];
				else
					currenttype=nullptr;
				break;
			}
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
			{
				if (state.defaultlocaltypes.size()>uint32_t(opcode-0xd0))
					currenttype=state.defaultlocaltypes[opcode-0xd0];
				else
					currenttype=nullptr;
				break;
			}
			case 0x86://astype
			case 0x80://coerce
			{
				uint32_t t = codetypes.readu30();
				multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (name->isStatic)
				{
					Type* tp = Type::getTypeFromMultiname(name, mi->context);
					currenttype = dynamic_cast<Class_base*>(tp);
				}
				else
					currenttype=nullptr;
				break;
			}
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				codetypes.readu30();
				break;
			case 0x85://coerce_s
				currenttype=Class<ASString>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x73://convert_i
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x74://convert_u
				currenttype=Class<UInteger>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x75://convert_d
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x76://convert_b
				currenttype=Class<Boolean>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2c://pushstring
				codetypes.readu30();
				currenttype=Class<ASString>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2d://pushint
				codetypes.readu30();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2e://pushuint
				codetypes.readu30();
				currenttype=Class<UInteger>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2f://pushdouble
				codetypes.readu30();
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x31://pushnamespace
				codetypes.readu30();
				currenttype=Class<Namespace>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x28://pushnan
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x63://setlocal
			{
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,currenttype);
				currenttype=nullptr;
				break;
			}
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
				state.unchangedlocals.erase(opcode-0xd4);
				setdefaultlocaltype(state,opcode-0xd4,currenttype);
				currenttype=nullptr;
				break;
			case 0x92://inclocal
			case 0x94://declocal
			{
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,Class<Number>::getRef(function->getSystemState()).getPtr());
				currenttype=nullptr;
				break;
			}
			case 0xc0://increment_i
			case 0xc1://decrement_i
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
			{
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,Class<Integer>::getRef(function->getSystemState()).getPtr());
				currenttype=nullptr;
				break;
			}
			case 0x10://jump
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p)
					skipunreachablecode(state,codetypes,false);
				if (!state.jumptargets.count(p1) && currenttype)
					state.jumptargeteresulttypes[p1] = currenttype;
				currenttype=nullptr;
				break;
			}
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
			{
				codetypes.reads24();
				currenttype=nullptr;
				break;
			}
			case 0x11://iftrue
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p && prevopcode==0x26) //pushtrue
				{
					skipunreachablecode(state,codetypes,false);
				}
				currenttype=nullptr;
				break;
			}
			case 0x12://iffalse
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p && prevopcode==0x27) //pushfalse
				{
					skipunreachablecode(state,codetypes,false);
				}
				currenttype=nullptr;
				break;
			}
			case 0x1b://lookupswitch
			{
				codetypes.reads24();
				uint32_t count = codetypes.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					codetypes.reads24();
				}
				currenttype=nullptr;
				break;
			}
			case 0x24://pushbyte
			{
				codetypes.readbyte();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x25://pushshort
			{
				codetypes.readu32();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x26://pushtrue
			case 0x27://pushfalse
			{
				currenttype=Class<Boolean>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x46://callproperty
			case 0x4c://callproplex
			case 0x4a://constructprop
			case 0x4e://callsupervoid
			case 0x4f://callpropvoid
			{
				codetypes.readu30();
				codetypes.readu30();
				currenttype=nullptr;
				break;
			}
			case 0xef://debug
			{
				codetypes.readbyte();
				codetypes.readu30();
				codetypes.readbyte();
				codetypes.readu30();
				break;
			}
			case 0xa5://lshift
			case 0xa6://rshift
			case 0xa8://bitand
			case 0xa9://bitor
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x09://label
			case 0x2a://dup
			case 0x82://coerce_a
				break;
			case 0x47://returnvoid
			case 0x48://returnvalue
			case 0x03://throw
				skipunreachablecode(state,codetypes,false);
				currenttype=nullptr;
				break;
			default:
				currenttype=nullptr;
				break;
		}
	}
#endif
}

variable* getTempVariableFromClass(Class_base* cls, asAtom& otmp, multiname* name, ASWorker* wrk)
{
	if (cls->is<Class_inherit>())
		cls->as<Class_inherit>()->checkScriptInit();

	if (cls != Class_object::getRef(cls->getSystemState()).getPtr()
		&& cls != Class<Global>::getRef(cls->getSystemState()).getPtr()
		&& !cls->isInterface)
		cls->getInstance(wrk,otmp,false,nullptr,0);
	ASObject* obj = asAtomHandler::getObject(otmp);
	if (obj)
	{
		cls->setupDeclaredTraits(obj,false);
		return obj->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
	}
	return nullptr;
}

Class_base* getSlotResultTypeFromClass(Class_base* cls, uint32_t slot_id, preloadstate& state)
{
	asAtom otmp = asAtomHandler::invalidAtom;
	if (cls->is<Class_inherit>())
		cls->as<Class_inherit>()->checkScriptInit();

	if (cls != Class_object::getRef(cls->getSystemState()).getPtr()
		&& cls != Class<Global>::getRef(cls->getSystemState()).getPtr()
		&& !cls->isInterface)
		cls->getInstance(state.worker,otmp,false,nullptr,0);
	ASObject* obj = asAtomHandler::getObject(otmp);
	if (obj)
	{
		cls->setupDeclaredTraits(obj,false);
		Class_base* resulttype = obj->getSlotType(slot_id,state.mi->context);
		obj->decRef();
		return resulttype;
	}
	return nullptr;
}

}
