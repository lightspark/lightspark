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
	operator uint32_t(){return val;}
};

class u30
{
friend std::istream& operator>>(std::istream& in, u30& v);
private:
	uint32_t val;
public:
	operator uint32_t(){return val;}
};

class u16
{
friend std::istream& operator>>(std::istream& in, u16& v);
private:
	uint32_t val;
public:
	operator uint32_t(){return val;}
};

class u8
{
friend std::istream& operator>>(std::istream& in, u8& v);
private:
	uint32_t val;
public:
	operator uint32_t(){return val;}
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
	operator std::string(){return val;}
};

class namespace_info
{
friend std::istream& operator>>(std::istream& in, namespace_info& v);
private:
	u8 kind;
	u30 name;
};

class ns_set_info
{
friend std::istream& operator>>(std::istream& in, ns_set_info& v);
private:
	u30 count;
	std::vector<u30> ns;
};

class multiname_info
{
friend std::istream& operator>>(std::istream& in, multiname_info& v);
private:
	u8 kind;
	u30 name;
	u30 ns;
	u30 ns_set;
};

class cpool_info
{
friend std::istream& operator>>(std::istream& in, cpool_info& v);
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

class ABCVm
{
private:
	u16 minor;
	u16 major;
	cpool_info constant_pool;
public:
	ABCVm(std::istream& in);
};

class DoABCTag: public Tag
{
private:
	UI32 Flags;
	STRING Name;
	ABCVm* vm;
public:
	DoABCTag(RECORDHEADER h, std::istream& in);
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

