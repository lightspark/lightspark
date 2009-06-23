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

class method_info
{
friend std::istream& operator>>(std::istream& in, method_info& v);
friend class ABCVm;
private:
	u30 param_count;
	u30 return_type;
	std::vector<u30> param_type;
	u30 name;
	u8 flags;

	u30 option_count;
	std::vector<option_detail> options;
//	param_info param_names

	llvm::Function* f;
	ISWFObject** locals;
	ISWFObject** stack;
	int max_stack_index;
	uint32_t stack_index;
	llvm::Value* dynamic_stack;
	llvm::Value* dynamic_stack_index;

public:
	ABCVm* vm;
	std::vector<ISWFObject*> scope_stack;
	method_body_info* body;
	void runtime_stack_push(ISWFObject* s);
	ISWFObject* runtime_stack_pop();
	ISWFObject* runtime_stack_peek();
	void llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val);
	llvm::Value* llvm_stack_peek(llvm::IRBuilder<>& builder) const;
	llvm::Value* llvm_stack_pop(llvm::IRBuilder<>& builder) const;
	method_info():body(NULL),f(NULL),locals(NULL),stack(NULL),stack_index(0),vm(NULL)
	{
	}
	void setStackLength(const llvm::ExecutionEngine* ex, int l);
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
friend class ABCVm;
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

class ABCVm
{
private:
	enum STACK_TYPE{STACK_OBJECT=0};
	typedef std::pair<llvm::Value*, STACK_TYPE> stack_entry;

	SystemState* m_sys;
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
	void printMethod(const method_info* m) const;
	void printClass(int m) const;
	ISWFObject* buildClass(int m);
	void printMultiname(int m) const;
	void printNamespace(int n) const;
	void printTrait(const traits_info* t) const;
	void buildTrait(ISWFObject* obj, const traits_info* t, Function::as_function deferred_initialization=NULL);
	void printNamespaceSet(const ns_set_info* m) const;
	std::string getString(unsigned int s) const;
	std::string getMultinameString(unsigned int m, method_info* th=NULL) const;

	llvm::Function* synt_method(method_info* m);
	llvm::FunctionType* synt_method_prototype();

	std::map<int,ISWFObject*> registers;
	std::map<std::string,int> valid_classes;
	ASObject Global;
	std::vector<ISWFObject*> stack;
	llvm::Module* module;

	static llvm::ExecutionEngine* ex;

	//Utility
	static void debug(method_info* th);
	static void not_impl(int p);
	static ISWFObject* argumentDumper(arguments* arg, uint32_t n);
	stack_entry static_stack_peek(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack, const method_info* m);
	stack_entry static_stack_pop(llvm::IRBuilder<>& builder, std::vector<stack_entry>& static_stack, const method_info* m);
	void static_stack_push(std::vector<stack_entry>& static_stack, const stack_entry& e);
	void syncStacks(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, bool jitted,std::vector<stack_entry>& static_stack, method_info* m);

	void registerClasses();

	void registerFunctions();
	//Interpreted AS instructions
	static void hasNext2(method_info* th, int n, int m); 
	static void callPropVoid(method_info* th, int n, int m); 
	static void callSuperVoid(method_info* th, int n, int m); 
	static void callSuper(method_info* th, int n, int m); 
	static void callProperty(method_info* th, int n, int m); 
	static void constructProp(method_info* th, int n, int m); 
	static void getLocal(method_info* th, int n); 
	static void newObject(method_info* th, int n); 
	static void newCatch(method_info* th, int n); 
	static void jump(method_info* th, int offset); 
	static void ifEq(method_info* th, int offset); 
	static void ifStrictEq(method_info* th, int offset); 
	static void ifNe(method_info* th, int offset); 
	static void ifLT(method_info* th, int offset); 
	static void ifNLT(method_info* th, int offset); 
	static void ifNGE(method_info* th, int offset); 
	static void ifStrictNE(method_info* th, int offset); 
	static void ifFalse(method_info* th, int offset); 
	static void ifTrue(method_info* th, int offset); 
	static void getSlot(method_info* th, int n); 
	static void setLocal(method_info* th, int n); 
	static void kill(method_info* th, int n); 
	static void setSlot(method_info* th, int n); 
	static void pushString(method_info* th, int n); 
	static void getLex(method_info* th, int n); 
	static void getScopeObject(method_info* th, int n); 
	static void deleteProperty(method_info* th, int n); 
	static void initProperty(method_info* th, int n); 
	static void newClass(method_info* th, int n); 
	static void newArray(method_info* th, int n); 
	static void findPropStrict(method_info* th, int n);
	static void findProperty(method_info* th, int n);
	static void getProperty(method_info* th, int n);
	static void pushByte(method_info* th, int n);
	static void pushShort(method_info* th, int n);
	static void pushInt(method_info* th, int n);
	static void pushDouble(method_info* th, int n);
	static void incLocal_i(method_info* th, int n);
	static void coerce(method_info* th, int n);
	static void setProperty(method_info* th, int n);
	static void call(method_info* th, int n);
	static void constructSuper(method_info* th, int n);
	static void construct(method_info* th, int n);
	static void newFunction(method_info* th, int n);
	static void pushScope(method_info* th);
	static void pushNull(method_info* th);
	static ISWFObject* pushUndefined(method_info* th);
	static void pushNaN(method_info* th);
	static void pushFalse(method_info* th);
	static void pushTrue(method_info* th);
	static void dup(method_info* th);
	static void equals(method_info* th);
	static void strictEquals(method_info* th);
	static void _not(method_info* th);
	static void pop(method_info* th);
	static void _throw(method_info* th);
	static void asTypelate(method_info* th);
	static void isTypelate(method_info* th);
	static void swap(method_info* th);
	static void add(method_info* th);
	static void multiply(method_info* th);
	static void divide(method_info* th);
	static void subtract(method_info* th);
	static void popScope(method_info* th);
	static void newActivation(method_info* th);
	static void coerce_s(method_info* th);
	static void coerce_a(method_info* th);
	static void convert_i(method_info* th);
	static void convert_b(method_info* th);
	static void convert_d(method_info* th);
	static void greaterThan(method_info* th);
	static void lessThan(method_info* th);
	static void nextName(method_info* th);
	static void nextValue(method_info* th);
	static void increment_i(method_info* th);
	static void increment(method_info* th);
	static void decrement_i(method_info* th);
	static void decrement(method_info* th);
	static void getGlobalScope(method_info* th);
	static void method_reset(method_info* th);

	//Synchronization
	sem_t mutex;
	sem_t sem_event_count;
	bool running;

	//Event handling
	bool shutdown;
	std::deque<std::pair<InteractiveObject*,Event*> > events_queue;
	void handleEvent();
public:
	ABCVm(SystemState* s,std::istream& in);
	static void Run(ABCVm* th);
	ISWFObject* buildNamedClass(const std::string& n, ASObject*, arguments* a);
	void addEvent(InteractiveObject*,Event*);
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
	std::multimap<int, bind_candidates> to_bind;
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

class Math: public ASObject
{
public:
	Math();
	ASFUNCTION(atan2);
	ASFUNCTION(floor);
	ASFUNCTION(sqrt);
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
