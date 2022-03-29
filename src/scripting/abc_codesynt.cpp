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

#ifdef LLVM_ENABLED

#ifdef LLVM_28
#define alignof alignOf
#define LLVMMAKEARRAYREF(T) T
#else
#define LLVMMAKEARRAYREF(T) makeArrayRef(T)
#endif

#include "compat.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#ifdef HAVE_PASSMANAGER_H
#  include <llvm/PassManager.h>
#else
#  include <llvm/IR/LegacyPassManager.h>
#endif
#ifdef HAVE_IR_DATALAYOUT_H
#  include <llvm/IR/Constants.h>
#  include <llvm/IR/DerivedTypes.h>
#  include <llvm/IR/Module.h>
#  include <llvm/IR/LLVMContext.h>
#else
#  include <llvm/Constants.h>
#  include <llvm/DerivedTypes.h>
#  include <llvm/Module.h>
#  include <llvm/LLVMContext.h>
#endif
#ifdef HAVE_IR_DATALAYOUT_H
#  include <llvm/IR/IRBuilder.h>
#elif defined HAVE_IRBUILDER_H
#  include <llvm/IRBuilder.h>
#else
#  include <llvm/Support/IRBuilder.h>
#endif
#ifdef HAVE_IR_DATALAYOUT_H
#  include <llvm/IR/DataLayout.h>
#elif defined HAVE_DATALAYOUT_H
#  include <llvm/DataLayout.h>
#else
#  include <llvm/Target/TargetData.h>
#endif
#include <llvm/ExecutionEngine/GenericValue.h>
#include <sstream>
#include "scripting/abc.h"
#include "scripting/class.h"
#include "swftypes.h"
#include "exceptions.h"

using namespace std;
using namespace lightspark;

namespace lightspark {
struct block_info
{
	llvm::BasicBlock* BB;
	/* current type of locals, changed through interpreting the opcodes in doAnalysis */
	std::vector<STACK_TYPE> locals;
	/* types of locals at the start of the block
	 * This is computed in doAnalysis. It is != STACK_NONE when all preceding blocks end with
	 * the local having the same type. */
	std::vector<STACK_TYPE> locals_start;
	/* if locals_start[i] != STACK_NONE, then locals_start_obj[i] is an Alloca of the given type.
	 * SyncLocals at the end of one block Store's the current locals to locals_start_obj. (if both have the same type)
	 * At the beginning of this block, static_locals[i] is initialized by a Load(locals_start_obj[i]). */
	std::vector<llvm::Value*> locals_start_obj;
	/* there is no need to transfer the given local to this block by a preceding block
	 * because this and all successive blocks will not read the local before writing to it.
	 */
	std::vector<bool> locals_reset;
	/* getlocal/setlocal in this block used the given local */
	std::vector<bool> locals_used;
	std::set<block_info*> preds; /* preceding blocks */
	std::set<block_info*> seqs; /* subsequent blocks */
	std::map<int,STACK_TYPE> push_types;

	//Needed for indexed access
	block_info():BB(NULL){abort();}
	block_info(const method_info* mi, const char* blockName);
	STACK_TYPE checkProactiveCasting(int local_ip,STACK_TYPE type);
};
}

static LLVMTYPE void_type = NULL;
static LLVMTYPE int_type = NULL;
static LLVMTYPE intptr_type = NULL;
static LLVMTYPE voidptr_type = NULL;
static LLVMTYPE number_type = NULL;
static LLVMTYPE numberptr_type = NULL;
static LLVMTYPE bool_type = NULL;
static LLVMTYPE boolptr_type = NULL;
static LLVMTYPE ptr_type = NULL;
static LLVMTYPE context_type = NULL;

void debug_d(number_t f)
{
	LOG(LOG_CALLS, "debug_d "<< f);
}

void debug_i(int32_t i)
{
	LOG(LOG_CALLS, "debug_i "<< i);
}

llvm::LLVMContext& ABCVm::llvm_context()
{
	static llvm::LLVMContext context;
	return context;
}

opcode_handler ABCVm::opcode_table_args_pointer_2int[]={
	{"getMultiname_i",(void*)&ABCContext::s_getMultiname_i}
};

opcode_handler ABCVm::opcode_table_args_pointer_number_int[]={
	{"getMultiname_d",(void*)&ABCContext::s_getMultiname_d}
};

opcode_handler ABCVm::opcode_table_args3_pointers[]={
	{"setProperty",(void*)&ABCVm::setProperty},
};

typed_opcode_handler ABCVm::opcode_table_uint32_t[]={
	{"bitAnd_oo",(void*)&ABCVm::bitAnd,ARGS_OBJ_OBJ},
	{"bitAnd_oi",(void*)&ABCVm::bitAnd_oi,ARGS_OBJ_INT},
	{"pushByte",(void*)&ABCVm::pushByte,ARGS_INT},
	{"pushShort",(void*)&ABCVm::pushShort,ARGS_INT},
	{"decrement_i",(void*)&ABCVm::decrement_i,ARGS_OBJ},
	{"increment_i",(void*)&ABCVm::increment_i,ARGS_OBJ},
	{"bitNot",(void*)&ABCVm::bitNot,ARGS_OBJ},
	{"bitXor",(void*)&ABCVm::bitXor,ARGS_OBJ_OBJ},
	{"bitOr",(void*)&ABCVm::bitOr,ARGS_OBJ_OBJ},
	{"bitOr_oi",(void*)&ABCVm::bitOr_oi,ARGS_OBJ_INT},
	{"lShift",(void*)&ABCVm::lShift,ARGS_OBJ_OBJ},
	{"lShift_io",(void*)&ABCVm::lShift_io,ARGS_INT_OBJ},
	{"urShift",(void*)&ABCVm::urShift,ARGS_OBJ_OBJ},
	{"rShift",(void*)&ABCVm::rShift,ARGS_OBJ_OBJ},
	{"urShift_io",(void*)&ABCVm::urShift_io,ARGS_INT_OBJ},
	{"getProperty_i",(void*)&ABCVm::getProperty_i,ARGS_OBJ_OBJ},
	{"convert_i",(void*)&ABCVm::convert_i,ARGS_OBJ},
	{"convert_u",(void*)&ABCVm::convert_u,ARGS_OBJ},
};

typed_opcode_handler ABCVm::opcode_table_number_t[]={
	{"increment",(void*)&ABCVm::increment,ARGS_OBJ},
	{"decrement",(void*)&ABCVm::decrement,ARGS_OBJ},
	{"multiply",(void*)&ABCVm::multiply,ARGS_OBJ_OBJ},
	{"multiply_oi",(void*)&ABCVm::multiply_oi,ARGS_OBJ_INT},
	{"divide",(void*)&ABCVm::divide,ARGS_OBJ_OBJ},
	{"subtract",(void*)&ABCVm::subtract,ARGS_OBJ_OBJ},
	{"subtract_oi",(void*)&ABCVm::subtract_oi,ARGS_OBJ_INT},
	{"subtract_io",(void*)&ABCVm::subtract_io,ARGS_INT_OBJ},
	{"subtract_do",(void*)&ABCVm::subtract_do,ARGS_NUMBER_OBJ},
	{"convert_d",(void*)&ABCVm::convert_d,ARGS_OBJ},
	{"negate",(void*)&ABCVm::negate,ARGS_OBJ},
};

typed_opcode_handler ABCVm::opcode_table_void[]={
	{"setSlot",(void*)&ABCVm::setSlot,ARGS_OBJ_OBJ_INT},
	{"debug_d",(void*)&debug_d,ARGS_NUMBER},
	{"debug_i",(void*)&debug_i,ARGS_INT},
	{"label",(void*)&ABCVm::label,ARGS_NONE},
	{"pop",(void*)&ABCVm::pop,ARGS_NONE},
	{"dup",(void*)&ABCVm::dup,ARGS_NONE},
	{"swap",(void*)&ABCVm::swap,ARGS_NONE},
	{"coerce_a",(void*)&ABCVm::coerce_a,ARGS_NONE},
	{"lookupswitch",(void*)&ABCVm::lookupswitch,ARGS_NONE},
	{"getLocal",(void*)&ABCVm::getLocal,ARGS_OBJ_INT},
	{"getLocal_short",(void*)&ABCVm::getLocal_short,ARGS_INT},
	{"getLocal_int",(void*)&ABCVm::getLocal_int,ARGS_INT_INT},
	{"setLocal",(void*)&ABCVm::setLocal,ARGS_INT},
	{"setLocal_int",(void*)&ABCVm::setLocal_int,ARGS_INT_INT},
	{"setLocal_obj",(void*)&ABCVm::setLocal_obj,ARGS_INT_OBJ},
	{"pushScope",(void*)&ABCVm::pushScope,ARGS_CONTEXT},
	{"pushWith",(void*)&ABCVm::pushWith,ARGS_CONTEXT},
	{"throw",(void*)&ABCVm::_throw,ARGS_CONTEXT},
	{"popScope",(void*)&ABCVm::popScope,ARGS_CONTEXT},
	{"pushDouble",(void*)&ABCVm::pushDouble,ARGS_CONTEXT_INT},
	{"pushInt",(void*)&ABCVm::pushInt,ARGS_CONTEXT_INT},
	{"pushUInt",(void*)&ABCVm::pushUInt,ARGS_CONTEXT_INT},
	{"getSuper",(void*)&ABCVm::getSuper,ARGS_CONTEXT_INT},
	{"setSuper",(void*)&ABCVm::setSuper,ARGS_CONTEXT_INT},
	{"newObject",(void*)&ABCVm::newObject,ARGS_CONTEXT_INT},
	{"getDescendants",(void*)&ABCVm::getDescendants,ARGS_CONTEXT_INT},
	{"call",(void*)&ABCVm::call,ARGS_CONTEXT_INT_INT},
	{"coerce",(void*)&ABCVm::coerce,ARGS_CONTEXT_INT},
	{"getLex",(void*)&ABCVm::getLex,ARGS_CONTEXT_INT},
	{"incLocal_i",(void*)&ABCVm::incLocal_i,ARGS_CONTEXT_INT},
	{"construct",(void*)&ABCVm::construct,ARGS_CONTEXT_INT},
	{"constructGenericType",(void*)&ABCVm::constructGenericType,ARGS_CONTEXT_INT},
	{"constructSuper",(void*)&ABCVm::constructSuper,ARGS_CONTEXT_INT},
	{"newArray",(void*)&ABCVm::newArray,ARGS_CONTEXT_INT},
	{"newClass",(void*)&ABCVm::newClass,ARGS_CONTEXT_INT},
	{"initProperty",(void*)&ABCVm::initProperty,ARGS_OBJ_OBJ_OBJ},
	{"kill",(void*)&ABCVm::kill,ARGS_INT},
	{"jump",(void*)&ABCVm::jump,ARGS_INT},
	{"callProperty",(void*)&ABCVm::callProperty,ARGS_CONTEXT_INT_INT_INT_BOOL},
	{"callPropLex",(void*)&ABCVm::callPropLex,ARGS_CONTEXT_INT_INT_INT_BOOL},
	{"constructProp",(void*)&ABCVm::constructProp,ARGS_CONTEXT_INT_INT},
	{"callSuper",(void*)&ABCVm::callSuper,ARGS_CONTEXT_INT_INT_INT_BOOL},
	{"not_impl",(void*)&ABCVm::not_impl,ARGS_INT},
	{"incRef",(void*)&ASObject::s_incRef,ARGS_OBJ},
	{"decRef",(void*)&ASObject::s_decRef,ARGS_OBJ},
	{"decRef_safe",(void*)&ASObject::s_decRef_safe,ARGS_OBJ_OBJ},
	{"wrong_exec_pos",(void*)&ABCVm::wrong_exec_pos,ARGS_NONE},
	{"dxns",(void*)&ABCVm::dxns,ARGS_CONTEXT_INT},
	{"dxnslate",(void*)&ABCVm::dxnslate,ARGS_CONTEXT_OBJ},
	{"callMethod",(void*)&ABCVm::callMethod,ARGS_CONTEXT_INT_INT},
};

typed_opcode_handler ABCVm::opcode_table_voidptr[]={
	{"add",(void*)&ABCVm::add,ARGS_OBJ_OBJ},
	{"add_oi",(void*)&ABCVm::add_oi,ARGS_OBJ_INT},
	{"add_od",(void*)&ABCVm::add_od,ARGS_OBJ_NUMBER},
	{"nextName",(void*)&ABCVm::nextName,ARGS_OBJ_OBJ},
	{"nextValue",(void*)&ABCVm::nextValue,ARGS_OBJ_OBJ},
//	{"abstract_d",(void*)&abstract_d,ARGS_OBJ_NUMBER},
//	{"abstract_i",(void*)&abstract_i,ARGS_OBJ_INT},
//	{"abstract_ui",(void*)&abstract_ui,ARGS_OBJ_INT},
//	{"abstract_b",(void*)&abstract_b,ARGS_BOOL},
	{"pushNaN",(void*)&ABCVm::pushNaN,ARGS_NONE},
	{"pushNull",(void*)&ABCVm::pushNull,ARGS_NONE},
	{"pushUndefined",(void*)&ABCVm::pushUndefined,ARGS_NONE},
	{"pushNamespace",(void*)&ABCVm::pushNamespace,ARGS_CONTEXT_INT},
	{"getProperty",(void*)&ABCVm::getProperty,ARGS_OBJ_OBJ},
	{"asTypelate",(void*)&ABCVm::asTypelate,ARGS_OBJ_OBJ},
	{"getGlobalScope",(void*)&ABCVm::getGlobalScope,ARGS_CONTEXT},
	{"findPropStrict",(void*)&ABCVm::findPropStrict,ARGS_CONTEXT_OBJ},
	{"findProperty",(void*)&ABCVm::findProperty,ARGS_CONTEXT_OBJ},
	{"getMultiname",(void*)&ABCContext::s_getMultiname,ARGS_OBJ_OBJ_OBJ_INT},
	{"typeOf",(void*)ABCVm::typeOf,ARGS_OBJ},
	{"coerce_s",(void*)&ABCVm::coerce_s,ARGS_OBJ},
	{"checkfilter",(void*)&ABCVm::checkfilter,ARGS_OBJ},
	{"pushString",(void*)&ABCVm::pushString,ARGS_CONTEXT_INT},
	{"newFunction",(void*)&ABCVm::newFunction,ARGS_CONTEXT_INT},
	{"newCatch",(void*)&ABCVm::newCatch,ARGS_CONTEXT_INT},
	{"getScopeObject",(void*)&ABCVm::getScopeObject,ARGS_CONTEXT_INT},
	{"getSlot",(void*)&ABCVm::getSlot,ARGS_OBJ_INT},
	{"convert_s",(void*)&ABCVm::convert_s,ARGS_OBJ},
	{"coerce_s",(void*)&ABCVm::coerce_s,ARGS_OBJ},
	{"esc_xattr",(void*)&ABCVm::esc_xattr,ARGS_OBJ},
	{"hasNext",(void*)&ABCVm::hasNext,ARGS_OBJ_OBJ},
};

typed_opcode_handler ABCVm::opcode_table_bool_t[]={
	{"not",(void*)&ABCVm::_not,ARGS_OBJ},
	{"equals",(void*)&ABCVm::equals,ARGS_OBJ_OBJ},
	{"strictEquals",(void*)&ABCVm::strictEquals,ARGS_OBJ_OBJ},
	{"greaterThan",(void*)&ABCVm::greaterThan,ARGS_OBJ_OBJ},
	{"greaterEquals",(void*)&ABCVm::greaterEquals,ARGS_OBJ_OBJ},
	{"lessThan",(void*)&ABCVm::lessThan,ARGS_OBJ_OBJ},
	{"lessEquals",(void*)&ABCVm::lessEquals,ARGS_OBJ_OBJ},
	{"ifEq",(void*)&ABCVm::ifEq,ARGS_OBJ_OBJ},
	{"ifNE",(void*)&ABCVm::ifNE,ARGS_OBJ_OBJ},
	{"ifNE_oi",(void*)&ABCVm::ifNE_oi,ARGS_OBJ_INT},
	{"ifLT",(void*)&ABCVm::ifLT,ARGS_OBJ_OBJ},
	{"ifNLT",(void*)&ABCVm::ifNLT,ARGS_OBJ_OBJ},
	{"ifLT_io",(void*)&ABCVm::ifLT_io,ARGS_INT_OBJ},
	{"ifLT_oi",(void*)&ABCVm::ifLT_oi,ARGS_OBJ_INT},
	{"ifGT",(void*)&ABCVm::ifGT,ARGS_OBJ_OBJ},
	{"ifNGT",(void*)&ABCVm::ifNGT,ARGS_OBJ_OBJ},
	{"ifGE",(void*)&ABCVm::ifGE,ARGS_OBJ_OBJ},
	{"ifNGE",(void*)&ABCVm::ifNGE,ARGS_OBJ_OBJ},
	{"ifLE",(void*)&ABCVm::ifLE,ARGS_OBJ_OBJ},
	{"ifNLE",(void*)&ABCVm::ifNLE,ARGS_OBJ_OBJ},
	{"in",(void*)&ABCVm::in,ARGS_OBJ_OBJ},
	{"ifStrictNE",(void*)&ABCVm::ifStrictNE,ARGS_OBJ_OBJ},
	{"ifStrictEq",(void*)&ABCVm::ifStrictEq,ARGS_OBJ_OBJ},
	{"pushTrue",(void*)&ABCVm::pushTrue,ARGS_NONE},
	{"pushFalse",(void*)&ABCVm::pushFalse,ARGS_NONE},
	{"isTypelate",(void*)&ABCVm::isTypelate,ARGS_OBJ_OBJ},
	{"ifTrue",(void*)&ABCVm::ifTrue,ARGS_OBJ},
	{"ifFalse",(void*)&ABCVm::ifFalse,ARGS_OBJ},
	{"convert_b",(void*)&ABCVm::convert_b,ARGS_OBJ},
	{"hasNext2",(void*)ABCVm::hasNext2,ARGS_CONTEXT_INT_INT},
	{"deleteProperty",(void*)&ABCVm::deleteProperty,ARGS_OBJ_OBJ},
	{"instanceOf",(void*)&ABCVm::instanceOf,ARGS_OBJ_OBJ},
};

void ABCVm::registerFunctions()
{
	vector<LLVMTYPE> sig;
	llvm::FunctionType* FT=NULL;

	//Create types
#ifdef LLVM_38
	ptr_type=ex->getDataLayout().getIntPtrType(llvm_context());
#else
#if defined HAVE_DATALAYOUT_H || defined HAVE_IR_DATALAYOUT_H
	ptr_type=ex->getDataLayout()->getIntPtrType(llvm_context());
#else
	ptr_type=ex->getTargetData()->getIntPtrType(llvm_context());
#endif
#endif
	//Pointer to 8 bit type, needed for pointer arithmetic
	voidptr_type=llvm::IntegerType::get(llvm_context(),8)->getPointerTo();
	number_type=llvm::Type::getDoubleTy(llvm_context());
	numberptr_type=llvm::Type::getDoublePtrTy(llvm_context());
	bool_type=llvm::IntegerType::get(llvm_context(),1);
	boolptr_type=bool_type->getPointerTo();
	void_type=llvm::Type::getVoidTy(llvm_context());
	int_type=llvm::IntegerType::get(llvm_context(),32);
	intptr_type=int_type->getPointerTo();

	//All the opcodes needs a pointer to the context
	std::vector<LLVMTYPE> struct_elems;
	struct_elems.push_back(voidptr_type->getPointerTo());
	struct_elems.push_back(voidptr_type->getPointerTo());
	struct_elems.push_back(int_type);
	struct_elems.push_back(int_type);
	context_type=llvm::PointerType::getUnqual(llvm::StructType::get(llvm_context(),LLVMMAKEARRAYREF(struct_elems),true));

	//newActivation needs method_info and context
	sig.push_back(context_type);
	sig.push_back(voidptr_type);
	FT=llvm::FunctionType::get(voidptr_type, LLVMMAKEARRAYREF(sig), false);
	llvm::Function* F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newActivation",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newActivation);

	//Lazy pushing, no context, (ASObject*, uint32_t, int)
	sig.clear();
	sig.push_back(voidptr_type);
	sig.push_back(int_type);
	sig.push_back(int_type);
	FT=llvm::FunctionType::get(voidptr_type, sig, false);
	int elems=sizeof(opcode_table_args_pointer_2int)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args_pointer_2int[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args_pointer_2int[i].addr);
	}

	sig.clear();
	sig.push_back(voidptr_type);
	sig.push_back(number_type);
	sig.push_back(int_type);
	FT=llvm::FunctionType::get(voidptr_type, sig, false);
	elems=sizeof(opcode_table_args_pointer_number_int)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args_pointer_number_int[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args_pointer_number_int[i].addr);
	}
	//End of lazy pushing

	//Lazy pushing, no context, (ASObject*, ASObject*, void*)
	sig.clear();
	sig.push_back(voidptr_type);
	sig.push_back(voidptr_type);
	sig.push_back(voidptr_type);
	FT=llvm::FunctionType::get(void_type, sig, false);
	elems=sizeof(opcode_table_args3_pointers)/sizeof(opcode_handler);
	for(int i=0;i<elems;i++)
	{
		F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,opcode_table_args3_pointers[i].name,module);
		ex->addGlobalMapping(F,opcode_table_args3_pointers[i].addr);
	}

	//Build the concrete interface
	sig[0]=int_type;
	FT=llvm::FunctionType::get(void_type, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setProperty_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setProperty_i);

	register_table(int_type,opcode_table_uint32_t,sizeof(opcode_table_uint32_t)/sizeof(typed_opcode_handler));
	register_table(number_type,opcode_table_number_t,sizeof(opcode_table_number_t)/sizeof(typed_opcode_handler));
	register_table(void_type,opcode_table_void,sizeof(opcode_table_void)/sizeof(typed_opcode_handler));
	register_table(voidptr_type,opcode_table_voidptr,sizeof(opcode_table_voidptr)/sizeof(typed_opcode_handler));
	register_table(bool_type,opcode_table_bool_t,sizeof(opcode_table_bool_t)/sizeof(typed_opcode_handler));
}

void ABCVm::register_table(LLVMTYPE ret_type,typed_opcode_handler* table, int table_len)
{
	vector<LLVMTYPE> sig_obj_obj;
	sig_obj_obj.push_back(voidptr_type);
	sig_obj_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_obj_obj_obj;
	sig_obj_obj_obj.push_back(voidptr_type);
	sig_obj_obj_obj.push_back(voidptr_type);
	sig_obj_obj_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_obj_int;
	sig_obj_int.push_back(voidptr_type);
	sig_obj_int.push_back(int_type);

	vector<LLVMTYPE> sig_int_obj;
	sig_int_obj.push_back(int_type);
	sig_int_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_obj_number;
	sig_obj_number.push_back(voidptr_type);
	sig_obj_number.push_back(number_type);

	vector<LLVMTYPE> sig_bool;
	sig_bool.push_back(bool_type);

	vector<LLVMTYPE> sig_int;
	sig_int.push_back(int_type);

	vector<LLVMTYPE> sig_number;
	sig_number.push_back(number_type);

	vector<LLVMTYPE> sig_obj;
	sig_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_none;

	vector<LLVMTYPE> sig_obj_obj_int;
	sig_obj_obj_int.push_back(voidptr_type);
	sig_obj_obj_int.push_back(voidptr_type);
	sig_obj_obj_int.push_back(int_type);

	vector<LLVMTYPE> sig_obj_obj_obj_int;
	sig_obj_obj_obj_int.push_back(voidptr_type);
	sig_obj_obj_obj_int.push_back(voidptr_type);
	sig_obj_obj_obj_int.push_back(voidptr_type);
	sig_obj_obj_obj_int.push_back(int_type);

	vector<LLVMTYPE> sig_number_obj;
	sig_number_obj.push_back(number_type);
	sig_number_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_int_int;
	sig_int_int.push_back(int_type);
	sig_int_int.push_back(int_type);

	vector<LLVMTYPE> sig_context;
	sig_context.push_back(context_type);

	vector<LLVMTYPE> sig_context_int;
	sig_context_int.push_back(context_type);
	sig_context_int.push_back(int_type);

	vector<LLVMTYPE> sig_context_int_int;
	sig_context_int_int.push_back(context_type);
	sig_context_int_int.push_back(int_type);
	sig_context_int_int.push_back(int_type);

	vector<LLVMTYPE> sig_context_int_int_int;
	sig_context_int_int_int.push_back(context_type);
	sig_context_int_int_int.push_back(int_type);
	sig_context_int_int_int.push_back(int_type);
	sig_context_int_int_int.push_back(int_type);

	vector<LLVMTYPE> sig_context_int_int_int_bool;
	sig_context_int_int_int_bool.push_back(context_type);
	sig_context_int_int_int_bool.push_back(int_type);
	sig_context_int_int_int_bool.push_back(int_type);
	sig_context_int_int_int_bool.push_back(int_type);
	sig_context_int_int_int_bool.push_back(bool_type);

	vector<LLVMTYPE> sig_context_obj;
	sig_context_obj.push_back(context_type);
	sig_context_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_context_obj_obj;
	sig_context_obj_obj.push_back(context_type);
	sig_context_obj_obj.push_back(voidptr_type);
	sig_context_obj_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_context_obj_obj_obj;
	sig_context_obj_obj_obj.push_back(context_type);
	sig_context_obj_obj_obj.push_back(voidptr_type);
	sig_context_obj_obj_obj.push_back(voidptr_type);
	sig_context_obj_obj_obj.push_back(voidptr_type);

	vector<LLVMTYPE> sig_context_obj_obj_int;
	sig_context_obj_obj_int.push_back(context_type);
	sig_context_obj_obj_int.push_back(voidptr_type);
	sig_context_obj_obj_int.push_back(voidptr_type);
	sig_context_obj_obj_int.push_back(int_type);

	llvm::FunctionType* FT=NULL;
	for(int i=0;i<table_len;i++)
	{
		switch(table[i].type)
		{
			case ARGS_OBJ_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_obj), false);
				break;
			case ARGS_OBJ_OBJ_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_obj_obj), false);
				break;
			case ARGS_NONE:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_none), false);
				break;
			case ARGS_INT_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_int_obj), false);
				break;
			case ARGS_OBJ_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_int), false);
				break;
			case ARGS_OBJ_NUMBER:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_number), false);
				break;
			case ARGS_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_int), false);
				break;
			case ARGS_NUMBER:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_number), false);
				break;
			case ARGS_BOOL:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_bool), false);
				break;
			case ARGS_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj), false);
				break;
			case ARGS_OBJ_OBJ_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_obj_int), false);
				break;
			case ARGS_OBJ_OBJ_OBJ_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_obj_obj_obj_int), false);
				break;
			case ARGS_NUMBER_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_number_obj), false);
				break;
			case ARGS_INT_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_int_int), false);
				break;
			case ARGS_CONTEXT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context), false);
				break;
			case ARGS_CONTEXT_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_int), false);
				break;
			case ARGS_CONTEXT_INT_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_int_int), false);
				break;
			case ARGS_CONTEXT_INT_INT_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_int_int_int), false);
				break;
			case ARGS_CONTEXT_INT_INT_INT_BOOL:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_int_int_int_bool), false);
				break;
			case ARGS_CONTEXT_OBJ_OBJ_INT:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_obj_obj_int), false);
				break;
			case ARGS_CONTEXT_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_obj), false);
				break;
			case ARGS_CONTEXT_OBJ_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_obj_obj), false);
				break;
			case ARGS_CONTEXT_OBJ_OBJ_OBJ:
				FT=llvm::FunctionType::get(ret_type, LLVMMAKEARRAYREF(sig_context_obj_obj_obj), false);
				break;
		}

		llvm::Function* F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,table[i].name,module);
		ex->addGlobalMapping(F,table[i].addr);
	}
}

std::ostream& lightspark::operator<<(std::ostream& o, const block_info& b)
{
	o << "this: " << &b
		<< " locals_start: " << b.locals_start
		<< " locals_reset: " << b.locals_reset
		<< " locals_used: " << b.locals_used
		<< " preds: " << b.preds
		<< " seqs: " << b.seqs;
	return o;
}

static llvm::Value* llvm_stack_pop(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	//decrement stack index
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm(getSys())->llvm_context(),32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);

	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

static llvm::Value* llvm_stack_peek(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm(getSys())->llvm_context(),32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

static void llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val,
		llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index);
	builder.CreateStore(val,dest);

	//increment stack index
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm(getSys())->llvm_context(),32), 1);
	llvm::Value* index2=builder.CreateAdd(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);
}

static stack_entry static_stack_pop(llvm::IRBuilder<>& builder, vector<stack_entry>& static_stack,
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

static stack_entry static_stack_peek(llvm::IRBuilder<>& builder, vector<stack_entry>& static_stack,
		llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	//try to get the tail value from the static stack
	if(!static_stack.empty())
		return static_stack.back();
	//try to load the tail value of the dynamic stack
	return stack_entry(llvm_stack_peek(builder,dynamic_stack,dynamic_stack_index),STACK_OBJECT);
}

/* Checks if both types correspond */
inline void checkStackTypeFromLLVMType(LLVMTYPE type, STACK_TYPE st)
{
	assert(st != STACK_NONE);
	assert(st != STACK_NUMBER || type == number_type);
	assert(st != STACK_INT || type == int_type);
	assert(st != STACK_UINT || type == int_type); //INT and UINT have the same llvm representation
	assert(st != STACK_OBJECT || type == voidptr_type);
	assert(st != STACK_BOOLEAN || type == bool_type);
}

static void static_stack_push(vector<stack_entry>& static_stack, const stack_entry& e)
{
	checkStackTypeFromLLVMType(e.first->getType(),e.second);
	static_stack.push_back(e);
}
inline void abstract_value(llvm::Module* module, llvm::IRBuilder<>& builder, stack_entry& e)
{
	switch(e.second)
	{
		case STACK_OBJECT:
			break;
		case STACK_INT:
			e.first=builder.CreateCall(module->getFunction("abstract_i"),e.first);
			break;
		case STACK_UINT:
			e.first=builder.CreateCall(module->getFunction("abstract_ui"),e.first);
			break;
		case STACK_NUMBER:
			e.first=builder.CreateCall(module->getFunction("abstract_d"),e.first);
			break;
		case STACK_BOOLEAN:
			e.first=builder.CreateCall(module->getFunction("abstract_b"),e.first);
			break;
		default:
			throw RunTimeException("Unexpected object type to abstract");
	}
	e.second = STACK_OBJECT;
}

//Helper functions to sync the static stack and locals to the memory 
inline void syncStacks(llvm::ExecutionEngine* ex,llvm::Module* module,llvm::IRBuilder<>& builder,
		vector<stack_entry>& static_stack,llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	for(unsigned int i=0;i<static_stack.size();i++)
	{
		abstract_value(module, builder, static_stack[i]);
		llvm_stack_push(ex,builder,static_stack[i].first,dynamic_stack,dynamic_stack_index);
	}
	static_stack.clear();
}

inline void syncLocals(llvm::Module* module,llvm::IRBuilder<>& builder,
	const vector<stack_entry>& static_locals,llvm::Value* locals,const vector<STACK_TYPE>& expected,
	const block_info& dest_block)
{
	for(unsigned int i=0;i<static_locals.size();i++)
	{
		if(static_locals[i].second==STACK_NONE)
		{
			assert(dest_block.locals_start[i] == STACK_NONE);
			continue;
		}

		//Let's sync with the expected values...
		if(static_locals[i].second!=expected[i])
			throw RunTimeException("Locals are not as expected");

		if(dest_block.locals_reset[i])
		{
			if(static_locals[i].second==STACK_OBJECT)
				builder.CreateCall(module->getFunction("decRef"), static_locals[i].first);
			continue;
		}

		if(static_locals[i].second==dest_block.locals_start[i])
		{
			//copy local over to dest_block's initial locals
			builder.CreateStore(static_locals[i].first,dest_block.locals_start_obj[i]);
		}
		else
		{
			//dest_block does not expect us to transfer locals,
			//so just write them to call_context->locals, overwriting (and decRef'ing) the old contents of call_context->locals
			assert(dest_block.locals_start[i] == STACK_NONE);
			llvm::Value* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm(getSys())->llvm_context(),32), i);
			llvm::Value* t=builder.CreateGEP(locals,constant);
			llvm::Value* old=builder.CreateLoad(t);
			if(static_locals[i].second==STACK_OBJECT)
			{
				builder.CreateCall(module->getFunction("decRef"), old);
				builder.CreateStore(static_locals[i].first,t);
			}
			else if(static_locals[i].second==STACK_INT)
			{
				//decRef the previous contents
				builder.CreateCall(module->getFunction("decRef"), old);
				llvm::Value* v=builder.CreateCall(module->getFunction("abstract_i"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_UINT)
			{
				//decRef the previous contents
				builder.CreateCall(module->getFunction("decRef"), old);
				llvm::Value* v=builder.CreateCall(module->getFunction("abstract_ui"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_NUMBER)
			{
				//decRef the previous contents
				builder.CreateCall(module->getFunction("decRef"), old);
				llvm::Value* v=builder.CreateCall(module->getFunction("abstract_d"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_BOOLEAN)
			{
				//decRef the previous contents
				builder.CreateCall(module->getFunction("decRef"), old);
				llvm::Value* v=builder.CreateCall(module->getFunction("abstract_b"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else
				throw RunTimeException("Unexpected object type");
		}
	}
}

STACK_TYPE block_info::checkProactiveCasting(int local_ip,STACK_TYPE type)
{
	pair< map<int,STACK_TYPE>::iterator, bool> result=push_types.insert(make_pair(local_ip,type));
	return result.first->second;
}

llvm::FunctionType* method_info::synt_method_prototype(llvm::ExecutionEngine* ex)
{
	//Initialize LLVM representation of method
	vector<LLVMTYPE> sig;
	sig.push_back(context_type);

	return llvm::FunctionType::get(voidptr_type, LLVMMAKEARRAYREF(sig), false);
}

void method_info::consumeStackForRTMultiname(static_stack_types_vector& stack, int multinameIndex) const
{
	int rtdata=context->getMultinameRTData(multinameIndex);
	for(int i=0;i<rtdata;i++)
	{
		if(!stack.empty())
			stack.pop_back();
		else
			break;
	}
}

/* Implements ECMA's ToNumber algorith */
static llvm::Value* llvm_ToNumber(llvm::Module* module, llvm::IRBuilder<>& Builder, stack_entry& e)
{
	switch(e.second)
	{
	case STACK_BOOLEAN:
	case STACK_INT:
		return Builder.CreateSIToFP(e.first,number_type);
	case STACK_UINT:
		return Builder.CreateUIToFP(e.first,number_type);
	case STACK_NUMBER:
		return e.first;
	default:
		return Builder.CreateCall(module->getFunction("convert_d"), e.first);
	}
}

/* Adds instructions to the builder to resolve the given multiname */
inline llvm::Value* getMultiname(llvm::Module* module,llvm::IRBuilder<>& Builder, vector<stack_entry>& static_stack,
				llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index,
				ABCContext* abccontext, int multinameIndex)
{
	int rtdata=abccontext->getMultinameRTData(multinameIndex);
	llvm::Value* name = 0;
	if(rtdata==0)
	{
		//Multinames without runtime date persist
		asAtom atom=asAtomHandler::invalidAtom;
		multiname* mname = ABCContext::s_getMultiname(abccontext,atom,NULL,multinameIndex);
		name = llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(ptr_type, (intptr_t)mname),voidptr_type);
	}
	else
	{
		llvm::Value* context = llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(ptr_type, (intptr_t)abccontext), voidptr_type);
		llvm::Value* mindx = llvm::ConstantInt::get(int_type, multinameIndex);
		if(rtdata==1)
		{
			llvm::Value* constnull = llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(int_type, 0), voidptr_type);
			stack_entry rt1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

			/*if(rt1.second==STACK_INT) //TODO: for them, first parameter is call_context, not ABCContext
#ifdef LLVM_37
				name = Builder.CreateCall(module->getFunction("getMultiname_i"), {context, rt1.first, mindx});
#else
				name = Builder.CreateCall3(module->getFunction("getMultiname_i"), context, rt1.first, mindx);
#endif
			else if(rt1.second==STACK_NUMBER)
#ifdef LLVM_37
				name = Builder.CreateCall(module->getFunction("getMultiname_d"), {context, rt1.first, mindx});
#else
				name = Builder.CreateCall3(module->getFunction("getMultiname_d"), context, rt1.first, mindx);
#endif
			else*/
			{
				abstract_value(module,Builder,rt1);
#ifdef LLVM_37
				name = Builder.CreateCall(module->getFunction("getMultiname"), {context, rt1.first, constnull, mindx});
#else
				name = Builder.CreateCall4(module->getFunction("getMultiname"), context, rt1.first, constnull, mindx);
#endif
			}
		}
		else if(rtdata==2)
		{
			stack_entry rt1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
			stack_entry rt2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
			abstract_value(module,Builder,rt1);
			abstract_value(module,Builder,rt2);
#ifdef LLVM_37
			name = Builder.CreateCall(module->getFunction("getMultiname"), {context, rt1.first, rt2.first, mindx});
#else
			name = Builder.CreateCall4(module->getFunction("getMultiname"), context, rt1.first, rt2.first, mindx);
#endif
		}
		else
			assert(false);
	}
	return name;
}

inline pair<unsigned int,STACK_TYPE> method_info::popTypeFromStack(static_stack_types_vector& stack, unsigned int local_ip) const
{
	pair<unsigned int, STACK_TYPE> ret;
	if(!stack.empty())
	{
		ret=stack.back();
		stack.pop_back();
	}
	else
		ret=make_pair(local_ip,STACK_OBJECT);

	return ret;
}

block_info::block_info(const method_info* mi, const char* blockName)
{
	BB=llvm::BasicBlock::Create(getVm(getSys())->llvm_context(), blockName, mi->llvmf);
	locals_start.resize(mi->body->local_count,STACK_NONE);
	locals_reset.resize(mi->body->local_count,false);
	locals_used.resize(mi->body->local_count,false);
}

void method_info::addBlock(map<unsigned int,block_info>& blocks, unsigned int ip, const char* blockName)
{
	if(blocks.find(ip)==blocks.end())
		blocks.insert(make_pair(ip, block_info(this, blockName)));
}

struct method_info::BuilderWrapper
{
	llvm::IRBuilder<>& Builder;
	BuilderWrapper(llvm::IRBuilder<>& B) : Builder(B) { }
};

void method_info::doAnalysis(std::map<unsigned int,block_info>& blocks, BuilderWrapper& builderWrapper)
{
	llvm::IRBuilder<>& Builder = builderWrapper.Builder;
	bool stop;
	stringstream code(body->code);
	for (unsigned int i=0;i<body->exceptions.size();i++)
	{
		exception_info_abc& exc=body->exceptions[i];
		LOG(LOG_TRACE,"Exception handler: from " << exc.from << " to " << exc.to << " handled by " << exc.target);
	}
	//We try to analyze the blocks first to find if locals can survive the jumps
	while(1)
	{
		//This is initialized to true so that on first iteration the entry block is used
		bool fallthrough=true;
		code.clear();
		code.seekg(0);
		stop=true;
		block_info* cur_block=NULL;
		static_stack_types_vector static_stack_types;
		unsigned int local_ip=0;
		u30 localIndex=0;
		u8 opcode;

		while(1)
		{
			local_ip=code.tellg();
			code >> opcode;
			if(code.eof())
				break;

			/* check if the local_ip is the beginning of a catch block */
			for (unsigned int i=0;i<body->exceptions.size();i++)
			{
				exception_info_abc& exc=body->exceptions[i];
				if(exc.target == local_ip)
				{
					addBlock(blocks,local_ip,"catch");
					static_stack_types.clear();
					cur_block=&blocks[local_ip];
					LOG(LOG_TRACE,"New block at " << local_ip);
					assert(!fallthrough);//we never enter a catch block by falling through
					break;
				}
			}

			//Check if we are expecting a new block start
			map<unsigned int,block_info>::iterator it=blocks.find(local_ip);
			if(it!=blocks.end())
			{
				block_info* next_block = &it->second;
				if(cur_block && fallthrough)
				{
					//we can get from cur_block to next_block
					next_block->preds.insert(cur_block);
					cur_block->seqs.insert(next_block);
				}
				cur_block=next_block;
				LOG(LOG_TRACE,"New block at " << local_ip);
				cur_block->locals=cur_block->locals_start;
				fallthrough=true;
				static_stack_types.clear();
			}
			else if(!fallthrough)
			{
				//the last instruction was a branch which cannot fallthrough,
				//but there is no new block registered at local_ip.
				//The only way that the next instructions are reachable
				//is when we have a label here.
				//ignore debugfile/debugline on the way
				switch(opcode)
				{
				case 0xef: /*debug*/
				case 0xf0: /*debugline*/
				case 0xf1: /*debugfile*/
				case 0x09: /*label*/
					break;
				default:
					//find the next block after local_ip
					map<unsigned int,block_info>::iterator bit=blocks.lower_bound(local_ip);
					if(bit==blocks.end())
					{
						LOG(LOG_TRACE,"Ignoring trailing opcodes at " << local_ip);
						break; //there is no block after this local_ip
					}
					else
					{
						unsigned int next_ip = bit->first;
						LOG(LOG_TRACE,"Ignoring from " << local_ip << " to " << next_ip);
						code.seekg(next_ip,ios_base::beg);
						continue;
					}
				}
			}
			switch(opcode)
			{
				case 0x03: //throw
				{

					//see also returnvoid
					fallthrough=false;
					static_stack_types.clear();
					for(unsigned int i=0;i<body->local_count;i++)
					{
						if(cur_block->locals_used[i]==false)
							cur_block->locals_reset[i]=true;
					}
					break;
				}
				case 0x04: //getsuper
				case 0x05: //setsuper
				{
					u30 t;
					code >> t;
					static_stack_types.clear();
					break;
				}
				case 0x06: //dxns
				{
					u30 t;
					code >> t;
					break;
				}
				case 0x07: //dxnslate
				{
					popTypeFromStack(static_stack_types,local_ip);
					break;
				}
				case 0x08: //kill
				{
					LOG(LOG_TRACE, "block analysis: kill" );
					u30 t;
					code >> t;

					cur_block->locals[t]=STACK_NONE;
					break;
				}
				case 0x09: //label
				{
					//Create a new block and insert it in the mapping
					unsigned int here=local_ip;
					LOG(LOG_TRACE,"create label at " << here);
					if(blocks.find(local_ip) == blocks.end())
					{
						addBlock(blocks,here,"label");
						//rewind one opcode, so that cur_block is set to the newly created block
						//this is different from the branch opcodes, because they create new branches
						//after local_ip, where 'label' creates a new branch at local_ip
						code.seekg(-1,ios_base::cur);
					}

					break;
				}
				case 0x0c: //ifnlt
				case 0x0d: //ifnle
				case 0x0e: //ifngt
				case 0x0f: //ifnge
				case 0x10: //jump
				case 0x11: //iftrue
				case 0x12: //iffalse
				case 0x13: //ifeq
				case 0x14: //ifne
				case 0x15: //iflt
				case 0x16: //ifle
				case 0x17: //ifge
				case 0x18: //ifgt
				case 0x19: //ifstricteq
				case 0x1a: //ifstrictne
				{
					LOG(LOG_TRACE,"block analysis: branches " << opcode);
					//TODO: implement common data comparison
					fallthrough=true;
					s24 t;
					code >> t;

					int here=code.tellg();
					int dest=here+t;
					if(opcode == 0x10 /*jump*/)
					{
						fallthrough = false;
					}
					else
					{
						//Even though a 'jump' may not lead us to 'fall', we have to create
						//this block to continue analysing. This code may be branched to by a later
						//instruction
						//Create a block for the fallthrough code and insert in the mapping
						addBlock(blocks,here,"fall");
					}

					//if the destination lies before this opcode, the destination block must
					//already exist
					assert(dest > here || blocks.find(dest) != blocks.end());
					//And for the branch destination, if they are not in the blocks mapping
					addBlock(blocks,dest,"then");
					blocks[dest].preds.insert(cur_block);
					cur_block->seqs.insert(&blocks[dest]);

					static_stack_types.clear();
					break;
				}
				case 0x1b: //lookupswitch
				{
					LOG(LOG_TRACE, "synt lookupswitch" );
					fallthrough=false;

					int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
					s24 t;
					code >> t;
					int defaultdest=here+t;
					LOG(LOG_TRACE,"default " << int(t));
					u30 count;
					code >> count;
					LOG(LOG_TRACE,"count "<< int(count));
					vector<s24> offsets(count+1);
					for(unsigned int i=0;i<count+1;i++)
					{
						code >> offsets[i];
						LOG(LOG_TRACE,"Case " << i << " " << offsets[i]);
					}
					static_stack_types.clear();

					assert(defaultdest > here || blocks.find(defaultdest) != blocks.end());
					addBlock(blocks,defaultdest,"switchdefault");
					blocks[defaultdest].preds.insert(cur_block);
					cur_block->seqs.insert(&blocks[defaultdest]);

					for(unsigned int i=0;i<offsets.size();i++)
					{
						int casedest=here+offsets[i];
						assert(casedest > here || blocks.find(casedest) != blocks.end());
						addBlock(blocks,casedest,"switchcase");
						blocks[casedest].preds.insert(cur_block);
						cur_block->seqs.insert(&blocks[casedest]);
					}

					break;
				}
				case 0x1c: //pushwith
				case 0x30: //pushscope
				{
					static_stack_types.clear();
					break;
				}
				case 0x31: //pushnamespace
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					break;
				}
				case 0x1d: //popscope
				{
					break;
				}
				case 0x1e: //nextname
				{
					popTypeFromStack(static_stack_types,local_ip);
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x1f: //hasnext
				{
					popTypeFromStack(static_stack_types,local_ip);
					popTypeFromStack(static_stack_types,local_ip);
					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0x20: //pushnull
				case 0x21: //pushundefined
				case 0x28: //pushnan
				{
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x23: //nextvalue
				{
					popTypeFromStack(static_stack_types,local_ip);
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x24: //pushbyte
				{
					int8_t t;
					code.read((char*)&t,1);
					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0x25: //pushshort
				{
					// specs say pushshort is a u30, but it's really a u32
					// see abc_interpreter on pushshort
					u32 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0x26: //pushtrue
				case 0x27: //pushfalse
				{
					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0x29: //pop
				{
					popTypeFromStack(static_stack_types,local_ip);
					break;
				}
				case 0x2a: //dup
				{
					pair <int, STACK_TYPE> val=popTypeFromStack(static_stack_types,local_ip);
					static_stack_types.push_back(val);
					val.first=local_ip;
					static_stack_types.push_back(val);

					cur_block->checkProactiveCasting(local_ip,val.second);
					break;
				}
				case 0x2b: //swap
				{
					pair <int, STACK_TYPE> t1,t2;
					t1=popTypeFromStack(static_stack_types,local_ip);
					t2=popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(t1);
					static_stack_types.push_back(t2);
					break;
				}
				case 0x2c: //pushstring
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x2d: //pushint
				case 0x2e: //pushuint
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0x2f: //pushdouble
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0x32: //hasnext2
				{
					u30 t;
					code >> t;
					cur_block->locals[t]=STACK_NONE;
					code >> t;
					cur_block->locals[t]=STACK_NONE;
					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0x40: //newfunction
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x41: //call
				case 0x42: //construct
				case 0x49: //constructsuper
				case 0x53: //constructgenerictype
				{
					static_stack_types.clear();
					u30 t;
					code >> t;
					break;
				}
				case 0x45: //callsuper
				case 0x4c: //callproplex
				case 0x46: //callproperty
				case 0x4a: //constructprop
				case 0x4e: //callsupervoid
				case 0x4f: //callpropvoid
				{
					static_stack_types.clear();
					u30 t;
					code >> t;
					code >> t;
					break;
				}
				case 0x47: //returnvoid
				case 0x48: //returnvalue
				{
					fallthrough=false;
					static_stack_types.clear();
					for(unsigned int i=0;i<body->local_count;i++)
					{
						if(cur_block->locals_used[i]==false)
							cur_block->locals_reset[i]=true;
					}
					break;
				}
				case 0x55: //newobject
				case 0x56: //newarray
				case 0x58: //newclass
				case 0x59: //getdescendants
				case 0x60: //getlex
				{
					static_stack_types.clear();
					u30 t;
					code >> t;
					break;
				}
				case 0x5d: //findpropstrict
				case 0x5e: //findproperty
				{
					u30 t;
					code >> t;
					consumeStackForRTMultiname(static_stack_types,t);
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x57: //newactivation
				{
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x5a: //newcatch
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x61: //setproperty
				{
					u30 t;
					code >> t;
					//the value
					popTypeFromStack(static_stack_types,local_ip);
					//the rt multiname stuff
					consumeStackForRTMultiname(static_stack_types,t);
					//the object
					popTypeFromStack(static_stack_types,local_ip);
					break;
				}
				case 0x64: //getglobalscope
				{
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x65: //getscopeobject
				{
					u30 t;
					code >> t;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x66: //getproperty
				{
					u30 t;
					code >> t;
					consumeStackForRTMultiname(static_stack_types,t);
					popTypeFromStack(static_stack_types,local_ip);
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					break;
				}
				case 0x68: //initproperty
				{
					u30 t;
					code >> t;
					popTypeFromStack(static_stack_types,local_ip);
					consumeStackForRTMultiname(static_stack_types,t);
					popTypeFromStack(static_stack_types,local_ip);
					break;
				}
				case 0x6a: //deleteproperty
				{
					u30 t;
					code >> t;
					popTypeFromStack(static_stack_types,local_ip);
					consumeStackForRTMultiname(static_stack_types,t);
					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					break;
				}
				case 0x6c: //getslot
				{
					u30 t;
					code >> t;
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x6d: //setslot
				{
					u30 t;
					code >> t;
					popTypeFromStack(static_stack_types,local_ip);
					popTypeFromStack(static_stack_types,local_ip);
					break;
				}
				case 0x72: //esc_xattr
				case 0x70: //convert_s
				{
					popTypeFromStack(static_stack_types,local_ip);
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x73: //convert_i
				case 0x74: //convert_u
				{
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0x75:
				{
					//convert_d
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0x76: //convert_b
				{
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0x78: //checkfilter
				{
					break;
				}
				case 0x80: //coerce
				{
					static_stack_types.clear();
					u30 t;
					code >> t;
					break;
				}
				case 0x82: //coerce_a
				{
					break;
				}
				case 0x85: //coerce_s
				{
					popTypeFromStack(static_stack_types,local_ip).second;
					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x87: //astypelate
				{
					popTypeFromStack(static_stack_types,local_ip).second;
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x90: //negate
				{
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0x95: //typeof
				{
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
					cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					break;
				}
				case 0x91: //increment
				case 0x93: //decrement
				{
					popTypeFromStack(static_stack_types,local_ip).second;
					static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0xc0: //increment_i
				case 0xc1: //decrement_i
				{
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0x96: //not
				{
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0x97: //bitnot
				{
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0xa0: //add
				{
					STACK_TYPE t1,t2;
					t1=popTypeFromStack(static_stack_types,local_ip).second;
					t2=popTypeFromStack(static_stack_types,local_ip).second;

					if(t1==STACK_OBJECT || t2==STACK_OBJECT)
					{
						static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
						cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
					}
					else /* both t1 and t2 are UINT or INT or NUMBER or BOOLEAN */
					{
						static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
						cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					}

					break;
				}
				case 0xa1: //subtract
				case 0xa2: //multiply
				{
					STACK_TYPE t1,t2;
					t1=popTypeFromStack(static_stack_types,local_ip).second;
					t2=popTypeFromStack(static_stack_types,local_ip).second;

					if(t1==STACK_INT && t2==STACK_INT)
					{
						static_stack_types.push_back(make_pair(local_ip,STACK_INT));
						cur_block->checkProactiveCasting(local_ip,STACK_INT);
					}
					else
					{
						static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
						cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					}
					break;
				}
				case 0xa3: //divide
				{
					popTypeFromStack(static_stack_types,local_ip).second;
					popTypeFromStack(static_stack_types,local_ip).second;

					static_stack_types.push_back(make_pair(local_ip,STACK_NUMBER));
					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0xa4: //modulo
				{
					popTypeFromStack(static_stack_types,local_ip).second;
					popTypeFromStack(static_stack_types,local_ip).second;

					cur_block->checkProactiveCasting(local_ip,STACK_NUMBER);
					break;
				}
				case 0xa5: //lshift
				case 0xa6: //rshift
				case 0xa7: //urshift
				case 0xa8: //bitand
				case 0xa9: //bitor
				case 0xaa: //bitxor
				{
					pair<unsigned int, STACK_TYPE> t1=popTypeFromStack(static_stack_types,local_ip);
					if(t1.first!=local_ip)
						cur_block->push_types[t1.first]=STACK_INT;
					pair<unsigned int, STACK_TYPE> t2=popTypeFromStack(static_stack_types,local_ip);
					if(t2.first!=local_ip)
						cur_block->push_types[t2.first]=STACK_INT;

					static_stack_types.push_back(make_pair(local_ip,STACK_INT));
					cur_block->checkProactiveCasting(local_ip,STACK_INT);
					break;
				}
				case 0xab: //equals
				case 0xac: //strictequals
				case 0xad: //lessthan
				case 0xae: //lessequals
				case 0xaf: //greaterthan
				case 0xb0: //greaterequals
				case 0xb1: //instanceOf
				case 0xb3: //istypelate
				case 0xb4: //in
				{
					popTypeFromStack(static_stack_types,local_ip);
					popTypeFromStack(static_stack_types,local_ip);

					static_stack_types.push_back(make_pair(local_ip,STACK_BOOLEAN));
					cur_block->checkProactiveCasting(local_ip,STACK_BOOLEAN);
					break;
				}
				case 0xc2: //inclocal_i
				{
					static_stack_types.clear();
					u30 t;
					code >> t;
					break;
				}
				case 0x62: //getlocal
					code >> localIndex;
					// falls through
				case 0xd0: //getlocal_n
				case 0xd1:
				case 0xd2:
				case 0xd3:
				{
					int i=(opcode==0x62)?((uint32_t)localIndex):(opcode&3);
					cur_block->locals_used[i]=true;
					if(cur_block->locals[i]==STACK_NONE)
					{
						static_stack_types.push_back(make_pair(local_ip,STACK_OBJECT));
						cur_block->checkProactiveCasting(local_ip,STACK_OBJECT);
						cur_block->locals[i]=STACK_OBJECT;
					}
					else
					{
						static_stack_types.push_back(make_pair(local_ip,cur_block->locals[i]));
						cur_block->checkProactiveCasting(local_ip,cur_block->locals[i]);
					}
					break;
				}
				case 0x63: //setlocal
					code >> localIndex;
					// falls through
				case 0xd4: //setlocal_n
				case 0xd5:
				case 0xd6:
				case 0xd7:
				{
					int i=(opcode==0x63)?((uint32_t)localIndex):(opcode&3);
					cur_block->locals_used[i]=true;
					STACK_TYPE t;
					if(!static_stack_types.empty())
					{
						t=static_stack_types.back().second;
						static_stack_types.pop_back();
					}
					else
						t=STACK_OBJECT;

					if(cur_block->locals[i]==STACK_NONE)
						cur_block->locals_reset[i]=true;
					cur_block->locals[i]=t;
					break;
				}
				case 0xef: //debug
				{
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
				case 0xf0: //debugline
				case 0xf1: //debugfile
				{
					u30 t;
					code >> t;
					break;
				}
				default:
					LOG(LOG_ERROR,"Not implemented instruction @" << code.tellg());
					u8 a,b,c;
					code >> a >> b >> c;
					LOG(LOG_ERROR,"dump " << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
							<< (unsigned int)b << ' ' << (unsigned int)c);
			}
		}

		//remove unreachable blocks
		map<unsigned int,block_info>::iterator bit=blocks.begin();
		bool changed = true;
		while(changed)
		{
			changed = false;
	                for(;bit!=blocks.end();)
			{
				block_info& cur=bit->second;
				if(bit->first == 0 || !cur.preds.empty())
				{ //never remove the first block or blocks with nonempty preds
					++bit;
					continue;
				}
				LOG(LOG_TRACE,"Removing unreachable block at " << bit->first << ": " << cur);
				//remove me from my seqs
				set<block_info*>::iterator seq=cur.seqs.begin();
				for(;seq!=cur.seqs.end();++seq)
					(*seq)->preds.erase(&cur);

				//remove block from method and free it
				cur.BB->eraseFromParent();
				//erase from blocks map
				map<unsigned int,block_info>::iterator toerase = bit;
				++bit;
				blocks.erase(toerase);

				changed = true;
			}
		}

		//Let's propagate locals reset information
		//cur.locals_reset[i] is true when
		// 1) it is not used in the current block and is reset in all subsequent blocks
		// 2) it is reset in the current block
		// i.e. it is true when entering the block will reset the local in any case
		while(1)
		{
			stop=true;
			for(bit=blocks.begin();bit!=blocks.end();++bit)
			{
				block_info& cur=bit->second;
				std::vector<bool> new_reset(body->local_count,false);
				for(unsigned int i=0;i<body->local_count;i++)
				{
					if(cur.locals_used[i]==false)
						new_reset[i]=true;
				}
				set<block_info*>::iterator seq=cur.seqs.begin();
				for(;seq!=cur.seqs.end();++seq)
				{
					for(unsigned int i=0;i<body->local_count;i++)
					{
						if((*seq)->locals_reset[i]==false)
							new_reset[i]=false;
					}
				}
				for(unsigned int i=0;i<body->local_count;i++)
				{
					if(cur.locals_reset[i]==true)
						new_reset[i]=true;
				}
				if(new_reset!=cur.locals_reset)
				{
					stop=false;
					cur.locals_reset=new_reset;
				}
			}

			if(stop)
				break;
		}

		//We can now search for locals that can be saved
		//If every predecessor blocks agree with the type of a local we pass it over
		for(bit=blocks.begin();bit!=blocks.end();++bit)
		{
			//locals_start for the first block must not be updated,
			//they are the parameters
			if(bit->first == 0)
				continue;
			block_info& cur=bit->second;
			vector<STACK_TYPE> new_start;
			new_start.resize(body->local_count,STACK_NONE);
			//if all preceeding block end with the same type of local[i]
			//then we take that as the new_start[i]
			if(!cur.preds.empty())
			{
				set<block_info*>::iterator pred=cur.preds.begin();
				new_start=(*pred)->locals;
				++pred;
				for(;pred!=cur.preds.end();++pred)
				{
					for(unsigned int j=0;j<(*pred)->locals.size();j++)
					{
						if(new_start[j]!=(*pred)->locals[j])
							new_start[j]=STACK_NONE;
					}
				}
			}
			//It's not useful to sync variables that are going to be resetted
			//(where 'reset' means 'written to before any read')
			for(unsigned int i=0;i<body->local_count;i++)
			{
				if(cur.locals_reset[i])
					new_start[i]=STACK_NONE;
			}

			if(new_start!=cur.locals_start)
			{
				stop=false;
				cur.locals_start=new_start;
			}
		}

		if(stop)
			break;
	}

	//For all locals that are transfered to a block, create a memory region where the preceeding block writes
	//the local to
	map<unsigned int,block_info>::iterator bit;
	for(bit=blocks.begin();bit!=blocks.end();++bit)
	{
		//For the first block, we won't have to allocate space
		//as those are direclty read from the parameters
		if(bit->first == 0)
			continue;
		block_info& cur=bit->second;
		cur.locals_start_obj.resize(cur.locals_start.size(),NULL);
		for(unsigned int i=0;i<cur.locals_start.size();i++)
		{
			switch(cur.locals_start[i])
			{
				case STACK_NONE:
					break;
				case STACK_OBJECT:
					cur.locals_start_obj[i]=Builder.CreateAlloca(voidptr_type);
					break;
				case STACK_UINT:
				case STACK_INT:
					cur.locals_start_obj[i]=Builder.CreateAlloca(int_type);
					break;
				case STACK_NUMBER:
					cur.locals_start_obj[i]=Builder.CreateAlloca(number_type);
					break;
				case STACK_BOOLEAN:
					cur.locals_start_obj[i]=Builder.CreateAlloca(bool_type);
					break;
				default:
					throw RunTimeException("Unsupported object type");
			}
		}
	}
}

SyntheticFunction::synt_function method_info::synt_method(SystemState* sys)
{
	if(f)
		return f;

	string method_name="method";
	method_name+=sys->getStringFromUniqueId(context->getString(info.name)).raw_buf();
	if(!body)
	{
		LOG(LOG_CALLS,"Method " << method_name << " should be intrinsic");
		return NULL;
	}
	llvm::ExecutionEngine* ex=getVm(sys)->ex;
	llvm::Module* module=getVm(sys)->module;
	llvm::LLVMContext& llvm_context=getVm(sys)->llvm_context();
	llvm::FunctionType* method_type=synt_method_prototype(ex);
	llvmf=llvm::Function::Create(method_type,llvm::Function::ExternalLinkage,method_name,getVm(sys)->module);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm_context,"entry", llvmf);
	llvm::IRBuilder<> Builder(llvm_context);
	Builder.SetInsertPoint(BB);

	//We define a couple of variables that will be used a lot
	llvm::Constant* constant;
	llvm::Constant* constant2;
	llvm::Constant* constant3;
	llvm::Constant* constant4;
	llvm::Value* value;
	//let's give access to method data to llvm
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)this);
	llvm::Value* th = llvm::ConstantExpr::getIntToPtr(constant, voidptr_type);

#ifdef LLVM_50
	auto it=llvmf->arg_begin();
#else
	auto it=llvmf->getArgumentList().begin();
#endif
	//The first and only argument to this function is the call_context*
#ifdef LLVM_38
	llvm::Value* context=&(*it);
#else
	llvm::Value* context=it;
#endif

	//let's give access to local data storage
	value=Builder.CreateStructGEP(
#ifdef LLVM_37
		nullptr,
#endif
		context,0);
	llvm::Value* locals=Builder.CreateLoad(value);

	//the stack is statically handled as much as possible to allow llvm optimizations
	//on branch and on interpreted/jitted code transition it is synchronized with the dynamic one
	vector<stack_entry> static_stack;
	static_stack.reserve(body->max_stack);
	//Get the pointer to the dynamic stack
	value=Builder.CreateStructGEP(
#ifdef LLVM_37
		nullptr,
#endif
		context,1);
	llvm::Value* dynamic_stack=Builder.CreateLoad(value);
	//Get the index of the dynamic stack
	llvm::Value* dynamic_stack_index=Builder.CreateStructGEP(
#ifdef LLVM_37
		nullptr,
#endif
		context,2);

	llvm::Value* exec_pos=Builder.CreateStructGEP(
#ifdef LLVM_37
		nullptr,
#endif
		context,3);

/*	//Allocate a fast dynamic stack based on LLVM alloca instruction
	//This is used on branches
	vactor<llvm::Value*> fast_dynamic_stack(body->max_stack);
	for(int i=0;i<body->max_stack;i++)
		fast_dynamic_stack[i]=Builder.CreateAlloca(voidptr_type);
	//Allocate also a stack pointer
	llvm::Value* fast_dynamic_stack_index=*/

	//the scope stack is not accessible to llvm code

	//Creating a mapping between blocks and starting address
	map<unsigned int,block_info> blocks;

	//Let's build a block for the real function code
	addBlock(blocks, 0, "begin");

	/* Setup locals. Those hold the parameters on entry.
	 * SyntheticFunction::call() coerces the objects to those types.
	 * Then we get a primitive type, we diretly load its 'val' member and
	 * set the corresponding static_local's type. */
	blocks[0].locals_start.resize(body->local_count,STACK_NONE);
	blocks[0].locals_start_obj.resize(body->local_count,NULL);
#define LOAD_LOCALPTR \
	/*local[0] corresponds to 'this', arguments start at 1*/ \
	constant = llvm::ConstantInt::get(int_type, i+1); \
	llvm::Value* t=Builder.CreateGEP(locals,constant); /*Compute locals[i] = locals + i*/ \
        t=Builder.CreateLoad(t,"Primitive*"); /*Load Primitive* n = locals[i]*/

	for(unsigned i=0;i<paramTypes.size();++i)
	{
		if(paramTypes[i] == Class<Number>::getClass(wrk->getSystemState()))
		{
			/* yield t = locals[i+1] */
			LOAD_LOCALPTR
			/*calc n+offsetof(Number,val) = &n->val*/
			t=Builder.CreateGEP(t, llvm::ConstantInt::get(int_type, offsetof(Number,dval)));
			t=Builder.CreateBitCast(t,numberptr_type); //cast t from int8* to number*
			blocks[0].locals_start[i+1] = STACK_NUMBER;
			//locals_start_obj should hold the pointer to the local's value
			blocks[0].locals_start_obj[i+1] = t;
			LOG(LOG_TRACE,"found STACK_NUMBER parameter for local " << i+1);
		}
		else if(paramTypes[i] == Class<Integer>::getClass(wrk->getSystemState()))
		{
			/* yield t = locals[i+1] */
			LOAD_LOCALPTR
			/*calc n+offsetof(Number,val) = &n->val*/
			t=Builder.CreateGEP(t, llvm::ConstantInt::get(int_type, offsetof(Integer,val)));
			t=Builder.CreateBitCast(t,intptr_type); //cast t from int8* to int32_t*
			blocks[0].locals_start[i+1] = STACK_INT;
			//locals_start_obj should hold the pointer to the local's value
			blocks[0].locals_start_obj[i+1] = t;
		}
		else if(paramTypes[i] == Class<UInteger>::getClass(wrk->getSystemState()))
		{
			/* yield t = locals[i+1] */
			LOAD_LOCALPTR
			/*calc n+offsetof(Number,val) = &n->val*/
			t=Builder.CreateGEP(t, llvm::ConstantInt::get(int_type, offsetof(UInteger,val)));
			t=Builder.CreateBitCast(t,intptr_type); //cast t from int8* to uint32_t*
			blocks[0].locals_start[i+1] = STACK_UINT;
			//locals_start_obj should hold the pointer to the local's value
			blocks[0].locals_start_obj[i+1] = t;
		}
		else if(paramTypes[i] == Class<Boolean>::getClass(wrk->getSystemState()))
		{
			/* yield t = locals[i+1] */
			LOAD_LOCALPTR
			/*calc n+offsetof(Number,val) = &n->val*/
			t=Builder.CreateGEP(t, llvm::ConstantInt::get(int_type, offsetof(Boolean,val)));
			t=Builder.CreateBitCast(t,boolptr_type); //cast t from int8* to bool*
			blocks[0].locals_start[i+1] = STACK_BOOLEAN;
			//locals_start_obj should hold the pointer to the local's value
			blocks[0].locals_start_obj[i+1] = t;
		}

	}
#undef LOAD_LOCALPTR
	BuilderWrapper wrapper(Builder);
	doAnalysis(blocks,wrapper);

	//Let's reset the stream
	stringstream code(body->code);
	vector<stack_entry> static_locals(body->local_count,make_stack_entry(NULL,STACK_NONE));
	block_info* cur_block=NULL;
	static_stack.clear();

	/* exception handling -> jump to exec_pos. exec_pos = 0 corresponds to normal execution */
	if(body->exceptions.size())
	{
		llvm::Value* vexec_pos = Builder.CreateLoad(exec_pos);
		llvm::BasicBlock* Default=llvm::BasicBlock::Create(llvm_context,"exec_pos_error", llvmf);
		llvm::SwitchInst* sw=Builder.CreateSwitch(vexec_pos,Default);
		/* it is an error if exec_pos is not 0 or one of the catch handlers */
		Builder.SetInsertPoint(Default);
#ifndef NDEBUG
		Builder.CreateCall(module->getFunction("wrong_exec_pos"));
#endif
		Builder.CreateBr(blocks[0].BB);

		/* start with ip zero if exec_pos is zero */
		llvm::BasicBlock* Case=llvm::BasicBlock::Create(llvm_context,"exec_pos_handler_init", llvmf);
		llvm::ConstantInt* constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(int_type, 0));
		sw->addCase(constant, Case);
		Builder.SetInsertPoint(Case);
		Builder.CreateBr(blocks[0].BB);

		/* start at an catch handler if its ip is given in exec_pos*/
		for (unsigned int i=0;i<body->exceptions.size();i++)
		{
			exception_info_abc& exc=body->exceptions[i];
			Case=llvm::BasicBlock::Create(llvm_context,"exec_pos_handler", llvmf);
			constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(int_type, exc.target));
			sw->addCase(constant, Case);
			Builder.SetInsertPoint(Case);
			Builder.CreateBr(blocks[exc.target].BB);
		}
	}
	else
	{	//this function has no exception handling, so just start from the beginning
		Builder.CreateBr(blocks[0].BB);
	}



	u8 opcode;
	bool last_is_branch=true; //the 'begin' block branching to blocks[0].BB is handled above

	int local_ip=0;
	u30 localIndex;
	//Each case block builds the correct parameters for the interpreter function and call it
	while(1)
	{
		local_ip=code.tellg();
		code >> opcode;
		if(code.eof())
			break;
		//Check if we are expecting a new block start
		map<unsigned int,block_info>::iterator it=blocks.find(local_ip);
		if(it!=blocks.end())
		{
			if(!last_is_branch)
			{
				//this happens, for example, at the end of catch handlers
				LOG(LOG_TRACE, "Last instruction before a new block was not a branch.");
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,it->second);
				Builder.CreateBr(it->second.BB);
			}
			cur_block=&it->second;
			LOG(LOG_TRACE,"Starting block at "<<local_ip << " " << *cur_block);
			Builder.SetInsertPoint(cur_block->BB);

			if(!cur_block->locals_start.empty())
			{
				//Generate prologue, LLVM should optimize register usage
				assert(static_locals.size()==cur_block->locals_start.size());
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					static_locals[i].second=cur_block->locals_start[i];
					if(cur_block->locals_start[i]!=STACK_NONE)
						static_locals[i].first=Builder.CreateLoad(cur_block->locals_start_obj[i]);
				}
			}

			last_is_branch=false;
		}
		else if(last_is_branch)
		{
			LOG(LOG_TRACE,"Ignoring at " << local_ip);
			continue;
		}

		if(body->exceptions.size())
		{ //if this function has a try/catch block, record the local_ip, so we can figure out where we were
		  //in case of an exception to find the right catch
		  //TODO: would be enough to set this once on enter of try-block
			constant = llvm::ConstantInt::get(int_type, local_ip);
			Builder.CreateStore(constant,exec_pos);
		}
		switch(opcode)
		{
			case 0x03:
			{
				//throw
				LOG(LOG_TRACE, "synt throw" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//syncLocals(module,Builder,static_locals,locals,cur_block->locals,*cur_block);
				Builder.CreateCall(module->getFunction("throw"), context);
				//Right now we set up like we do for retunrvoid
				last_is_branch=true;
				constant = llvm::ConstantInt::get(ptr_type, 0);
				value = llvm::ConstantExpr::getIntToPtr(constant, voidptr_type);
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x04:
			{
				//getsuper
				LOG(LOG_TRACE, "synt getsuper" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("getSuper"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("getSuper"), context, constant);
#endif
				break;
			}
			case 0x05:
			{
				//setsuper
				LOG(LOG_TRACE, "synt setsuper" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("setSuper"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("setSuper"), context, constant);
#endif
				break;
			}
			case 0x06:
			{
				//dxns
				LOG(LOG_TRACE, "synt dxns" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("dxns"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("dxns"), context, constant);
#endif
				break;
			}
			case 0x07:
			{
				//dxnslate
				LOG(LOG_TRACE, "synt dxnslate" );
				stack_entry v=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("dxns"), {context, v.first});
#else
				Builder.CreateCall2(module->getFunction("dxns"), context, v.first);
#endif
				break;
			}
			case 0x08:
			{
				//kill
				u30 t;
				code >> t;
				LOG(LOG_TRACE, "synt kill " << t );
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall(module->getFunction("kill"), constant);
				}
				int i=t;
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(module->getFunction("decRef"), static_locals[i].first);

				static_locals[i].second=STACK_NONE;

				break;
			}
			case 0x09:
			{
				//label
				//Create a new block and insert it in the mapping
				LOG(LOG_TRACE, "synt label" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("label"));
				break;
			}
			case 0x0c:
			{
				//ifnlt
				LOG(LOG_TRACE, "synt ifnlt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				llvm::Value* cond;
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					cond=Builder.CreateICmpSGE(v2.first,v1.first); //GE == NLT
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifNLT"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifNLT"), v1.first, v2.first);
#endif
				}

				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0d:
			{
				//ifnle
				LOG(LOG_TRACE, "synt ifnle");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				llvm::Value* cond=Builder.CreateCall(module->getFunction("ifNLE"), {v1.first, v2.first});
#else
				llvm::Value* cond=Builder.CreateCall2(module->getFunction("ifNLE"), v1.first, v2.first);
#endif
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0e:
			{
				//ifngt
				LOG(LOG_TRACE, "synt ifngt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				llvm::Value* cond=Builder.CreateCall(module->getFunction("ifNGT"), {v1.first, v2.first});
#else
				llvm::Value* cond=Builder.CreateCall2(module->getFunction("ifNGT"), v1.first, v2.first);
#endif
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0f:
			{
				//ifnge
				LOG(LOG_TRACE, "synt ifnge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Make comparision

				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				llvm::Value* cond=Builder.CreateCall(module->getFunction("ifNGE"), {v1.first, v2.first});
#else
				llvm::Value* cond=Builder.CreateCall2(module->getFunction("ifNGE"), v1.first, v2.first);
#endif
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x10:
			{
				//jump
				last_is_branch=true;

				s24 t;
				code >> t;
				LOG(LOG_TRACE, "synt jump " << t );
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall(module->getFunction("jump"), constant);
				}
				int here=code.tellg();
				int dest=here+t;
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x11:
			{
				//iftrue
				LOG(LOG_TRACE, "synt iftrue" );
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_BOOLEAN)
					cond=v1.first;
				else
				{
					abstract_value(module,Builder,v1);
					cond=Builder.CreateCall(module->getFunction("ifTrue"), v1.first);
				}
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x12:
			{
				//iffalse

				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(LOG_TRACE, "synt iffalse " << t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_BOOLEAN)
					cond=Builder.CreateNot(v1.first);
				else
				{
					abstract_value(module,Builder,v1);
					cond=Builder.CreateCall(module->getFunction("ifFalse"), v1.first);
				}
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x13:
			{
				//ifeq
				LOG(LOG_TRACE, "synt ifeq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				//Make comparision
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifEq"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifEq"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					cond=Builder.CreateFCmpOEQ(v1.first,v2.first);
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifEq"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifEq"), v1.first, v2.first);
#endif
				}

				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x14:
			{
				//ifne
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(LOG_TRACE, "synt ifne " << t );
			
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifNE_oi"), {v2.first, v1.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifNE_oi"), v2.first, v1.first);
#endif
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifNE_oi"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifNE_oi"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					cond=Builder.CreateFCmpONE(v1.first,v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					cond=Builder.CreateICmpNE(v1.first,v2.first);
				}
				else
				{
					//Abstract default
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifNE"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifNE"), v1.first, v2.first);
#endif
				}
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x15:
			{
				//iflt
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(LOG_TRACE, "synt iflt "<< t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLT"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLT"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLT_io"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLT_io"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLT_oi"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLT_oi"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					cond=Builder.CreateICmpSLT(v2.first,v1.first);
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLT"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLT"), v1.first, v2.first);
#endif
				}
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x16:
			{
				//ifle
				LOG(LOG_TRACE, "synt ifle" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLE"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLE"), v1.first, v2.first);
#endif
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					cond=Builder.CreateCall(module->getFunction("ifLE"), {v1.first, v2.first});
#else
					cond=Builder.CreateCall2(module->getFunction("ifLE"), v1.first, v2.first);
#endif
				}
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x17:
			{
				//ifgt
				LOG(LOG_TRACE, "synt ifgt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				cond=Builder.CreateCall(module->getFunction("ifGT"), {v1.first, v2.first});
#else
				cond=Builder.CreateCall2(module->getFunction("ifGT"), v1.first, v2.first);
#endif
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x18:
			{
				//ifge
				LOG(LOG_TRACE, "synt ifge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				cond=Builder.CreateCall(module->getFunction("ifGE"), {v1.first, v2.first});
#else
				cond=Builder.CreateCall2(module->getFunction("ifGE"), v1.first, v2.first);
#endif
			
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x19:
			{
				//ifstricteq
				LOG(LOG_TRACE, "synt ifstricteq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
#ifdef LLVM_37
				llvm::Value* cond=Builder.CreateCall(module->getFunction("ifStrictEq"), {v1, v2});
#else
				llvm::Value* cond=Builder.CreateCall2(module->getFunction("ifStrictEq"), v1, v2);
#endif

				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				LOG(LOG_TRACE, "synt ifstrictne" );
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				cond=Builder.CreateCall(module->getFunction("ifStrictNE"), {v1.first, v2.first});
#else
				cond=Builder.CreateCall2(module->getFunction("ifStrictNE"), v1.first, v2.first);
#endif

				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				LOG(LOG_TRACE, "synt lookupswitch" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("lookupswitch"));
				last_is_branch=true;

				int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
				s24 t;
				code >> t;
				int defaultdest=here+t;
				LOG(LOG_TRACE,"default " << int(t));
				u30 count;
				code >> count;
				LOG(LOG_TRACE,"count "<< int(count));
				vector<s24> offsets(count+1);
				for(unsigned int i=0;i<count+1;i++)
				{
					code >> offsets[i];
					LOG(LOG_TRACE,"Case " << i << " " << offsets[i]);
				}
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT);
				else if(e.second==STACK_OBJECT)
					e.first=Builder.CreateCall(module->getFunction("convert_i"),e.first);
				else
					throw UnsupportedException("Unsupported type for lookupswitch");
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				//Generate epilogue for default dest
				llvm::BasicBlock* Default=llvm::BasicBlock::Create(llvm_context,"epilogueDest", llvmf);

				llvm::SwitchInst* sw=Builder.CreateSwitch(e.first,Default);
				Builder.SetInsertPoint(Default);
				syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[defaultdest]);
				Builder.CreateBr(blocks[defaultdest].BB);

				for(unsigned int i=0;i<offsets.size();i++)
				{
					int casedest=here+offsets[i];
					llvm::ConstantInt* constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(int_type, i));
					llvm::BasicBlock* Case=llvm::BasicBlock::Create(llvm_context,"epilogueCase", llvmf);
					sw->addCase(constant,Case);
					Builder.SetInsertPoint(Case);
					syncLocals(module,Builder,static_locals,locals,cur_block->locals,blocks[casedest]);
					Builder.CreateBr(blocks[casedest].BB);
				}

				break;
			}
			case 0x1c:
			{
				//pushwith
				LOG(LOG_TRACE, "synt pushwith" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(module->getFunction("pushWith"), context);
				break;
			}
			case 0x1d:
			{
				//popscope
				LOG(LOG_TRACE, "synt popscope" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(module->getFunction("popScope"), context);
				break;
			}
			case 0x1e:
			{
				//nextname
				LOG(LOG_TRACE, "synt nextname" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("nextName"), {v1, v2});
#else
				value=Builder.CreateCall2(module->getFunction("nextName"), v1, v2);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x1f:
			{
				//hasnext
				LOG(LOG_TRACE, "synt hasnext" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("hasNext"), {v1, v2});
#else
				value=Builder.CreateCall2(module->getFunction("hasNext"), v1, v2);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x20:
			{
				//pushnull
				LOG(LOG_TRACE, "synt pushnull" );
				value=Builder.CreateCall(module->getFunction("pushNull"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x21:
			{
				//pushundefined
				LOG(LOG_TRACE, "synt pushundefined" );
				value=Builder.CreateCall(module->getFunction("pushUndefined"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x23:
			{
				//nextvalue
				LOG(LOG_TRACE, "synt nextvalue" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("nextValue"), {v1, v2});
#else
				value=Builder.CreateCall2(module->getFunction("nextValue"), v1, v2);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x24:
			{
				//pushbyte
				LOG(LOG_TRACE, "synt pushbyte" );
				int8_t t;
				code.read((char*)&t,1);
				constant = llvm::ConstantInt::get(int_type, (int)t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("pushByte"), constant);
				break;
			}
			case 0x25:
			{
				//pushshort
				LOG(LOG_TRACE, "synt pushshort" );
				//pushshort
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				u32 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("pushShort"), constant);
				break;
			}
			case 0x26:
			{
				//pushtrue
				LOG(LOG_TRACE, "synt pushtrue" );
				value=Builder.CreateCall(module->getFunction("pushTrue"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x27:
			{
				//pushfalse
				LOG(LOG_TRACE, "synt pushfalse" );
				value=Builder.CreateCall(module->getFunction("pushFalse"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x28:
			{
				//pushnan
				LOG(LOG_TRACE, "synt pushnan" );
				value=Builder.CreateCall(module->getFunction("pushNaN"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x29:
			{
				//pop
				LOG(LOG_TRACE, "synt pop" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("pop"));
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_OBJECT)
					Builder.CreateCall(module->getFunction("decRef"), e.first);
				break;
			}
			case 0x2a:
			{
				//dup
				LOG(LOG_TRACE, "synt dup" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("dup"));
				stack_entry e=static_stack_peek(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e);

				if(e.second==STACK_OBJECT)
					Builder.CreateCall(module->getFunction("incRef"), e.first);
				break;
			}
			case 0x2b:
			{
				//swap
				LOG(LOG_TRACE, "synt swap" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("swap"));
				stack_entry e1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry e2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e1);
				static_stack_push(static_stack,e2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				LOG(LOG_TRACE, "synt pushstring" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("pushString"), {context, constant});
#else
				value=Builder.CreateCall2(module->getFunction("pushString"), context, constant);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x2d:
			{
				//pushint
				LOG(LOG_TRACE, "synt pushint" );
				u30 t;
				code >> t;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
					Builder.CreateCall(module->getFunction("pushInt"), {context, constant});
#else
					Builder.CreateCall2(module->getFunction("pushInt"), context, constant);
#endif
				}
				s32 i=this->context->constant_pool.integer[t];
				constant = llvm::ConstantInt::get(int_type, i);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				break;
			}
			case 0x2e:
			{
				//pushuint
				LOG(LOG_TRACE, "synt pushuint" );
				u30 t;
				code >> t;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
					Builder.CreateCall(module->getFunction("pushUInt"), {context, constant});
#else
					Builder.CreateCall2(module->getFunction("pushUInt"), context, constant);
#endif
				}
				u32 i=this->context->constant_pool.uinteger[t];
				constant = llvm::ConstantInt::get(int_type, i);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				break;
			}
			case 0x2f:
			{
				//pushdouble
				LOG(LOG_TRACE, "synt pushdouble" );
				u30 t;
				code >> t;

				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
					Builder.CreateCall(module->getFunction("pushDouble"), {context, constant});
#else
					Builder.CreateCall2(module->getFunction("pushDouble"), context, constant);
#endif
				}
				number_t d=this->context->constant_pool.doubles[t];
				constant = llvm::ConstantFP::get(number_type,d);
				static_stack_push(static_stack,stack_entry(constant,STACK_NUMBER));
				break;
			}
			case 0x30:
			{
				//pushscope
				LOG(LOG_TRACE, "synt pushscope" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(module->getFunction("pushScope"), context);
				break;
			}
			case 0x31:
			{
				//pushnamespace
				LOG(LOG_TRACE, "synt pushnamespace" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				value = Builder.CreateCall(module->getFunction("pushNamespace"), {context, constant});
#else
				value = Builder.CreateCall2(module->getFunction("pushNamespace"), context, constant);
#endif
				static_stack_push(static_stack, stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x32:
			{
				//hasnext2
				LOG(LOG_TRACE, "synt hasnext2" );
				u30 t,t2;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t2;
				constant2 = llvm::ConstantInt::get(int_type, t2);
				//Sync the locals to memory
				if(static_locals[t].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(module->getFunction("decRef"), old);
					abstract_value(module,Builder,static_locals[t]);
					Builder.CreateStore(static_locals[t].first,gep);
					static_locals[t].second=STACK_NONE;
				}

				if(static_locals[t2].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant2);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(module->getFunction("decRef"), old);
					abstract_value(module,Builder,static_locals[t2]);
					Builder.CreateStore(static_locals[t2].first,gep);
					static_locals[t2].second=STACK_NONE;
				}

#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("hasNext2"), {context, constant, constant2});
#else
				value=Builder.CreateCall3(module->getFunction("hasNext2"), context, constant, constant2);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x40:
			{
				//newfunction
				LOG(LOG_TRACE, "synt newfunction" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("newFunction"), {context, constant});
#else
				value=Builder.CreateCall2(module->getFunction("newFunction"), context, constant);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x41:
			{
				//call
				LOG(LOG_TRACE, "synt call" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				constant2 = llvm::ConstantInt::get(int_type, 0);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("call"), {context, constant, constant2});
#else
				Builder.CreateCall3(module->getFunction("call"), context, constant, constant2);
#endif
				break;
			}
			case 0x42:
			{
				//construct
				LOG(LOG_TRACE, "synt construct" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("construct"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("construct"), context, constant);
#endif
				break;
			}
			case 0x43:
			{
				//callMethod
				LOG(LOG_TRACE, "synt callMethod" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callMethod"), {context, constant, constant2});
#else
				Builder.CreateCall3(module->getFunction("callMethod"), context, constant, constant2);
#endif
				break;
			}
			case 0x45:
			{
				//callsuper
				LOG(LOG_TRACE, "synt callsuper" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 1);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callSuper"), {context, constant, constant2, constant3, constant4});
#else
				Builder.CreateCall5(module->getFunction("callSuper"), context, constant, constant2, constant3, constant4);
#endif
				break;
			}
			case 0x46: //callproperty
			{
				//TODO: Implement static resolution where possible
				LOG(LOG_TRACE, "synt callproperty" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 1);
	/*				//Pop the stack arguments
				vector<llvm::Value*> args(t+1);
				for(int i=0;i<t;i++)
					args[t-i]=static_stack_pop(Builder,static_stack,m).first;*/
				//Call the function resolver, static case could be resolved at this time (TODO)
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callProperty"), {context, constant, constant2, constant3, constant4});
#else
				Builder.CreateCall5(module->getFunction("callProperty"), context, constant, constant2, constant3, constant4);
#endif
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
				LOG(LOG_TRACE, "synt returnvoid" );
				last_is_branch=true;
				constant = llvm::ConstantInt::get(ptr_type, 0);
				value = llvm::ConstantExpr::getIntToPtr(constant, voidptr_type);
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x48:
			{
				//returnvalue
				//TODO: Should coerce the return type to the expected one
				LOG(LOG_TRACE, "synt returnvalue" );
				last_is_branch=true;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT)
					e.first=Builder.CreateCall(module->getFunction("abstract_i"),e.first);
				else if(e.second==STACK_NUMBER)
					e.first=Builder.CreateCall(module->getFunction("abstract_d"),e.first);
				else if(e.second==STACK_OBJECT);
				else if(e.second==STACK_BOOLEAN)
					e.first=Builder.CreateCall(module->getFunction("abstract_b"),e.first);
				else
					throw UnsupportedException("Unsupported type for returnvalue");
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(module->getFunction("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(e.first);
				break;
			}
			case 0x49:
			{
				//constructsuper
				LOG(LOG_TRACE, "synt constructsuper" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("constructSuper"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("constructSuper"), context, constant);
#endif
				break;
			}
			case 0x4a:
			{
				//constructprop
				LOG(LOG_TRACE, "synt constructprop" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("constructProp"), {context, constant, constant2});
#else
				Builder.CreateCall3(module->getFunction("constructProp"), context, constant, constant2);
#endif
				break;
			}
			case 0x4c: //callproplex
			{
				//TODO: Implement static resolution where possible
				LOG(LOG_TRACE, "synt callproperty" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 1);
	/*				//Pop the stack arguments
				vector<llvm::Value*> args(t+1);
				for(int i=0;i<t;i++)
					args[t-i]=static_stack_pop(Builder,static_stack,m).first;*/
				//Call the function resolver, static case could be resolved at this time (TODO)
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callPropLex"), {context, constant, constant2, constant3, constant4});
#else
				Builder.CreateCall5(module->getFunction("callPropLex"), context, constant, constant2, constant3, constant4);
#endif
	/*				//Pop the function object, and then the object itself
				llvm::Value* fun=static_stack_pop(Builder,static_stack,m).first;

				llvm::Value* fun2=Builder.CreateBitCast(fun,synt_method_prototype(t));
				args[0]=static_stack_pop(Builder,static_stack,m).first;
				Builder.CreateCall(fun2,args.begin(),args.end());*/

				break;
			}
			case 0x4e:
			{
				//callsupervoid
				LOG(LOG_TRACE, "synt callsupervoid" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 0);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callSuper"), {context, constant, constant2, constant3, constant4});
#else
				Builder.CreateCall5(module->getFunction("callSuper"), context, constant, constant2, constant3, constant4);
#endif
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				LOG(LOG_TRACE, "synt callpropvoid" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 0);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("callProperty"), {context, constant, constant2, constant3, constant4});
#else
				Builder.CreateCall5(module->getFunction("callProperty"), context, constant, constant2, constant3, constant4);
#endif
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				LOG(LOG_TRACE, "synt constructgenerictype" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("constructGenericType"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("constructGenericType"), context, constant);
#endif
				break;
			}
			case 0x55:
			{
				//newobject
				LOG(LOG_TRACE, "synt newobject" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("newObject"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("newObject"), context, constant);
#endif
				break;
			}
			case 0x56:
			{
				//newarray
				LOG(LOG_TRACE, "synt newarray" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("newArray"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("newArray"), context, constant);
#endif
				break;
			}
			case 0x57:
			{
				//newactivation
				LOG(LOG_TRACE, "synt newactivation" );
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("newActivation"), {context, th});
#else
				value=Builder.CreateCall2(module->getFunction("newActivation"), context, th);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x58:
			{
				//newclass
				LOG(LOG_TRACE, "synt newclass" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("newClass"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("newClass"), context, constant);
#endif
				break;
			}
			case 0x59:
			{
				//getdescendants
				LOG(LOG_TRACE, "synt getdescendants" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("getDescendants"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("getDescendants"), context, constant);
#endif
				break;
			}
			case 0x5a:
			{
				//newcatch
				LOG(LOG_TRACE, "synt newcatch" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("newCatch"), {context, constant});
#else
				value=Builder.CreateCall2(module->getFunction("newCatch"), context, constant);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				LOG(LOG_TRACE, "synt findpropstrict" );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("findPropStrict"), {context, name});
#else
				value=Builder.CreateCall2(module->getFunction("findPropStrict"), context, name);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x5e:
			{
				//findproperty
				LOG(LOG_TRACE, "synt findproperty" );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("findProperty"), {context, name});
#else
				value=Builder.CreateCall2(module->getFunction("findProperty"), context, name);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x60:
			{
				//getlex
				LOG(LOG_TRACE, "synt getlex" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("getLex"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("getLex"), context, constant);
#endif
				break;
			}
			case 0x61:
			{
				//setproperty
				LOG(LOG_TRACE, "synt setproperty" );
				u30 t;
				code >> t;
				stack_entry value=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(value.second==STACK_INT)
#ifdef LLVM_37
					Builder.CreateCall(module->getFunction("setProperty_i"),{value.first, obj.first, name});
#else
					Builder.CreateCall3(module->getFunction("setProperty_i"),value.first, obj.first, name);
#endif
				else
				{
					abstract_value(module,Builder,value);
#ifdef LLVM_37
					Builder.CreateCall(module->getFunction("setProperty"),{value.first, obj.first, name});
#else
					Builder.CreateCall3(module->getFunction("setProperty"),value.first, obj.first, name);
#endif
				}
				break;
			}
			case 0x62: //getlocal
				code >> localIndex;
				// falls through
			case 0xd0: //getlocal_0
			case 0xd1: //getlocal_1
			case 0xd2: //getlocal_2
			case 0xd3: //getlocal_3
			{
				int i=(opcode==0x62)?((uint32_t)localIndex):(opcode&3);
				LOG(LOG_TRACE, "synt getlocal " << i);
				constant = llvm::ConstantInt::get(int_type, i);
				if(static_locals[i].second==STACK_NONE)
				{
					llvm::Value* t=Builder.CreateGEP(locals,constant);
					t=Builder.CreateLoad(t,"stack");
					static_stack_push(static_stack,stack_entry(t,STACK_OBJECT));
					static_locals[i]=stack_entry(t,STACK_OBJECT);
					Builder.CreateCall(module->getFunction("incRef"), t);
					Builder.CreateCall(module->getFunction("incRef"), t);
					if(Log::getLevel()>=LOG_CALLS)
#ifdef LLVM_37
						Builder.CreateCall(module->getFunction("getLocal"), {t, constant});
#else
						Builder.CreateCall2(module->getFunction("getLocal"), t, constant);
#endif

				}
				else if(static_locals[i].second==STACK_OBJECT)
				{
					Builder.CreateCall(module->getFunction("incRef"), static_locals[i].first);
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
#ifdef LLVM_37
						Builder.CreateCall(module->getFunction("getLocal"), {static_locals[i].first, constant});
#else
						Builder.CreateCall2(module->getFunction("getLocal"), static_locals[i].first, constant);
#endif
				}
				else if(static_locals[i].second==STACK_INT
					|| static_locals[i].second==STACK_UINT)
				{
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
#ifdef LLVM_37
						Builder.CreateCall(module->getFunction("getLocal_int"), {constant, static_locals[i].first});
#else
						Builder.CreateCall2(module->getFunction("getLocal_int"), constant, static_locals[i].first);
#endif
				}
				else if(static_locals[i].second==STACK_NUMBER ||
						static_locals[i].second==STACK_BOOLEAN)
				{
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
						Builder.CreateCall(module->getFunction("getLocal_short"), constant);
				}
				else
					throw UnsupportedException("Unsupported type for getlocal");

				break;
			}
			case 0x64:
			{
				//getglobalscope
				LOG(LOG_TRACE, "synt getglobalscope" );
				value=Builder.CreateCall(module->getFunction("getGlobalScope"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x65:
			{
				//getscopeobject
				LOG(LOG_TRACE, "synt getscopeobject" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("getScopeObject"), {context, constant});
#else
				value=Builder.CreateCall2(module->getFunction("getScopeObject"), context, constant);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x66:
			{
				//getproperty
				LOG(LOG_TRACE, "synt getproperty" );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);

				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,obj);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("getProperty"), {obj.first, name});
#else
				value=Builder.CreateCall2(module->getFunction("getProperty"), obj.first, name);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				/*if(cur_block->push_types[local_ip]==STACK_OBJECT ||
					cur_block->push_types[local_ip]==STACK_BOOLEAN)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("getProperty"), {obj.first, name});
#else
					value=Builder.CreateCall2(module->getFunction("getProperty"), obj.first, name);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(cur_block->push_types[local_ip]==STACK_INT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("getProperty_i"), {obj.first, name});
#else
					value=Builder.CreateCall2(module->getFunction("getProperty_i"), obj.first, name);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else
					throw UnsupportedException("Unsupported type for getproperty");
					*/
				break;
			}
			case 0x68:
			{
				//initproperty
				LOG(LOG_TRACE, "synt initproperty" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				stack_entry val=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("initProperty"), {obj.first, val.first, name});
#else
				Builder.CreateCall3(module->getFunction("initProperty"), obj.first, val.first, name);
#endif
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				LOG(LOG_TRACE, "synt deleteproperty" );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(module,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry v=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("deleteProperty"), {v.first, name});
#else
				value=Builder.CreateCall2(module->getFunction("deleteProperty"), v.first, name);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x6c:
			{
				//getslot
				LOG(LOG_TRACE, "synt getslot" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("getSlot"), {v1, constant});
#else
				value=Builder.CreateCall2(module->getFunction("getSlot"), v1, constant);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x6d:
			{
				//setslot
				LOG(LOG_TRACE, "synt setslot" );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("setSlot"), {v1.first, v2.first, constant});
#else
				Builder.CreateCall3(module->getFunction("setSlot"), v1.first, v2.first, constant);
#endif
				break;
			}
			case 0x70:
			{
				//convert_s
				LOG(LOG_TRACE, "synt convert_s" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("convert_s"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x72: //esc_xattr
			{
				LOG(LOG_TRACE, "synt esc_xattr" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("esc_xattr"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x73:
			{
				//convert_i
				LOG(LOG_TRACE, "synt convert_i" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_NUMBER)
					value=Builder.CreateFPToSI(v1.first,int_type);
				else if(v1.second==STACK_INT) //Nothing to do
					value=v1.first;
				else
					value=Builder.CreateCall(module->getFunction("convert_i"), v1.first);

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x74:
			{
				//convert_u
				LOG(LOG_TRACE, "synt convert_u" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_NUMBER)
					value=Builder.CreateFPToUI(v1.first,int_type);
				else if(v1.second==STACK_INT) //Nothing to do
					value=v1.first;
				else
					value=Builder.CreateCall(module->getFunction("convert_u"), v1.first);

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x75:
			{
				//convert_d
				LOG(LOG_TRACE, "synt convert_d" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				value = llvm_ToNumber(module, Builder, v1);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x76:
			{
				//convert_b
				LOG(LOG_TRACE, "synt convert_b" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_BOOLEAN)
					static_stack_push(static_stack,v1);
				else
				{
					abstract_value(module,Builder,v1);
					value=Builder.CreateCall(module->getFunction("convert_b"), v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				}
				break;
			}
			case 0x78:
			{
				//checkfilter
				LOG(LOG_TRACE, "synt checkfilter" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("checkfilter"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x80:
			{
				//coerce
				LOG(LOG_TRACE, "synt coerce" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("coerce"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("coerce"), context, constant);
#endif
				break;
			}
			case 0x82:
			{
				//coerce_a
				LOG(LOG_TRACE, "synt coerce_a" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(module->getFunction("coerce_a"));
				/*stack_entry v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				else
					value=v1.first;
				
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));*/
				break;
			}
			case 0x85:
			{
				//coerce_s
				LOG(LOG_TRACE, "synt coerce_s" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("coerce_s"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x87:
			{
				//astypelate
				LOG(LOG_TRACE, "synt astypelate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				assert_and_throw(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("asTypelate"),{v1.first,v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("asTypelate"),v1.first,v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x90:
			{
				//negate
				LOG(LOG_TRACE, "synt negate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("negate"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x91:
			{
				//increment
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				LOG(LOG_TRACE, "synt increment " << v1.second);
				value = llvm_ToNumber(module, Builder, v1);
				//Create floating point '1' by converting an int '1'
				constant = llvm::ConstantInt::get(int_type, 1);
				value=Builder.CreateFAdd(value, Builder.CreateSIToFP(constant, number_type));
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x93:
			{
				//decrement
				LOG(LOG_TRACE, "synt decrement" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				value = llvm_ToNumber(module, Builder, v1);
				//Create floating point '1' by converting an int '1'
				constant = llvm::ConstantInt::get(int_type, 1);
				value=Builder.CreateFSub(value, Builder.CreateSIToFP(constant, number_type));
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x95:
			{
				//typeof
				LOG(LOG_TRACE, "synt typeof" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				value=Builder.CreateCall(module->getFunction("typeOf"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x96:
			{
				//not
				LOG(LOG_TRACE, "synt not" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_BOOLEAN)
					value=Builder.CreateNot(v1.first);
				else
				{
					abstract_value(module,Builder,v1);
					value=Builder.CreateCall(module->getFunction("not"), v1.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x97:
			{
				//bitnot
				LOG(LOG_TRACE, "synt bitnot" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT)
				{
					value=Builder.CreateNot(v1.first);
				}
				else
				{
					abstract_value(module,Builder,v1);
					value=Builder.CreateCall(module->getFunction("bitNot"), v1.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa0:
			{
				//add
				LOG(LOG_TRACE, "synt add" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("add_oi"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("add_oi"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_NUMBER)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("add_od"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("add_od"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT || v2.second==STACK_OBJECT)
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("add"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("add"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else /* v1 and v2 are BOOLEAN, UINT, INT or NUMBER */
				{
					/* adding promotes everything to NUMBER */
					value=Builder.CreateFAdd(llvm_ToNumber(module, Builder, v1), llvm_ToNumber(module, Builder, v2));
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				break;
			}
			case 0xa1:
			{
				//subtract
				LOG(LOG_TRACE, "synt subtract" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateFSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateFSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateFSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("subtract_oi"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("subtract_oi"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("subtract_do"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("subtract_do"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("subtract_io"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("subtract_io"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("subtract"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("subtract"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa2:
			{
				//multiply
				LOG(LOG_TRACE, "synt multiply" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("multiply_oi"), {v2.first, v1.first});
#else
					value=Builder.CreateCall2(module->getFunction("multiply_oi"), v2.first, v1.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("multiply_oi"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("multiply_oi"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateFMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateFMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateFMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("multiply"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("multiply"), v1.first, v2.first);
#endif
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa3:
			{
				//divide
				LOG(LOG_TRACE, "synt divide" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("divide"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("divide"), v1.first, v2.first);
#endif
				}
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0xa4:
			{
				//modulo
				LOG(LOG_TRACE, "synt modulo" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				v1.first = llvm_ToNumber(module, Builder, v1);
				v2.first = llvm_ToNumber(module, Builder, v2);
				value=Builder.CreateFRem(v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0xa5:
			{
				//lshift
				LOG(LOG_TRACE, "synt lshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("lShift_io"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("lShift_io"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 31); //Mask for v1
					v1.first=Builder.CreateAnd(v1.first,constant);
					value=Builder.CreateShl(v2.first,v1.first);
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("lShift"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("lShift"), v1.first, v2.first);
#endif
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa6:
			{
				//rshift
				LOG(LOG_TRACE, "synt rshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("rShift"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("rShift"), v1.first, v2.first);
#endif

				static_stack_push(static_stack,make_pair(value,STACK_INT));
				break;
			}
			case 0xa7:
			{
				//urshift
				LOG(LOG_TRACE, "synt urshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("urShift_io"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("urShift_io"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateIntCast(v2.first,int_type,false);
					v1.first=Builder.CreateIntCast(v1.first,int_type,false);
					value=Builder.CreateLShr(v2.first,v1.first); //Check for trucation of v1.first
					value=Builder.CreateIntCast(value,int_type,false);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v2.first=Builder.CreateFPToSI(v2.first,int_type);
					value=Builder.CreateLShr(v2.first,v1.first); //Check for trucation of v1.first
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("urShift"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("urShift"), v1.first, v2.first);
#endif
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa8:
			{
				//bitand
				LOG(LOG_TRACE, "synt bitand" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitAnd_oi"), {v2.first, v1.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitAnd_oi"), v2.first, v1.first);
#endif
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitAnd_oi"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitAnd_oi"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateAnd(v1.first,v2.first);
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v1.first=Builder.CreateFPToUI(v1.first,int_type);
					value=Builder.CreateAnd(v1.first,v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v2.first=Builder.CreateFPToUI(v2.first,int_type);
					value=Builder.CreateAnd(v1.first,v2.first);
				}
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitAnd_oo"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitAnd_oo"), v1.first, v2.first);
#endif
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa9:
			{
				//bitor
				LOG(LOG_TRACE, "synt bitor" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateOr(v1.first,v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitOr_oi"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitOr_oi"), v1.first, v2.first);
#endif
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitOr_oi"), {v2.first, v1.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitOr_oi"), v2.first, v1.first);
#endif
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitOr"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitOr"), v1.first, v2.first);
#endif
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xaa:
			{
				//bitxor
				LOG(LOG_TRACE, "synt bitxor" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateXor(v1.first,v2.first);
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("bitXor"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("bitXor"), v1.first, v2.first);
#endif
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xab:
			{
				//equals
				LOG(LOG_TRACE, "synt equals" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateICmpEQ(v1.first,v2.first);
				else
				{
					abstract_value(module,Builder,v1);
					abstract_value(module,Builder,v2);
#ifdef LLVM_37
					value=Builder.CreateCall(module->getFunction("equals"), {v1.first, v2.first});
#else
					value=Builder.CreateCall2(module->getFunction("equals"), v1.first, v2.first);
#endif
				}
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xac:
			{
				//strictequals
				LOG(LOG_TRACE, "synt strictequals" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("strictEquals"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("strictEquals"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xad:
			{
				//lessthan
				LOG(LOG_TRACE, "synt lessthan" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("lessThan"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("lessThan"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xae:
			{
				//lessequals
				LOG(LOG_TRACE, "synt lessequals" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("lessEquals"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("lessEquals"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xaf:
			{
				//greaterthan
				LOG(LOG_TRACE, "synt greaterthan" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("greaterThan"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("greaterThan"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb0:
			{
				//greaterequals
				LOG(LOG_TRACE, "synt greaterequals" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("greaterEquals"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("greaterEquals"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb1:
			{
				//instanceOf
				LOG(LOG_TRACE, "synt instanceof" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("instanceOf"), {v1.first, v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("instanceOf"), v1.first, v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb3:
			{
				//istypelate
				LOG(LOG_TRACE, "synt istypelate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(module,Builder,v1);
				abstract_value(module,Builder,v2);
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("isTypelate"),{v1.first,v2.first});
#else
				value=Builder.CreateCall2(module->getFunction("isTypelate"),v1.first,v2.first);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb4:
			{
				//in
				LOG(LOG_TRACE, "synt in" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
#ifdef LLVM_37
				value=Builder.CreateCall(module->getFunction("in"), {v1, v2});
#else
				value=Builder.CreateCall2(module->getFunction("in"), v1, v2);
#endif
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xc0:
			{
				//increment_i
				LOG(LOG_TRACE, "synt increment_i" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(module->getFunction("increment_i"), v1.first);
				else if(v1.second==STACK_INT || v1.second==STACK_UINT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateAdd(v1.first,constant);
				}
				else
					throw UnsupportedException("Unsupported type for increment_i");
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xc1:
			{
				//decrement_i
				LOG(LOG_TRACE, "synt decrement_i" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(module->getFunction("decrement_i"), v1.first);
				else if(v1.second==STACK_INT || v1.second==STACK_UINT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateSub(v1.first,constant);
				}
				else
					throw UnsupportedException("Unsupported type for decrement_i");
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				LOG(LOG_TRACE, "synt inclocal_i" );
				syncStacks(ex,module,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				//Sync the local to memory
				if(static_locals[t].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(module->getFunction("decRef"), old);
					abstract_value(module,Builder,static_locals[t]);
					Builder.CreateStore(static_locals[t].first,gep);
					static_locals[t].second=STACK_NONE;
				}
#ifdef LLVM_37
				Builder.CreateCall(module->getFunction("incLocal_i"), {context, constant});
#else
				Builder.CreateCall2(module->getFunction("incLocal_i"), context, constant);
#endif
				break;
			}
			case 0x63: //setlocal
				code >> localIndex;
				// falls through
			case 0xd4: //setlocal_0
			case 0xd5: //setlocal_1
			case 0xd6: //setlocal_2
			case 0xd7: //setlocal_3
			{
				int i=(opcode==0x63)?((uint32_t)localIndex):(opcode&3);
				LOG(LOG_TRACE, "synt setlocal_n " << i );
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//DecRef previous local
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(module->getFunction("decRef"), static_locals[i].first);

				static_locals[i]=e;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, i);
					if(e.second==STACK_INT)
#ifdef LLVM_37
						Builder.CreateCall(module->getFunction("setLocal_int"), {constant, e.first});
#else
						Builder.CreateCall2(module->getFunction("setLocal_int"), constant, e.first);
#endif
					else if(e.second==STACK_OBJECT)
#ifdef LLVM_37
						Builder.CreateCall(module->getFunction("setLocal_obj"), {constant, e.first});
#else
						Builder.CreateCall2(module->getFunction("setLocal_obj"), constant, e.first);
#endif
					else
						Builder.CreateCall(module->getFunction("setLocal"), constant);
				}
				break;
			}
			case 0xef:
			{
				//debug
				LOG(LOG_TRACE, "synt debug" );
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
			{
				//debugline
				LOG(LOG_TRACE, "synt debugline" );
				u30 t;
				code >> t;
				break;
			}
			case 0xf1:
			{
				//debugfile
				LOG(LOG_TRACE, "synt debugfile" );
				u30 t;
				code >> t;
				break;
			}
			default:
				LOG(LOG_ERROR,"Not implemented instruction @" << code.tellg());
				u8 a,b,c;
				code >> a >> b >> c;
				LOG(LOG_ERROR,"dump " << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
						<< (unsigned int)b << ' ' << (unsigned int)c);
				constant = llvm::ConstantInt::get(int_type, opcode);
				Builder.CreateCall(module->getFunction("not_impl"), constant);
				Builder.CreateRetVoid();

				f=(SyntheticFunction::synt_function)getVm(sys)->ex->getPointerToFunction(llvmf);
				return f;
		}
	}

	map<unsigned int,block_info>::iterator it2=blocks.begin();
	for(;it2!=blocks.end();++it2)
	{
		if(it2->second.BB->getTerminator()==NULL)
		{
			LOG(LOG_ERROR, "start at " << it2->first);
			throw RunTimeException("Missing terminator");
		}
	}

	//llvmf->print(llvm::dbgs()); llvm::dbgs()<<'\n'; //dump before optimization
#ifdef LLVM_34
	getVm(sys)->FPM->doInitialization();
	getVm(sys)->FPM->run(*llvmf);
	getVm(sys)->FPM->doFinalization();
#else
	getVm(sys)->FPM->run(*llvmf);
#endif
	f=(SyntheticFunction::synt_function)getVm(sys)->ex->getFunctionAddress(llvmf->getName());
	//llvmf->print(llvm::dbgs()); llvm::dbgs()<<'\n'; //dump after optimization
	body->codeStatus = method_body_info::JITTED;
	return f;
}

void ABCVm::wrong_exec_pos()
{
	assert_and_throw(false && "wrong_exec_pos");
}
#endif //LLVM_ENABLED
