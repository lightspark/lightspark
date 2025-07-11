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
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

namespace lightspark
{
uint32_t skipIgnorables(memorystream& code,uint32_t pos)
{
	bool done = false;
	while (!done)
	{
		uint8_t b = code.peekbyteFromPosition(pos);
		switch (b)
		{
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			case 0x09://label
			case 0x76://convert_b
				pos++;
				break;
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				pos = code.skipu30FromPosition(pos);
				pos++;
				break;
			default:
				done=true;
				break;
		}
	}
	return pos;
}

void preload_dup(preloadstate& state,std::vector<typestackentry>& typestack,memorystream& code,int opcode,Class_base** lastlocalresulttype, std::map<int32_t,int32_t>& jumppositions,std::map<int32_t,int32_t>& jumpstartpositions)
{
	int32_t p = code.tellg();
#ifdef ENABLE_OPTIMIZATION
	if (state.jumptargets.find(p) != state.jumptargets.end())
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		clearOperands(state,true,lastlocalresulttype);
		typestack.push_back(typestack.back());
		return;
	}
	bool dupoperand = state.operandlist.empty() && state.lastlocalresultpos != UINT32_MAX;
	if (dupoperand)
		state.operandlist.push_back(operands(OP_LOCAL,state.localtypes[state.lastlocalresultpos],state.lastlocalresultpos,0,0));
	state.lastlocalresultpos=UINT32_MAX;
	if (state.operandlist.size() > 0)
	{
		if (state.operandlist.back().type == OP_LOCAL || state.operandlist.back().type == OP_CACHED_SLOT)
		{
			Class_base* restype = nullptr;
			if (typestack.back().obj && typestack.back().obj->is<Class_base>())
				restype=typestack.back().obj->as<Class_base>();
			operands op = state.operandlist.back();
			// if this dup is followed by increment/decrement and setlocal, it can be skipped and converted into a single increment/decrement call
			// if this dup is followed by iftrue/iffalse and pop, it can be skipped and converted into a single iftrue/iffalse call
			bool handled = false;
			uint8_t b = code.peekbyte();
			uint32_t opcode_optimized=0;
			uint32_t pos =p+1;
			uint32_t num=0;
			int32_t jump=0;
			bool is_iftruefalse=false;
			if (state.jumptargets.find(pos+1) == state.jumptargets.end() && b == 0x76)//convert_b
			{
				switch (code.peekbyteFromPosition(pos))
				{
					case 0x11://iftrue
					case 0x12://iffalse
					{
						if (code.peeks24FromPosition(pos+1)<0)
							break;
						// dup is followed by convert_b and iftrue/iffalse, convert_b can be ignored
						b=code.peekbyteFromPosition(pos);
						pos++;
						p++;
						break;
					}
					default:
						break;
				}
			}
			switch (b)
			{
				case 0x91://increment
					opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC;
					break;
				case 0x93://decrement
					opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+1;
					break;
				case 0xc0://increment_i
					opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+2;
					break;
				case 0xc1://decrement_i
					opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+3;
					break;
				case 0x11://iftrue
					if (state.jumptargets.find(pos) != state.jumptargets.end())
						break;
					is_iftruefalse=true;
					jump = code.peeks24FromPosition(pos);
					if (jump >0)
					{
						// optimize common case of checking for multiple conditions like "if (a>0 || b>0)"
						int jumppos = pos+jump+3+1;
						bool dupskippable=false;
						int lastskipdup=1;
						int numjumps=0;
						auto it = state.jumptargets.find(jumppos);
						while (it != state.jumptargets.end())
						{
							jumppos = skipIgnorables(code,jumppos);
							int skipdup = code.peekbyteFromPosition(jumppos-1) ==0x2a ? 1 : 0; //dup
							if (code.peekbyteFromPosition(jumppos-1+skipdup) ==0x11)//iftrue
							{
								if (!numjumps)
								{
									if ((*it).second == 1)
										state.jumptargets.erase(it);
									else
										(*it).second--;
								}
								dupskippable= lastskipdup;
								int32_t jumpdelta = code.peeks24FromPosition(jumppos+skipdup);
								jump += jumpdelta+3+1+skipdup;
								jumppos += jumpdelta+3+1+skipdup;
								it = state.jumptargets.find(jumppos);
								numjumps++;
							}
							else if (code.peekbyteFromPosition(jumppos-1+skipdup) ==0x12)//iffalse
							{
								if (!numjumps)
								{
									if ((*it).second == 1)
										state.jumptargets.erase(it);
									else
										(*it).second--;
								}
								dupskippable= lastskipdup;
								jump += 3+1+skipdup; // jump to first opcode after iffalse
								jumppos = pos+jump+3+1;
								it = state.jumptargets.find(jumppos);
								numjumps++;
							}
							else
							{
								dupskippable= !lastskipdup;
								break;
							}
							lastskipdup=skipdup;
						}
						if (numjumps)
						{
							pos +=3;
							is_iftruefalse=true;
							opcode_optimized=dupskippable ? ABC_OP_OPTIMZED_IFTRUE+1:ABC_OP_OPTIMZED_IFTRUE_DUP+1;
							if (it == state.jumptargets.end())
								state.jumptargets[jumppos]=1;
							else
								(*it).second++;
							break;
						}
					}

					pos +=3;
					opcode_optimized=ABC_OP_OPTIMZED_IFTRUE_DUP+1;
					break;
				case 0x12://iffalse
					if (state.jumptargets.find(pos) != state.jumptargets.end())
						break;
					jump = code.peeks24FromPosition(pos);
					if (jump >0)
					{
						// optimize common case of checking for multiple conditions like "if (a>0 && b>0)"
						int jumppos = pos+jump+3+1;
						int numjumps=0;
						bool dupskippable = false;
						int lastskipdup=1;
						auto it = state.jumptargets.find(jumppos);
						while (it != state.jumptargets.end())
						{
							jumppos = skipIgnorables(code,jumppos);
							int skipdup = code.peekbyteFromPosition(jumppos-1) ==0x2a ? 1 : 0; //dup
							if (code.peekbyteFromPosition(jumppos-1+skipdup) ==0x12)//iffalse
							{
								if (!numjumps)
								{
									if ((*it).second == 1)
										state.jumptargets.erase(it);
									else
										(*it).second--;
								}
								dupskippable= lastskipdup;
								int32_t jumpdelta = code.peeks24FromPosition(jumppos+skipdup);
								jump += jumpdelta+3+1+skipdup;
								jumppos += jumpdelta+3+1+skipdup;
								it = state.jumptargets.find(jumppos);
								numjumps++;
							}
							else if (code.peekbyteFromPosition(jumppos-1+skipdup) ==0x11)//iftrue
							{
								if (!numjumps)
								{
									if ((*it).second == 1)
										state.jumptargets.erase(it);
									else
										(*it).second--;
								}
								dupskippable= lastskipdup;
								jump += 3+1+skipdup; // jump to first opcode after iftrue
								jumppos = pos+jump+3+1;
								it = state.jumptargets.find(jumppos);
								numjumps++;
							}
							else
							{
								dupskippable= !lastskipdup;
								break;
							}
							lastskipdup=skipdup;
						}
						if (numjumps)
						{
							pos +=3;
							is_iftruefalse=true;
							opcode_optimized=dupskippable ? ABC_OP_OPTIMZED_IFFALSE+1 : ABC_OP_OPTIMZED_IFFALSE_DUP+1;
							if (it == state.jumptargets.end())
								state.jumptargets[jumppos]=1;
							else
								(*it).second++;
							break;
						}
					}
					pos +=3;
					is_iftruefalse=true;
					opcode_optimized=ABC_OP_OPTIMZED_IFFALSE_DUP+1;
					break;
				default:
					break;
			}
			if (opcode_optimized)
			{
				b = code.peekbyteFromPosition(pos);
				switch (b)
				{
					case 0x63://setlocal
					{
						num = code.peeku30FromPosition(pos+1);
						pos = code.skipu30FromPosition(pos+1);
						handled = true;
						is_iftruefalse=false;
						break;
					}
					case 0xd4: //setlocal_0
					case 0xd5: //setlocal_1
					case 0xd6: //setlocal_2
					case 0xd7: //setlocal_3
						num = b-0xd4;
						pos++;
						handled = true;
						is_iftruefalse=false;
						break;
					case 0x29: //pop
						if (is_iftruefalse)
						{
							if (state.jumptargets.find(pos) != state.jumptargets.end())
							{
								is_iftruefalse=false;
								break;
							}
							pos++;
							handled = true;
						}
						break;
					default:
						is_iftruefalse=false;
						break;
				}
			}
			if (is_iftruefalse)
			{
				// remove used operand
				auto it = state.operandlist.end();
				(--it)->removeArg(state);
				state.operandlist.pop_back();
				state.canlocalinitialize.clear();
				state.preloadedcode.push_back(opcode_optimized);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.local_pos1 = op.index;
				state.preloadedcode.back().cachedslot1 = op.type == OP_CACHED_SLOT;
				jumppositions[state.preloadedcode.size()-1] = jump;
				jumpstartpositions[state.preloadedcode.size()-1] = p+3+1;
				state.oldnewpositions[p+3+1] = (int32_t)state.preloadedcode.size();
				// skip pop
				code.seekg(pos);
				clearOperands(state,true,lastlocalresulttype);
				return;
			}
			else if (handled)
			{
				op.removeArg(state);
				state.preloadedcode.push_back(opcode_optimized);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.local_pos1 = op.index;
				state.preloadedcode.back().cachedslot1 = op.type == OP_CACHED_SLOT;
				state.preloadedcode.back().pcode.local_pos2 = num;
				state.localtypes.push_back(restype);
				state.defaultlocaltypes.push_back(restype);
				state.defaultlocaltypescacheable.push_back(true);
				uint32_t num2 = state.mi->body->getReturnValuePos()+1+state.mi->body->localresultcount;
				state.mi->body->localresultcount++;
				state.preloadedcode.back().pcode.local3.pos = num2;
				code.seekg(pos);
				removeOperands(state,false,nullptr,1);
				state.preloadedcode.push_back(0x62);//getlocal
				state.preloadedcode.back().pcode.arg3_uint = num2;
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.operandlist.push_back(operands(OP_LOCAL,restype,num2,1,state.preloadedcode.size()-1));
				state.operandlist.back().duparg1=true;
				return;
			}
			// this ensures that the "old" value is stored in a localresult and can be used later, as the duplicated value may be changed by an increment etc.
			setupInstructionOneArgument(state,ABC_OP_OPTIMZED_DUP,opcode,code,false,true,restype,code.tellg(),opcode_optimized==0,false,true,true,ABC_OP_OPTIMZED_DUP_SETSLOT);
			if (!dupoperand)
				addOperand(state,op,code);
		}
		else
		{
			operands op = state.operandlist.back();
			uint32_t preloadedcodepos = op.codecount ? op.preloadedcodepos : state.preloadedcode.size()-1;
			uint32_t val = state.preloadedcode[preloadedcodepos].pcode.arg3_uint;
			state.preloadedcode.push_back(state.preloadedcode[preloadedcodepos].opcode);
			state.preloadedcode.back().pcode.arg3_uint=val;
			state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
			state.operandlist.push_back(operands(op.type,op.objtype,op.index,1,state.preloadedcode.size()-1));
		}
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
	}
	typestack.push_back(typestack.back());
}

}
