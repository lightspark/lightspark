/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include "compat.h"
#include "abcutils.h"
#include "toplevel/toplevel.h"
#include "toplevel/ASString.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

void ABCVm::writeInt32(std::ostream& o, int32_t val)
{
	o.write((char*)&val, 4);
}

void ABCVm::writeDouble(std::ostream& o, double val)
{
	o.write((char*)&val, 8);
}

void ABCVm::verifyBranch(std::map<uint32_t,BasicBlock> basicBlocks, int oldStart,
			 int here, int offset, int code_len)
{
	//Compute the destination block
	//If backward verifies that the address is the start of a block.
	//If forward add a new block at that address
	int dest=here+offset;
	if(dest >= code_len)
		throw ParseException("Jump out of bounds in optimizer");
	if(offset==0)
		throw ParseException("Bad jump in optmizer");
	else if(offset<0)
	{
		//Backward branch
		if(basicBlocks.find(dest)==basicBlocks.end())
			throw ParseException("Bad jump in optmizer");
	}
	else//if(t>0)
	{
		//Forward branch
		auto it=basicBlocks.find(oldStart);
		if(it!=basicBlocks.end())
			throw ParseException("Bad code in optmizer");
		BasicBlock* predBlock=&(it->second);
		BasicBlock* nextBlock=&(basicBlocks.insert(
			make_pair(dest,BasicBlock(predBlock))).first->second);
		nextBlock->pred.push_back(oldStart);
	}
}

void ABCVm::optimizeFunction(SyntheticFunction* function)
{
	method_info* mi=function->mi;

	istringstream code(mi->body->code);
	const int code_len=code.str().length();
	ostringstream out;

	u8 opcode;

	std::map<uint32_t, BasicBlock> basicBlocks;

	uint32_t curStart=0;
	BasicBlock* curBlock=&(basicBlocks.insert(
			make_pair(0,BasicBlock(NULL))).first->second);

	//Rewrite optimized code for faster execution, the new format is
	//uint8 opcode, [uint32 operand]* | [ASObject* pre resolved object]
	//Analize validity of basic blocks
	//Understand types of the values on the local scope stack
	//Optimize away getLex
	while(1)
	{
		code >> opcode;
		if(code.eof())
		{
			//As the code has ended we must be in no block
			if(curBlock==NULL)
				break;
			else
				throw ParseException("End of code in optimizer");
		}
		if(curBlock==NULL)
		{
			//Try to find the block for this instruction
			int here=code.tellg();
			auto it=basicBlocks.find(here-1);
			if(it!=basicBlocks.end())
			{
				curBlock=&(it->second);
				curStart=it->first;
			}
			else
			{
				LOG(LOG_ERROR,"Unreachable code");
				continue;
			}
		}

		switch(opcode)
		{
			case 0x02:
			{
				//nop
				break;
			}
			case 0x03:
			{
				//throw
				curBlock->popStack(1);
				out << (uint8_t)opcode;
				curBlock=NULL;
				break;
			}
			case 0x04:
			{
				//getsuper
				u30 t;
				code >> t;
				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1);
				curBlock->pushStack(Type::anyType);
				out << (uint8_t)opcode;
				writeInt32(out, t);
				break;
			}
			case 0x05:
			{
				//setsuper
				u30 t;
				code >> t;
				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+2);
				out << (uint8_t)opcode;
				writeInt32(out, t);
				break;
			}
			case 0x06:
			{
				//dxns
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out, t);
				break;
			}
			case 0x07:
			{
				//dxnslate
				curBlock->popStack(1);
				out << (uint8_t)opcode;
				break;
			}
			case 0x08:
			{
				//kill
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out, t);
				break;
			}
			case 0x09:
			{
				//label
				//Create a new basic block
				BasicBlock* oldBlock=curBlock;
				uint32_t oldStart=curStart;
				int here=code.tellg();
				curStart=here-1;
				curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(oldBlock))).first->second);
				curBlock->pred.push_back(oldStart);
				break;
			}
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x0f:
			case 0x13:
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x17:
			case 0x18:
			case 0x19:
			case 0x1a:
			{
				//ifnlt
				//ifnle
				//ifngt
				//ifnge
				//ifeq
				//ifne
				//iflt
				//ifle
				//ifgt
				//ifge
				//ifstricteq
				//ifstrictne
				s24 t;
				code >> t;
				curBlock->popStack(2);
				out << (uint8_t)opcode;
				writeInt32(out, t);
				
				BasicBlock* oldBlock=curBlock;
				uint32_t oldStart=curStart;
				//The new block starts after this function
				int here=code.tellg();
				curStart=here;
				curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(oldBlock))).first->second);
				curBlock->pred.push_back(oldStart);
				verifyBranch(basicBlocks,curStart,here,t,code_len);
				break;
			}
			case 0x10:
			{
				//jump
				s24 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out, t);

				//The new block starts after this function
				int here=code.tellg();
				curStart=here;
				verifyBranch(basicBlocks,curStart,here,t,code_len);
				//Reset the block to NULL
				curBlock=NULL;
				break;
			}
			case 0x11:
			case 0x12:
			{
				//iftrue
				//iffalse
				s24 t;
				code >> t;
				curBlock->popStack(1);
				out << (uint8_t)opcode;
				writeInt32(out, t);

				BasicBlock* oldBlock=curBlock;
				uint32_t oldStart=curStart;
				//The new block starts after this function
				int here=code.tellg();
				curStart=here;
				curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(oldBlock))).first->second);
				curBlock->pred.push_back(oldStart);
				verifyBranch(basicBlocks,curStart,here,t,code_len);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
				s24 t;
				code >> t;
				u30 count;
				code >> count;
				s24* offsets=g_newa(s24, count+1);
				for(unsigned int i=0;i<count+1;i++)
					code >> offsets[i];
				curBlock->popStack(1);
				out << (uint8_t)opcode;
				writeInt32(out, t);
				writeInt32(out, count);
				for(int i=0;i<(count+1);i++)
					writeInt32(out, (int32_t)offsets[i]);

				verifyBranch(basicBlocks,curStart,here,t,code_len);
				for(int i=0;i<(count+1);i++)
					verifyBranch(basicBlocks,curStart,here,offsets[i],code_len);
				curBlock=NULL;
				break;
			}
			case 0x1c:
			{
				//pushwith
				curBlock->popStack(1);
				curBlock->pushScopeStack(Type::anyType);
				out << (uint8_t)opcode;
				break;
			}
			case 0x1d:
			{
				//popscope
				curBlock->popScopeStack();
				out << (uint8_t)opcode;
				break;
			}
			case 0x1e:
			{
				//nextname
				curBlock->popStack(2);
				curBlock->pushStack(Type::anyType);
				out << (uint8_t)opcode;
				break;
			}
			case 0x20:
			{
				//pushnull
				curBlock->pushStack(Type::anyType);
				out << (uint8_t)opcode;
				break;
			}
			case 0x21:
			{
				//pushundefined
				curBlock->pushStack(Type::anyType);
				out << (uint8_t)opcode;
				break;
			}
			case 0x23:
			{
				//nextvalue
				curBlock->popStack(2);
				curBlock->pushStack(Type::anyType);
				out << (uint8_t)opcode;
				break;
			}
			case 0x24:
			{
				//pushbyte
				int8_t t;
				code.read((char*)&t,1);
				out << (uint8_t)opcode;
				out << t;
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x25:
			{
				//pushshort
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				u32 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out, t);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x26:
			{
				//pushtrue
				out << (uint8_t)opcode;
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0x27:
			{
				//pushfalse
				out << (uint8_t)opcode;
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0x28:
			{
				//pushnan
				out << (uint8_t)opcode;
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x29:
			{
				//pop
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				break;
			}
			case 0x2a:
			{
				//dup
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x2b:
			{
				//swap
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				curBlock->pushStack(Type::anyType);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x2c:
			{
				//pushstring
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out, t);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x2d:
			{
				//pushint
				u30 t;
				code >> t;
				//TODO: collabpse on pushshort
				s32 val=mi->context->constant_pool.integer[t];
				out << (uint8_t)opcode;
				writeInt32(out, val);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x2e:
			{
				//pushuint
				u30 t;
				code >> t;
				u32 val=mi->context->constant_pool.uinteger[t];
				out << (uint8_t)opcode;
				writeInt32(out, val);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x2f:
			{
				//pushdouble
				u30 t;
				code >> t;
				double val=mi->context->constant_pool.doubles[t];
				out << (uint8_t)opcode;
				writeDouble(out, val);
				curBlock->pushStack(Class<Number>::getClass());
				break;
			}
			case 0x30:
			{
				//pushscope
				const Type* t=curBlock->peekStack();
				curBlock->popStack(1);
				curBlock->pushScopeStack(t);
				out << (uint8_t)opcode;
				break;
			}
			case 0x31:
			{
				//pushnamespace
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x32:
			{
				//hasnext2
				u30 t,t2;
				code >> t;
				code >> t2;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				writeInt32(out,t2);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			//Alchemy opcodes
			case 0x35:
			case 0x36:
			case 0x37:
			{
				//li8
				//li16
				//li32
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x3a:
			case 0x3b:
			case 0x3c:
			{
				//si8
				//si16
				//si32
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				break;
			}
			case 0x40:
			{
				//newfunction
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x41:
			{
				//call
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				curBlock->popStack(t+2);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x42:
			{
				//construct
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				curBlock->popStack(t+1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x45:
			case 0x46:
			case 0x4c: //callproplex seems to be exactly like callproperty
			{
				//callsuper
				//callproperty
				u30 t,t2;
				code >> t;
				code >> t2;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				writeInt32(out,t2);
				
				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1+t2);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x47:
			{
				//returnvoid
				out << (uint8_t)opcode;
				curBlock=NULL;
				break;
			}
			case 0x48:
			{
				//returnvalue
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock=NULL;
				break;
			}
			case 0x49:
			{
				//constructsuper
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				curBlock->popStack(t+1);
				break;
			}
			case 0x4a:
			case 0x4e:
			case 0x4f:
			{
				//constructprop
				//callsupervoid
				//callpropvoid
				u30 t,t2;
				code >> t;
				code >> t2;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				writeInt32(out,t2);
				
				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1+t2);
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				
				curBlock->popStack(t+1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x55:
			{
				//newobject
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				
				curBlock->popStack(t*2);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x56:
			{
				//newarray
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				
				curBlock->popStack(t);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x57:
			{
				//newactivation
				out << (uint8_t)opcode;
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x58:
			{
				//newclass
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				
				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x59:
			{
				//getdescendants
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				
				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x5a:
			{
				//newcatch
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x5e:
			{
				//findproperty
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x60:
			{
				//getlex
				u30 t;
				code >> t;
				//TODO: optimize
				out << (uint8_t)opcode;
				writeInt32(out,t);
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");

				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x61:
			{
				//setproperty
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+2);
				break;
			}
			case 0x62:
			{
				//getlocal
				u30 i;
				code >> i;
				out << (uint8_t)opcode;
				writeInt32(out,i);

				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x63:
			{
				//setlocal
				u30 i;
				code >> i;
				out << (uint8_t)opcode;
				writeInt32(out,i);

				curBlock->popStack(1);
				break;
			}
			case 0x64:
			{
				//getglobalscope
				out << (uint8_t)opcode;
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x65:
			{
				//getscopeobject
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x66:
			{
				//getproperty
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x68:
			{
				//initproperty
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+2);
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+1);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0x6c:
			{
				//getslot
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x6d:
			{
				//setslot
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);

				curBlock->popStack(2);
				break;
			}
			case 0x70:
			case 0x71:
			case 0x72:
			case 0x85:
			case 0x95:
			{
				//convert_s
				//esc_xelem
				//esc_xattr
				//coerce_s
				//typeof
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<ASString>::getClass());
				break;
			}
			case 0x73:
			{
				//convert_i
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0x74:
			{
				//convert_u
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<UInteger>::getClass());
				break;
			}
			case 0x75:
			{
				//convert_d
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Number>::getClass());
				break;
			}
			case 0x76:
			{
				//convert_b
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0x78:
			{
				//checkfilter
				//TODO: it may be optimized here
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x80:
			{
				//coerce
				//TODO: it may be optimized here
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");
				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x82:
			{
				//coerce_a
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x86:
			{
				//astype
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");
				//TODO: write the right type
				curBlock->popStack(1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x87:
			{
				//astypelate
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x90:
			case 0x91:
			case 0x93:
			{
				//negate
				//increment
				//decrement
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Class<Number>::getClass());
				break;
			}
			case 0x92:
			case 0x94:
			{
				//inclocal
				//declocal
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				break;
			}
			case 0x96:
			{
				//not
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0x97:
			case 0xc0:
			case 0xc1:
			case 0xc4:
			{
				//bitnot
				//increment_i
				//decrement_i
				//negate_i
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0xa0:
			{
				//add
				out << (uint8_t)opcode;
				//TODO: some type inference may be done here
				curBlock->popStack(2);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			{
				//subtract
				//multiply
				//divide
				//modulo
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				curBlock->pushStack(Class<Number>::getClass());
				break;
			}
			case 0xa5:
			case 0xa6:
			case 0xa7:
			case 0xa8:
			case 0xa9:
			case 0xaa:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			{
				//lshift
				//rshift
				//urshift
				//bitand
				//bitor
				//bitxor
				//add_i
				//subtract_i
				//multiply_i
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				curBlock->pushStack(Class<Integer>::getClass());
				break;
			}
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xae:
			case 0xaf:
			case 0xb0:
			case 0xb1:
			case 0xb3:
			case 0xb4:
			{
				//equals
				//strictequals
				//lessthan
				//lessequals
				//greaterthan
				//greaterequals
				//instanceof
				//istypelate
				//in
				out << (uint8_t)opcode;
				curBlock->popStack(2);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0xb2:
			{
				//istype
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");
				curBlock->popStack(1);
				curBlock->pushStack(Class<Boolean>::getClass());
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				break;
			}
			case 0xc3:
			{
				//declocal_i
				u30 t;
				code >> t;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				//TODO: collapse on getlocal
				out << (uint8_t)opcode;
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0xd4:
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				//TODO: collapse on setlocal
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				break;
			}
			case 0xef:
			{
				//debug, ignore
				uint8_t debug_type;
				u30 index;
				uint8_t reg;
				u30 extra;
				code.read((char*)&debug_type,1);
				code >> index;
				code.read((char*)&reg,1);
				code >> extra;
				break;
			}
			case 0xf0:
			case 0xf1:
			{
				//debugline, ignore
				//debugfile, ignore
				u30 t;
				code >> t;
				break;
			}
			default:
				LOG(LOG_ERROR,_("Not optimizable instruction @") << code.tellg());
				LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << dec);
				throw ParseException("Not implemented instruction in optimizer");
		}
	}
	//Overwrite the old code
	mi->body->code=out.str();
}
