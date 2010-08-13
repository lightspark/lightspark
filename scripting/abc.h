/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _ABC_H
#define _ABC_H

#include "compat.h"
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/PassManagers.h> 
#include <llvm/LLVMContext.h>
#include "parsing/tags.h"
#include "frame.h"
#include "logger.h"
#include <vector>
#include <deque>
#include <map>
#include <set>
#include "swf.h"

namespace lightspark
{

class s24
{
friend std::istream& operator>>(std::istream& in, s24& v);
private:
	int32_t val;
public:
	operator int32_t(){return val;}
};

class s32
{
friend std::istream& operator>>(std::istream& in, s32& v);
private:
	int32_t val;
public:
	operator int32_t(){return val;}
};

class u32
{
friend std::istream& operator>>(std::istream& in, u32& v);
private:
	uint32_t val;
public:
	operator uint32_t() const{return val;}
};

class u30
{
friend std::istream& operator>>(std::istream& in, u30& v);
private:
	uint32_t val;
public:
	u30():val(0){}
	u30(int v):val(v){}
	operator uint32_t() const{return val;}
};

class u16
{
friend std::istream& operator>>(std::istream& in, u16& v);
private:
	uint32_t val;
public:
	operator uint32_t() const{return val;}
};

class u8
{
friend std::istream& operator>>(std::istream& in, u8& v);
private:
	uint32_t val;
public:
	operator uint32_t() const{return val;}
};

class d64
{
friend std::istream& operator>>(std::istream& in, d64& v);
private:
	double val;
public:
	operator double(){return val;}
};

class string_info
{
friend std::istream& operator>>(std::istream& in, string_info& v);
private:
	u30 size;
	tiny_string val;
public:
	operator const tiny_string&() const{return val;}
};

struct namespace_info
{
	u8 kind;
	u30 name;
};

struct ns_set_info
{
	u30 count;
	std::vector<u30> ns;
};

struct multiname_info
{
	u8 kind;
	u30 name;
	u30 ns;
	u30 ns_set;
	u30 type_definition;
	std::vector<u30> param_types;
	multiname* cached;
	multiname_info():cached(NULL){}
	~multiname_info(){delete cached;}
};

struct cpool_info
{
	u30 int_count;
	std::vector<s32> integer;
	u30 uint_count;
	std::vector<u32> uinteger;
	u30 double_count;
	std::vector<d64> doubles;
	u30 string_count;
	std::vector<string_info> strings;
	u30 namespace_count;
	std::vector<namespace_info> namespaces;
	u30 ns_set_count;
	std::vector<ns_set_info> ns_sets;
	u30 multiname_count;
	std::vector<multiname_info> multinames;
};

struct option_detail
{
	u30 val;
	u8 kind;
};

struct method_body_info;
class method_info;
class ABCContext;
class ABCVm;

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
	std::vector<ASObject*> scope_stack;
	void runtime_stack_push(ASObject* s);
	ASObject* runtime_stack_pop();
	ASObject* runtime_stack_peek();
	call_context(method_info* th, int l, ASObject* const* args, const unsigned int numArgs);
	~call_context();
};

struct block_info
{
	llvm::BasicBlock* BB;
	std::vector<STACK_TYPE> locals;
	std::vector<STACK_TYPE> locals_start;
	std::vector<llvm::Value*> locals_start_obj;
	std::vector<bool> locals_reset;
	std::vector<bool> locals_used;
	std::set<block_info*> preds;
	std::set<block_info*> seqs;
	std::map<int,STACK_TYPE> push_types;

	//Needed for indexed access
	block_info():BB(NULL){abort();}
	block_info(const method_info* mi, const char* blockName);
	STACK_TYPE checkProactiveCasting(int local_ip,STACK_TYPE type);
};

typedef std::pair<llvm::Value*, STACK_TYPE> stack_entry;
inline stack_entry make_stack_entry(llvm::Value* v, STACK_TYPE t)
{
	return std::make_pair(v, t);
}

class method_info
{
friend std::istream& operator>>(std::istream& in, method_info& v);
friend struct block_info;
private:
	enum { NEED_ARGUMENTS=1,NEED_REST=4, HAS_OPTIONAL=8};
	u30 param_count;
	u30 return_type;
	std::vector<u30> param_type;
	std::vector<option_detail> options;
	u30 name;
	u8 flags;

	std::vector<u30> param_names;

	//Helper functions to peek/pop/push in the static stack
	stack_entry static_stack_peek(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack,
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	stack_entry static_stack_pop(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack,
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	void static_stack_push(std::vector<stack_entry>& static_stack, const stack_entry& e);
	//Helper function to generate the right object for a concrete type
	void abstract_value(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, stack_entry& e);

	//Helper functions that generates LLVM to access the stack at runtime
	llvm::Value* llvm_stack_pop(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);
	llvm::Value* llvm_stack_peek(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);
	void llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val,
			llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);

	//Helper functions to sync the static stack and locals to the memory 
	void syncStacks(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack, 
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	void syncLocals(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder,
			const std::vector<stack_entry>& static_locals,llvm::Value* locals,
			const std::vector<STACK_TYPE>& expected,const block_info& dest_block);
	typedef std::vector<std::pair<int, STACK_TYPE> > static_stack_types_vector;
	//Helper function to sync only part of the static stack to the memory
	void consumeStackForRTMultiname(static_stack_types_vector& stack, int multinameIndex) const;

	//Helper function to add a basic block
	void addBlock(std::map<unsigned int,block_info>& blocks, unsigned int ip, const char* blockName);
	std::pair<unsigned int, STACK_TYPE> popTypeFromStack(static_stack_types_vector& stack, unsigned int localIp) const;
	llvm::FunctionType* synt_method_prototype(llvm::ExecutionEngine* ex);
	llvm::Function* llvmf;

	//Does analysis on function code to find optimization chances
	void doAnalysis(std::map<unsigned int,block_info>& blocks, llvm::IRBuilder<>& Builder);

public:
	u30 option_count;
	SyntheticFunction::synt_function f;
	ABCContext* context;
	method_body_info* body;
	SyntheticFunction::synt_function synt_method();
	bool needsArgs() { return (flags & NEED_ARGUMENTS) != 0;}
	bool needsRest() { return (flags & NEED_REST) != 0;}
	bool hasOptional() { return (flags & HAS_OPTIONAL) != 0;}
	ASObject* getOptional(unsigned int i);
	int numArgs() { return param_count; }
	method_info():option_count(0),f(NULL),context(NULL),body(NULL)
	{
	}
};

struct item_info
{
	u30 key;
	u30 value;
};

struct metadata_info
{
	u30 name;
	u30 item_count;
	std::vector<item_info> items;
};

struct traits_info
{
	enum { Slot=0,Method=1,Getter=2,Setter=3,Class=4,Function=5,Const=6};
	enum { Final=0x10, Override=0x20, Metadata=0x40};
	u30 name;
	u8 kind;

	u30 slot_id;
	u30 type_name;
	u30 vindex;
	u8 vkind;
	u30 classi;
	u30 function;
	u30 disp_id;
	u30 method;

	u30 metadata_count;
	std::vector<u30> metadata;
};

struct instance_info
{
	enum { ClassSealed=0x01,ClassFinal=0x02,ClassInterface=0x04,ClassProtectedNs=0x08};
	u30 name;
	u30 supername;
	u8 flags;
	u30 protectedNs;
	u30 interface_count;
	std::vector<u30> interfaces;
	u30 init;
	u30 trait_count;
	std::vector<traits_info> traits;
};

struct class_info
{
	u30 cinit;
	u30 trait_count;
	std::vector<traits_info> traits;
};

struct script_info
{
	u30 init;
	u30 trait_count;
	std::vector<traits_info> traits;
};

class exception_info
{
friend std::istream& operator>>(std::istream& in, exception_info& v);
private:
	u30 from;
	u30 to;
	u30 target;
	u30 exc_type;
	u30 var_name;
};

struct method_body_info
{
	u30 method;
	u30 max_stack;
	u30 local_count;
	u30 init_scope_depth;
	u30 max_scope_depth;
	u30 code_length;
	//This is a string to use it in a stringstream
	std::string code;
	u30 exception_count;
	std::vector<exception_info> exceptions;
	u30 trait_count;
	std::vector<traits_info> traits;
};

struct opcode_handler
{
	const char* name;
	void* addr;
};

enum ARGS_TYPE { ARGS_OBJ_OBJ=0, ARGS_OBJ_INT, ARGS_OBJ, ARGS_INT, ARGS_OBJ_OBJ_INT, ARGS_NUMBER, ARGS_OBJ_NUMBER, 
	ARGS_BOOL, ARGS_INT_OBJ, ARGS_NONE, ARGS_NUMBER_OBJ, ARGS_INT_INT, ARGS_CONTEXT, ARGS_CONTEXT_INT, ARGS_CONTEXT_INT_INT};

struct typed_opcode_handler
{
	const char* name;
	void* addr;
	ARGS_TYPE type;
};

class ABCContext
{
friend class ABCVm;
friend class method_info;
private:
	method_info* get_method(unsigned int m);
	const tiny_string& getString(unsigned int s) const;
	//Qname getQname(unsigned int m, call_context* th=NULL) const;
	static multiname* s_getMultiname(call_context*, ASObject*, int m);
	static multiname* s_getMultiname_i(call_context*, uintptr_t i , int m);
	static multiname* s_getMultiname_d(call_context*, number_t i , int m);
	int getMultinameRTData(int n) const;
	ASObject* getConstant(int kind, int index);
public:
	u16 minor;
	u16 major;
	cpool_info constant_pool;
	u30 method_count;
	std::vector<method_info> methods;
	u30 metadata_count;
	std::vector<metadata_info> metadata;
	u30 class_count;
	std::vector<instance_info> instances;
	std::vector<class_info> classes;
	u30 script_count;
	std::vector<script_info> scripts;
	u30 method_body_count;
	std::vector<method_body_info> method_body;
	void buildTrait(ASObject* obj, const traits_info* t, bool bind, IFunction* deferred_initialization=NULL);
	void linkTrait(Class_base* obj, const traits_info* t);
	void getOptionalConstant(const option_detail& opt);
	multiname* getMultiname(unsigned int m, call_context* th);
	void buildInstanceTraits(ASObject* obj, int class_index);
	ABCContext(std::istream& in) DLL_PUBLIC;
	void exec();
};

struct thisAndLevel
{
	ASObject* cur_this;
	int cur_level;
	thisAndLevel(ASObject* t,int l):cur_this(t),cur_level(l){}
};

class ABCVm
{
friend class ABCContext;
friend class method_info;
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;

	llvm::Module* module;

	void registerClasses();

	void registerFunctions();
	//Interpreted AS instructions
	static bool hasNext2(call_context* th, int n, int m); 
	static void callPropVoid(call_context* th, int n, int m); 
	static void callSuperVoid(call_context* th, int n, int m); 
	static void callSuper(call_context* th, int n, int m); 
	static void callProperty(call_context* th, int n, int m); 
	static void constructProp(call_context* th, int n, int m); 
	static void setLocal(int n); 
	static void setLocal_int(int n,int v); 
	static void setLocal_obj(int n,ASObject* v);
	static void getLocal(ASObject* o, int n); 
	static void getLocal_short(int n); 
	static void getLocal_int(int n, int v); 
	static void newObject(call_context* th, int n); 
	static void getDescendants(call_context* th, int n); 
	static ASObject* newCatch(call_context* th, int n); 
	static void jump(int offset); 
	static bool ifEq(ASObject*, ASObject*); 
	static bool ifStrictEq(ASObject*, ASObject*); 
	static bool ifNE(ASObject*, ASObject*); 
	static bool ifNE_oi(ASObject*, intptr_t); 
	static bool ifLT(ASObject*, ASObject*); 
	static bool ifGT(ASObject*, ASObject*); 
	static bool ifLE(ASObject*, ASObject*); 
	static bool ifLT_oi(ASObject*, intptr_t); 
	static bool ifLT_io(intptr_t, ASObject*); 
	static bool ifNLT(ASObject*, ASObject*); 
	static bool ifNGT(ASObject*, ASObject*); 
	static bool ifNGE(ASObject*, ASObject*); 
	static bool ifGE(ASObject*, ASObject*); 
	static bool ifNLE(ASObject*, ASObject*); 
	static bool ifStrictNE(ASObject*, ASObject*); 
	static bool ifFalse(ASObject*); 
	static bool ifTrue(ASObject*); 
	static ASObject* getSlot(ASObject* th, int n); 
	static void setSlot(ASObject*, ASObject*, int n); 
	static void kill(int n); 
	static ASObject* pushString(call_context* th, int n); 
	static void getLex(call_context* th, int n); 
	static ASObject* getScopeObject(call_context* th, int n); 
	static void deleteProperty(call_context* th, int n); 
	static void initProperty(call_context* th, int n); 
	static void newClass(call_context* th, int n); 
	static void newArray(call_context* th, int n); 
	static ASObject* findPropStrict(call_context* th, int n);
	static ASObject* findProperty(call_context* th, int n);
	static intptr_t pushByte(intptr_t n);
	static intptr_t pushShort(intptr_t n);
	static void pushInt(call_context* th, int n);
	static void pushUInt(call_context* th, int n);
	static void pushDouble(call_context* th, int n);
	static void incLocal_i(call_context* th, int n);
	static void coerce(call_context* th, int n);
	static ASObject* getProperty(ASObject* obj, multiname* name);
	static intptr_t getProperty_i(ASObject* obj, multiname* name);
	static void setProperty(ASObject* value,ASObject* obj, multiname* name);
	static void setProperty_i(intptr_t value,ASObject* obj, multiname* name);
	static void call(call_context* th, int n);
	static void constructSuper(call_context* th, int n);
	static void construct(call_context* th, int n);
	static void constructGenericType(call_context* th, int n);
	static ASObject* newFunction(call_context* th, int n);
	static void setSuper(call_context* th, int n);
	static void getSuper(call_context* th, int n);
	static void pushScope(call_context* obj);
	static void pushWith(call_context* th);
	static ASObject* pushNull();
	static ASObject* pushUndefined();
	static ASObject* pushNaN();
	static bool pushFalse();
	static bool pushTrue();
	static void dup();
	static bool in(ASObject*,ASObject*);
	static bool _not(ASObject*);
	static bool equals(ASObject*,ASObject*);
	static number_t negate(ASObject*);
	static void pop();
	static ASObject* typeOf(ASObject*);
	static void _throw(call_context* th);
	static ASObject* asTypelate(ASObject* type, ASObject* obj);
	static bool isTypelate(ASObject* type, ASObject* obj);
	static bool isType(ASObject* obj, multiname* name);
	static void swap();
	static ASObject* add(ASObject*,ASObject*);
	static ASObject* add_oi(ASObject*,intptr_t);
	static ASObject* add_od(ASObject*,number_t);
	static uintptr_t bitAnd(ASObject*,ASObject*);
	static uintptr_t bitNot(ASObject*);
	static uintptr_t bitAnd_oi(ASObject* val1, intptr_t val2);
	static uintptr_t bitOr(ASObject*,ASObject*);
	static uintptr_t bitOr_oi(ASObject*,uintptr_t);
	static uintptr_t bitXor(ASObject*,ASObject*);
	static uintptr_t rShift(ASObject*,ASObject*);
	static uintptr_t urShift(ASObject*,ASObject*);
	static uintptr_t urShift_io(uintptr_t,ASObject*);
	static uintptr_t lShift(ASObject*,ASObject*);
	static uintptr_t lShift_io(uintptr_t,ASObject*);
	static number_t multiply(ASObject*,ASObject*);
	static number_t multiply_oi(ASObject*, intptr_t);
	static number_t divide(ASObject*,ASObject*);
	static intptr_t modulo(ASObject*,ASObject*);
	static number_t subtract(ASObject*,ASObject*);
	static number_t subtract_oi(ASObject*, intptr_t);
	static number_t subtract_io(intptr_t, ASObject*);
	static number_t subtract_do(number_t, ASObject*);
	static void popScope(call_context* th);
	static ASObject* newActivation(call_context* th, method_info*);
	static ASObject* coerce_s(ASObject*);
	static ASObject* checkfilter(ASObject*);
	static void coerce_a();
	static void label();
	static void lookupswitch();
	static intptr_t convert_i(ASObject*);
	static uintptr_t convert_u(ASObject*);
	static number_t convert_d(ASObject*);
	static bool convert_b(ASObject*);
	static bool greaterThan(ASObject*,ASObject*);
	static bool greaterEquals(ASObject*,ASObject*);
	static bool lessEquals(ASObject*,ASObject*);
	static bool lessThan(ASObject*,ASObject*);
	static ASObject* nextName(ASObject* index, ASObject* obj);
	static ASObject* nextValue(ASObject* index, ASObject* obj);
	static uintptr_t increment_i(ASObject*);
	static uintptr_t increment(ASObject*);
	static uintptr_t decrement(ASObject*);
	static uintptr_t decrement_i(ASObject*);
	static ASObject* getGlobalScope();
	static bool strictEquals(ASObject*,ASObject*);
	//Utility
	static void not_impl(int p);
	ASFUNCTION(print);

	//Internal utilities
	static void method_reset(method_info* th);
	static void newClassRecursiveLink(Class_base* target, Class_base* c);

	//Opcode tables
	void register_table(const llvm::Type* ret_type,typed_opcode_handler* table, int table_len);
	static opcode_handler opcode_table_args_pointer_2int[];
	static opcode_handler opcode_table_args_pointer_number_int[];
	static opcode_handler opcode_table_args3_pointers[];
	static typed_opcode_handler opcode_table_uintptr_t[];
	static typed_opcode_handler opcode_table_number_t[];
	static typed_opcode_handler opcode_table_void[];
	static typed_opcode_handler opcode_table_voidptr[];
	static typed_opcode_handler opcode_table_bool_t[];

	//Synchronization
	sem_t event_queue_mutex;
	sem_t sem_event_count;

	//Event handling
	bool shuttingdown;
	std::deque<std::pair<EventDispatcher*,Event*> > events_queue;
	void handleEvent(std::pair<EventDispatcher*,Event*> e);

	void buildClassAndInjectBase(const std::string& n, ASObject*, ASObject* const* a, const unsigned int argslen, bool isRoot);

	//These are used to keep track of the current 'this' for class methods, and relative level
	//It's sane to have them per-Vm, as anyway the vm is single by specs, single threaded
	std::vector<thisAndLevel> method_this_stack;

public:
	ASObject* Global;
	Manager* int_manager;
	Manager* number_manager;
	llvm::ExecutionEngine* ex;
	llvm::FunctionPassManager* FPM;
	llvm::LLVMContext llvm_context;
	ABCVm(SystemState* s) DLL_PUBLIC;
	/**
		Destroys the VM

		@pre shutdown must have been called
	*/
	~ABCVm();
	static void Run(ABCVm* th);
	static ASObject* executeFunction(SyntheticFunction* function, call_context* context);
	bool addEvent(EventDispatcher*,Event*) DLL_PUBLIC;
	int getEventQueueSize();
	void shutdown();
	void wait();

	void pushObjAndLevel(ASObject* o, int l);
	thisAndLevel popObjAndLevel();
	thisAndLevel getCurObjAndLevel()
	{
		return method_this_stack.back();
	}

	static bool strictEqualImpl(ASObject*, ASObject*);

};

class DoABCTag: public ControlTag
{
private:
	UI32 Flags;
	STRING Name;
	ABCContext* context;
	pthread_t thread;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
	~DoABCTag();
	void execute(RootMovieClip* root);
};

class SymbolClassTag: public ControlTag
{
private:
	UI16 NumSymbols;
	std::vector<UI16> Tags;
	std::vector<STRING> Names;
public:
	SymbolClassTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root);
};

ASObject* undefinedFunction(ASObject* obj,ASObject* const* args, const unsigned int argslen);

inline ASObject* getGlobal()
{
	return sys->currentVm->Global;
}

inline ABCVm* getVm()
{
	return sys->currentVm;
}

std::istream& operator>>(std::istream& in, u8& v);
std::istream& operator>>(std::istream& in, u16& v);
std::istream& operator>>(std::istream& in, u30& v);
std::istream& operator>>(std::istream& in, u32& v);
std::istream& operator>>(std::istream& in, s32& v);
std::istream& operator>>(std::istream& in, d64& v);
std::istream& operator>>(std::istream& in, string_info& v);
std::istream& operator>>(std::istream& in, namespace_info& v);
std::istream& operator>>(std::istream& in, ns_set_info& v);
std::istream& operator>>(std::istream& in, multiname_info& v);
std::istream& operator>>(std::istream& in, cpool_info& v);
std::istream& operator>>(std::istream& in, method_info& v);
std::istream& operator>>(std::istream& in, method_body_info& v);
std::istream& operator>>(std::istream& in, instance_info& v);
std::istream& operator>>(std::istream& in, traits_info& v);
std::istream& operator>>(std::istream& in, script_info& v);
std::istream& operator>>(std::istream& in, metadata_info& v);
std::istream& operator>>(std::istream& in, class_info& v);

};

#endif
