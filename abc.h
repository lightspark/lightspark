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

#ifndef _SWF_H
#define _SWF_H

#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/IRBuilder.h>
#include "tags.h"
#include "frame.h"
#include "logger.h"
#include <vector>
#include <deque>
#include <map>
#include "swf.h"

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
	std::string val;
public:
	operator std::string() const{return val;}
};

class namespace_info
{
friend std::istream& operator>>(std::istream& in, namespace_info& v);
friend class ABCVm;
private:
	u8 kind;
	u30 name;
};

class ns_set_info
{
friend std::istream& operator>>(std::istream& in, ns_set_info& v);
friend class ABCVm;
private:
	u30 count;
	std::vector<u30> ns;
};

class multiname_info
{
friend std::istream& operator>>(std::istream& in, multiname_info& v);
friend class ABCVm;
private:
	u8 kind;
	u30 name;
	u30 ns;
	u30 ns_set;
};

class cpool_info
{
friend std::istream& operator>>(std::istream& in, cpool_info& v);
friend class ABCVm;
private:
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

class method_body_info;
class ABCVm;

struct call_context
{
	struct
	{
		ISWFObject** locals;
		ISWFObject** stack;
		uint32_t stack_index;
	} __attribute__((packed));
	ABCVm* vm;
	std::vector<ISWFObject*> scope_stack;
	void runtime_stack_push(ISWFObject* s);
	ISWFObject* runtime_stack_pop();
	ISWFObject* runtime_stack_peek();
	call_context(method_info* th);
};

class method_info
{
friend std::istream& operator>>(std::istream& in, method_info& v);
private:
	u30 param_count;
	u30 return_type;
	std::vector<u30> param_type;
	u30 name;
	u8 flags;

	u30 option_count;
	std::vector<option_detail> options;
//	param_info param_names

	enum STACK_TYPE{STACK_OBJECT=0};
	typedef std::pair<llvm::Value*, STACK_TYPE> stack_entry;
	static ISWFObject* argumentDumper(arguments* arg, uint32_t n);
	stack_entry static_stack_peek(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack,
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	stack_entry static_stack_pop(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack,
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	void static_stack_push(std::vector<stack_entry>& static_stack, const stack_entry& e);
	llvm::Value* llvm_stack_pop(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);
	llvm::Value* llvm_stack_peek(llvm::IRBuilder<>& builder,llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);
	void llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val,
			llvm::Value* dynamic_stack,llvm::Value* dynamic_stack_index);
	void syncStacks(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, bool jitted,
			std::vector<stack_entry>& static_stack, 
			llvm::Value* dynamic_stack, llvm::Value* dynamic_stack_index);
	llvm::FunctionType* synt_method_prototype(llvm::ExecutionEngine* ex);

public:
	llvm::Function* synt_method();
	llvm::Function* f;
	ABCVm* vm;
	method_body_info* body;
	method_info():body(NULL),f(NULL),vm(NULL)
	{
	}
};

struct item_info
{
	u30 key;
	u30 value;
};

class metadata_info
{
friend std::istream& operator>>(std::istream& in, metadata_info& v);
private:
	u30 name;
	u30 item_count;
	std::vector<item_info> items;
};

class traits_info
{
friend std::istream& operator>>(std::istream& in, traits_info& v);
friend class ABCVm;
public:
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

class instance_info
{
friend std::istream& operator>>(std::istream& in, instance_info& v);
friend class ABCVm;
private:
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

class class_info
{
friend std::istream& operator>>(std::istream& in, class_info& v);
friend class ABCVm;
private:
	u30 cinit;
	u30 trait_count;
	std::vector<traits_info> traits;
};

class script_info
{
friend std::istream& operator>>(std::istream& in, script_info& v);
friend class ABCVm;
private:
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

class method_body_info
{
friend std::istream& operator>>(std::istream& in, method_body_info& v);
friend class method_info;
friend class ABCVm;
friend class call_context;
private:
	u30 method;
	u30 max_stack;
	u30 local_count;
	u30 init_scope_depth;
	u30 max_scope_depth;
	u30 code_length;
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

class ABCVm
{
friend class method_info;
private:
	SystemState* m_sys;
	pthread_t t;
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
	method_info* get_method(unsigned int m);
	//void printMethod(const method_info* m) const;
	//void printClass(int m) const;
	ISWFObject* buildClass(int m);
	void printMultiname(int m) const;
	void printNamespace(int n) const;
	//void printTrait(const traits_info* t) const;
	void buildTrait(ISWFObject* obj, const traits_info* t, IFunction* deferred_initialization=NULL);
	void printNamespaceSet(const ns_set_info* m) const;
	std::string getString(unsigned int s) const;
	multiname getMultiname(unsigned int m, call_context* th=NULL) const;
	Qname getQname(unsigned int m, call_context* th=NULL) const;

	ASObject Global;
	//std::vector<ISWFObject*> stack;
	llvm::Module* module;

	void registerClasses();

	void registerFunctions();
	//Interpreted AS instructions
	static ISWFObject* hasNext2(call_context* th, int n, int m); 
	static void callPropVoid(call_context* th, int n, int m); 
	static void callSuperVoid(call_context* th, int n, int m); 
	static void callSuper(call_context* th, int n, int m); 
	static void callProperty(call_context* th, int n, int m); 
	static void constructProp(call_context* th, int n, int m); 
	static void getLocal(call_context* th, int n); 
	static void newObject(call_context* th, int n); 
	static ISWFObject* newCatch(call_context* th, int n); 
	static void jump(call_context* th, int offset); 
	static bool ifEq(ISWFObject*, ISWFObject*, int offset); 
	static bool ifStrictEq(ISWFObject*, ISWFObject*, int offset); 
	static bool ifNE(ISWFObject*, ISWFObject*, int offset); 
	static bool ifLT(ISWFObject*, ISWFObject*, int offset); 
	static bool ifNLT(ISWFObject*, ISWFObject*, int offset); 
	static bool ifNGT(ISWFObject*, ISWFObject*, int offset); 
	static bool ifNGE(ISWFObject*, ISWFObject*, int offset); 
	static bool ifGE(ISWFObject*, ISWFObject*, int offset); 
	static bool ifNLE(ISWFObject*, ISWFObject*, int offset); 
	static bool ifStrictNE(ISWFObject*, ISWFObject*, int offset); 
	static bool ifFalse(ISWFObject*, int offset); 
	static bool ifTrue(ISWFObject*, int offset); 
	static ISWFObject* getSlot(ISWFObject* th, int n); 
	static void setLocal(call_context* th, int n); 
	static void kill(call_context* th, int n); 
	static ISWFObject* setSlot(ISWFObject*, ISWFObject*, int n); 
	static ISWFObject* pushString(call_context* th, int n); 
	static void getLex(call_context* th, int n); 
	static ISWFObject* getScopeObject(call_context* th, int n); 
	static void deleteProperty(call_context* th, int n); 
	static void initProperty(call_context* th, int n); 
	static void newClass(call_context* th, int n); 
	static void newArray(call_context* th, int n); 
	static void findPropStrict(call_context* th, int n);
	static void findProperty(call_context* th, int n);
	static void getProperty(call_context* th, int n);
	static ISWFObject* pushByte(call_context* th, int n);
	static ISWFObject* pushShort(call_context* th, int n);
	static ISWFObject* pushInt(call_context* th, int n);
	static ISWFObject* pushDouble(call_context* th, int n);
	static void incLocal_i(call_context* th, int n);
	static void coerce(call_context* th, int n);
	static void setProperty(call_context* th, int n);
	static void call(call_context* th, int n);
	static void constructSuper(call_context* th, int n);
	static void construct(call_context* th, int n);
	static ISWFObject* newFunction(call_context* th, int n);
	static void setSuper(call_context* th, int n);
	static void getSuper(call_context* th, int n);
	static void pushScope(call_context* th);
	static void pushWith(call_context* th);
	static ISWFObject* pushNull(call_context* th);
	static ISWFObject* pushUndefined(call_context* th);
	static ISWFObject* pushNaN(call_context* th);
	static ISWFObject* pushFalse(call_context* th);
	static ISWFObject* pushTrue(call_context* th);
	static void dup(call_context* th);
	static ISWFObject* equals(ISWFObject*,ISWFObject*);
	static ISWFObject* in(ISWFObject*,ISWFObject*);
	static ISWFObject* strictEquals(ISWFObject*,ISWFObject*);
	static ISWFObject* _not(ISWFObject*);
	static ISWFObject* negate(ISWFObject*);
	static void pop(call_context* th);
	static ISWFObject* typeOf(ISWFObject*);
	static void _throw(call_context* th);
	static void asTypelate(call_context* th);
	static void isTypelate(call_context* th);
	static void swap(call_context* th);
	static ISWFObject* add(ISWFObject*,ISWFObject*);
	static ISWFObject* multiply(ISWFObject*,ISWFObject*);
	static ISWFObject* divide(ISWFObject*,ISWFObject*);
	static ISWFObject* modulo(ISWFObject*,ISWFObject*);
	static ISWFObject* subtract(ISWFObject*,ISWFObject*);
	static void popScope(call_context* th);
	static ISWFObject* newActivation(call_context* th, method_info*);
	static ISWFObject* coerce_s(ISWFObject*);
	static ISWFObject* coerce_a(ISWFObject*);
	static ISWFObject* convert_i(ISWFObject*);
	static ISWFObject* convert_b(ISWFObject*);
	static ISWFObject* convert_d(ISWFObject*);
	static ISWFObject* greaterThan(ISWFObject*,ISWFObject*);
	static ISWFObject* lessThan(ISWFObject*,ISWFObject*);
	static ISWFObject* nextName(ISWFObject* index, ISWFObject* obj);
	static ISWFObject* nextValue(ISWFObject* index, ISWFObject* obj);
	static ISWFObject* increment_i(ISWFObject*);
	static ISWFObject* increment(ISWFObject*);
	static ISWFObject* decrement_i(ISWFObject*);
	static ISWFObject* decrement(ISWFObject*);
	static ISWFObject* getGlobalScope(call_context* th);
	//Utility
	static void not_impl(int p);
	ASFUNCTION(print);

	//Internal utilities
	static void method_reset(method_info* th);

	//Opcode tables
	static opcode_handler opcode_table_args0[];
	static opcode_handler opcode_table_args0_lazy[];
	static opcode_handler opcode_table_args1[];
	static opcode_handler opcode_table_args1_lazy[];
	static opcode_handler opcode_table_args1_pointers[];
	static opcode_handler opcode_table_args1_branches[];
	static opcode_handler opcode_table_args1_pointers_int[];
	static opcode_handler opcode_table_args2_pointers[];
	static opcode_handler opcode_table_args2_branches[];
	static opcode_handler opcode_table_args2_pointers_int[];

	//Synchronization
	sem_t mutex;
	sem_t sem_event_count;
	sem_t started;

	//Event handling
	bool shutdown;
	std::deque<std::pair<EventDispatcher*,Event*> > events_queue;
	void handleEvent();
public:
	static llvm::ExecutionEngine* ex;
	ABCVm(SystemState* s,std::istream& in);
	~ABCVm();
	static void Run(ABCVm* th);
	ISWFObject* buildNamedClass(const std::string& n, ASObject*, arguments* a);
	void addEvent(EventDispatcher*,Event*);
	void start() { sem_post(&started);}
};

class DoABCTag: public DisplayListTag
{
private:
	UI32 Flags;
	STRING Name;
	ABCVm* vm;
	pthread_t thread;
	bool done;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const;
};

class SymbolClassTag: public DisplayListTag
{
private:
	UI16 NumSymbols;
	std::vector<UI16> Tags;
	std::vector<STRING> Names;
	bool done;
public:
	SymbolClassTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const;
};

class Boolean: public ASObject
{
friend bool Boolean_concrete(ISWFObject* obj);
private:
	bool val;
public:
	Boolean(bool v):val(v){}
	SWFOBJECT_TYPE getObjectType() const { return T_BOOLEAN; }
	
};

bool Boolean_concrete(ISWFObject* obj);
ISWFObject* parseInt(ISWFObject* obj,arguments* args);

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
std::istream& operator>>(std::istream& in, instance_info& v);

#endif
