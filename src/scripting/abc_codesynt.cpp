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

#ifdef LLVM_28
#define alignof alignOf
#define LLVMMAKEARRAYREF(T) T
#else
#define LLVMMAKEARRAYREF(T) makeArrayRef(T)
#endif

#include "compat.h"
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/PassManager.h>
#include <llvm/Constants.h> 
#include <llvm/Support/IRBuilder.h> 
#include <llvm/LLVMContext.h>
#include <llvm/Target/TargetData.h>
#include <sstream>
#include "scripting/abc.h"
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
	LOG(LOG_CALLS, _("debug_d ")<< f);
}

void debug_i(int32_t i)
{
	LOG(LOG_CALLS, _("debug_i ")<< i);
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
	{"constructProp",(void*)&ABCVm::constructProp,ARGS_CONTEXT_INT_INT},
	{"callSuper",(void*)&ABCVm::callSuper,ARGS_CONTEXT_INT_INT_INT_BOOL},
	{"not_impl",(void*)&ABCVm::not_impl,ARGS_INT},
	{"incRef",(void*)&ASObject::s_incRef,ARGS_OBJ},
	{"decRef",(void*)&ASObject::s_decRef,ARGS_OBJ},
	{"decRef_safe",(void*)&ASObject::s_decRef_safe,ARGS_OBJ_OBJ},
	{"wrong_exec_pos",(void*)&ABCVm::wrong_exec_pos,ARGS_NONE},
	{"dxns",(void*)&ABCVm::dxns,ARGS_CONTEXT_INT},
	{"dxnslate",(void*)&ABCVm::dxnslate,ARGS_CONTEXT_OBJ},
};

typed_opcode_handler ABCVm::opcode_table_voidptr[]={
	{"add",(void*)&ABCVm::add,ARGS_OBJ_OBJ},
	{"add_oi",(void*)&ABCVm::add_oi,ARGS_OBJ_INT},
	{"add_od",(void*)&ABCVm::add_od,ARGS_OBJ_NUMBER},
	{"nextName",(void*)&ABCVm::nextName,ARGS_OBJ_OBJ},
	{"nextValue",(void*)&ABCVm::nextValue,ARGS_OBJ_OBJ},
	{"abstract_d",(void*)&abstract_d,ARGS_NUMBER},
	{"abstract_i",(void*)&abstract_i,ARGS_INT},
	{"abstract_ui",(void*)&abstract_ui,ARGS_INT},
	{"abstract_b",(void*)&abstract_b,ARGS_BOOL},
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
	ptr_type=ex->getTargetData()->getIntPtrType(llvm_context());
	//Pointer to 8 bit type, needed for pointer arithmetic
	voidptr_type=llvm::IntegerType::get(getVm()->llvm_context(),8)->getPointerTo();
	number_type=llvm::Type::getDoubleTy(llvm_context());
	numberptr_type=llvm::Type::getDoublePtrTy(llvm_context());
	bool_type=llvm::IntegerType::get(llvm_context(),1);
	boolptr_type=bool_type->getPointerTo();
	void_type=llvm::Type::getVoidTy(llvm_context());
	int_type=llvm::IntegerType::get(getVm()->llvm_context(),32);
	intptr_type=int_type->getPointerTo();

	//All the opcodes needs a pointer to the context
	std::vector<LLVMTYPE> struct_elems;
	struct_elems.push_back(voidptr_type->getPointerTo());
	struct_elems.push_back(voidptr_type->getPointerTo());
	struct_elems.push_back(int_type);
	struct_elems.push_back(int_type);
	context_type=llvm::PointerType::getUnqual(llvm::StructType::get(llvm_context(),LLVMMAKEARRAYREF(struct_elems),true));

	//newActivation needs both method_info and the context
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
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm()->llvm_context(),32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);

	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

static llvm::Value* llvm_stack_peek(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm()->llvm_context(),32), 1);
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
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm()->llvm_context(),32), 1);
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
inline void abstract_value(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, stack_entry& e)
{
	switch(e.second)
	{
		case STACK_OBJECT:
			break;
		case STACK_INT:
			e.first=builder.CreateCall(ex->FindFunctionNamed("abstract_i"),e.first);
			break;
		case STACK_UINT:
			e.first=builder.CreateCall(ex->FindFunctionNamed("abstract_ui"),e.first);
			break;
		case STACK_NUMBER:
			e.first=builder.CreateCall(ex->FindFunctionNamed("abstract_d"),e.first);
			break;
		case STACK_BOOLEAN:
			e.first=builder.CreateCall(ex->FindFunctionNamed("abstract_b"),e.first);
			break;
		default:
			throw RunTimeException("Unexpected object type to abstract");
	}
	e.second = STACK_OBJECT;
}

//Helper functions to sync the static stack and locals to the memory 
inline void syncStacks(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& builder,
		vector<stack_entry>& static_stack,llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index) 
{
	for(unsigned int i=0;i<static_stack.size();i++)
	{
		abstract_value(ex, builder, static_stack[i]);
		llvm_stack_push(ex,builder,static_stack[i].first,dynamic_stack,dynamic_stack_index);
	}
	static_stack.clear();
}

inline void syncLocals(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& builder,
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
				builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);
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
			llvm::Value* constant = llvm::ConstantInt::get(llvm::IntegerType::get(getVm()->llvm_context(),32), i);
			llvm::Value* t=builder.CreateGEP(locals,constant);
			llvm::Value* old=builder.CreateLoad(t);
			if(static_locals[i].second==STACK_OBJECT)
			{
				builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
				builder.CreateStore(static_locals[i].first,t);
			}
			else if(static_locals[i].second==STACK_INT)
			{
				//decRef the previous contents
				builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
				llvm::Value* v=builder.CreateCall(ex->FindFunctionNamed("abstract_i"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_UINT)
			{
				//decRef the previous contents
				builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
				llvm::Value* v=builder.CreateCall(ex->FindFunctionNamed("abstract_ui"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_NUMBER)
			{
				//decRef the previous contents
				builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
				llvm::Value* v=builder.CreateCall(ex->FindFunctionNamed("abstract_d"),static_locals[i].first);
				builder.CreateStore(v,t);
			}
			else if(static_locals[i].second==STACK_BOOLEAN)
			{
				//decRef the previous contents
				builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
				llvm::Value* v=builder.CreateCall(ex->FindFunctionNamed("abstract_b"),static_locals[i].first);
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
static llvm::Value* llvm_ToNumber(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& Builder, stack_entry& e)
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
		return Builder.CreateCall(ex->FindFunctionNamed("convert_d"), e.first);
	}
}

/* Adds instructions to the builder to resolve the given multiname */
inline llvm::Value* getMultiname(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& Builder, vector<stack_entry>& static_stack,
				llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index,
				ABCContext* abccontext, int multinameIndex)
{
	int rtdata=abccontext->getMultinameRTData(multinameIndex);
	llvm::Value* name = 0;
	if(rtdata==0)
	{
		//Multinames without runtime date persist
		multiname* mname = ABCContext::s_getMultiname(abccontext,NULL,NULL,multinameIndex);
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
				name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_i"), context, rt1.first, mindx);
			else if(rt1.second==STACK_NUMBER)
				name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_d"), context, rt1.first, mindx);
			else*/
			{
				abstract_value(ex,Builder,rt1);
				name = Builder.CreateCall4(ex->FindFunctionNamed("getMultiname"), context, rt1.first, constnull, mindx);
			}
		}
		else if(rtdata==2)
		{
			stack_entry rt1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
			stack_entry rt2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
			abstract_value(ex,Builder,rt1);
			abstract_value(ex,Builder,rt2);
			name = Builder.CreateCall4(ex->FindFunctionNamed("getMultiname"), context, rt1.first, rt2.first, mindx);
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
	BB=llvm::BasicBlock::Create(getVm()->llvm_context(), blockName, mi->llvmf);
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
		exception_info& exc=body->exceptions[i];
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
				exception_info& exc=body->exceptions[i];
				if(exc.target == local_ip)
				{
					addBlock(blocks,local_ip,"catch");
					static_stack_types.clear();
					cur_block=&blocks[local_ip];
					LOG(LOG_TRACE,_("New block at ") << local_ip);
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
				LOG(LOG_TRACE,_("New block at ") << local_ip);
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
					LOG(LOG_TRACE, _("block analysis: kill") );
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
					LOG(LOG_TRACE, _("synt lookupswitch") );
					fallthrough=false;

					int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
					s24 t;
					code >> t;
					int defaultdest=here+t;
					LOG(LOG_TRACE,_("default ") << int(t));
					u30 count;
					code >> count;
					LOG(LOG_TRACE,_("count ")<< int(count));
					vector<s24> offsets(count+1);
					for(unsigned int i=0;i<count+1;i++)
					{
						code >> offsets[i];
						LOG(LOG_TRACE,_("Case ") << i << _(" ") << offsets[i]);
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
					LOG(LOG_ERROR,_("Not implemented instruction @") << code.tellg());
					u8 a,b,c;
					code >> a >> b >> c;
					LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
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

SyntheticFunction::synt_function method_info::synt_method()
{
	if(f)
		return f;

	string method_name="method";
	method_name+=context->getString(info.name).raw_buf();
	if(!body)
	{
		LOG(LOG_CALLS,_("Method ") << method_name << _(" should be intrinsic"));;
		return NULL;
	}
	llvm::ExecutionEngine* ex=getVm()->ex;
	llvm::LLVMContext& llvm_context=getVm()->llvm_context();
	llvm::FunctionType* method_type=synt_method_prototype(ex);
	llvmf=llvm::Function::Create(method_type,llvm::Function::ExternalLinkage,method_name,getVm()->module);

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

	llvm::Function::ArgumentListType::iterator it=llvmf->getArgumentList().begin();
	//The first and only argument to this function is the call_context*
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

	llvm::Value* exec_pos=Builder.CreateStructGEP(context,3);

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
		if(paramTypes[i] == Class<Number>::getClass())
		{
			/* yield t = locals[i+1] */
			LOAD_LOCALPTR
			/*calc n+offsetof(Number,val) = &n->val*/
			t=Builder.CreateGEP(t, llvm::ConstantInt::get(int_type, offsetof(Number,val)));
			t=Builder.CreateBitCast(t,numberptr_type); //cast t from int8* to number*
			blocks[0].locals_start[i+1] = STACK_NUMBER;
			//locals_start_obj should hold the pointer to the local's value
			blocks[0].locals_start_obj[i+1] = t;
			LOG(LOG_TRACE,"found STACK_NUMBER parameter for local " << i+1);
		}
		else if(paramTypes[i] == Class<Integer>::getClass())
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
		else if(paramTypes[i] == Class<UInteger>::getClass())
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
		else if(paramTypes[i] == Class<Boolean>::getClass())
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
		Builder.CreateCall(ex->FindFunctionNamed("wrong_exec_pos"));
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
			exception_info& exc=body->exceptions[i];
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
				LOG(LOG_TRACE, _("Last instruction before a new block was not a branch."));
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,it->second);
				Builder.CreateBr(it->second.BB);
			}
			cur_block=&it->second;
			LOG(LOG_TRACE,_("Starting block at ")<<local_ip << " " << *cur_block);
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
				LOG(LOG_TRACE, _("synt throw") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//syncLocals(ex,Builder,static_locals,locals,cur_block->locals,*cur_block);
				Builder.CreateCall(ex->FindFunctionNamed("throw"), context);
				//Right now we set up like we do for retunrvoid
				last_is_branch=true;
				constant = llvm::ConstantInt::get(ptr_type, 0);
				value = llvm::ConstantExpr::getIntToPtr(constant, voidptr_type);
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x04:
			{
				//getsuper
				LOG(LOG_TRACE, _("synt getsuper") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getSuper"), context, constant);
				break;
			}
			case 0x05:
			{
				//setsuper
				LOG(LOG_TRACE, _("synt setsuper") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("setSuper"), context, constant);
				break;
			}
			case 0x06:
			{
				//dxns
				LOG(LOG_TRACE, _("synt dxns") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("dxns"), context, constant);
				break;
			}
			case 0x07:
			{
				//dxnslate
				LOG(LOG_TRACE, _("synt dxnslate") );
				stack_entry v=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v);
				Builder.CreateCall2(ex->FindFunctionNamed("dxns"), context, v.first);
				break;
			}
			case 0x08:
			{
				//kill
				u30 t;
				code >> t;
				LOG(LOG_TRACE, _("synt kill ") << t );
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall(ex->FindFunctionNamed("kill"), constant);
				}
				int i=t;
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);

				static_locals[i].second=STACK_NONE;

				break;
			}
			case 0x09:
			{
				//label
				//Create a new block and insert it in the mapping
				LOG(LOG_TRACE, _("synt label") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("label"));
				break;
			}
			case 0x0c:
			{
				//ifnlt
				LOG(LOG_TRACE, _("synt ifnlt") );
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNLT"), v1.first, v2.first);
				}

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0d:
			{
				//ifnle
				LOG(LOG_TRACE, _("synt ifnle"));
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNLE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0e:
			{
				//ifngt
				LOG(LOG_TRACE, _("synt ifngt") );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNGT"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0f:
			{
				//ifnge
				LOG(LOG_TRACE, _("synt ifnge"));
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
				//Make comparision

				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNGE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x10:
			{
				//jump
				last_is_branch=true;

				s24 t;
				code >> t;
				LOG(LOG_TRACE, _("synt jump ") << t );
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall(ex->FindFunctionNamed("jump"), constant);
				}
				int here=code.tellg();
				int dest=here+t;
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x11:
			{
				//iftrue
				LOG(LOG_TRACE, _("synt iftrue") );
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
					abstract_value(ex,Builder,v1);
					cond=Builder.CreateCall(ex->FindFunctionNamed("ifTrue"), v1.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x12:
			{
				//iffalse

				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(LOG_TRACE, _("synt iffalse ") << t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_BOOLEAN)
					cond=Builder.CreateNot(v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					cond=Builder.CreateCall(ex->FindFunctionNamed("ifFalse"), v1.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x13:
			{
				//ifeq
				LOG(LOG_TRACE, _("synt ifeq") );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				//Make comparision
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					cond=Builder.CreateFCmpOEQ(v1.first,v2.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				}

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
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
				LOG(LOG_TRACE, _("synt ifne ") << t );
			
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE_oi"), v2.first, v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE_oi"), v1.first, v2.first);
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
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
				LOG(LOG_TRACE, _("synt iflt ")<< t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT_io"), v1.first, v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					cond=Builder.CreateICmpSLT(v2.first,v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x16:
			{
				//ifle
				LOG(LOG_TRACE, _("synt ifle") );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLE"), v1.first, v2.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLE"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x17:
			{
				//ifgt
				LOG(LOG_TRACE, _("synt ifgt") );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				cond=Builder.CreateCall2(ex->FindFunctionNamed("ifGT"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x18:
			{
				//ifge
				LOG(LOG_TRACE, _("synt ifge"));
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				cond=Builder.CreateCall2(ex->FindFunctionNamed("ifGE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x19:
			{
				//ifstricteq
				LOG(LOG_TRACE, _("synt ifstricteq") );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code >> t;
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictEq"), v1, v2);

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				LOG(LOG_TRACE, _("synt ifstrictne") );
				last_is_branch=true;
				s24 t;
				code >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictNE"), v1.first, v2.first);

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				LOG(LOG_TRACE, _("synt lookupswitch") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("lookupswitch"));
				last_is_branch=true;

				int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
				s24 t;
				code >> t;
				int defaultdest=here+t;
				LOG(LOG_TRACE,_("default ") << int(t));
				u30 count;
				code >> count;
				LOG(LOG_TRACE,_("count ")<< int(count));
				vector<s24> offsets(count+1);
				for(unsigned int i=0;i<count+1;i++)
				{
					code >> offsets[i];
					LOG(LOG_TRACE,_("Case ") << i << _(" ") << offsets[i]);
				}
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT);
				else if(e.second==STACK_OBJECT)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("convert_i"),e.first);
				else
					throw UnsupportedException("Unsupported type for lookupswitch");
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				//Generate epilogue for default dest
				llvm::BasicBlock* Default=llvm::BasicBlock::Create(llvm_context,"epilogueDest", llvmf);

				llvm::SwitchInst* sw=Builder.CreateSwitch(e.first,Default);
				Builder.SetInsertPoint(Default);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[defaultdest]);
				Builder.CreateBr(blocks[defaultdest].BB);

				for(unsigned int i=0;i<offsets.size();i++)
				{
					int casedest=here+offsets[i];
					llvm::ConstantInt* constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(int_type, i));
					llvm::BasicBlock* Case=llvm::BasicBlock::Create(llvm_context,"epilogueCase", llvmf);
					sw->addCase(constant,Case);
					Builder.SetInsertPoint(Case);
					syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[casedest]);
					Builder.CreateBr(blocks[casedest].BB);
				}

				break;
			}
			case 0x1c:
			{
				//pushwith
				LOG(LOG_TRACE, _("synt pushwith") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("pushWith"), context);
				break;
			}
			case 0x1d:
			{
				//popscope
				LOG(LOG_TRACE, _("synt popscope") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("popScope"), context);
				break;
			}
			case 0x1e:
			{
				//nextname
				LOG(LOG_TRACE, _("synt nextname") );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextName"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x20:
			{
				//pushnull
				LOG(LOG_TRACE, _("synt pushnull") );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushNull"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x21:
			{
				//pushundefined
				LOG(LOG_TRACE, _("synt pushundefined") );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushUndefined"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x23:
			{
				//nextvalue
				LOG(LOG_TRACE, _("synt nextvalue") );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextValue"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x24:
			{
				//pushbyte
				LOG(LOG_TRACE, _("synt pushbyte") );
				int8_t t;
				code.read((char*)&t,1);
				constant = llvm::ConstantInt::get(int_type, (int)t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("pushByte"), constant);
				break;
			}
			case 0x25:
			{
				//pushshort
				LOG(LOG_TRACE, _("synt pushshort") );
				//pushshort
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				u32 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("pushShort"), constant);
				break;
			}
			case 0x26:
			{
				//pushtrue
				LOG(LOG_TRACE, _("synt pushtrue") );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushTrue"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x27:
			{
				//pushfalse
				LOG(LOG_TRACE, _("synt pushfalse") );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushFalse"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x28:
			{
				//pushnan
				LOG(LOG_TRACE, _("synt pushnan") );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushNaN"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x29:
			{
				//pop
				LOG(LOG_TRACE, _("synt pop") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("pop"));
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), e.first);
				break;
			}
			case 0x2a:
			{
				//dup
				LOG(LOG_TRACE, _("synt dup") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("dup"));
				stack_entry e=static_stack_peek(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e);

				if(e.second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), e.first);
				break;
			}
			case 0x2b:
			{
				//swap
				LOG(LOG_TRACE, _("synt swap") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("swap"));
				stack_entry e1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry e2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e1);
				static_stack_push(static_stack,e2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				LOG(LOG_TRACE, _("synt pushstring") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushString"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x2d:
			{
				//pushint
				LOG(LOG_TRACE, _("synt pushint") );
				u30 t;
				code >> t;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("pushInt"), context, constant);
				}
				s32 i=this->context->constant_pool.integer[t];
				constant = llvm::ConstantInt::get(int_type, i);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				break;
			}
			case 0x2e:
			{
				//pushuint
				LOG(LOG_TRACE, _("synt pushuint") );
				u30 t;
				code >> t;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("pushUInt"), context, constant);
				}
				u32 i=this->context->constant_pool.uinteger[t];
				constant = llvm::ConstantInt::get(int_type, i);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				break;
			}
			case 0x2f:
			{
				//pushdouble
				LOG(LOG_TRACE, _("synt pushdouble") );
				u30 t;
				code >> t;

				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("pushDouble"), context, constant);
				}
				number_t d=this->context->constant_pool.doubles[t];
				constant = llvm::ConstantFP::get(number_type,d);
				static_stack_push(static_stack,stack_entry(constant,STACK_NUMBER));
				break;
			}
			case 0x30:
			{
				//pushscope
				LOG(LOG_TRACE, _("synt pushscope") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("pushScope"), context);
				break;
			}
			case 0x31:
			{
				//pushnamespace
				LOG(LOG_TRACE, _("synt pushnamespace") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value = Builder.CreateCall2(ex->FindFunctionNamed("pushNamespace"), context, constant);
				static_stack_push(static_stack, stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x32:
			{
				//hasnext2
				LOG(LOG_TRACE, _("synt hasnext2") );
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
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
					abstract_value(ex,Builder,static_locals[t]);
					Builder.CreateStore(static_locals[t].first,gep);
					static_locals[t].second=STACK_NONE;
				}

				if(static_locals[t2].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant2);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
					abstract_value(ex,Builder,static_locals[t2]);
					Builder.CreateStore(static_locals[t2].first,gep);
					static_locals[t2].second=STACK_NONE;
				}

				value=Builder.CreateCall3(ex->FindFunctionNamed("hasNext2"), context, constant, constant2);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x40:
			{
				//newfunction
				LOG(LOG_TRACE, _("synt newfunction") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newFunction"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x41:
			{
				//call
				LOG(LOG_TRACE, _("synt call") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				constant2 = llvm::ConstantInt::get(int_type, 0);
				Builder.CreateCall3(ex->FindFunctionNamed("call"), context, constant, constant2);
				break;
			}
			case 0x42:
			{
				//construct
				LOG(LOG_TRACE, _("synt construct") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("construct"), context, constant);
				break;
			}
			case 0x45:
			{
				//callsuper
				LOG(LOG_TRACE, _("synt callsuper") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 1);
				Builder.CreateCall5(ex->FindFunctionNamed("callSuper"), context, constant, constant2, constant3, constant4);
				break;
			}
			case 0x4c: //callproplex
			case 0x46: //callproperty
			{
				//Both opcodes are fully equal
				//TODO: Implement static resolution where possible
				LOG(LOG_TRACE, _("synt callproperty") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
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
				Builder.CreateCall5(ex->FindFunctionNamed("callProperty"), context, constant, constant2, constant3, constant4);
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
				LOG(LOG_TRACE, _("synt returnvoid") );
				last_is_branch=true;
				constant = llvm::ConstantInt::get(ptr_type, 0);
				value = llvm::ConstantExpr::getIntToPtr(constant, voidptr_type);
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x48:
			{
				//returnvalue
				//TODO: Should coerce the return type to the expected one
				LOG(LOG_TRACE, _("synt returnvalue") );
				last_is_branch=true;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),e.first);
				else if(e.second==STACK_NUMBER)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_d"),e.first);
				else if(e.second==STACK_OBJECT);
				else if(e.second==STACK_BOOLEAN)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_b"),e.first);
				else
					throw UnsupportedException("Unsupported type for returnvalue");
				for(unsigned int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(unsigned int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(e.first);
				break;
			}
			case 0x49:
			{
				//constructsuper
				LOG(LOG_TRACE, _("synt constructsuper") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructSuper"), context, constant);
				break;
			}
			case 0x4a:
			{
				//constructprop
				LOG(LOG_TRACE, _("synt constructprop") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall3(ex->FindFunctionNamed("constructProp"), context, constant, constant2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				LOG(LOG_TRACE, _("synt callsupervoid") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 0);
				Builder.CreateCall5(ex->FindFunctionNamed("callSuper"), context, constant, constant2, constant3, constant4);
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				LOG(LOG_TRACE, _("synt callpropvoid") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				constant3 = llvm::ConstantInt::get(int_type, 0);
				constant4 = llvm::ConstantInt::get(bool_type, 0);
				Builder.CreateCall5(ex->FindFunctionNamed("callProperty"), context, constant, constant2, constant3, constant4);
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				LOG(LOG_TRACE, _("synt constructgenerictype") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructGenericType"), context, constant);
				break;
			}
			case 0x55:
			{
				//newobject
				LOG(LOG_TRACE, _("synt newobject") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("newObject"), context, constant);
				break;
			}
			case 0x56:
			{
				//newarray
				LOG(LOG_TRACE, _("synt newarray") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("newArray"), context, constant);
				break;
			}
			case 0x57:
			{
				//newactivation
				LOG(LOG_TRACE, _("synt newactivation") );
				value=Builder.CreateCall2(ex->FindFunctionNamed("newActivation"), context, th);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x58:
			{
				//newclass
				LOG(LOG_TRACE, _("synt newclass") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("newClass"), context, constant);
				break;
			}
			case 0x59:
			{
				//getdescendants
				LOG(LOG_TRACE, _("synt getdescendants") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getDescendants"), context, constant);
				break;
			}
			case 0x5a:
			{
				//newcatch
				LOG(LOG_TRACE, _("synt newcatch") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newCatch"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				LOG(LOG_TRACE, _("synt findpropstrict") );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("findPropStrict"), context, name);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x5e:
			{
				//findproperty
				LOG(LOG_TRACE, _("synt findproperty") );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("findProperty"), context, name);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x60:
			{
				//getlex
				LOG(LOG_TRACE, _("synt getlex") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getLex"), context, constant);
				break;
			}
			case 0x61:
			{
				//setproperty
				LOG(LOG_TRACE, _("synt setproperty") );
				u30 t;
				code >> t;
				stack_entry value=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(value.second==STACK_INT)
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty_i"),value.first, obj.first, name);
				else
				{
					abstract_value(ex,Builder,value);
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty"),value.first, obj.first, name);
				}
				break;
			}
			case 0x62: //getlocal
				code >> localIndex;
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
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
					if(Log::getLevel()>=LOG_CALLS)
						Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), t, constant);

				}
				else if(static_locals[i].second==STACK_OBJECT)
				{
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), static_locals[i].first);
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
						Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), static_locals[i].first, constant);
				}
				else if(static_locals[i].second==STACK_INT
					|| static_locals[i].second==STACK_UINT)
				{
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
						Builder.CreateCall2(ex->FindFunctionNamed("getLocal_int"), constant, static_locals[i].first);
				}
				else if(static_locals[i].second==STACK_NUMBER ||
						static_locals[i].second==STACK_BOOLEAN)
				{
					static_stack_push(static_stack,static_locals[i]);
					if(Log::getLevel()>=LOG_CALLS)
						Builder.CreateCall(ex->FindFunctionNamed("getLocal_short"), constant);
				}
				else
					throw UnsupportedException("Unsupported type for getlocal");

				break;
			}
			case 0x64:
			{
				//getglobalscope
				LOG(LOG_TRACE, _("synt getglobalscope") );
				value=Builder.CreateCall(ex->FindFunctionNamed("getGlobalScope"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x65:
			{
				//getscopeobject
				LOG(LOG_TRACE, _("synt getscopeobject") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("getScopeObject"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x66:
			{
				//getproperty
				LOG(LOG_TRACE, _("synt getproperty") );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);

				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,obj);
				value=Builder.CreateCall2(ex->FindFunctionNamed("getProperty"), obj.first, name);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				/*if(cur_block->push_types[local_ip]==STACK_OBJECT ||
					cur_block->push_types[local_ip]==STACK_BOOLEAN)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("getProperty"), obj.first, name);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(cur_block->push_types[local_ip]==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("getProperty_i"), obj.first, name);
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
				LOG(LOG_TRACE, _("synt initproperty") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				stack_entry val=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall3(ex->FindFunctionNamed("initProperty"), obj.first, val.first, name);
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				LOG(LOG_TRACE, _("synt deleteproperty") );
				u30 t;
				code >> t;
				llvm::Value* name = getMultiname(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index,this->context,t);
				stack_entry v=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v);
				value=Builder.CreateCall2(ex->FindFunctionNamed("deleteProperty"), v.first, name);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x6c:
			{
				//getslot
				LOG(LOG_TRACE, _("synt getslot") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("getSlot"), v1, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x6d:
			{
				//setslot
				LOG(LOG_TRACE, _("synt setslot") );
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				Builder.CreateCall3(ex->FindFunctionNamed("setSlot"), v1.first, v2.first, constant);
				break;
			}
			case 0x70:
			{
				//convert_s
				LOG(LOG_TRACE, _("synt convert_s") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_s"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x72: //esc_xattr
			{
				LOG(LOG_TRACE, _("synt esc_xattr") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("esc_xattr"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
			}
			case 0x73:
			{
				//convert_i
				LOG(LOG_TRACE, _("synt convert_i") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_NUMBER)
					value=Builder.CreateFPToSI(v1.first,int_type);
				else if(v1.second==STACK_INT) //Nothing to do
					value=v1.first;
				else
					value=Builder.CreateCall(ex->FindFunctionNamed("convert_i"), v1.first);

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x74:
			{
				//convert_u
				LOG(LOG_TRACE, _("synt convert_u") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_NUMBER)
					value=Builder.CreateFPToUI(v1.first,int_type);
				else if(v1.second==STACK_INT) //Nothing to do
					value=v1.first;
				else
					value=Builder.CreateCall(ex->FindFunctionNamed("convert_u"), v1.first);

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x75:
			{
				//convert_d
				LOG(LOG_TRACE, _("synt convert_d") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				value = llvm_ToNumber(ex, Builder, v1);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x76:
			{
				//convert_b
				LOG(LOG_TRACE, _("synt convert_b") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_BOOLEAN)
					static_stack_push(static_stack,v1);
				else
				{
					abstract_value(ex,Builder,v1);
					value=Builder.CreateCall(ex->FindFunctionNamed("convert_b"), v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				}
				break;
			}
			case 0x78:
			{
				//checkfilter
				LOG(LOG_TRACE, _("synt checkfilter") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("checkfilter"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x80:
			{
				//coerce
				LOG(LOG_TRACE, _("synt coerce") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("coerce"), context, constant);
				break;
			}
			case 0x82:
			{
				//coerce_a
				LOG(LOG_TRACE, _("synt coerce_a") );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("coerce_a"));
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
				LOG(LOG_TRACE, _("synt coerce_s") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("coerce_s"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x87:
			{
				//astypelate
				LOG(LOG_TRACE, _("synt astypelate") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				assert_and_throw(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT);
				value=Builder.CreateCall2(ex->FindFunctionNamed("asTypelate"),v1.first,v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x90:
			{
				//negate
				LOG(LOG_TRACE, _("synt negate") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("negate"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x91:
			{
				//increment
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				LOG(LOG_TRACE, "synt increment " << v1.second);
				value = llvm_ToNumber(ex, Builder, v1);
				//Create floating point '1' by converting an int '1'
				constant = llvm::ConstantInt::get(int_type, 1);
				value=Builder.CreateFAdd(value, Builder.CreateSIToFP(constant, number_type));
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x93:
			{
				//decrement
				LOG(LOG_TRACE, _("synt decrement") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				value = llvm_ToNumber(ex, Builder, v1);
				//Create floating point '1' by converting an int '1'
				constant = llvm::ConstantInt::get(int_type, 1);
				value=Builder.CreateFSub(value, Builder.CreateSIToFP(constant, number_type));
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0x95:
			{
				//typeof
				LOG(LOG_TRACE, _("synt typeof") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("typeOf"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x96:
			{
				//not
				LOG(LOG_TRACE, _("synt not") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_BOOLEAN)
					value=Builder.CreateNot(v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					value=Builder.CreateCall(ex->FindFunctionNamed("not"), v1.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x97:
			{
				//bitnot
				LOG(LOG_TRACE, _("synt bitnot") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT)
				{
					value=Builder.CreateNot(v1.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					value=Builder.CreateCall(ex->FindFunctionNamed("bitNot"), v1.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa0:
			{
				//add
				LOG(LOG_TRACE, _("synt add") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_oi"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_od"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT || v2.second==STACK_OBJECT)
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("add"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else /* v1 and v2 are BOOLEAN, UINT, INT or NUMBER */
				{
					/* adding promotes everything to NUMBER */
					value=Builder.CreateFAdd(llvm_ToNumber(ex, Builder, v1), llvm_ToNumber(ex, Builder, v2));
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				break;
			}
			case 0xa1:
			{
				//subtract
				LOG(LOG_TRACE, _("synt subtract") );
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
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_oi"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_do"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_io"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa2:
			{
				//multiply
				LOG(LOG_TRACE, _("synt multiply") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply_oi"), v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply_oi"), v1.first, v2.first);
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa3:
			{
				//divide
				LOG(LOG_TRACE, _("synt divide") );
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("divide"), v1.first, v2.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0xa4:
			{
				//modulo
				LOG(LOG_TRACE, _("synt modulo") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				v1.first = llvm_ToNumber(ex, Builder, v1);
				v2.first = llvm_ToNumber(ex, Builder, v2);
				value=Builder.CreateFRem(v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0xa5:
			{
				//lshift
				LOG(LOG_TRACE, _("synt lshift") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("lShift_io"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 31); //Mask for v1
					v1.first=Builder.CreateAnd(v1.first,constant);
					value=Builder.CreateShl(v2.first,v1.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("lShift"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa6:
			{
				//rshift
				LOG(LOG_TRACE, _("synt rshift") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("rShift"), v1.first, v2.first);

				static_stack_push(static_stack,make_pair(value,STACK_INT));
				break;
			}
			case 0xa7:
			{
				//urshift
				LOG(LOG_TRACE, _("synt urshift") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("urShift_io"), v1.first, v2.first);
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("urShift"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa8:
			{
				//bitand
				LOG(LOG_TRACE, _("synt bitand") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oi"), v2.first, v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oi"), v1.first, v2.first);
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
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oo"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa9:
			{
				//bitor
				LOG(LOG_TRACE, _("synt bitor") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateOr(v1.first,v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr_oi"), v2.first, v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xaa:
			{
				//bitxor
				LOG(LOG_TRACE, _("synt bitxor") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateXor(v1.first,v2.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitXor"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xab:
			{
				//equals
				LOG(LOG_TRACE, _("synt equals") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateICmpEQ(v1.first,v2.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1.first, v2.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xac:
			{
				//strictequals
				LOG(LOG_TRACE, _("synt strictequals") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("strictEquals"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xad:
			{
				//lessthan
				LOG(LOG_TRACE, _("synt lessthan") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("lessThan"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xae:
			{
				//lessequals
				LOG(LOG_TRACE, _("synt lessequals") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("lessEquals"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xaf:
			{
				//greaterthan
				LOG(LOG_TRACE, _("synt greaterthan") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("greaterThan"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb0:
			{
				//greaterequals
				LOG(LOG_TRACE, _("synt greaterequals") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("greaterEquals"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb1:
			{
				//instanceOf
				LOG(LOG_TRACE, _("synt instanceof") );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("instanceOf"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb3:
			{
				//istypelate
				LOG(LOG_TRACE, _("synt istypelate") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("isTypelate"),v1.first,v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb4:
			{
				//in
				LOG(LOG_TRACE, _("synt in") );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("in"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xc0:
			{
				//increment_i
				LOG(LOG_TRACE, _("synt increment_i") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("increment_i"), v1.first);
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
				LOG(LOG_TRACE, _("synt decrement_i") );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("decrement_i"), v1.first);
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
				LOG(LOG_TRACE, _("synt inclocal_i") );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				//Sync the local to memory
				if(static_locals[t].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
					abstract_value(ex,Builder,static_locals[t]);
					Builder.CreateStore(static_locals[t].first,gep);
					static_locals[t].second=STACK_NONE;
				}
				Builder.CreateCall2(ex->FindFunctionNamed("incLocal_i"), context, constant);
				break;
			}
			case 0x63: //setlocal
				code >> localIndex;
			case 0xd4: //setlocal_0
			case 0xd5: //setlocal_1
			case 0xd6: //setlocal_2
			case 0xd7: //setlocal_3
			{
				int i=(opcode==0x63)?((uint32_t)localIndex):(opcode&3);
				LOG(LOG_TRACE, _("synt setlocal_n ") << i );
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//DecRef previous local
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);

				static_locals[i]=e;
				if(Log::getLevel()>=LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, i);
					if(e.second==STACK_INT)
						Builder.CreateCall2(ex->FindFunctionNamed("setLocal_int"), constant, e.first);
					else if(e.second==STACK_OBJECT)
						Builder.CreateCall2(ex->FindFunctionNamed("setLocal_obj"), constant, e.first);
					else
						Builder.CreateCall(ex->FindFunctionNamed("setLocal"), constant);
				}
				break;
			}
			case 0xef:
			{
				//debug
				LOG(LOG_TRACE, _("synt debug") );
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
				LOG(LOG_TRACE, _("synt debugline") );
				u30 t;
				code >> t;
				break;
			}
			case 0xf1:
			{
				//debugfile
				LOG(LOG_TRACE, _("synt debugfile") );
				u30 t;
				code >> t;
				break;
			}
			default:
				LOG(LOG_ERROR,_("Not implemented instruction @") << code.tellg());
				u8 a,b,c;
				code >> a >> b >> c;
				LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
						<< (unsigned int)b << ' ' << (unsigned int)c);
				constant = llvm::ConstantInt::get(int_type, opcode);
				Builder.CreateCall(ex->FindFunctionNamed("not_impl"), constant);
				Builder.CreateRetVoid();

				f=(SyntheticFunction::synt_function)getVm()->ex->getPointerToFunction(llvmf);
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

	//llvmf->dump(); //dump before optimization
	getVm()->FPM->run(*llvmf);
	f=(SyntheticFunction::synt_function)getVm()->ex->getPointerToFunction(llvmf);
	//llvmf->dump(); //dump after optimization
	body->codeStatus = method_body_info::JITTED;
	return f;
}

void ABCVm::wrong_exec_pos()
{
	assert_and_throw(false && "wrong_exec_pos");
}
