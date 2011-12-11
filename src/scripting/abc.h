/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifdef LLVM_28
#define alignof alignOf
#endif
#ifdef LLVM_3
#define LLVMTYPE llvm::Type*
#define LLVMMAKEARRAYREF(T) makeArrayRef(T)
#else
#define LLVMTYPE const llvm::Type*
#define LLVMMAKEARRAYREF(T) T
#endif

#include "compat.h"
#include <cstddef>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/PassManager.h> 
#include <llvm/LLVMContext.h>
#include "parsing/tags.h"
#include "logger.h"
#include <vector>
#include <deque>
#include <map>
#include <set>
#include "swf.h"

namespace lightspark
{

bool isVmThread();

class u8
{
friend std::istream& operator>>(std::istream& in, u8& v);
private:
	uint32_t val;
public:
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

class s24
{
friend std::istream& operator>>(std::istream& in, s24& v);
private:
	int32_t val;
public:
	operator int32_t(){return val;}
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

class s32
{
friend std::istream& operator>>(std::istream& in, s32& v);
private:
	int32_t val;
public:
	operator int32_t(){return val;}
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
	bool isAttributeName() const;
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
std::ostream& operator<<(std::ostream& o, const block_info& b);

typedef std::pair<llvm::Value*, STACK_TYPE> stack_entry;
inline stack_entry make_stack_entry(llvm::Value* v, STACK_TYPE t)
{
	return std::make_pair(v, t);
}

class method_body_info;

class method_info
{
friend std::istream& operator>>(std::istream& in, method_info& v);
friend struct block_info;
friend class SyntheticFunction;
private:
	enum { NEED_ARGUMENTS=1,NEED_REST=4, HAS_OPTIONAL=8};
	u30 param_count;
	u30 return_type;
	std::vector<u30> param_type;
	std::vector<option_detail> options;
	u8 flags;

	std::vector<u30> param_names;

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
#ifdef PROFILING_SUPPORT
	std::map<method_info*,uint64_t> profCalls;
	std::vector<uint64_t> profTime;
	tiny_string profName;
	bool validProfName;
#endif

	u30 option_count;
	SyntheticFunction::synt_function f;
	u30 name;
	ABCContext* context;
	method_body_info* body;
	SyntheticFunction::synt_function synt_method();
	bool needsArgs() { return (flags & NEED_ARGUMENTS) != 0;}
	bool needsRest() { return (flags & NEED_REST) != 0;}
	bool hasOptional() { return (flags & HAS_OPTIONAL) != 0;}
	ASObject* getOptional(unsigned int i);
	uint32_t numArgs() { return param_count; }
	const multiname* paramTypeName(uint32_t i) const;
	const multiname* returnTypeName() const;

	std::vector<const Type*> paramTypes;
	const Type* returnType;
	bool hasExplicitTypes;
	method_info():
#ifdef PROFILING_SUPPORT
		profTime(0),
		validProfName(false),
#endif
		option_count(0),f(NULL),context(NULL),body(NULL),returnType(NULL)
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
	bool isInterface() const { return flags&ClassInterface; }
	bool isSealed() const { return flags&ClassSealed; }
	bool isFinal() const { return flags&ClassFinal; }
	bool isProtectedNs() const { return flags&ClassProtectedNs; }
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

struct exception_info
{
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
	ARGS_BOOL, ARGS_INT_OBJ, ARGS_NONE, ARGS_NUMBER_OBJ, ARGS_INT_INT, ARGS_CONTEXT, ARGS_CONTEXT_INT, ARGS_CONTEXT_INT_INT,
	ARGS_CONTEXT_INT_INT_INT, ARGS_CONTEXT_INT_INT_INT_BOOL, ARGS_CONTEXT_OBJ_OBJ_INT, ARGS_CONTEXT_OBJ, ARGS_CONTEXT_OBJ_OBJ,
	ARGS_CONTEXT_OBJ_OBJ_OBJ, ARGS_OBJ_OBJ_OBJ_INT, ARGS_OBJ_OBJ_OBJ };

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
public:
	method_info* get_method(unsigned int m);
	const tiny_string& getString(unsigned int s) const;
	//Qname getQname(unsigned int m, call_context* th=NULL) const;
	static multiname* s_getMultiname(ABCContext*, ASObject* rt1, ASObject* rt2, int m);
	static multiname* s_getMultiname_i(call_context*, uint32_t i , int m);
	static multiname* s_getMultiname_d(call_context*, number_t i , int m);
	ASObject* getConstant(int kind, int index);
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

	std::vector<bool> hasRunScriptInit;
	/**
		Construct and insert in the a object a given trait
		@param obj the tarhget object
		@param t the trait to build
		@param isBorrowed True if we're building a trait on behalf of an object, False otherwise
		@param deferred_initialization A pointer to a function that can be used to build the given trait later
	*/
	void buildTrait(ASObject* obj, const traits_info* t, bool isBorrowed, int scriptid=-1);
	void runScriptInit(unsigned int scriptid, ASObject* g);

	void linkTrait(Class_base* obj, const traits_info* t);
	void getOptionalConstant(const option_detail& opt);
	int getMultinameRTData(int n) const;
	multiname* getMultiname(unsigned int m, call_context* th);
	multiname* getMultinameImpl(ASObject* rt1, ASObject* rt2, unsigned int m);
	void buildInstanceTraits(ASObject* obj, int class_index);
	ABCContext(std::istream& in) DLL_PUBLIC;
	void exec(bool lazy);

	static bool isinstance(ASObject* obj, multiname* name);

#ifdef PROFILING_SUPPORT
	void dumpProfilingData(std::ostream& f) const;
#endif
};

class ABCVm
{
friend class ABCContext;
friend class method_info;
private:
	std::vector<ABCContext*> contexts;
	SystemState* m_sys;
	Thread* t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	STATUS status;

	llvm::Module* module;

	void registerClasses();

	void registerFunctions();
	//Interpreted AS instructions
	//If you change a definition here, update the opcode_table_* entry in abc_codesynth
	static bool hasNext2(call_context* th, int n, int m); 
	static void callSuper(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callProperty(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callImpl(call_context* th, ASObject* f, ASObject* obj, ASObject** args, int m, method_info** called_mi, bool keepReturn);
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
	static bool ifNE_oi(ASObject*, int32_t);
	static bool ifLT(ASObject*, ASObject*); 
	static bool ifGT(ASObject*, ASObject*); 
	static bool ifLE(ASObject*, ASObject*); 
	static bool ifLT_oi(ASObject*, int32_t);
	static bool ifLT_io(int32_t, ASObject*);
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
	static bool deleteProperty(ASObject* obj, multiname* name);
	static void initProperty(ASObject* obj, ASObject* val, multiname* name);
	static void newClass(call_context* th, int n); 
	static void newArray(call_context* th, int n); 
	static ASObject* findPropStrict(call_context* th, multiname* name);
	static ASObject* findProperty(call_context* th, multiname* name);
	static int32_t pushByte(intptr_t n);
	static int32_t pushShort(intptr_t n);
	static void pushInt(call_context* th, int n);
	static void pushUInt(call_context* th, int n);
	static void pushDouble(call_context* th, int n);
	static void incLocal_i(call_context* th, int n);
	static void incLocal(call_context* th, int n);
	static void decLocal_i(call_context* th, int n);
	static void decLocal(call_context* th, int n);
	static void coerce(call_context* th, int n);
	static ASObject* getProperty(ASObject* obj, multiname* name);
	static int32_t getProperty_i(ASObject* obj, multiname* name);
	static void setProperty(ASObject* value,ASObject* obj, multiname* name);
	static void setProperty_i(int32_t value,ASObject* obj, multiname* name);
	static void call(call_context* th, int n, method_info** called_mi);
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
	static int32_t negate_i(ASObject*);
	static void pop();
	static ASObject* typeOf(ASObject*);
	static void _throw(call_context* th);
	static ASObject* asType(ASObject* obj, multiname* name);
	static ASObject* asTypelate(ASObject* type, ASObject* obj);
	static bool isTypelate(ASObject* type, ASObject* obj);
	static bool isType(ASObject* obj, multiname* name);
	static void swap();
	static ASObject* add(ASObject*,ASObject*);
	static int32_t add_i(ASObject*,ASObject*);
	static ASObject* add_oi(ASObject*,int32_t);
	static ASObject* add_od(ASObject*,number_t);
	static uint32_t bitAnd(ASObject*,ASObject*);
	static uint32_t bitNot(ASObject*);
	static uint32_t bitAnd_oi(ASObject* val1, int32_t val2);
	static uint32_t bitOr(ASObject*,ASObject*);
	static uint32_t bitOr_oi(ASObject*,uint32_t);
	static uint32_t bitXor(ASObject*,ASObject*);
	static uint32_t rShift(ASObject*,ASObject*);
	static uint32_t urShift(ASObject*,ASObject*);
	static uint32_t urShift_io(uint32_t,ASObject*);
	static uint32_t lShift(ASObject*,ASObject*);
	static uint32_t lShift_io(uint32_t,ASObject*);
	static number_t multiply(ASObject*,ASObject*);
	static int32_t multiply_i(ASObject*,ASObject*);
	static number_t multiply_oi(ASObject*, int32_t);
	static number_t divide(ASObject*,ASObject*);
	static number_t modulo(ASObject*,ASObject*);
	static number_t subtract(ASObject*,ASObject*);
	static int32_t subtract_i(ASObject*,ASObject*);
	static number_t subtract_oi(ASObject*, int32_t);
	static number_t subtract_io(int32_t, ASObject*);
	static number_t subtract_do(number_t, ASObject*);
	static void popScope(call_context* th);
	static ASObject* newActivation(call_context* th, method_info*);
	static ASObject* coerce_s(ASObject*);
	static ASObject* checkfilter(ASObject*);
	static void coerce_a();
	static void label();
	static void lookupswitch();
	static int32_t convert_i(ASObject*);
	static uint32_t convert_u(ASObject*);
	static number_t convert_d(ASObject*);
	static ASObject* convert_s(ASObject*);
	static bool convert_b(ASObject*);
	static bool greaterThan(ASObject*,ASObject*);
	static bool greaterEquals(ASObject*,ASObject*);
	static bool lessEquals(ASObject*,ASObject*);
	static bool lessThan(ASObject*,ASObject*);
	static ASObject* nextName(ASObject* index, ASObject* obj);
	static ASObject* nextValue(ASObject* index, ASObject* obj);
	static uint32_t increment_i(ASObject*);
	static number_t increment(ASObject*);
	static number_t decrement(ASObject*);
	static uint32_t decrement_i(ASObject*);
	static bool strictEquals(ASObject*,ASObject*);
	static ASObject* esc_xattr(ASObject* o);
	static bool instanceOf(ASObject* value, ASObject* type);
	static Namespace* pushNamespace(call_context* th, int n);
	//Utility
	static void not_impl(int p);
	static void wrong_exec_pos();

	//Internal utilities
	static void method_reset(method_info* th);
	static void newClassRecursiveLink(Class_base* target, Class_base* c);
	static ASObject* constructFunction(call_context* th, IFunction* f, ASObject** args, int argslen);

	//Opcode tables
	void register_table(LLVMTYPE ret_type,typed_opcode_handler* table, int table_len);
	static opcode_handler opcode_table_args_pointer_2int[];
	static opcode_handler opcode_table_args_pointer_number_int[];
	static opcode_handler opcode_table_args3_pointers[];
	static typed_opcode_handler opcode_table_uint32_t[];
	static typed_opcode_handler opcode_table_number_t[];
	static typed_opcode_handler opcode_table_void[];
	static typed_opcode_handler opcode_table_voidptr[];
	static typed_opcode_handler opcode_table_bool_t[];

	//Synchronization
	Mutex event_queue_mutex;
	Cond sem_event_cond;

	//Event handling
	volatile bool shuttingdown;
	std::deque<std::pair<_NR<EventDispatcher>,_R<Event> > > events_queue;
	void handleEvent(std::pair<_NR<EventDispatcher>,_R<Event> > e);
	void signalEventWaiters();
	void buildClassAndBindTag(const std::string& s, _R<DictionaryTag> t);
	void buildClassAndInjectBase(const std::string& s, _R<RootMovieClip> base);
	Class_inherit* findClassInherit(const std::string& s);

	//Profiling support
	static uint64_t profilingCheckpoint(uint64_t& startTime);
public:
	Global* curGlobalObj;
	GlobalObject* global;
	Manager* int_manager;
	Manager* uint_manager;
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
	/**
	  	Start the VM thread
	*/
	void start() DLL_PUBLIC;
	void finalize();
	static void Run(ABCVm* th);
	static ASObject* executeFunction(const SyntheticFunction* function, call_context* context);
	bool addEvent(_NR<EventDispatcher>,_R<Event> ) DLL_PUBLIC;
	int getEventQueueSize();
	void shutdown();

	static Global* getGlobalScope(call_context* th);
	static bool strictEqualImpl(ASObject*, ASObject*);
	static void publicHandleEvent(_R<EventDispatcher> dispatcher, _R<Event> event);

	/* The current recursion level. Each call increases this by one,
	 * each return from a call decreases this. */
	uint32_t cur_recursion;

	struct abc_limits {
		/* maxmium number of recursion allowed. See ScriptLimitsTag */
		uint32_t max_recursion;
		/* maxmium number of seconds for script execution. See ScriptLimitsTag */
		uint32_t script_timeout;
	} limits;

};

class DoABCTag: public ControlTag
{
private:
	ABCContext* context;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const{ return ABC_TAG; }
	void execute(RootMovieClip* root);
};

class DoABCDefineTag: public ControlTag
{
private:
	UI32_SWF Flags;
	STRING Name;
	ABCContext* context;
public:
	DoABCDefineTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const{ return ABC_TAG; }
	void execute(RootMovieClip* root);
};

class SymbolClassTag: public ControlTag
{
private:
	UI16_SWF NumSymbols;
	std::vector<UI16_SWF> Tags;
	std::vector<STRING> Names;
public:
	SymbolClassTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const{ return ABC_TAG; }
	void execute(RootMovieClip* root);
};

ASObject* undefinedFunction(ASObject* obj,ASObject* const* args, const unsigned int argslen);

inline GlobalObject* getGlobal()
{
	return getSys()->currentVm->global;
}

inline ABCVm* getVm()
{
	return getSys()->currentVm;
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
std::istream& operator>>(std::istream& in, exception_info& v);
std::istream& operator>>(std::istream& in, method_body_info& v);
std::istream& operator>>(std::istream& in, instance_info& v);
std::istream& operator>>(std::istream& in, traits_info& v);
std::istream& operator>>(std::istream& in, script_info& v);
std::istream& operator>>(std::istream& in, metadata_info& v);
std::istream& operator>>(std::istream& in, class_info& v);

};

#endif
