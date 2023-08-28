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

#include "scripting/abctypes.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

istream& lightspark::operator>>(istream& in, s32& v)
{
	int i=0;
	v.val=0;
	uint8_t t;
	//bool signExtend=true;
	do
	{
		in.read((char*)&t,1);
		//No more than 5 bytes should be read
		if(i==28)
		{
			//Only the first 4 bits should be used to reach 32 bits
			if((t&0xf0))
				LOG(LOG_ERROR,"Error in s32");
			uint8_t t2=(t&0xf);
			v.val|=(t2<<i);
			//The number is filled, no sign extension
			//signExtend=false;
			break;
		}
		else
		{
			uint8_t t2=(t&0x7f);
			v.val|=(t2<<i);
			i+=7;
		}
	}
	while(t&0x80);
/*	//Sign extension usage not clear at all
	if(signExtend && t&0x40)
	{
		//Sign extend
		v.val|=(0xffffffff<<i);
	}*/
	return in;
}

istream& lightspark::operator>>(istream& in, s24& v)
{
	uint32_t ret=0;
	in.read((char*)&ret,3);
	v.val=LittleEndianToSignedHost24(ret);
	return in;
}

istream& lightspark::operator>>(istream& in, u30& v)
{
	u32 vv;
	in >> vv;
	uint32_t val = vv;
	if(val&0xc0000000)
		assert_and_throw(false); //TODO: make this VerifierError
	v.val = val;
	return in;
}

istream& lightspark::operator>>(istream& in, u8& v)
{
	uint8_t t;
	in.read((char*)&t,1);
	v.val=t;
	return in;
}

istream& lightspark::operator>>(istream& in, u16& v)
{
	uint16_t t;
	in.read((char*)&t,2);
	v.val=GINT16_FROM_LE(t);
	return in;
}

istream& lightspark::operator>>(istream& in, d64& v)
{
	union double_reader
	{
		uint64_t dump;
		double value;
	};
	double_reader dummy;
	in.read((char*)&dummy.dump,8);
	dummy.dump=GINT64_FROM_LE(dummy.dump);
	v.val=dummy.value;
	return in;
}

istream& lightspark::operator>>(istream& in, string_info& v)
{
	u30 size;
	in >> size;
	v.val = getSys()->getUniqueStringId(tiny_string(in,size));
	return in;
}

istream& lightspark::operator>>(istream& in, namespace_info& v)
{
	in >> v.kind >> v.name;
	if(v.kind!=0x05 && v.kind!=0x08 && v.kind!=0x16 && v.kind!=0x17 && v.kind!=0x18 && v.kind!=0x19 && v.kind!=0x1a)
		throw UnsupportedException("Unexpected namespace kind");
	//Collapse PACKAGE_NAMESPACE on NAMESPACE
	if(v.kind==PACKAGE_NAMESPACE)
		v.kind=(int)NAMESPACE;

	return in;
}

istream& lightspark::operator>>(istream& in, method_info_simple& v)
{
	in >> v.param_count;
	in >> v.return_type;

	v.param_type.resize(v.param_count);
	for(unsigned int i=0;i<v.param_count;i++)
		in >> v.param_type[i];
	
	in >> v.name >> v.flags;
	if(v.flags&0x08)
	{
		in >> v.option_count;
		v.options.resize(v.option_count);
		for(unsigned int i=0;i<v.option_count;i++)
		{
			in >> v.options[i].val >> v.options[i].kind;
			if(v.options[i].kind>0x1a)
				LOG(LOG_ERROR,"Unexpected options type");
		}
	}
	if(v.flags&0x80)
	{
		v.param_names.resize(v.param_count);
		for(unsigned int i=0;i<v.param_count;i++)
			in >> v.param_names[i];
	}
	return in;
}

istream& lightspark::operator>>(istream& in, method_body_info& v)
{
	u30 code_length;
	in >> v.method >> v.max_stack >> v.local_count >> v.init_scope_depth >> v.max_scope_depth >> code_length;
	v.returnvaluepos=v.local_count+1;
	v.code.resize(code_length);
	in.read(&v.code[0],code_length);
	u30 exception_count;
	in >> exception_count;
	v.exceptions.resize(exception_count);
	for(unsigned int i=0;i<exception_count;i++)
		in >> v.exceptions[i];

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(unsigned int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& lightspark::operator >>(istream& in, ns_set_info& v)
{
	in >> v.count;

	v.ns.resize(v.count);
	for(unsigned int i=0;i<v.count;i++)
	{
		in >> v.ns[i];
		if(v.ns[i]==0)
			LOG(LOG_ERROR,"0 not allowed");
	}
	return in;
}

istream& lightspark::operator>>(istream& in, multiname_info& v)
{
	in >> v.kind;

	switch(v.kind)
	{
		case 0x07:
		case 0x0d:
			in >> v.ns >> v.name;
			break;
		case 0x0f:
		case 0x10:
			in >> v.name;
			v.runtimeargs = 1;
			break;
		case 0x11:
		case 0x12:
			v.runtimeargs = 2;
			break;
		case 0x09:
		case 0x0e:
			in >> v.name >> v.ns_set;
			break;
		case 0x1b:
		case 0x1c:
			in >> v.ns_set;
			v.runtimeargs = 1;
			break;
		case 0x1d:
		{
			in >> v.type_definition;
			u8 num_params;
			in >> num_params;
			v.param_types.resize(num_params);
			for(unsigned int i=0;i<num_params;i++)
			{
				u30 t;
				in >> t;
				v.param_types[i]=t;
			}
			break;
		}
		default:
			LOG(LOG_ERROR,"Unexpected multiname kind " << hex << v.kind);
			throw UnsupportedException("Unexpected namespace kind");
	}
	return in;
}

istream& lightspark::operator>>(istream& in, script_info& v)
{
	in >> v.init >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(unsigned int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& lightspark::operator>>(istream& in, class_info& v)
{
	in >> v.cinit >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(unsigned int i=0;i<v.trait_count;i++)
	{
		in >> v.traits[i];
	}
	return in;
}

istream& lightspark::operator>>(istream& in, metadata_info& v)
{
	in >> v.name;
	in >> v.item_count;

	v.items.resize(v.item_count);
	// it seems that metadata is stored as key1,key2,key3...,value2,value2,value3...
	for(unsigned int i=0;i<v.item_count;i++)
	{
		in >> v.items[i].key;
	}
	for(unsigned int i=0;i<v.item_count;i++)
	{
		in >> v.items[i].value;
	}
	return in;
}

istream& lightspark::operator>>(istream& in, traits_info& v)
{
	in >> v.name >> v.kind;
	switch(v.kind&0xf)
	{
		case traits_info::Slot:
		case traits_info::Const:
			in >> v.slot_id >> v.type_name >> v.vindex;
			if(v.vindex)
				in >> v.vkind;
			break;
		case traits_info::Class:
			in >> v.slot_id >> v.classi;
			break;
		case traits_info::Function:
			in >> v.slot_id >> v.function;
			break;
		case traits_info::Getter:
		case traits_info::Setter:
		case traits_info::Method:
			in >> v.disp_id >> v.method;
			break;
		default:
			LOG(LOG_ERROR,"Unexpected kind " << v.kind);
			break;
	}

	if(v.kind&traits_info::Metadata)
	{
		in >> v.metadata_count;
		v.metadata.resize(v.metadata_count);
		for(unsigned int i=0;i<v.metadata_count;i++)
			in >> v.metadata[i];
	}
	return in;
}

istream& lightspark::operator >>(istream& in, exception_info_abc& v)
{
	u30 from, to, target;
	in >> from >> to >> target >> v.exc_type >> v.var_name;
	v.from = from;
	v.to = to;
	v.target = target;
	v.exc_class=nullptr;
	return in;
}

istream& lightspark::operator>>(istream& in, instance_info& v)
{
	in >> v.name >> v.supername >> v.flags;
	if(v.isProtectedNs())
		in >> v.protectedNs;

	in >> v.interface_count;
	v.interfaces.resize(v.interface_count);
	for(unsigned int i=0;i<v.interface_count;i++)
	{
		in >> v.interfaces[i];
		if(v.interfaces[i]==0)
			throw ParseException("Invalid interface specified");
	}

	in >> v.init;

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(unsigned int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	v.overriddenmethods=nullptr;
	return in;
}

istream& lightspark::operator>>(istream& in, cpool_info& v)
{
	in >> v.int_count;
	v.integer.resize(v.int_count);
	for(unsigned int i=1;i<v.int_count;i++)
		in >> v.integer[i];

	in >> v.uint_count;
	v.uinteger.resize(v.uint_count);
	for(unsigned int i=1;i<v.uint_count;i++)
		in >> v.uinteger[i];

	in >> v.double_count;
	v.doubles.resize(v.double_count);
	for(unsigned int i=1;i<v.double_count;i++)
		in >> v.doubles[i];

	in >> v.string_count;
	v.strings.resize(v.string_count);
	for(unsigned int i=1;i<v.string_count;i++)
		in >> v.strings[i];

	in >> v.namespace_count;
	v.namespaces.resize(v.namespace_count);
	for(unsigned int i=1;i<v.namespace_count;i++)
		in >> v.namespaces[i];

	in >> v.ns_set_count;
	v.ns_sets.resize(v.ns_set_count);
	for(unsigned int i=1;i<v.ns_set_count;i++)
		in >> v.ns_sets[i];

	in >> v.multiname_count;
	v.multinames.resize(v.multiname_count);
	for(unsigned int i=1;i<v.multiname_count;i++)
		in >> v.multinames[i];

	return in;
}

cpool_info::cpool_info(MemoryAccount* m):
	integer(reporter_allocator<s32>(m)),
	uinteger(reporter_allocator<u32>(m)),
	doubles(reporter_allocator<d64>(m)),
	strings(reporter_allocator<string_info>(m)),
	namespaces(reporter_allocator<namespace_info>(m)),
	ns_sets(reporter_allocator<ns_set_info>(m)),
	multinames(reporter_allocator<multiname_info>(m))
{
}

method_body_info::~method_body_info()
{
	if (localsinitialvalues)
		delete[] localsinitialvalues;
}
