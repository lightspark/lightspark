/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Constants.h> 
#include <llvm/Support/IRBuilder.h> 
#include <llvm/Target/TargetData.h>
#include <sstream>
#include "abc.h"
#include "swftypes.h"

void debug(int* i)
{
	LOG(CALLS, "debug "<< *i);
}

opcode_handler ABCVm::opcode_table_args0_lazy[]={
	{"pushNaN",(void*)&ABCVm::pushNaN},
	{"coerce_a",(void*)&ABCVm::coerce_a},
	{"pushNull",(void*)&ABCVm::pushNull},
	{"pushTrue",(void*)&ABCVm::pushTrue},
	{"pushFalse",(void*)&ABCVm::pushFalse},
	{"pushUndefined",(void*)&ABCVm::pushUndefined},
	{"getGlobalScope",(void*)&ABCVm::getGlobalScope}
};

opcode_handler ABCVm::opcode_table_args0[]={
	{"pushScope",(void*)&ABCVm::pushScope},
	{"pushWith",(void*)&ABCVm::pushWith},
	{"throw",(void*)&ABCVm::_throw},
	{"pop",(void*)&ABCVm::pop},
	{"dup",(void*)&ABCVm::dup},
	{"swap",(void*)&ABCVm::swap},
	{"popScope",(void*)&ABCVm::popScope},
	{"isTypelate",(void*)&ABCVm::isTypelate},
	{"asTypelate",(void*)&ABCVm::asTypelate},
};

opcode_handler ABCVm::opcode_table_args1_lazy[]={
	{"pushString",(void*)&ABCVm::pushString},
	{"pushDouble",(void*)&ABCVm::pushDouble},
	{"pushInt",(void*)&ABCVm::pushInt},
	{"newFunction",(void*)&ABCVm::newFunction},
	{"newCatch",(void*)&ABCVm::newCatch},
	{"pushByte",(void*)&ABCVm::pushByte},
	{"getScopeObject",(void*)&ABCVm::getScopeObject}
};

opcode_handler ABCVm::opcode_table_args1_branches[]={
	{"ifTrue",(void*)&ABCVm::ifTrue},
	{"ifFalse",(void*)&ABCVm::ifFalse},
};

opcode_handler ABCVm::opcode_table_args2_branches[]={
	{"ifLT",(void*)&ABCVm::ifLT},
	{"ifNLT",(void*)&ABCVm::ifNLT},
	{"ifNGT",(void*)&ABCVm::ifNGT},
	{"ifGT",(void*)&ABCVm::ifGT},
	{"ifNGE",(void*)&ABCVm::ifNGE},
	{"ifNLE",(void*)&ABCVm::ifNLE},
	{"ifGE",(void*)&ABCVm::ifGE},
	{"ifNE",(void*)&ABCVm::ifNE},
	{"ifStrictNE",(void*)&ABCVm::ifStrictNE},
	{"ifStrictEq",(void*)&ABCVm::ifStrictEq},
	{"ifEq",(void*)&ABCVm::ifEq},
};

opcode_handler ABCVm::opcode_table_args1[]={
	{"getSuper",(void*)&ABCVm::getSuper},
	{"setSuper",(void*)&ABCVm::setSuper},
	{"newObject",(void*)&ABCVm::newObject},
	{"deleteProperty",(void*)&ABCVm::deleteProperty},
	{"jump",(void*)&ABCVm::jump},
	{"getLocal",(void*)&ABCVm::getLocal},
	{"setLocal",(void*)&ABCVm::setLocal},
	{"call",(void*)&ABCVm::call},
	{"coerce",(void*)&ABCVm::coerce},
	{"getLex",(void*)&ABCVm::getLex},
	{"findPropStrict",(void*)&ABCVm::findPropStrict},
	{"incLocal_i",(void*)&ABCVm::incLocal_i},
	{"getProperty",(void*)&ABCVm::getProperty},
	{"findProperty",(void*)&ABCVm::findProperty},
	{"construct",(void*)&ABCVm::construct},
	{"constructSuper",(void*)&ABCVm::constructSuper},
	{"newArray",(void*)&ABCVm::newArray},
	{"newClass",(void*)&ABCVm::newClass},
	{"initProperty",(void*)&ABCVm::initProperty},
	{"kill",(void*)&ABCVm::kill}
};

opcode_handler ABCVm::opcode_table_args1_pointers_int[]={
	{"getSlot",(void*)&ABCVm::getSlot}
};

opcode_handler ABCVm::opcode_table_args1_pointers[]={
	{"convert_d",(void*)&ABCVm::convert_d},
	{"convert_b",(void*)&ABCVm::convert_b},
	{"convert_i",(void*)&ABCVm::convert_i},
	{"coerce_s",(void*)&ABCVm::coerce_s},
	{"decrement",(void*)&ABCVm::decrement},
	{"decrement_i",(void*)&ABCVm::decrement_i},
	{"increment",(void*)&ABCVm::increment},
	{"increment_i",(void*)&ABCVm::increment_i},
	{"negate",(void*)&ABCVm::negate},
	{"not",(void*)&ABCVm::_not},
	{"typeOf",(void*)ABCVm::typeOf}
};

opcode_handler ABCVm::opcode_table_args2_pointers_int[]={
	{"setSlot",(void*)&ABCVm::setSlot},
	{"getMultiname",(void*)&ABCContext::s_getMultiname}
};

opcode_handler ABCVm::opcode_table_args3_pointers[]={
	{"setProperty",(void*)&ABCVm::setProperty},
};

opcode_handler ABCVm::opcode_table_args2_pointers[]={
	{"nextName",(void*)&ABCVm::nextName},
	{"nextValue",(void*)&ABCVm::nextValue},
	{"subtract",(void*)&ABCVm::subtract},
	{"divide",(void*)&ABCVm::divide},
	{"modulo",(void*)&ABCVm::modulo},
	{"multiply",(void*)&ABCVm::multiply},
	{"add",(void*)&ABCVm::add},
	{"bitOr",(void*)&ABCVm::bitOr},
	{"bitXor",(void*)&ABCVm::bitXor},
	{"urShift",(void*)&ABCVm::urShift},
	{"lShift",(void*)&ABCVm::lShift},
	{"lessThan",(void*)&ABCVm::lessThan},
	{"greaterThan",(void*)&ABCVm::greaterThan},
	{"strictEquals",(void*)&ABCVm::strictEquals},
	{"in",(void*)&ABCVm::in},
	{"equals",(void*)&ABCVm::equals}
};

typed_opcode_handler ABCVm::opcode_table_uintptr_t[]={
	{"bitAnd_oo",(void*)&ABCVm::bitAnd,ARGS_OBJ_OBJ},
	{"bitAnd_oi",(void*)&ABCVm::bitAnd_oi,ARGS_OBJ_INT},
	{"pushShort",(void*)&ABCVm::pushShort,ARGS_INT}
};

extern __thread SystemState* sys;
extern __thread ParseThread* pt;

using namespace std;

//Be careful, arguments nubering starts from 1
ISWFObject* argumentDumper(arguments* arg, uint32_t n)
{
	//Really implement default values, we now fill with Undefined
	if(n-1<arg->size())
	{
		arg->at(n-1)->incRef();
		return arg->at(n-1);
	}
	else
		return new Undefined;
}

ISWFObject* createRest()
{
	ASArray* ret=new ASArray();
	ret->_constructor(ret,NULL);
	return ret;
}

void ABCVm::registerFunctions()
{
	vector<const llvm::Type*> sig;
	llvm::FunctionType* FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"abort",module);

	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();
	const llvm::Type* int_type=ex->getTargetData()->getIntPtrType();
	const llvm::Type* voidptr_type=llvm::PointerType::getUnqual(ptr_type);

	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	llvm::Function* F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"not_impl",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::not_impl);
	sig.clear();

	sig.push_back(llvm::PointerType::getUnqual(llvm::IntegerType::get(32)));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"debug",module);
	ex->addGlobalMapping(F,(void*)debug);
	sig.clear();

	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"createRest",module);
	ex->addGlobalMapping(F,(void*)createRest);
	sig.clear();

	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"argumentDumper",module);
	ex->addGlobalMapping(F,(void*)&argumentDumper);
	sig.clear();

	//All the opcodes needs a pointer to the context
	std::vector<const llvm::Type*> struct_elems;
	struct_elems.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));
	struct_elems.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));
	struct_elems.push_back(llvm::IntegerType::get(32));
	llvm::Type* context_type=llvm::PointerType::getUnqual(llvm::StructType::get(struct_elems,true));

	//newActivation needs both method_info and the context
	sig.push_back(context_type);
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newActivation",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newActivation);
	sig.clear();

	// (method_info*)
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(context_type, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"decRef",module);
	ex->addGlobalMapping(F,(void*)&ISWFObject::s_decRef);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"incRef",module);
	ex->addGlobalMapping(F,(void*)&ISWFObject::s_incRef);

	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(context_type, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"decRef_safe",module);
	ex->addGlobalMapping(F,(void*)&ISWFObject::s_decRef_safe);
	sig.clear();

	// (call_context*)
	sig.push_back(context_type);
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);

	int elems=sizeof(opcode_table_args0)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args0[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args0[i].addr);
	}
	//Lazy pushing
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args0_lazy)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args0_lazy[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args0_lazy[i].addr);
	}
	//End of lazy pushing

	// (call_context*,int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	elems=sizeof(opcode_table_args1)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args1[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args1[i].addr);
	}
	//Lazy pushing
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args1_lazy)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args1_lazy[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args1_lazy[i].addr);
	}
	//End of lazy pushing

	// (call_context*,int,int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callPropVoid",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callPropVoid);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callSuper",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callSuper);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callSuperVoid",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callSuperVoid);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"constructProp",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::constructProp);


	//Lazy pushing
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"hasNext2",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::hasNext2);
	//End of lazy pushing
	
	//Lazy pushing, no context, (ISWFObject*)
	sig.clear();
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args1_pointers)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args1_pointers[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args1_pointers[i].addr);
	}
	//End of lazy pushing

	//Lazy pushing, no context, (ISWFObject*, int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args1_pointers_int)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args1_pointers_int[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args1_pointers_int[i].addr);
	}
	//End of lazy pushing

	//Branches, no context, (ISWFObject*, int)
	FT=llvm::FunctionType::get(llvm::IntegerType::get(1), sig, false);
	elems=sizeof(opcode_table_args1_branches)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args1_branches[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args1_branches[i].addr);
	}
	//End of lazy pushing

	//Lazy pushing, no context, (ISWFObject*, ISWFObject*)
	sig.clear();
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args2_pointers)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args2_pointers[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args2_pointers[i].addr);
	}
	//End of lazy pushing

	//Lazy pushing, no context, (ISWFObject*, ISWFObject*, int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args2_pointers_int)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args2_pointers_int[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args2_pointers_int[i].addr);
	}
	//End of lazy pushing
	
	//Branches, no context, (ISWFObject*, ISWFObject*, int)
	FT=llvm::FunctionType::get(llvm::IntegerType::get(1), sig, false);
	elems=sizeof(opcode_table_args2_branches)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args2_branches[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args2_branches[i].addr);
	}
	//End of lazy pushing

	//Lazy pushing, no context, (ISWFObject*, ISWFObject*, void*)
	sig.clear();
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	elems=sizeof(opcode_table_args3_pointers)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args3_pointers[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args3_pointers[i].addr);
	}

	//Build the concrete interface
	sig[0]=int_type;
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setProperty_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setProperty_i);

	vector<const llvm::Type*> sig_obj_obj;
	sig_obj_obj.push_back(voidptr_type);
	sig_obj_obj.push_back(voidptr_type);

	vector<const llvm::Type*> sig_obj_int;
	sig_obj_int.push_back(voidptr_type);
	sig_obj_int.push_back(int_type);

	vector<const llvm::Type*> sig_int;
	sig_int.push_back(int_type);

	//Build abstracter
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig_int, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"abstract_i",module);
	ex->addGlobalMapping(F,(void*)&abstract_i);

	//Lazy pushing, no context, concrete intptr_t
	elems=sizeof(opcode_table_uintptr_t)/sizeof(typed_opcode_handler);
	for(int i=0;i<elems;i++)
	{
		if(opcode_table_uintptr_t[i].type==ARGS_OBJ_OBJ)
			FT=llvm::FunctionType::get(int_type, sig_obj_obj, false);
		else if(opcode_table_uintptr_t[i].type==ARGS_OBJ_INT)
			FT=llvm::FunctionType::get(int_type, sig_obj_int, false);
		else if(opcode_table_uintptr_t[i].type==ARGS_INT)
			FT=llvm::FunctionType::get(int_type, sig_int, false);
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_uintptr_t[i].name,module);
		ex->addGlobalMapping(F,opcode_table_uintptr_t[i].addr);
	}
	//End of lazy pushing

}

llvm::Value* method_info::llvm_stack_pop(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	//decrement stack index
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);

	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

llvm::Value* method_info::llvm_stack_peek(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

void method_info::llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val,
		llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index);
	builder.CreateStore(val,dest);

	//increment stack index
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateAdd(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);
}

inline method_info::stack_entry method_info::static_stack_pop(llvm::IRBuilder<>& builder, vector<method_info::stack_entry>& static_stack, 
		llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	//try to get the tail value from the static stack
	if(!static_stack.empty())
	{
		stack_entry ret=static_stack.back();
		static_stack.pop_back();
		return ret;
	}
	//try to pop the tail value of the dynamic stack
	return stack_entry(llvm_stack_pop(builder,dynamic_stack,dynamic_stack_index),STACK_OBJECT);
}

inline method_info::stack_entry method_info::static_stack_peek(llvm::IRBuilder<>& builder, vector<method_info::stack_entry>& static_stack, 
		llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	//try to get the tail value from the static stack
	if(!static_stack.empty())
		return static_stack.back();
	//try to load the tail value of the dynamic stack
	return stack_entry(llvm_stack_peek(builder,dynamic_stack,dynamic_stack_index),STACK_OBJECT);
}

inline void method_info::static_stack_push(vector<stack_entry>& static_stack, const stack_entry& e)
{
	static_stack.push_back(e);
}

inline void method_info::syncStacks(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& builder, bool jitted,
		vector<stack_entry>& static_stack,llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	if(jitted)
	{
		for(int i=0;i<static_stack.size();i++)
		{
			if(static_stack[i].second==STACK_OBJECT)
			{
			}
			else if(static_stack[i].second==STACK_INT)
				static_stack[i].first=builder.CreateCall(ex->FindFunctionNamed("abstract_i"),static_stack[i].first);
			llvm_stack_push(ex,builder,static_stack[i].first,dynamic_stack,dynamic_stack_index);
		}
		static_stack.clear();
	}
}

inline void method_info::syncLocals(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& builder,
		vector<stack_entry>& static_locals,llvm::Value* locals)
{
	for(int i=0;i<static_locals.size();i++)
	{
		if(static_locals[i].second==STACK_NONE)
			continue;

		llvm::Value* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
		llvm::Value* t=builder.CreateGEP(locals,constant);
		llvm::Value* old=builder.CreateLoad(t);
		if(static_locals[i].second==STACK_OBJECT)
		{
			//decRef the previous contents, if different from new
			builder.CreateCall2(ex->FindFunctionNamed("decRef_safe"), old, static_locals[i].first);
			builder.CreateStore(static_locals[i].first,t);
		}
		else
			abort();

		static_locals[i].second=STACK_NONE;
	}
}

llvm::FunctionType* method_info::synt_method_prototype(llvm::ExecutionEngine* ex)
{
	//whatever pointer is good
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();

	std::vector<const llvm::Type*> struct_elems;
	struct_elems.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));
	struct_elems.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));
	struct_elems.push_back(llvm::IntegerType::get(32));
	llvm::Type* context_type=llvm::PointerType::getUnqual(llvm::StructType::get(struct_elems,true));

	//Initialize LLVM representation of method
	vector<const llvm::Type*> sig;
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(context_type);

	return llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
}

SyntheticFunction::synt_function method_info::synt_method()
{
	if(f)
		return f;

	string n="method";
	n+=context->getString(name);
	if(!body)
	{
		LOG(CALLS,"Method " << n << " should be intrinsic");;
		return NULL;
	}
	stringstream code(body->code);
	llvm::ExecutionEngine* ex=context->vm->ex;
	llvm::FunctionType* method_type=synt_method_prototype(ex);
	llvmf=llvm::Function::Create(method_type,llvm::Function::ExternalLinkage,n,context->vm->module);

	//The pointer size compatible int type will be useful
	//TODO: void*
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();
	
	llvm::BasicBlock *BB = llvm::BasicBlock::Create("entry", llvmf);
	llvm::IRBuilder<> Builder;
	Builder.SetInsertPoint(BB);

	//We define a couple of variables that will be used a lot
	llvm::Constant* constant;
	llvm::Constant* constant2;
	llvm::Value* value;
	//let's give access to method data to llvm
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)this);
	llvm::Value* th = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(ptr_type));

	//the current execution context is allocated here
	llvm::Function::ArgumentListType::iterator it=llvmf->getArgumentList().begin();
	it++;
	it++;
	llvm::Value* context=it;

	//let's give access to local data storage
	value=Builder.CreateStructGEP(context,0);
	llvm::Value* locals=Builder.CreateLoad(value);

	//the stack is statically handled as much as possible to allow llvm optimizations
	//on branch and on interpreted/jitted code transition it is synchronized with the dynamic one
	vector<stack_entry> static_stack;
	static_stack.reserve(body->max_stack);
	//Get the pointer to the dynamic stack
	value=Builder.CreateStructGEP(context,1);
	llvm::Value* dynamic_stack=Builder.CreateLoad(value);
	//Get the index of the dynamic stack
	llvm::Value* dynamic_stack_index=Builder.CreateStructGEP(context,2);

	//the scope stack is not accessible to llvm code
	
	//Creating a mapping between blocks and starting address
	//The current header block is ended
	llvm::BasicBlock *StartBB = llvm::BasicBlock::Create("entry", llvmf);
	Builder.CreateBr(StartBB);
	//CHECK: maybe not needed
	Builder.SetInsertPoint(StartBB);
	map<int,llvm::BasicBlock*> blocks;
	blocks.insert(pair<int,llvm::BasicBlock*>(0,StartBB));

	bool jitted=false;

	//This is initialized to true so that on first iteration the entry block is used
	bool last_is_branch=true;

	//We fill locals with function arguments
	//First argument is the 'this'
	constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 0);
	llvm::Value* t=Builder.CreateGEP(locals,constant);
	it=llvmf->getArgumentList().begin();
	llvm::Value* arg=it;
	Builder.CreateStore(arg,t);
	//Second argument is the arguments pointer
	it++;
	for(int i=0;i<param_count;i++)
	{
		constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i+1);
		t=Builder.CreateGEP(locals,constant);
		arg=Builder.CreateCall2(ex->FindFunctionNamed("argumentDumper"), it, constant);
		Builder.CreateStore(arg,t);
	}
	if(flags&0x01) //NEED_ARGUMENTS
	{
		constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), param_count+1);
		t=Builder.CreateGEP(locals,constant);
		Builder.CreateStore(it,t);
	}
	if(flags&0x04) //TODO: NEED_REST
	{
		llvm::Value* rest=Builder.CreateCall(ex->FindFunctionNamed("createRest"));
		constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), param_count+1);
		t=Builder.CreateGEP(locals,constant);
		Builder.CreateStore(rest,t);
	}

	vector<stack_entry> static_locals;
	static_locals.resize(body->local_count,stack_entry(NULL,STACK_NONE));

	//Each case block builds the correct parameters for the interpreter function and call it
	u8 opcode;
	while(1)
	{
		code >> opcode;
		if(code.eof())
			break;
		//Check if we are expecting a new block start
		map<int,llvm::BasicBlock*>::iterator it=blocks.find(int(code.tellg())-1);
		if(it!=blocks.end())
		{
			//A new block starts, the last instruction should have been a branch?
			if(!last_is_branch)
			{
				LOG(TRACE, "Last instruction before a new block was not a branch.");
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateBr(it->second);
			}
			LOG(TRACE,"Starting block at "<<int(code.tellg())-1);
			Builder.SetInsertPoint(it->second);
			last_is_branch=false;
		}
		else if(last_is_branch)
		{
			//TODO: Check this. It seems that there may be invalid code after
			//block end
			LOG(TRACE,"Ignoring at " << int(code.tellg())-1);
			continue;
			/*LOG(CALLS,"Inserting block at "<<int(code.tellg())-1);
			abort();
			llvm::BasicBlock* A=llvm::BasicBlock::Create("fall", m->f);
			Builder.CreateBr(A);
			Builder.SetInsertPoint(A);*/
		}

		switch(opcode)
		{
			case 0x03:
			{
				//throw
				LOG(TRACE, "synt throw" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("throw"), context);
				Builder.CreateRetVoid();
				break;
			}
			case 0x04:
			{
				//getsuper
				LOG(TRACE, "synt getsuper" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getSuper"), context, constant);
				break;
			}
			case 0x05:
			{
				//setsuper
				LOG(TRACE, "synt setsuper" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("setSuper"), context, constant);
				break;
			}
			case 0x08:
			{
				//kill
				LOG(TRACE, "synt kill" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("kill"), context, constant);
				break;
			}
			case 0x09:
			{
				//label
				//Create a new block and insert it in the mapping
				LOG(TRACE, "synt label" );
				last_is_branch=true;
				llvm::BasicBlock* A;
				int here=code.tellg();
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(here);
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(here,A));
				}
				Builder.CreateBr(A);
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0c:
			{
				//ifnlt
				LOG(TRACE, "synt ifnlt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
				else
					abort();
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);

				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifNLT"), v1.first, v2.first, constant);
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0d:
			{
				//ifnle
				LOG(TRACE, "synt ifnle");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
				else
					abort();
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);

				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifNLE"), v1.first, v2.first, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0e:
			{
				//ifngt
				LOG(TRACE, "synt ifngt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifNGT"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0f:
			{
				//ifnge
				LOG(TRACE, "synt ifnge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifNGE"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x10:
			{
				//jump
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				last_is_branch=true;

				s24 t;
				code >> t;
				LOG(TRACE, "synt jump " << t );
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("jump"), context, constant);

				//Create a block for the fallthrough code and insert it in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}
				//Create a block for the landing code and insert it in the mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("jump_land", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}

				Builder.CreateBr(B);
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x11:
			{
				//iftrue
				LOG(TRACE, "synt iftrue" );
				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifTrue"), v1, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x12:
			{
				//iffalse

				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(TRACE, "synt iffalse " << t );

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifFalse"), v1, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x13:
			{
				//ifeq
				LOG(TRACE, "synt ifeq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifEq"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x14:
			{
				//ifne
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(TRACE, "synt ifne " << t );
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
				else
					abort();
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifNE"), v1.first, v2.first, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x15:
			{
				//iflt
				LOG(TRACE, "synt iflt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifLT"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x17:
			{
				//ifgt
				LOG(TRACE, "synt ifgt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifGT"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x18:
			{
				//ifge
				LOG(TRACE, "synt ifge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifGE"), v1, v2, constant);
			
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x19:
			{
				//ifstricteq
				LOG(TRACE, "synt ifstricteq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifStrictEq"), v1, v2, constant);

				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				LOG(TRACE, "synt ifstrictne" );
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", llvmf);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* cond=Builder.CreateCall3(ex->FindFunctionNamed("ifStrictNE"), v1, v2, constant);

				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				Builder.CreateCondBr(cond,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				LOG(TRACE, "synt lookupswitch" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals);
				jitted=false;
				s24 t;
				code >> t;
				LOG(TRACE,"default " << int(t));
				u30 count;
				code >> count;
				LOG(TRACE,"count "<< int(count));
				for(int i=0;i<count+1;i++)
					code >> t;
				Builder.CreateCall(ex->FindFunctionNamed("abort"));

				break;
			}
			case 0x1c:
			{
				//pushwith
				LOG(TRACE, "synt pushwith" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushWith"), context);
				break;
			}
			case 0x1d:
			{
				//popscope
				LOG(TRACE, "synt popscope" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("popScope"), context);
				break;
			}
			case 0x1e:
			{
				//nextname
				LOG(TRACE, "synt nextname" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextName"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x20:
			{
				//pushnull
				LOG(TRACE, "synt pushnull" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushNull"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x21:
			{
				//pushundefined
				LOG(TRACE, "synt pushundefined" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushUndefined"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x23:
			{
				//nextvalue
				LOG(TRACE, "synt nextvalue" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextValue"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x24:
			{
				//pushbyte
				LOG(TRACE, "synt pushbyte" );
				uint8_t t;
				code.read((char*)&t,1);
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushByte"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x25:
			{
				//pushshort
				LOG(TRACE, "synt pushshort" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(ptr_type, t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()==TRACE)
					Builder.CreateCall(ex->FindFunctionNamed("pushShort"), constant);
				jitted=true;
				break;
			}
			case 0x26:
			{
				//pushtrue
				LOG(TRACE, "synt pushtrue" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushTrue"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x27:
			{
				//pushfalse
				LOG(TRACE, "synt pushfalse" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushFalse"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x28:
			{
				//pushnan
				LOG(TRACE, "synt pushnan" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushNaN"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x29:
			{
				//pop
				LOG(TRACE, "synt pop" );
				Builder.CreateCall(ex->FindFunctionNamed("pop"), context);
				stack_entry e=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("decRef"), e.first);
				jitted=true;
				break;
			}
			case 0x2a:
			{
				//dup
				LOG(TRACE, "synt dup" );
				Builder.CreateCall(ex->FindFunctionNamed("dup"), context);
				stack_entry e=
					static_stack_peek(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e);

				if(e.second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), e.first);
				jitted=true;
				break;
			}
			case 0x2b:
			{
				//swap
				LOG(TRACE, "synt swap" );
				Builder.CreateCall(ex->FindFunctionNamed("swap"), context);
				stack_entry e1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry e2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e1);
				static_stack_push(static_stack,e2);
				jitted=true;
				break;
			}
			case 0x2c:
			{
				//pushstring
				LOG(TRACE, "synt pushstring" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushString"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x2d:
			{
				//pushint
				LOG(TRACE, "synt pushint" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushInt"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x2f:
			{
				//pushdouble
				LOG(TRACE, "synt pushdouble" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushDouble"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x30:
			{
				//pushscope
				LOG(TRACE, "synt pushscope" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushScope"), context);
				break;
			}
			case 0x32:
			{
				//hasnext2
				LOG(TRACE, "synt hasnext2" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall(ex->FindFunctionNamed("abort"));
				value=Builder.CreateCall3(ex->FindFunctionNamed("hasNext2"), context, constant, constant2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x40:
			{
				//newfunction
				LOG(TRACE, "synt newfunction" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newFunction"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x41:
			{
				//call
				LOG(TRACE, "synt call" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("call"), context, constant);
				break;
			}
			case 0x42:
			{
				//construct
				LOG(TRACE, "synt construct" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("construct"), context, constant);
				break;
			}
			case 0x45:
			{
				//callsuper
				LOG(TRACE, "synt callsuper" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuper"), context, constant, constant2);
				break;
			}
			case 0x46:
			{
				//callproperty
				//TODO: Implement static resolution where possible
				LOG(TRACE, "synt callproperty" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);

/*				//Pop the stack arguments
				vector<llvm::Value*> args(t+1);
				for(int i=0;i<t;i++)
					args[t-i]=static_stack_pop(Builder,static_stack,m).first;*/
				//Call the function resolver, static case could be resolved at this time (TODO)
				Builder.CreateCall3(ex->FindFunctionNamed("callProperty"), context, constant, constant2);
/*				//Pop the function object, and then the object itself
				llvm::Value* fun=static_stack_pop(Builder,static_stack,m).first;

				llvm::Value* fun2=Builder.CreateBitCast(fun,synt_method_prototype(t));
				args[0]=static_stack_pop(Builder,static_stack,m).first;
				Builder.CreateCall(fun2,args.begin(),args.end());*/

				break;
			}
			case 0x47:
			{
				//returnvoid
				LOG(TRACE, "synt returnvoid" );
				last_is_branch=true;
				constant = llvm::ConstantInt::get(ptr_type, NULL);
				value = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(ptr_type));
				Builder.CreateRet(value);
				for(int i=0;i<static_locals.size();i++)
					static_locals[i].second=STACK_NONE;
				break;
			}
			case 0x48:
			{
				//returnvalue
				//TODO: Should coerce the return type to the expected one
				LOG(TRACE, "synt returnvalue" );
				last_is_branch=true;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),e.first);
				else if(e.second==STACK_OBJECT)
				{
				}
				else
					abort();
				Builder.CreateRet(e.first);
				for(int i=0;i<static_locals.size();i++)
					static_locals[i].second=STACK_NONE;
				break;
			}
			case 0x49:
			{
				//constructsuper
				LOG(TRACE, "synt constructsuper" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructSuper"), context, constant);
				break;
			}
			case 0x4a:
			{
				//constructprop
				LOG(TRACE, "synt constructprop" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("constructProp"), context, constant, constant2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				LOG(TRACE, "synt callsupervoid" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuperVoid"), context, constant, constant2);
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				LOG(TRACE, "synt callpropvoid" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callPropVoid"), context, constant, constant2);
				break;
			}
			case 0x55:
			{
				//newobject
				LOG(TRACE, "synt newobject" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newObject"), context, constant);
				break;
			}
			case 0x56:
			{
				//newarray
				LOG(TRACE, "synt newarray" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newArray"), context, constant);
				break;
			}
			case 0x57:
			{
				//newactivation
				LOG(TRACE, "synt newactivation" );
				value=Builder.CreateCall2(ex->FindFunctionNamed("newActivation"), context, th);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x58:
			{
				//newclass
				LOG(TRACE, "synt newclass" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newClass"), context, constant);
				break;
			}
			case 0x5a:
			{
				//newcatch
				LOG(TRACE, "synt newcatch" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newCatch"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				LOG(TRACE, "synt findpropstrict" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("findPropStrict"), context, constant);
				break;
			}
			case 0x5e:
			{
				//findproperty
				LOG(TRACE, "synt findproperty" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("findProperty"), context, constant);
				break;
			}
			case 0x60:
			{
				//getlex
				LOG(TRACE, "synt getlex" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getLex"), context, constant);
				break;
			}
			case 0x61:
			{
				//setproperty
				LOG(TRACE, "synt setproperty" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				int rtdata=this->context->getMultinameRTData(t);
				stack_entry value=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name;
				//HACK: we need to reinterpret the pointer to the generic type
				llvm::Value* reint_context=Builder.CreateBitCast(context,llvm::PointerType::getUnqual(ptr_type));
				if(rtdata==0)
				{
					//We pass a dummy second context param
					name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, reint_context, constant);
				}
				else if(rtdata==1)
				{
					stack_entry rt1=
						static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

					name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, rt1.first, constant);
				}
				stack_entry obj=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(value.second==STACK_INT)
				{
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty_i"),value.first, obj.first, name);
				}
				else 
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty"),value.first, obj.first, name);
				jitted=true;
				break;
			}
			case 0x62:
			{
				//getlocal
				LOG(TRACE, "synt getlocal" );
				u30 i;
				code >> i;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
				if(Log::getLevel()==TRACE)
					Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), context, constant);

				if(static_locals[i].second==STACK_NONE)
				{
					llvm::Value* t=Builder.CreateGEP(locals,constant);
					t=Builder.CreateLoad(t,"stack");
					static_stack_push(static_stack,stack_entry(t,STACK_OBJECT));
					static_locals[i]=stack_entry(t,STACK_OBJECT);
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
				}
				else if(static_locals[i].second==STACK_OBJECT)
				{
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), static_locals[i].first);
					static_stack_push(static_stack,static_locals[i]);
				}
				else
					abort();

				jitted=true;
				break;
			}
			case 0x63:
			{
				//setlocal
				LOG(TRACE, "synt setlocal" );
				u30 i;
				code >> i;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_locals[i]=e;
				if(Log::getLevel()==TRACE)
				{
					constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
					Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), context, constant);
				}
				jitted=true;
				break;
			}
			case 0x64:
			{
				//getglobalscope
				LOG(TRACE, "synt getglobalscope" );
				value=Builder.CreateCall(ex->FindFunctionNamed("getGlobalScope"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x65:
			{
				//getscopeobject
				LOG(TRACE, "synt getscopeobject" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("getScopeObject"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x66:
			{
				//getproperty
				LOG(TRACE, "synt getproperty" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getProperty"), context, constant);
				break;
			}
			case 0x68:
			{
				//initproperty
				LOG(TRACE, "synt initproperty" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("initProperty"), context, constant);
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				LOG(TRACE, "synt deleteproperty" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("deleteProperty"), context, constant);
				break;
			}
			case 0x6c:
			{
				//getslot
				LOG(TRACE, "synt getslot" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("getSlot"), v1, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x6d:
			{
				//setslot
				LOG(TRACE, "synt setslot" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else
					abort();

				Builder.CreateCall3(ex->FindFunctionNamed("setSlot"), v1.first, v2.first, constant);
				jitted=true;
				break;
			}
			case 0x73:
			{
				//convert_i
				LOG(TRACE, "synt convert_i" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_i"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x75:
			{
				//convert_d
				LOG(TRACE, "synt convert_d" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_d"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x76:
			{
				//convert_b
				LOG(TRACE, "synt convert_b" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_b"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x80:
			{
				//coerce
				LOG(TRACE, "synt coerce" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("coerce"), context, constant);
				break;
			}
			case 0x82:
			{
				//coerce_a
				LOG(TRACE, "synt coerce_a" );
				Builder.CreateCall(ex->FindFunctionNamed("coerce_a"), context);
				stack_entry v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT)
					value=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else
					value=v1.first;
				
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x85:
			{
				//coerce_s
				LOG(TRACE, "synt coerce_s" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("coerce_s"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x87:
			{
				//astypelate
				LOG(TRACE, "synt astypelate" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("asTypelate"), context);
				break;
			}
			case 0x90:
			{
				//negate
				LOG(TRACE, "synt negate" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("negate"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x91:
			{
				//increment
				LOG(TRACE, "synt increment" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("increment"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x93:
			{
				//decrement
				LOG(TRACE, "synt decrement" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("decrement"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x95:
			{
				//typeof
				LOG(TRACE, "synt typeof" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("typeOf"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x96:
			{
				//not
				LOG(TRACE, "synt not" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("not"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa0:
			{
				//add
				LOG(TRACE, "synt add" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("add"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa1:
			{
				//subtract
				LOG(TRACE, "synt subtract" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else
					abort();

				value=Builder.CreateCall2(ex->FindFunctionNamed("subtract"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa2:
			{
				//multiply
				LOG(TRACE, "synt multiply" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
				else
					abort();
				value=Builder.CreateCall2(ex->FindFunctionNamed("multiply"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa3:
			{
				//divide
				LOG(TRACE, "synt divide" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
				else
					abort();

				value=Builder.CreateCall2(ex->FindFunctionNamed("divide"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa4:
			{
				//modulo
				LOG(TRACE, "synt modulo" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else
					abort();
				value=Builder.CreateCall2(ex->FindFunctionNamed("modulo"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa5:
			{
				//lshift
				LOG(TRACE, "synt lshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);

				value=Builder.CreateCall2(ex->FindFunctionNamed("lShift"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa7:
			{
				//urshift
				LOG(TRACE, "synt urshift" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("urShift"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xa8:
			{
				//bitand
				LOG(TRACE, "synt bitand" );
				stack_entry v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				const char* f;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					f="bitAnd_oo";
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					//bitAnd is commutative, but llvm is not :-)
					f="bitAnd_oi";
					value=v1.first;
					v1.first=v2.first;
					v2.first=value;
				}
				else
					abort();

				value=Builder.CreateCall2(ex->FindFunctionNamed(f), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				jitted=true;
				break;
			}
			case 0xa9:
			{
				//bitor
				LOG(TRACE, "synt bitor" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xaa:
			{
				//bitxor
				LOG(TRACE, "synt bitxor" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("bitXor"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xab:
			{
				//equals
				LOG(TRACE, "synt equals" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xac:
			{
				//strictequals
				LOG(TRACE, "synt strictequals" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("strictEquals"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xad:
			{
				//lessthan
				LOG(TRACE, "synt lessthan" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("lessThan"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xaf:
			{
				//greaterthan
				LOG(TRACE, "synt greaterthan" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("greaterThan"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xb3:
			{
				//istypelate
				LOG(TRACE, "synt istypelate" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("isTypelate"), context);
				break;
			}
			case 0xb4:
			{
				//in
				LOG(TRACE, "synt in" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("in"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xc0:
			{
				//increment_i
				LOG(TRACE, "synt increment_i" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("increment_i"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xc1:
			{
				//decrement_i
				LOG(TRACE, "synt decrement_i" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("decrement_i"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				LOG(TRACE, "synt inclocal_i" );
				syncStacks(ex,Builder,jitted,static_stack,dynamic_stack,dynamic_stack_index);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("incLocal_i"), context, constant);
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				LOG(TRACE, "synt getlocal_n" );
				int i=opcode&3;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
				if(Log::getLevel()==TRACE)
					Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), context, constant);

				if(static_locals[i].second==STACK_NONE)
				{
					llvm::Value* t=Builder.CreateGEP(locals,constant);
					t=Builder.CreateLoad(t,"stack");
					static_stack_push(static_stack,stack_entry(t,STACK_OBJECT));
					static_locals[i]=stack_entry(t,STACK_OBJECT);
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
				}
				else if(static_locals[i].second==STACK_OBJECT)
				{
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), static_locals[i].first);
					static_stack_push(static_stack,static_locals[i]);
				}
				else
					abort();

				jitted=true;
				break;
			}
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				LOG(TRACE, "synt setlocal_n" );
				int i=opcode&3;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_locals[i]=e;
				if(Log::getLevel()==TRACE)
				{
					constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode&3);
					Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), context, constant);
				}
				jitted=true;
				break;
			}
			default:
				LOG(ERROR,"Not implemented instruction @" << code.tellg());
				u8 a,b,c;
				code >> a >> b >> c;
				LOG(ERROR,"dump " << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
						<< (unsigned int)b << ' ' << (unsigned int)c);
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode);
				Builder.CreateCall(ex->FindFunctionNamed("not_impl"), constant);
				Builder.CreateRetVoid();

				f=(SyntheticFunction::synt_function)this->context->vm->ex->getPointerToFunction(llvmf);
				return f;
		}
	}

	map<int,llvm::BasicBlock*>::iterator it2=blocks.begin();
	for(it2;it2!=blocks.end();it2++)
	{
		if(it2->second->getTerminator()==NULL)
		{
			cout << "start at " << it2->first << endl;
			abort();
		}
	}

	this->context->vm->FPM->run(*llvmf);
	//llvmf->dump();
	f=(SyntheticFunction::synt_function)this->context->vm->ex->getPointerToFunction(llvmf);
	return f;
}
