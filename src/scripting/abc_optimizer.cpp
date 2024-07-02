/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Null.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/display/RootMovieClip.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

enum SPECIAL_OPCODES { SET_SLOT_NO_COERCE = 0xfb, COERCE_EARLY = 0xfc, GET_SCOPE_AT_INDEX = 0xfd, GET_LEX_ONCE = 0xfe, PUSH_EARLY = 0xff };

struct lightspark::InferenceData
{
	Type* type;
	const ASObject* obj;
	InferenceData():type(NULL),obj(NULL){}
	InferenceData(Type* t):type(t),obj(NULL){}
	InferenceData(const ASObject* o):type(NULL),obj(o){}
	bool isValid() const { return type!=NULL || obj!=NULL; }
	/* Method to understand if the passed InferenceData is of the type of this InferenceData
	 * \param c Must be not NULL
	 */
	bool isOfType(Class_base* c)
	{
		if(this->type)
		{
			Class_base* classType=dynamic_cast<Class_base*>((Type*)this->type);
			if(classType && classType->isSubClass(c))
				return true;
		}
		else if(this->obj)
		{
			//Both null and undefined can be of any class
			if(this->obj->getObjectType()==T_NULL || this->obj->getObjectType()==T_UNDEFINED)
				return true;
		}
		return false;
	}
};

struct lightspark::BasicBlock
{
	BasicBlock(BasicBlock* pred):realStart(0xffffffff),realEnd(0xffffffff),originalEnd(0xffffffff)
	{
		//Any predecessor block is fine. Consistency for all predecessors
		//will be verified at the end of the optimization
		if(pred)
		{
			stackTypes=initialStackTypes=pred->stackTypes;
			scopeStackTypes=initialScopeStackTypes=pred->scopeStackTypes;
		}
	}
	std::vector<InferenceData> initialStackTypes;
	std::vector<InferenceData> stackTypes;
	std::vector<InferenceData> initialScopeStackTypes;
	std::vector<InferenceData> scopeStackTypes;
	std::vector<BasicBlock*> predBlocks;
	/*
	 * Pointers that must be set the actual offset of this block in optmized code
	 */
	std::vector<uint32_t> fixups;
	uint32_t realStart;
	uint32_t realEnd;
	uint32_t originalEnd;
	/*
	 * This methods reset all the values that depends on the
	 * actual translated code. Predecessor blocks and pending fixups will be left alone
	 */
	void resetCode()
	{
		stackTypes=initialStackTypes;
		scopeStackTypes=initialScopeStackTypes;
		realStart = 0xffffffff;
		realEnd = 0xffffffff;
		originalEnd = 0xffffffff;
	}
	const InferenceData peekStack() const
	{
		return stackTypes.back();
	}
	void popStack(uint32_t n)
	{
		if(stackTypes.size()<n)
			throw ParseException("Invalid code in optimizer");
		for(uint32_t i=0;i<n;i++)
			stackTypes.pop_back();
	}
	void pushStack(Type* t)
	{
		stackTypes.push_back(InferenceData(t));
	}
	void pushStack(InferenceData t)
	{
		stackTypes.push_back(t);
	}
	void popScopeStack()
	{
		if(scopeStackTypes.empty())
			throw ParseException("Invalid code in optimizer");
		scopeStackTypes.pop_back();
	}
	void pushScopeStack(Type* t)
	{
		scopeStackTypes.push_back(InferenceData(t));
	}
	void pushScopeStack(InferenceData t)
	{
		scopeStackTypes.push_back(t);
	}
};

EARLY_BIND_STATUS ABCVm::earlyBindForScopeStack(ostream& out, const SyntheticFunction* f,
		const std::vector<InferenceData>& scopeStack, const multiname* name, InferenceData& inferredData)
{
	//We try to find out the position on the scope stack where the name is found
	uint32_t totalScopeStackLen = f->func_scope->scope.size() + scopeStack.size();

	bool found=false;
	//Traverse in reverse order the scope stack. Stops at the first any/unknown type.
	for(auto it=scopeStack.rbegin();it!=scopeStack.rend();++it)
	{
		totalScopeStackLen--;
		if(!it->isValid())
		{
			std::cerr << "No inferred data" << std::endl;
			return CANNOT_BIND;
		}
		if(it->type)
		{
			EARLY_BIND_STATUS status=it->type->resolveMultinameStatically(*name);
			if(status==CANNOT_BIND)
				return CANNOT_BIND;
			if(status==BINDED)
			{
				found=true;
				inferredData=*it;
				break;
			}
		}
		else //if(it->obj)
		{
			std::cerr << "Scope lookup on objects is not supported" << std::endl;
			return CANNOT_BIND;
		}
	}

	if(found==false)
	{
		//TODO: We can also push directly those objects since they are fixed
		std::cerr << "End of local stack" << std::endl;
		//Now look in the objects that are stored in the function save scope stack
		for(auto it=f->func_scope->scope.rbegin();it!=f->func_scope->scope.rend();++it)
		{
			totalScopeStackLen--;
			if(it->considerDynamic)
			{
				//We can't early bind
				return CANNOT_BIND;
			}

			const variable* var=asAtomHandler::toObject(it->object,f->getInstanceWorker())->findVariableByMultiname(*name, asAtomHandler::toObject(it->object,f->getInstanceWorker())->getClass(),nullptr,nullptr,false,f->getInstanceWorker());
			if(var)
			{
				found=true;
				inferredData.obj=asAtomHandler::toObject(it->object,f->getInstanceWorker());
				break;
			}
		}
	}
	if(found)
	{
		out << (uint8_t)GET_SCOPE_AT_INDEX;
		writeInt32(out, totalScopeStackLen);
		return BINDED;
	}
	return NOT_BINDED;
}

InferenceData ABCVm::earlyBindFindPropStrict(ostream& out, const SyntheticFunction* f,
		const std::vector<InferenceData>& scopeStack, const multiname* name)
{
	InferenceData ret;
	EARLY_BIND_STATUS status=earlyBindForScopeStack(out, f, scopeStack, name, ret);
	if(status==BINDED || status==CANNOT_BIND)
		return ret;
	//Look on the application domain
	ASObject* target;
	bool found = f->mi->context->root->applicationDomain->findTargetByMultiname(*name, target,f->mi->context->root->getInstanceWorker());
	if(found)
	{
		//If we found the property on the application domain we can safely use the target verbatim
		std::cerr << "OPT EARLY" << *name << std::endl;
		out << (uint8_t)PUSH_EARLY;
		writePtr(out, target);
		ret.obj=target;
		return ret;
	}
	return ret;
}

InferenceData ABCVm::earlyBindGetLex(ostream& out, const SyntheticFunction* f, const std::vector<InferenceData>& scopeStack,
		const multiname* name, uint32_t nameIndex)
{
	InferenceData ret;
	EARLY_BIND_STATUS status=earlyBindForScopeStack(out, f, scopeStack, name, ret);
	if(status==BINDED)
	{
		//Synthetize a getProperty here
		std::cerr << "SYNT GET " << *name << std::endl;
		out << (uint8_t)0x66;
		writeInt32(out,nameIndex);
		//We can't return the inferredData directly, since we don't know the type of the getted object
		return InferenceData(Type::anyType);
	}
	else if(status==CANNOT_BIND)
		return ret;
	//Now look in the application domain
	ASObject* target;
	//Now we should serach in the applicationDomain. The system domain is the first one searched. We can safely
	//early bind for it, but not for custom domains, since we may change the expected order of evaluation
	asAtom o=asAtomHandler::invalidAtom;
	f->getSystemState()->systemDomain->getVariableAndTargetByMultiname(o,*name, target,f->getSystemState()->worker);
	if(asAtomHandler::isValid(o))
	{
		//Output a special opcode
		out << (uint8_t)PUSH_EARLY;
		writePtr(out, asAtomHandler::toObject(o,f->getInstanceWorker()));
		ret.obj=asAtomHandler::getObject(o);
		return ret;
	}
	//About custom domains. We can't resolve the object now. But we can output a special getLex opcode that will
	//rewrite itself to a PUSH_EARLY when it's executed.
	//NOTE: We use findVariableByMultiname because we don't want to actually run the init scripts now
	bool found = f->mi->context->root->applicationDomain->findTargetByMultiname(*name, target,f->mi->context->root->getInstanceWorker());
	if(found)
	{
		out << (uint8_t)GET_LEX_ONCE;
		//Write directly the multiname pointer
		writePtr(out, name);
		//We need to set the returned InferenceData to a valid state
		ret.type=Type::anyType;
		return ret;
	}
	return ret;
}

Type* ABCVm::getLocalType(const SyntheticFunction* f, unsigned localIndex)
{
	if(localIndex==0 && f->isMethod())
		return f->inClass;
	else if((localIndex-1) < f->mi->paramTypes.size())
		return f->mi->paramTypes[localIndex-1];

	return Type::anyType;
}

void ABCVm::writeInt32(std::ostream& o, int32_t val)
{
	o.write((char*)&val, 4);
}

void ABCVm::writeDouble(std::ostream& o, double val)
{
	o.write((char*)&val, 8);
}

void ABCVm::writePtr(std::ostream& o, const void* val)
{
	o.write((char*)&val, 8);
}

void ABCVm::verifyBranch(std::set<uint32_t>& pendingBlocks,
		std::map<uint32_t,BasicBlock>& basicBlocks, int oldStart,
			 int here, int offset, int code_len)
{
	//Compute the destination block
	//If backward verifies that the address is the start of a block.
	//If forward add a new block at that address
	int dest=here+offset;
	if(dest >= code_len)
		throw ParseException("Jump out of bounds in optimizer");

	auto it=basicBlocks.find(oldStart);
	if(it==basicBlocks.end())
		throw ParseException("Bad code in optmizer");
	BasicBlock* predBlock=&(it->second);
	it=basicBlocks.find(dest);
	if(it==basicBlocks.end())
	{
		//The destination has not been done already. Add a new block to the map
		//and to the pending blocks vector
		BasicBlock* nextBlock=&(basicBlocks.insert(
			make_pair(dest,BasicBlock(predBlock))).first->second);
		nextBlock->predBlocks.push_back(predBlock);
		pendingBlocks.insert(dest);
	}
	else
	{
		//The destination already exists, add this block to the pred
		it->second.predBlocks.push_back(predBlock);
	}
}

void ABCVm::writeBranchAddress(std::map<uint32_t,BasicBlock>& basicBlocks, int here, int offset, std::ostream& out)
{
	int dest=here+offset;
	auto it=basicBlocks.find(dest);
	assert(it!=basicBlocks.end());
	//We need to add a fixup for this branch
	it->second.fixups.push_back(out.tellp());
	writeInt32(out, 0xffffffff);
}

void ABCVm::optimizeFunction(SyntheticFunction* function)
{
	method_info* mi=function->mi;
	SystemState* sys = function->getSystemState();
	
	ActivationType activationType(mi);

	istringstream code(mi->body->code);
	const int code_len=mi->body->code.size();
	ostringstream out;

	u8 opcode;

	std::map<uint32_t, BasicBlock> basicBlocks;
	std::set<uint32_t> pendingBlocks;

	uint32_t curStart=0;
	BasicBlock* curBlock=NULL;
	basicBlocks.insert(make_pair(0,BasicBlock(NULL)));
	pendingBlocks.insert(0);

	//Create a map of addresses to fixups to rewrite the exception data: from, to and target
	for(uint32_t i=0;i<mi->body->exceptions.size();i++)
	{
		exception_info_abc& ei=mi->body->exceptions[i];
		//Also create a new basic block at the exception target address,
		//otherwise that code might be considered unreachable and lost
		if(basicBlocks.find(ei.target)==basicBlocks.end())
		{
			BasicBlock* expBlock=&(basicBlocks.insert(make_pair(ei.target,BasicBlock(NULL))).first->second);
			//Those blocks starts with the exception on the stack
			expBlock->pushStack(Type::anyType);
			expBlock->initialStackTypes = expBlock->stackTypes;
			pendingBlocks.insert(ei.target);
		}
	}

	//Instructions map, useful to translate exceptions and validate just addresses
	std::map<uint32_t, uint32_t> instructionsMap;

	//Rewrite optimized code for faster execution, the new format is
	//uint8 opcode, [uint32 operand]* | [ASObject* pre resolved object]
	//Analize validity of basic blocks
	//Understand types of the values on the local scope stack
	//Optimize away getLex
	while(1)
	{
		uint32_t here=code.tellg();
		uint32_t there=out.tellp();
		if(curBlock && curStart!=here)
		{
			//Try to find the block for this instruction
			auto it=basicBlocks.find(here);
			if(it!=basicBlocks.end())
			{
				if(it->second.realStart!=0xffffffff)
				{
					//Fall into an already translated block
					//Create a jump to it
					out << (uint8_t)0x10;
					writeInt32(out, 0xffffffff);
					there = out.tellp();
					it->second.fixups.push_back(there-4);
					//End the current block, so that a pending one can be selected
					curBlock->realEnd=there;
					curBlock->originalEnd=here;
					curBlock=NULL;
				}
				else
				{
					BasicBlock* predBlock=curBlock;
					predBlock->realEnd=there;
					predBlock->originalEnd=here;
					curBlock=&(it->second);
					curBlock->realStart=there;
					curBlock->predBlocks.push_back(predBlock);
					curStart=it->first;
				}
			}
		}

		while(curBlock==NULL)
		{
			//If there is no currently active block search for the next pending one

			if(pendingBlocks.empty())
				break;
			//It's possible that a pending block gets done while translating
			//the previous one, so check if the block has been already done
			auto it=pendingBlocks.begin();
			uint32_t pendingStart=*it;
			pendingBlocks.erase(it);
			auto blockIt=basicBlocks.find(pendingStart);
			assert(blockIt!=basicBlocks.end());
			if(blockIt->second.realStart==(0xffffffff))
			{
				//The block has not been traslated yet
				curStart=pendingStart;
				curBlock=&blockIt->second;
				curBlock->realStart=there;
				code.seekg(curStart);
				here=curStart;
			}
		}
		if(curBlock==NULL)
			break;

		bool success=instructionsMap.insert(make_pair(here,there)).second;
		if(success==false)
		{
			//This instruction has been already added
			//it means that we discovered that a block starts here after the previous
			//block has been already translated by fallthrough
			//Solve the problem by invalidating the old block and adding it to the pending blocks
			auto it=basicBlocks.lower_bound(here);
			assert(it!=basicBlocks.begin());
			it--;
			uint32_t nextBlockStart=it->second.originalEnd;
			uint32_t oldStart=it->first;

			it->second.resetCode();

			pendingBlocks.insert(oldStart);
			//Delete all instructions in the deleted block from the instructionsMap
			auto instructionsMapStart=instructionsMap.lower_bound(oldStart);
			auto instructionsMapEnd=instructionsMap.lower_bound(nextBlockStart);
			instructionsMap.erase(instructionsMapStart,instructionsMapEnd);
		}

		code >> opcode;
		if(code.eof())
		{
			//As the code has ended we must be in no block
			if(curBlock==NULL)
				break;
			else
				throw ParseException("End of code in optimizer");
		}
		assert(curBlock);

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
				curBlock->realEnd=out.tellp();
				curBlock->originalEnd=code.tellg();
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
				BasicBlock* predBlock=curBlock;
				predBlock->realEnd=there;
				int here=code.tellg();
				predBlock->originalEnd=here;
				curStart=here-1;
				curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(predBlock))).first->second);
				curBlock->realStart=there;
				curBlock->predBlocks.push_back(predBlock);
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
				
				BasicBlock* const predBlock=curBlock;
				uint32_t oldStart=curStart;
				//The new block starts after this function
				int here=code.tellg();
				verifyBranch(pendingBlocks,basicBlocks,oldStart,here,t,code_len);
				out << (uint8_t)opcode;
				writeBranchAddress(basicBlocks, here, t, out);
				predBlock->realEnd=out.tellp();
				predBlock->originalEnd=here;

				auto it=basicBlocks.find(curStart);
				if(it==basicBlocks.end())
				{
					curStart=here;
					curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(predBlock))).first->second);
					curBlock->predBlocks.push_back(predBlock);
					curBlock->realStart=out.tellp();
				}
				//If the fall through block alredy exists, just go on
				//A jump will be added when the fallthrough is detected
				break;
			}
			case 0x10:
			{
				//jump
				s24 t;
				code >> t;

				//The new block starts after this function
				int here=code.tellg();
				verifyBranch(pendingBlocks,basicBlocks,curStart,here,t,code_len);
				out << (uint8_t)opcode;
				writeBranchAddress(basicBlocks, here, t, out);
				//Reset the block to NULL
				curBlock->realEnd=out.tellp();
				curBlock->originalEnd=here;
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

				BasicBlock* const predBlock=curBlock;
				predBlock->realEnd=there;
				uint32_t oldStart=curStart;
				//The new block starts after this function
				int here=code.tellg();
				verifyBranch(pendingBlocks,basicBlocks,oldStart,here,t,code_len);
				out << (uint8_t)opcode;
				writeBranchAddress(basicBlocks, here, t, out);
				predBlock->realEnd=out.tellp();
				predBlock->originalEnd=here;

				auto it=basicBlocks.find(curStart);
				if(it==basicBlocks.end())
				{
					curStart=here;
					curBlock=&(basicBlocks.insert(make_pair(curStart,BasicBlock(predBlock))).first->second);
					curBlock->predBlocks.push_back(predBlock);
					curBlock->realStart=out.tellp();
				}
				//If the fall through block alredy exists, just go on
				//A jump will be added when the fallthrough is detected
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
				//Verify default branch and output it's translation
				verifyBranch(pendingBlocks,basicBlocks,curStart,here,t,code_len);
				out << (uint8_t)opcode;
				writeBranchAddress(basicBlocks, here, t, out);
				//Verify other branches and output their translations
				for(unsigned i=0;i<(count+1);i++)
					verifyBranch(pendingBlocks,basicBlocks,curStart,here,offsets[i],code_len);
				writeInt32(out, count);
				for(unsigned i=0;i<(count+1);i++)
					writeBranchAddress(basicBlocks,here, (int)offsets[i], out);
				curBlock->realEnd=out.tellp();
				curBlock->originalEnd=code.tellg();
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
			case 0x1f:
			{
				//hasnext
				curBlock->popStack(2);
				curBlock->pushStack(Class<Integer>::getClass(sys));
				out << (uint8_t)opcode;
				break;
			}
			case 0x20:
			{
				//pushnull
				ASObject* ret=sys->getNullRef();
				//We don't really need a reference to it
				ret->decRef();
				curBlock->pushStack(InferenceData(ret));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
				break;
			}
			case 0x26:
			{
				//pushtrue
				out << (uint8_t)opcode;
				curBlock->pushStack(Class<Boolean>::getClass(sys));
				break;
			}
			case 0x27:
			{
				//pushfalse
				out << (uint8_t)opcode;
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				InferenceData data=curBlock->peekStack();
				curBlock->popStack(1);
				curBlock->pushStack(data);
				curBlock->pushStack(data);
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
				curBlock->pushStack(Class<Number>::getClass(sys));
				break;
			}
			case 0x30:
			{
				//pushscope
				InferenceData t=curBlock->peekStack();
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
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
			case 0x43:
			{
				//callmethod
				u30 t,t2;
				code >> t;
				code >> t2;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				writeInt32(out,t2);
				curBlock->popStack(t2+1);
				curBlock->pushStack(Type::anyType);
				break;
			}
			case 0x4a:
			{
				//constructprop
				u30 t,t2;
				code >> t;
				code >> t2;
				out << (uint8_t)opcode;
				writeInt32(out,t);
				writeInt32(out,t2);

				int numRT=mi->context->getMultinameRTData(t);
				curBlock->popStack(numRT+t2+1);
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
				curBlock->popStack(numRT+t2);
				InferenceData baseData=curBlock->peekStack();
				//Try to infer the return type
				InferenceData inferredData;
				if(baseData.isValid() && numRT==0)
				{
					const multiname* name=mi->context->getMultiname(t,NULL);
					if(baseData.type)
					{
						const Class_base* objType=dynamic_cast<const Class_base*>(baseData.type);
						if(objType)
						{
							const variable* var=objType->findBorrowedGettable(*name);
							if(var && asAtomHandler::isFunction(var->var))
							{
								SyntheticFunction* calledFunc=dynamic_cast<SyntheticFunction*>(asAtomHandler::getObject(var->var));
								if(calledFunc)
									inferredData.type=calledFunc->mi->returnType;
							}
						}
					}
				}
				//The object is the last thing in the stack
				curBlock->popStack(1);
				curBlock->pushStack(inferredData);
				break;
			}
			case 0x47:
			{
				//returnvoid
				out << (uint8_t)opcode;
				curBlock->realEnd=out.tellp();
				curBlock->originalEnd=code.tellg();
				curBlock=NULL;
				break;
			}
			case 0x48:
			{
				//returnvalue
				out << (uint8_t)opcode;
				curBlock->popStack(1);
				curBlock->realEnd=out.tellp();
				curBlock->originalEnd=code.tellg();
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
			case 0x4e:
			case 0x4f:
			{
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
				curBlock->pushStack(&activationType);
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

				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				InferenceData inferredData;
				if(numRT==0 && function->isMethod())
				{
					//Attempt early binding
					const multiname* name=mi->context->getMultiname(t,NULL);
					inferredData=earlyBindFindPropStrict(out, function, curBlock->scopeStackTypes, name);
				}

				curBlock->popStack(numRT);
				if(!inferredData.isValid())
				{
					out << (uint8_t)opcode;
					writeInt32(out,t);
				}
				curBlock->pushStack(inferredData);
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
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");
				const multiname* name=mi->context->getMultiname(t,NULL);

				InferenceData inferredData;
				//Only methods can be early binded, anonymous functions do
				//not have a fixed function scope stack
				if(function->isMethod())
					inferredData=earlyBindGetLex(out, function, curBlock->scopeStackTypes, name, t);
				if(!inferredData.isValid())
				{
					//Early binding failed, use normal translation
					out << (uint8_t)opcode;
					writeInt32(out,t);
				}

				curBlock->pushStack(inferredData);
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

				Type* t=getLocalType(function, i);
				curBlock->pushStack(t);
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
				//NOTE: getscopeobject only resolves the local part of the scope stack
				//not the one inherited
				curBlock->pushStack(curBlock->scopeStackTypes[t]);
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
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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

				InferenceData valueData = curBlock->peekStack();
				curBlock->popStack(1);
				InferenceData objData = curBlock->peekStack();
				curBlock->popStack(1);
				if(objData.type)
				{
					const multiname* slotType = objData.type->resolveSlotTypeName(t);
					if(slotType)
					{
						ASObject* ret=mi->context->root->applicationDomain->getVariableByMultinameOpportunistic(*slotType,mi->context->root->getInstanceWorker());
						if(ret && ret->getObjectType()==T_CLASS)
						{
							Class_base* c=static_cast<Class_base*>(ret);
							if(valueData.isOfType(c))
							{
								//We know the value is already of the right type
								//Let's skip coercion
								out << (uint8_t)SET_SLOT_NO_COERCE;
								writeInt32(out,t);
								break;
							}
						}
					}
				}
				//If we did not manage to optimize the call, translate it directly
				out << (uint8_t)opcode;
				writeInt32(out,t);
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
				curBlock->pushStack(Class<ASString>::getClass(sys));
				break;
			}
			case 0x73:
			{
				//convert_i
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Integer>::getClass(sys));
				break;
			}
			case 0x74:
			{
				//convert_u
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<UInteger>::getClass(sys));
				break;
			}
			case 0x75:
			{
				//convert_d
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Number>::getClass(sys));
				break;
			}
			case 0x76:
			{
				//convert_b
				out << (uint8_t)opcode;

				curBlock->popStack(1);
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				u30 t;
				code >> t;
				int numRT=mi->context->getMultinameRTData(t);
				//No runtime multinames are accepted
				if(numRT)
					throw ParseException("Bad code in optimizer");
				InferenceData baseData=curBlock->peekStack();
				const multiname* name=mi->context->getMultiname(t,NULL);

				Class_base* coerceToClass = NULL;
				InferenceData inferredData;

				//Try to resolve the type is it is already defined
				ASObject* ret=mi->context->root->applicationDomain->getVariableByMultinameOpportunistic(*name,mi->context->root->getInstanceWorker());
				if(ret && ret->getObjectType()==T_CLASS)
				{
					coerceToClass=static_cast<Class_base*>(ret);
					//Also extract the type information for later use if the type has been
					//already resolved
					inferredData.type = coerceToClass;
				}

				if(baseData.isOfType(coerceToClass))
				{
					//We can skip the coercion
					break;
				}
				out << (uint8_t)opcode;
				//Translate coerce to a rewriting opcode
				//The pointer to the multiname will become the pointer to
				//the type after the first execution
				writePtr(out,name);
				curBlock->popStack(1);
				curBlock->pushStack(inferredData);
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
				curBlock->pushStack(Class<Number>::getClass(sys));
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
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
				curBlock->pushStack(Class<Number>::getClass(sys));
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
				curBlock->pushStack(Class<Integer>::getClass(sys));
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
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				curBlock->pushStack(Class<Boolean>::getClass(sys));
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
				//Infer the type of the object when possible
				Type* t=getLocalType(function, opcode-0xd0);
				curBlock->pushStack(t);
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
				LOG(LOG_ERROR,"Not optimizable instruction @" << code.tellg());
				LOG(LOG_ERROR,"dump " << hex << (unsigned int)opcode << dec);
				throw ParseException("Not implemented instruction in optimizer");
		}
	}

	assert(!basicBlocks.empty());

	//The original exception ranges must be translated to one
	//or more exception ranges as the blocks have been reordered
	uint32_t originalExceptionSize=mi->body->exceptions.size();
	for(uint32_t i=0;i<originalExceptionSize;i++)
	{
		exception_info_abc* ei=&mi->body->exceptions[i];
		//Find out where the exception begins
		uint32_t excStart = ei->from;
		assert(instructionsMap.find(ei->target)!=instructionsMap.end());
		assert(instructionsMap.find(ei->from)!=instructionsMap.end());
		ei->target=instructionsMap.find(ei->target)->second;
		ei->from=instructionsMap.find(ei->from)->second;
		//Find out what block it starts into
		auto it=(--basicBlocks.upper_bound(excStart));

		uint32_t lastRealEnd = it->second.realEnd;

		it++;

		//Find out if the reordered blocks are linear,
		//if not duplicate exception entry
		uint32_t originalEnd = ei->to;
		while(it!=basicBlocks.end() && it->first <= originalEnd)
		{
			if(it->second.realStart != lastRealEnd)
			{
				//Duplicate the exception
				ei->to = lastRealEnd;
				mi->body->exceptions.push_back(*ei);
				//Careful! ei is invalidated by now!
				ei=&mi->body->exceptions.back();
				ei->from = it->second.realStart;
			}
			lastRealEnd = it->second.realEnd;
			++it;
		}

		auto instructionIterator=instructionsMap.lower_bound(originalEnd);
		if(instructionIterator==instructionsMap.end() || instructionIterator->first!=originalEnd)
		{
			//If the to instruction has not been traslated, limit the range to the previous one
			--instructionIterator;
		}
		ei->to = instructionIterator->second;
		assert(ei->to >= ei->from);
	}

	//Loop over the basic blocks to do
	//1) branch fixups
	//3) consistency checks
	for(auto it=basicBlocks.begin();it!=basicBlocks.end();++it)
	{
		//Fixups
		const BasicBlock& bb=it->second;
		assert(bb.realStart!=0xffffffff);
		assert(bb.realEnd!=0xffffffff);
		assert(bb.originalEnd!=0xffffffff);
		for(uint32_t i=0;i<bb.fixups.size();i++)
		{
			uint32_t strOffset=bb.fixups[i];
			out.seekp(strOffset);
			writeInt32(out, bb.realStart);
		}
		if(bb.predBlocks.size()==0)
		{
			//It may be the starting block or an exception handling block
			continue;
		}
		const vector<InferenceData>& predStackTypes=bb.predBlocks[0]->stackTypes;
		const vector<InferenceData>& predScopeStackTypes=bb.predBlocks[0]->scopeStackTypes;
		for(uint32_t i=0;i<bb.predBlocks.size();i++)
		{
			//TODO: should check
			//until then, silence warning about unused variables:
			(void) predStackTypes;
			(void) predScopeStackTypes;
		}
	}
	//Overwrite the old code
	mi->body->code=out.str();
	mi->body->codeStatus = method_body_info::OPTIMIZED;
}
