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

#ifndef SCRIPTING_ABCTYPES_H
#define SCRIPTING_ABCTYPES_H 1

#include "swftypes.h"
#include "memory_support.h"
#include <unordered_set>

class memorystream;

namespace lightspark
{
struct variable;

class u8
{
friend std::istream& operator>>(std::istream& in, u8& v);
friend memorystream& operator>>(memorystream& in, u8& v);
private:
	uint32_t val;
public:
	operator uint32_t() const{return val;}
	u8& operator=(uint32_t v)
	{
		val=v;
		return *this;
	}
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
friend memorystream& operator>>(memorystream& in, s24& v);
private:
	int32_t val;
public:
	operator int32_t(){return val;}
};

class u30
{
friend std::istream& operator>>(std::istream& in, u30& v);
friend memorystream& operator>>(memorystream& in, u30& v);
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
	uint32_t val;
public:
	operator uint32_t() const{return val;}
};

struct namespace_info
{
	u8 kind;
	u30 name;
	bool cachefilled;
	nsNameAndKind nscached;
	namespace_info():cachefilled(false) { kind=0; }
	~namespace_info(){ cachefilled=false; }
	const nsNameAndKind& getNS(ABCContext * c, uint32_t nsContextIndex)
	{
		if (!cachefilled)
		{
			cachefilled = true;
			nscached = nsNameAndKind(c,nsContextIndex);
		}
		return nscached;
	}
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
	int runtimeargs;
	multiname* cached;
	multiname* dynamic;
	multiname_info():runtimeargs(0),cached(NULL),dynamic(NULL){}
	~multiname_info(){delete cached;if (dynamic) {delete dynamic;};}
	bool isAttributeName() const;
};

struct cpool_info
{
	cpool_info(MemoryAccount* m);
	u30 int_count;
	std::vector<s32, reporter_allocator<s32>> integer;
	u30 uint_count;
	std::vector<u32, reporter_allocator<u32>> uinteger;
	u30 double_count;
	std::vector<d64, reporter_allocator<d64>> doubles;
	u30 string_count;
	std::vector<string_info, reporter_allocator<string_info>> strings;
	u30 namespace_count;
	std::vector<namespace_info, reporter_allocator<namespace_info>> namespaces;
	u30 ns_set_count;
	std::vector<ns_set_info, reporter_allocator<ns_set_info>> ns_sets;
	u30 multiname_count;
	std::vector<multiname_info, reporter_allocator<multiname_info>> multinames;
};

struct option_detail
{
	u30 val;
	u8 kind;
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
	std::unordered_set<uint32_t>* overriddenmethods;
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
class Class_base;
struct exception_info_abc
{
	uint32_t from;
	uint32_t to;
	uint32_t target;
	u30 exc_type;
	u30 var_name;
	Class_base* exc_class;
};

struct method_info_simple
{
	enum { NEED_ARGUMENTS=0x01, NEED_ACTIVATION=0x02, NEED_REST=0x04, HAS_OPTIONAL=0x08, SET_DXNS=0x40, HAS_PARAM_NAMES=0x80 };
	bool needsArgs() { return (flags & NEED_ARGUMENTS) != 0;}
	bool needsActivation() { return (flags & NEED_ACTIVATION) != 0;}
	bool needsRest() { return (flags & NEED_REST) != 0;}
	bool hasOptional() { return (flags & HAS_OPTIONAL) != 0;}
	bool hasDXNS() { return (flags & SET_DXNS) != 0;}
	bool hasParamNames() { return (flags & HAS_PARAM_NAMES) != 0;}
	u30 param_count;
	u30 return_type;
	std::vector<u30> param_type;
	u30 name;
	u8 flags;
	u30 option_count;
	std::vector<option_detail> options;
	std::vector<u30> param_names;
};
typedef void (*abc_function)(struct call_context*);

struct preloadedcodedata
{
	abc_function func;
	union
	{
		ASObject* cacheobj1;
		asAtom* arg1_constant;
		uint32_t local_pos1;
		int32_t arg1_int;
		uint32_t arg1_uint;
	};
	union
	{
		ASObject* cacheobj2;
		multiname* cachedmultiname2;
		variable* cachedvar2;
		asAtom* arg2_constant;
		struct
		{
			uint16_t pos;
			uint16_t flags;
		} local2;
		uint32_t local_pos2;
		int32_t arg2_int;
		uint32_t arg2_uint;
	};
	union
	{
		ASObject* cacheobj3;
		multiname* cachedmultiname3;
		variable* cachedvar3;
		asAtom* arg3_constant;
		struct
		{
			uint16_t pos;
			uint16_t flags;
		} local3;
		int32_t arg3_int;
		uint32_t arg3_uint;
	};
	preloadedcodedata():func(nullptr),cacheobj1(nullptr),cacheobj2(nullptr),cacheobj3(nullptr) {}
};
struct localconstantslot
{
	uint32_t local_pos;
	uint32_t slot_number;
};

struct method_body_info
{
	method_body_info():localresultcount(0),hit_count(0),codeStatus(ORIGINAL),localsinitialvalues(nullptr){}
	~method_body_info();
	u30 method;
	u30 max_stack;
	u30 local_count;
	u30 init_scope_depth;
	u30 max_scope_depth;
	std::string code;
	std::vector<exception_info_abc> exceptions;
	u30 trait_count;
	std::vector<traits_info> traits;
	uint16_t localresultcount;
	//The hit_count belongs here, since it is used to manipulate the code
	uint16_t hit_count;
	uint16_t returnvaluepos;
	//The code status
	enum CODE_STATUS { ORIGINAL = 0, USED, OPTIMIZED, JITTED, PRELOADING, PRELOADED };
	CODE_STATUS codeStatus;
	// list of local/slot pairs that were optimized away
	std::vector<localconstantslot> localconstantslots;
	std::vector<preloadedcodedata> preloadedcode;
	asAtom* localsinitialvalues;
	inline uint16_t getReturnValuePos() const { return returnvaluepos; }
};

std::istream& operator>>(std::istream& in, u8& v);
std::istream& operator>>(std::istream& in, u16& v);
std::istream& operator>>(std::istream& in, u30& v);
std::istream& operator>>(std::istream& in, u32& v);
std::istream& operator>>(std::istream& in, s24& v);
std::istream& operator>>(std::istream& in, s32& v);
std::istream& operator>>(std::istream& in, d64& v);
std::istream& operator>>(std::istream& in, string_info& v);
std::istream& operator>>(std::istream& in, namespace_info& v);
std::istream& operator>>(std::istream& in, ns_set_info& v);
std::istream& operator>>(std::istream& in, multiname_info& v);
std::istream& operator>>(std::istream& in, cpool_info& v);
std::istream& operator>>(std::istream& in, exception_info_abc& v);
std::istream& operator>>(std::istream& in, method_info_simple& v);
std::istream& operator>>(std::istream& in, method_body_info& v);
std::istream& operator>>(std::istream& in, instance_info& v);
std::istream& operator>>(std::istream& in, traits_info& v);
std::istream& operator>>(std::istream& in, script_info& v);
std::istream& operator>>(std::istream& in, metadata_info& v);
std::istream& operator>>(std::istream& in, class_info& v);

};

#endif /* SCRIPTING_ABCTYPES_H */
