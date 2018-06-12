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

#ifndef SCRIPTING_ABC_H
#define SCRIPTING_ABC_H 1

#include "compat.h"
#include <cstddef>
#include "parsing/tags.h"
#include "logger.h"
#include <vector>
#include <deque>
#include <map>
#include <set>
#include "swf.h"
#include "scripting/abcutils.h"
#include "scripting/abctypes.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/toplevel/toplevel.h"

namespace llvm {
	class ExecutionEngine;
#ifdef LLVM_36
namespace legacy {
	class FunctionPassManager;
}
#else
	class FunctionPassManager;
#endif
	class FunctionType;
	class Function;
	class Module;
	class Type;
	class Value;
	class LLVMContext;
}

namespace lightspark
{
struct block_info;
#ifdef LLVM_28
typedef const llvm::Type* LLVMTYPE;
#else
typedef llvm::Type* LLVMTYPE;
#endif

bool isVmThread();

std::ostream& operator<<(std::ostream& o, const block_info& b);

typedef std::pair<llvm::Value*, STACK_TYPE> stack_entry;
inline stack_entry make_stack_entry(llvm::Value* v, STACK_TYPE t)
{
	return std::make_pair(v, t);
}

class method_info
{
friend std::istream& operator>>(std::istream& in, method_info& v);
friend struct block_info;
friend class SyntheticFunction;
private:
	struct method_info_simple info;

	typedef std::vector<std::pair<int, STACK_TYPE> > static_stack_types_vector;
	//Helper function to sync only part of the static stack to the memory
	void consumeStackForRTMultiname(static_stack_types_vector& stack, int multinameIndex) const;

	//Helper function to add a basic block
	void addBlock(std::map<unsigned int,block_info>& blocks, unsigned int ip, const char* blockName);
	std::pair<unsigned int, STACK_TYPE> popTypeFromStack(static_stack_types_vector& stack, unsigned int localIp) const;
	llvm::FunctionType* synt_method_prototype(llvm::ExecutionEngine* ex);
	llvm::Function* llvmf;

	// Wrapper needed because llvm::IRBuilder is a template, cannot forward declare
	struct BuilderWrapper;
	//Does analysis on function code to find optimization chances
	void doAnalysis(std::map<unsigned int,block_info>& blocks, BuilderWrapper& builderWrapper);

public:
#ifdef PROFILING_SUPPORT
	std::map<method_info*,uint64_t> profCalls;
	std::vector<uint64_t> profTime;
	tiny_string profName;
	bool validProfName;
#endif

	SyntheticFunction::synt_function f;
	ABCContext* context;
	method_body_info* body;
	SyntheticFunction::synt_function synt_method(SystemState* sys);
	bool needsArgs() { return info.needsArgs(); }
	bool needsActivation() { return info.needsActivation(); }
	bool needsRest() { return info.needsRest(); }
	bool hasOptional() { return info.hasOptional(); }
	bool hasDXNS() { return info.hasDXNS(); }
	bool hasParamNames() { return info.hasParamNames(); }
	void getOptional(asAtom &ret, unsigned int i);
	uint32_t numOptions() { return info.option_count; }
	uint32_t numArgs() { return info.param_count; }
	const multiname* paramTypeName(uint32_t i) const;
	const multiname* returnTypeName() const;

	std::vector<const Type*> paramTypes;
	const Type* returnType;
	bool hasExplicitTypes;
	method_info():
		llvmf(NULL),
#ifdef PROFILING_SUPPORT
		profTime(0),
		validProfName(false),
#endif
		f(NULL),context(NULL),body(NULL),returnType(NULL),hasExplicitTypes(false)
	{
	}
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

enum OPERANDTYPES { 
	OP_UNDEFINED=0x00, OP_STRING=0x01, OP_INTEGER=0x03, OP_UINTEGER=0x04, OP_DOUBLE=0x06, OP_NAMESPACE=0x08, 
	OP_FALSE=0x0a, OP_TRUE=0x0b, OP_NULL=0x0c, 
	OP_LOCAL=0x10, OP_BYTE=0x20, OP_SHORT=0x30};

#define ABC_OP_CACHED 0x10000000 
#define ABC_OP_NOTCACHEABLE 0x20000000 

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
	_R<RootMovieClip> root;

	method_info* get_method(unsigned int m);
	uint32_t getString(unsigned int s) const;
	//Qname getQname(unsigned int m, call_context* th=NULL) const;
	static multiname* s_getMultiname(ABCContext*, asAtom& rt1, ASObject* rt2, int m);
	static multiname* s_getMultiname_i(call_context*, uint32_t i , int m);
	static multiname* s_getMultiname_d(call_context*, number_t i , int m);
	void getConstant(asAtom& ret, int kind, int index);
	asAtom* getConstantAtom(OPERANDTYPES kind, int index);
	u16 minor;
	u16 major;
	cpool_info constant_pool;
	u30 method_count;
	std::vector<method_info, reporter_allocator<method_info>> methods;
	u30 metadata_count;
	std::vector<metadata_info, reporter_allocator<metadata_info>> metadata;
	u30 class_count;
	std::vector<instance_info, reporter_allocator<instance_info>> instances;
	std::vector<class_info, reporter_allocator<class_info>> classes;
	u30 script_count;
	std::vector<script_info, reporter_allocator<script_info>> scripts;
	u30 method_body_count;
	std::vector<method_body_info, reporter_allocator<method_body_info>> method_body;
	//Base for namespaces in this context
	uint32_t namespaceBaseId;

	
	std::vector<bool> hasRunScriptInit;
	std::vector<asAtom> constantAtoms_integer;
	std::vector<asAtom> constantAtoms_uinteger;
	std::vector<asAtom> constantAtoms_doubles;
	std::vector<asAtom> constantAtoms_strings;
	std::vector<asAtom> constantAtoms_namespaces;
	std::vector<asAtom> constantAtoms_byte;
	std::vector<asAtom> constantAtoms_short;
	/**
		Construct and insert in the a object a given trait
		@param obj the tarhget object
		@param additionalslots multiname of t will be added if slot_is is 0
		@param t the trait to build
		@param isBorrowed True if we're building a trait on behalf of an object, False otherwise
		@param deferred_initialization A pointer to a function that can be used to build the given trait later
	*/
	void buildTrait(ASObject* obj, std::vector<multiname *> &additionalslots, const traits_info* t, bool isBorrowed, int scriptid=-1, bool checkExisting=true);
	void runScriptInit(unsigned int scriptid, asAtom& g);

	void linkTrait(Class_base* obj, const traits_info* t);
	void getOptionalConstant(const option_detail& opt);

	/* This function determines how many stack values are needed for
	 * resolving the multiname at index mi
	 */
	inline int getMultinameRTData(int mi) const
	{
		if(mi==0)
			return 0;

		const multiname_info* m=&constant_pool.multinames[mi];
		switch(m->kind)
		{
			case 0x07: //QName
			case 0x0d: //QNameA
			case 0x09: //Multiname
			case 0x0e: //MultinameA
			case 0x1d: //Templated name
				return 0;
			case 0x0f: //RTQName
			case 0x10: //RTQNameA
			case 0x1b: //MultinameL
			case 0x1c: //MultinameLA
				return 1;
			case 0x11: //RTQNameL
			case 0x12: //RTQNameLA
				return 2;
			default:
				LOG(LOG_ERROR,_("getMultinameRTData not yet implemented for this kind ") << m->kind);
				throw UnsupportedException("kind not implemented for getMultinameRTData");
		}
	}
	
	multiname* getMultiname(unsigned int m, call_context* th);
	multiname* getMultinameImpl(asAtom& rt1, ASObject* rt2, unsigned int m, bool isrefcounted = true);
	void buildInstanceTraits(ASObject* obj, int class_index);
	ABCContext(_R<RootMovieClip> r, std::istream& in, ABCVm* vm) DLL_PUBLIC;
	void exec(bool lazy);

	bool isinstance(ASObject* obj, multiname* name);
#ifdef PROFILING_SUPPORT
	void dumpProfilingData(std::ostream& f) const;
#endif
};

struct BasicBlock;
struct InferenceData;

class ABCVm
{
friend class ABCContext;
friend class method_info;
friend class asAtom;
private:
	std::vector<ABCContext*> contexts;
	SystemState* m_sys;
	Thread* t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	STATUS status;

	void registerClassesToplevel(Global* builtin);
	void registerClasses();

	void registerFunctions();
	//Interpreted AS instructions
	//If you change a definition here, update the opcode_table_* entry in abc_codesynth
	static ASObject *hasNext(ASObject* obj, ASObject* cur_index); 
	static bool hasNext2(call_context* th, int n, int m); 
	template<class T>
	static void loadIntN(call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=arg1->toUInt();
		_R<ApplicationDomain> appDomain = getCurrentApplicationDomain(th);
		T ret=appDomain->readFromDomainMemory<T>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtom(ret));
	}
	template<class T>
	static void storeIntN(call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		uint32_t addr=arg1->toUInt();
		ASATOM_DECREF_POINTER(arg1);
		int32_t val=arg2->toInt();
		ASATOM_DECREF_POINTER(arg2);
		_R<ApplicationDomain> appDomain = getCurrentApplicationDomain(th);
		appDomain->writeToDomainMemory<T>(addr, val);
	}
	static void loadFloat(call_context* th);
	static void loadDouble(call_context* th);
	static void storeFloat(call_context* th);
	static void storeDouble(call_context* th);

	static void callStatic(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callSuper(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callProperty(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callPropLex(call_context* th, int n, int m, method_info** called_mi, bool keepReturn);
	static void callPropIntern(call_context* th, int n, int m, bool keepReturn, bool callproplex, preloadedcodedata *instrptr);
	static void callMethod(call_context* th, int n, int m);
	static void callImpl(call_context* th, asAtom& f, asAtom &obj, asAtom *args, int m, bool keepReturn);
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
	static bool getLex(call_context* th, int n, int localresult=0); 
	static ASObject* getScopeObject(call_context* th, int n); 
	static bool deleteProperty(ASObject* obj, multiname* name);
	static void initProperty(ASObject* obj, ASObject* val, multiname* name);
	static void newClass(call_context* th, int n);
	static void newArray(call_context* th, int n); 
	static ASObject* findPropStrict(call_context* th, multiname* name);
	static void findPropStrictCache(asAtom & ret, call_context* th);
	static ASObject* findProperty(call_context* th, multiname* name);
	static int32_t pushByte(intptr_t n);
	static int32_t pushShort(intptr_t n);
	static void pushInt(call_context* th, int32_t val);
	static void pushUInt(call_context* th, uint32_t val);
	static void pushDouble(call_context* th, double val);
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
	static ASObject* asType(ABCContext* context, ASObject* obj, multiname* name);
	static ASObject* asTypelate(ASObject* type, ASObject* obj);
	static bool isTypelate(ASObject* type, ASObject* obj);
	static bool isType(ABCContext* context, ASObject* obj, multiname* name);
	static void swap();
	static ASObject* add(ASObject*,ASObject*);
	static int32_t add_i(ASObject*,ASObject*);
	static ASObject* add_oi(ASObject*,int32_t);
	static ASObject* add_od(ASObject*,number_t);
	static int32_t bitAnd(ASObject*,ASObject*);
	static int32_t bitNot(ASObject*);
	static int32_t bitAnd_oi(ASObject* val1, int32_t val2);
	static int32_t bitOr(ASObject*,ASObject*);
	static int32_t bitOr_oi(ASObject*,int32_t);
	static int32_t bitXor(ASObject*,ASObject*);
	static int32_t rShift(ASObject*,ASObject*);
	static uint32_t urShift(ASObject*,ASObject*);
	static uint32_t urShift_io(uint32_t,ASObject*);
	static int32_t lShift(ASObject*,ASObject*);
	static int32_t lShift_io(uint32_t,ASObject*);
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
	static ASObject* newActivation(call_context* th, method_info* mi);
	static ASObject* coerce_s(ASObject*);
	static ASObject* checkfilter(ASObject*);
	static void coerce_a();
	static void label();
	static void lookupswitch();
	static int32_t convert_i(ASObject*);
	static int64_t convert_di(ASObject*);
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
	static uint64_t increment_di(ASObject*);
	static number_t increment(ASObject*);
	static number_t decrement(ASObject*);
	static uint32_t decrement_i(ASObject*);
	static uint64_t decrement_di(ASObject*);
	static bool strictEquals(ASObject*,ASObject*);
	static ASObject* esc_xattr(ASObject* o);
	static ASObject* esc_xelem(ASObject* o);
	static bool instanceOf(ASObject* value, ASObject* type);
	static Namespace* pushNamespace(call_context* th, int n);
	static void dxns(call_context* th, int n);
	static void dxnslate(call_context* th, ASObject* value);
	//Utility
	static void not_impl(int p);
	static void wrong_exec_pos();

	//Internal utilities
	static void method_reset(method_info* th);

	static void SetAllClassLinks();
	static void AddClassLinks(Class_base* target);
	static bool newClassRecursiveLink(Class_base* target, Class_base* c);
	static void constructFunction(asAtom & ret, call_context* th, asAtom& f, asAtom* args, int argslen);
	void parseRPCMessage(_R<ByteArray> message, _NR<ASObject> client, _NR<Responder> responder);

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
	typedef std::pair<_NR<EventDispatcher>,_R<Event>> eventType;
	std::deque<eventType, reporter_allocator<eventType>> events_queue;
	void handleEvent(std::pair<_NR<EventDispatcher>,_R<Event> > e);
	void handleFrontEvent();
	void signalEventWaiters();
	void buildClassAndInjectBase(const std::string& s, _R<RootMovieClip> base);
	Class_inherit* findClassInherit(const std::string& s, RootMovieClip* r);

	//Profiling support
	static uint64_t profilingCheckpoint(uint64_t& startTime);
	// The base to assign to the next loaded context
	ATOMIC_INT32(nextNamespaceBase);

	typedef void (*abc_function)(call_context*);
	
	static void abc_bkpt(call_context* context);// 0x01
	static void abc_nop(call_context* context);
	static void abc_throw(call_context* context);
	static void abc_getSuper(call_context* context);
	static void abc_setSuper(call_context* context);
	static void abc_dxns(call_context* context);
	static void abc_dxnslate(call_context* context);
	static void abc_kill(call_context* context);
	static void abc_label(call_context* context);
	static void abc_ifnlt(call_context* context);
	static void abc_ifnle(call_context* context);
	static void abc_ifngt(call_context* context);
	static void abc_ifnge(call_context* context);
	
	static void abc_jump(call_context* context);// 0x10
	static void abc_iftrue(call_context* context);
	static void abc_iftrue_constant(call_context* context);
	static void abc_iftrue_local(call_context* context);
	static void abc_iffalse(call_context* context);
	static void abc_iffalse_constant(call_context* context);
	static void abc_iffalse_local(call_context* context);
	static void abc_ifeq(call_context* context);
	static void abc_ifeq_constant_constant(call_context* context);
	static void abc_ifeq_local_constant(call_context* context);
	static void abc_ifeq_constant_local(call_context* context);
	static void abc_ifeq_local_local(call_context* context);
	static void abc_ifne(call_context* context);
	static void abc_ifne_constant_constant(call_context* context);
	static void abc_ifne_local_constant(call_context* context);
	static void abc_ifne_constant_local(call_context* context);
	static void abc_ifne_local_local(call_context* context);
	static void abc_iflt(call_context* context);
	static void abc_iflt_constant_constant(call_context* context);
	static void abc_iflt_local_constant(call_context* context);
	static void abc_iflt_constant_local(call_context* context);
	static void abc_iflt_local_local(call_context* context);
	static void abc_ifle(call_context* context);
	static void abc_ifle_constant_constant(call_context* context);
	static void abc_ifle_local_constant(call_context* context);
	static void abc_ifle_constant_local(call_context* context);
	static void abc_ifle_local_local(call_context* context);
	static void abc_ifgt(call_context* context);
	static void abc_ifgt_constant_constant(call_context* context);
	static void abc_ifgt_local_constant(call_context* context);
	static void abc_ifgt_constant_local(call_context* context);
	static void abc_ifgt_local_local(call_context* context);
	static void abc_ifge(call_context* context);
	static void abc_ifge_constant_constant(call_context* context);
	static void abc_ifge_local_constant(call_context* context);
	static void abc_ifge_constant_local(call_context* context);
	static void abc_ifge_local_local(call_context* context);
	static void abc_ifstricteq(call_context* context);
	static void abc_ifstricteq_constant_constant(call_context* context);
	static void abc_ifstricteq_local_constant(call_context* context);
	static void abc_ifstricteq_constant_local(call_context* context);
	static void abc_ifstricteq_local_local(call_context* context);
	static void abc_ifstrictne(call_context* context);
	static void abc_ifstrictne_constant_constant(call_context* context);
	static void abc_ifstrictne_local_constant(call_context* context);
	static void abc_ifstrictne_constant_local(call_context* context);
	static void abc_ifstrictne_local_local(call_context* context);
	static void abc_lookupswitch(call_context* context);
	static void abc_pushwith(call_context* context);
	static void abc_popscope(call_context* context);
	static void abc_nextname(call_context* context);
	static void abc_hasnext(call_context* context);

	static void abc_pushnull(call_context* context);// 0x20
	static void abc_pushundefined(call_context* context);
	static void abc_nextvalue(call_context* context);
	static void abc_pushbyte(call_context* context);
	static void abc_pushshort(call_context* context);
	static void abc_pushtrue(call_context* context);
	static void abc_pushfalse(call_context* context);
	static void abc_pushnan(call_context* context);
	static void abc_pop(call_context* context);
	static void abc_dup(call_context* context);
	static void abc_swap(call_context* context);
	static void abc_pushstring(call_context* context);
	static void abc_pushint(call_context* context);
	static void abc_pushuint(call_context* context);
	static void abc_pushdouble(call_context* context);

	static void abc_pushScope(call_context* context);// 0x30
	static void abc_pushScope_constant(call_context* context);// 0x30
	static void abc_pushScope_local(call_context* context);// 0x30
	static void abc_pushnamespace(call_context* context);
	static void abc_hasnext2(call_context* context);
	static void abc_li8(call_context* context);
	static void abc_li16(call_context* context);
	static void abc_li32(call_context* context);
	static void abc_lf32(call_context* context);
	static void abc_lf64(call_context* context);
	static void abc_si8(call_context* context);
	static void abc_si16(call_context* context);
	static void abc_si32(call_context* context);
	static void abc_sf32(call_context* context);
	static void abc_sf64(call_context* context);

	static void abc_newfunction(call_context* context);// 0x40
	static void abc_call(call_context* context);
	static void abc_construct(call_context* context);
	static void abc_callMethod(call_context* context);
	static void abc_callstatic(call_context* context);
	static void abc_callsuper(call_context* context);
	static void abc_callproperty(call_context* context);
	static void abc_callpropertyStaticName_localresult(call_context* context);
	static void abc_callpropertyStaticName_constant_constant(call_context* context);
	static void abc_callpropertyStaticName_local_constant(call_context* context);
	static void abc_callpropertyStaticName_constant_local(call_context* context);
	static void abc_callpropertyStaticName_local_local(call_context* context);
	static void abc_callpropertyStaticName_constant_constant_localresult(call_context* context);
	static void abc_callpropertyStaticName_local_constant_localresult(call_context* context);
	static void abc_callpropertyStaticName_constant_local_localresult(call_context* context);
	static void abc_callpropertyStaticName_local_local_localresult(call_context* context);
	static void abc_returnvoid(call_context* context);
	static void abc_returnvalue(call_context* context);
	static void abc_returnvalue_constant(call_context* context);
	static void abc_returnvalue_local(call_context* context);
	static void abc_constructsuper(call_context* context);
	static void abc_constructprop(call_context* context);
	static void abc_callproplex(call_context* context);
	static void abc_callsupervoid(call_context* context);
	static void abc_callpropvoid(call_context* context);
	
	static void abc_sxi1(call_context* context); // 0x50
	static void abc_sxi8(call_context* context);
	static void abc_sxi16(call_context* context);
	static void abc_constructgenerictype(call_context* context);
	static void abc_newobject(call_context* context);
	static void abc_newarray(call_context* context);
	static void abc_newactivation(call_context* context);
	static void abc_newclass(call_context* context);
	static void abc_getdescendants(call_context* context);
	static void abc_newcatch(call_context* context);
	static void abc_findpropstrict(call_context* context);
	static void abc_findproperty(call_context* context);
	static void abc_finddef(call_context* context);
	
	static void abc_getlex(call_context* context);// 0x60
	static void abc_getlex_localresult(call_context* context);// 0x60
	static void abc_setproperty(call_context* context);
	static void abc_getlocal(call_context* context);
	static void abc_setlocal(call_context* context);
	static void abc_getglobalscope(call_context* context);
	static void abc_getscopeobject(call_context* context);
	static void abc_getProperty(call_context* context);
	static void abc_getProperty_constant_constant(call_context* context);
	static void abc_getProperty_local_constant(call_context* context);
	static void abc_getProperty_constant_local(call_context* context);
	static void abc_getProperty_local_local(call_context* context);
	static void abc_getProperty_constant_constant_localresult(call_context* context);
	static void abc_getProperty_local_constant_localresult(call_context* context);
	static void abc_getProperty_constant_local_localresult(call_context* context);
	static void abc_getProperty_local_local_localresult(call_context* context);
	static void abc_getPropertyStaticName_constant(call_context* context);
	static void abc_getPropertyStaticName_local(call_context* context);
	static void abc_getPropertyStaticName_constant_localresult(call_context* context);
	static void abc_getPropertyStaticName_local_localresult(call_context* context);
	static void abc_initproperty(call_context* context);
	static void abc_deleteproperty(call_context* context);
	static void abc_getslot(call_context* context);
	static void abc_setslot(call_context* context);
	static void abc_getglobalSlot(call_context* context);
	static void abc_setglobalSlot(call_context* context);

	static void abc_convert_s(call_context* context);// 0x70
	static void abc_esc_xelem(call_context* context);
	static void abc_esc_xattr(call_context* context);
	static void abc_convert_i(call_context* context);
	static void abc_convert_u(call_context* context);
	static void abc_convert_d(call_context* context);
	static void abc_convert_d_constant(call_context* context);
	static void abc_convert_d_local(call_context* context);
	static void abc_convert_d_constant_localresult(call_context* context);
	static void abc_convert_d_local_localresult(call_context* context);
	static void abc_convert_b(call_context* context);
	static void abc_convert_o(call_context* context);
	static void abc_checkfilter(call_context* context);

	static void abc_coerce(call_context* context); // 0x80
	static void abc_coerce_a(call_context* context);
	static void abc_coerce_s(call_context* context);
	static void abc_astype(call_context* context);
	static void abc_astypelate(call_context* context);

	static void abc_negate(call_context* context); // 0x90
	static void abc_increment(call_context* context);
	static void abc_increment_local(call_context* context);
	static void abc_increment_local_localresult(call_context* context);
	static void abc_inclocal(call_context* context);
	static void abc_decrement(call_context* context);
	static void abc_decrement_local(call_context* context);
	static void abc_decrement_local_localresult(call_context* context);
	static void abc_declocal(call_context* context);
	static void abc_typeof(call_context* context);
	static void abc_not(call_context* context);
	static void abc_bitnot(call_context* context);

	static void abc_add(call_context* context); //0xa0
	static void abc_add_constant_constant(call_context* context);
	static void abc_add_local_constant(call_context* context);
	static void abc_add_constant_local(call_context* context);
	static void abc_add_local_local(call_context* context);
	static void abc_add_constant_constant_localresult(call_context* context);
	static void abc_add_local_constant_localresult(call_context* context);
	static void abc_add_constant_local_localresult(call_context* context);
	static void abc_add_local_local_localresult(call_context* context);
	static void abc_subtract(call_context* context);
	static void abc_subtract_constant_constant(call_context* context);
	static void abc_subtract_local_constant(call_context* context);
	static void abc_subtract_constant_local(call_context* context);
	static void abc_subtract_local_local(call_context* context);
	static void abc_subtract_constant_constant_localresult(call_context* context);
	static void abc_subtract_local_constant_localresult(call_context* context);
	static void abc_subtract_constant_local_localresult(call_context* context);
	static void abc_subtract_local_local_localresult(call_context* context);
	static void abc_multiply(call_context* context);
	static void abc_multiply_constant_constant(call_context* context);
	static void abc_multiply_local_constant(call_context* context);
	static void abc_multiply_constant_local(call_context* context);
	static void abc_multiply_local_local(call_context* context);
	static void abc_multiply_constant_constant_localresult(call_context* context);
	static void abc_multiply_local_constant_localresult(call_context* context);
	static void abc_multiply_constant_local_localresult(call_context* context);
	static void abc_multiply_local_local_localresult(call_context* context);
	static void abc_divide(call_context* context);
	static void abc_divide_constant_constant(call_context* context);
	static void abc_divide_local_constant(call_context* context);
	static void abc_divide_constant_local(call_context* context);
	static void abc_divide_local_local(call_context* context);
	static void abc_divide_constant_constant_localresult(call_context* context);
	static void abc_divide_local_constant_localresult(call_context* context);
	static void abc_divide_constant_local_localresult(call_context* context);
	static void abc_divide_local_local_localresult(call_context* context);
	static void abc_modulo(call_context* context);
	static void abc_modulo_constant_constant(call_context* context);
	static void abc_modulo_local_constant(call_context* context);
	static void abc_modulo_constant_local(call_context* context);
	static void abc_modulo_local_local(call_context* context);
	static void abc_modulo_constant_constant_localresult(call_context* context);
	static void abc_modulo_local_constant_localresult(call_context* context);
	static void abc_modulo_constant_local_localresult(call_context* context);
	static void abc_modulo_local_local_localresult(call_context* context);
	static void abc_lshift(call_context* context);
	static void abc_lshift_constant_constant(call_context* context);
	static void abc_lshift_local_constant(call_context* context);
	static void abc_lshift_constant_local(call_context* context);
	static void abc_lshift_local_local(call_context* context);
	static void abc_lshift_constant_constant_localresult(call_context* context);
	static void abc_lshift_local_constant_localresult(call_context* context);
	static void abc_lshift_constant_local_localresult(call_context* context);
	static void abc_lshift_local_local_localresult(call_context* context);
	static void abc_rshift(call_context* context);
	static void abc_rshift_constant_constant(call_context* context);
	static void abc_rshift_local_constant(call_context* context);
	static void abc_rshift_constant_local(call_context* context);
	static void abc_rshift_local_local(call_context* context);
	static void abc_rshift_constant_constant_localresult(call_context* context);
	static void abc_rshift_local_constant_localresult(call_context* context);
	static void abc_rshift_constant_local_localresult(call_context* context);
	static void abc_rshift_local_local_localresult(call_context* context);
	static void abc_urshift(call_context* context);
	static void abc_urshift_constant_constant(call_context* context);
	static void abc_urshift_local_constant(call_context* context);
	static void abc_urshift_constant_local(call_context* context);
	static void abc_urshift_local_local(call_context* context);
	static void abc_urshift_constant_constant_localresult(call_context* context);
	static void abc_urshift_local_constant_localresult(call_context* context);
	static void abc_urshift_constant_local_localresult(call_context* context);
	static void abc_urshift_local_local_localresult(call_context* context);
	static void abc_bitand(call_context* context);
	static void abc_bitand_constant_constant(call_context* context);
	static void abc_bitand_local_constant(call_context* context);
	static void abc_bitand_constant_local(call_context* context);
	static void abc_bitand_local_local(call_context* context);
	static void abc_bitand_constant_constant_localresult(call_context* context);
	static void abc_bitand_local_constant_localresult(call_context* context);
	static void abc_bitand_constant_local_localresult(call_context* context);
	static void abc_bitand_local_local_localresult(call_context* context);
	static void abc_bitor(call_context* context);
	static void abc_bitor_constant_constant(call_context* context);
	static void abc_bitor_local_constant(call_context* context);
	static void abc_bitor_constant_local(call_context* context);
	static void abc_bitor_local_local(call_context* context);
	static void abc_bitor_constant_constant_localresult(call_context* context);
	static void abc_bitor_local_constant_localresult(call_context* context);
	static void abc_bitor_constant_local_localresult(call_context* context);
	static void abc_bitor_local_local_localresult(call_context* context);
	static void abc_bitxor(call_context* context);
	static void abc_bitxor_constant_constant(call_context* context);
	static void abc_bitxor_local_constant(call_context* context);
	static void abc_bitxor_constant_local(call_context* context);
	static void abc_bitxor_local_local(call_context* context);
	static void abc_bitxor_constant_constant_localresult(call_context* context);
	static void abc_bitxor_local_constant_localresult(call_context* context);
	static void abc_bitxor_constant_local_localresult(call_context* context);
	static void abc_bitxor_local_local_localresult(call_context* context);
	static void abc_equals(call_context* context);
	static void abc_strictequals(call_context* context);
	static void abc_lessthan(call_context* context);
	static void abc_lessequals(call_context* context);
	static void abc_greaterthan(call_context* context);
	static void abc_greaterthan_constant_constant(call_context* context);
	static void abc_greaterthan_local_constant(call_context* context);
	static void abc_greaterthan_constant_local(call_context* context);
	static void abc_greaterthan_local_local(call_context* context);
	static void abc_greaterthan_constant_constant_localresult(call_context* context);
	static void abc_greaterthan_local_constant_localresult(call_context* context);
	static void abc_greaterthan_constant_local_localresult(call_context* context);
	static void abc_greaterthan_local_local_localresult(call_context* context);
	static void abc_greaterequals(call_context* context);// 0xb0
	static void abc_instanceof(call_context* context);
	static void abc_istype(call_context* context);
	static void abc_istypelate(call_context* context);
	static void abc_in(call_context* context);
	
	static void abc_increment_i(call_context* context); // 0xc0
	static void abc_decrement_i(call_context* context);
	static void abc_inclocal_i(call_context* context);
	static void abc_declocal_i(call_context* context);
	static void abc_negate_i(call_context* context);
	static void abc_add_i(call_context* context);
	static void abc_subtract_i(call_context* context);
	static void abc_multiply_i(call_context* context);

	static void abc_getlocal_0(call_context* context); // 0xd0
	static void abc_getlocal_1(call_context* context);
	static void abc_getlocal_2(call_context* context);
	static void abc_getlocal_3(call_context* context);
	static void abc_setlocal_0(call_context* context);
	static void abc_setlocal_1(call_context* context);
	static void abc_setlocal_2(call_context* context);
	static void abc_setlocal_3(call_context* context);

	static void abc_debug(call_context* context); // 0xef

	static void abc_debugline(call_context* context); //0xf0
	static void abc_debugfile(call_context* context);
	static void abc_bkptline(call_context* context);
	static void abc_timestamp(call_context* context);

	static void abc_invalidinstruction(call_context* context);

	static abc_function abcfunctions[];
public:
	call_context* currentCallContext;

	MemoryAccount* vmDataMemory;

	llvm::ExecutionEngine* ex;
	llvm::Module* module;

#ifdef LLVM_36
	llvm::legacy::FunctionPassManager* FPM;
#else
	llvm::FunctionPassManager* FPM;
#endif
	llvm::LLVMContext& llvm_context();

	ABCVm(SystemState* s, MemoryAccount* m) DLL_PUBLIC;
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
	static void executeFunction(call_context* context);
	static void preloadFunction(const SyntheticFunction* function);
	static ASObject* executeFunctionFast(const SyntheticFunction* function, call_context* context, ASObject *caller);
	static void optimizeFunction(SyntheticFunction* function);
	static void verifyBranch(std::set<uint32_t>& pendingBlock,std::map<uint32_t,BasicBlock>& basicBlocks,
			int oldStart, int here, int offset, int code_len);
	static void writeBranchAddress(std::map<uint32_t,BasicBlock>& basicBlocks, int here, int offset, std::ostream& out);
	static void writeInt32(std::ostream& out, int32_t val);
	static void writeDouble(std::ostream& out, double val);
	static void writePtr(std::ostream& out, const void* val);

	static InferenceData earlyBindGetLex(std::ostream& out, const SyntheticFunction* f,
			const std::vector<InferenceData>& scopeStack, const multiname* name, uint32_t name_index);
	static InferenceData earlyBindFindPropStrict(std::ostream& out, const SyntheticFunction* f,
			const std::vector<InferenceData>& scopeStack, const multiname* name);
	static EARLY_BIND_STATUS earlyBindForScopeStack(std::ostream& out, const SyntheticFunction* f,
			const std::vector<InferenceData>& scopeStack, const multiname* name, InferenceData& inferredData);
	static const Type* getLocalType(const SyntheticFunction* f, unsigned localIndex);

	bool addEvent(_NR<EventDispatcher>,_R<Event> ) DLL_PUBLIC;
	bool prependEvent(_NR<EventDispatcher>,_R<Event> ) DLL_PUBLIC;
	int getEventQueueSize();
	void shutdown();
	bool hasEverStarted() const { return status!=CREATED; }

	static Global* getGlobalScope(call_context* th);
	static bool strictEqualImpl(ASObject*, ASObject*);
	static void publicHandleEvent(_R<EventDispatcher> dispatcher, _R<Event> event);
	static _R<ApplicationDomain> getCurrentApplicationDomain(call_context* th);
	static _R<SecurityDomain> getCurrentSecurityDomain(call_context* th);

	/* The current recursion level. Each call increases this by one,
	 * each return from a call decreases this. */
	uint32_t cur_recursion;
	std::vector<std::pair<uint32_t,asAtom> > stacktrace;

	struct abc_limits {
		/* maxmium number of recursion allowed. See ScriptLimitsTag */
		uint32_t max_recursion;
		/* maxmium number of seconds for script execution. See ScriptLimitsTag */
		uint32_t script_timeout;
	} limits;

	uint32_t getAndIncreaseNamespaceBase(uint32_t nsNum);

	tiny_string getDefaultXMLNamespace();
	uint32_t getDefaultXMLNamespaceID();

	bool buildClassAndBindTag(const std::string& s, DictionaryTag* t);
	void checkExternalCallEvent() DLL_PUBLIC;
};

class DoABCTag: public ControlTag
{
private:
	ABCContext* context;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const{ return ABC_TAG; }
	void execute(RootMovieClip* root) const;
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
	void execute(RootMovieClip* root) const;
};

class SymbolClassTag: public ControlTag
{
private:
	UI16_SWF NumSymbols;
	std::vector<UI16_SWF> Tags;
	std::vector<STRING> Names;
public:
	SymbolClassTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const{ return SYMBOL_CLASS_TAG; }
	void execute(RootMovieClip* root) const;
};

inline ABCVm* getVm(SystemState* sys)
{
	return sys->currentVm;
}

std::istream& operator>>(std::istream& in, method_info& v);

};

#endif /* SCRIPTING_ABC_H */
