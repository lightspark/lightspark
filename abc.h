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

#include "tags.h"
#include "frame.h"
#include "logger.h"
#include <vector>
#include <map>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

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
public:
	method_body_info* body;
	method_info():body(NULL),f(NULL){}
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
private:
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
	SWFObject buildClass(int m);
	void printMultiname(int m) const;
	void printNamespace(int n) const;
	void printTrait(const traits_info* t) const;
	void buildTrait(const traits_info* t);
	void printNamespaceSet(const ns_set_info* m) const;
	std::string getString(unsigned int s) const;

	llvm::Function* synt_method(method_info* m);

	std::map<int,SWFObject> registers;
	std::map<int,SWFObject> valid_classes;
	ASObject Global;
	std::vector<SWFObject> stack;
	llvm::Module module;
	llvm::ExecutionEngine* ex;

	void registerFunctions();
	//Interpreted AS instructions
	static void getLocal(ABCVm* th, int n); 
	static void getLex(ABCVm* th, int n); 
	static void getScopeObject(ABCVm* th, int n); 
	static void initProperty(ABCVm* th, int n); 
	static void newClass(ABCVm* th, int n); 
	static void findPropStrict(ABCVm* th, int n);
	static void pushScope(ABCVm* th);
	static void pushNull(ABCVm* th);
	static void popScope(ABCVm* th);
public:
	ABCVm(std::istream& in);
	void Run();
};

class DoABCTag: public DisplayListTag
{
private:
	UI32 Flags;
	STRING Name;
	ABCVm* vm;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const;
};

class SymbolClassTag: public Tag
{
public:
	SymbolClassTag(RECORDHEADER h, std::istream& in);
};

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
