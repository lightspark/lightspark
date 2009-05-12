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

#include "abc.h"
#include "logger.h"
#include "swftypes.h"
//#define __STDC_LIMIT_MACROS
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Constants.h> 
#include <llvm/Support/IRBuilder.h> 
#include <llvm/Target/TargetData.h>
#include <sstream>


extern __thread SystemState* sys;

using namespace std;
long timeDiff(timespec& s, timespec& d);

void ignore(istream& i, int count);

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> Flags >> Name;
	LOG(CALLS,"DoABCTag Name: " << Name);

	vm=new ABCVm(in);

	if(dest!=in.tellg())
		LOG(ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
}

int DoABCTag::getDepth() const
{
	return 0x20001;
}

void DoABCTag::Render()
{
	LOG(NOT_IMPLEMENTED,"ABC Exec " << Name);
	vm->Run();
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(NOT_IMPLEMENTED,"SymbolClassTag");
	skip(in);
}

void ABCVm::registerFunctions()
{
	vector<const llvm::Type*> sig;
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();

	// (ABCVm*)
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	llvm::FunctionType* FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	llvm::Function* F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushScope",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushScope);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushNull",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushNull);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"popScope",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::popScope);

	// (ABCVm*,int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getLocal",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getLocal);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getLex",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getLex);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"findPropStrict",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::findPropStrict);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newClass",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newClass);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"initProperty",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::initProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getScopeObject",&module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getScopeObject);
}

ABCVm::ABCVm(istream& in):module("abc jit")
{
	ex=llvm::ExecutionEngine::create(&module);

	in >> minor >> major;
	LOG(CALLS,"ABCVm version " << major << '.' << minor);
	in >> constant_pool;

	in >> method_count;
	methods.resize(method_count);
	for(int i=0;i<method_count;i++)
		in >> methods[i];

	in >> metadata_count;
	metadata.resize(metadata_count);
	for(int i=0;i<metadata_count;i++)
		in >> metadata[i];

	in >> class_count;
	instances.resize(class_count);
	for(int i=0;i<class_count;i++)
		in >> instances[i];
	classes.resize(class_count);
	for(int i=0;i<class_count;i++)
		in >> classes[i];

	in >> script_count;
	scripts.resize(script_count);
	for(int i=0;i<script_count;i++)
		in >> scripts[i];

	in >> method_body_count;
	method_body.resize(method_body_count);
	for(int i=0;i<method_body_count;i++)
	{
		in >> method_body[i];
		//Link method body with method signature
		if(methods[method_body[i].method].body!=NULL)
			LOG(ERROR,"Duplicate body assigment")
		else
			methods[method_body[i].method].body=&method_body[i];
	}

	registerFunctions();
}

inline method_info* ABCVm::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(ERROR,"Requested invalid method");
		return NULL;
	}
}

void ABCVm::popScope(ABCVm* th)
{
	cout << "popScope" << endl;
}

void ABCVm::pushNull(ABCVm* th)
{
	cout << "pushNull" << endl;
}

void ABCVm::pushScope(ABCVm* th)
{
	cout << "pushScope" << endl;
}

void ABCVm::findPropStrict(ABCVm* th, int n)
{
	cout << "findPropStrict " << n << endl;
	th->printMultiname(n);
}

void ABCVm::initProperty(ABCVm* th, int n)
{
	cout << "initProperty " << n << endl;
	th->printMultiname(n);
}

void ABCVm::newClass(ABCVm* th, int n)
{
	cout << "newClass " << n << endl;
	th->printClass(n);
}

void ABCVm::getScopeObject(ABCVm* th, int n)
{
	cout << "getScopeObject " << n << endl;
}

void ABCVm::getLex(ABCVm* th, int n)
{
	cout << "getLex " << n << endl;
	th->printMultiname(n);
}

void ABCVm::getLocal(ABCVm* th, int n)
{
	cout << "getLocal " << n << endl;
}

llvm::Function* ABCVm::synt_method(method_info* m)
{
	if(m->f)
		return m->f;

	if(!m->body)
	{
		string n=getString(m->name);
		LOG(CALLS,"Method " << n << " should be intrinsic");
		return NULL;
	}
	stringstream code(m->body->code);

	//The pointer size compatible int type will be useful
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();

	//Initialize LLVM representation of method
	vector<const llvm::Type*> sig;
	if(m->param_count)
		LOG(ERROR,"Arguments not supported");

	llvm::FunctionType* method_type=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	m->f=llvm::Function::Create(method_type,llvm::Function::ExternalLinkage,"method",&module);
	llvm::BasicBlock *BB = llvm::BasicBlock::Create("entry", m->f);
	llvm::IRBuilder<> Builder;
	Builder.SetInsertPoint(BB);

	//We define a couple of variables that will be used a lot
	llvm::Constant* constant;
	llvm::Value* value;
	//let's give access to 'this' pointer to llvm
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)this);
	llvm::Value* th = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(ptr_type));

	//Each case block builds the correct parameters for the interpreter function and call it
	u8 opcode;
	while(1)
	{
		code >> opcode;
		if(code.eof())
			break;

		switch(opcode)
		{
			case 0x1d:
			{
				//popscope
				Builder.CreateCall(ex->FindFunctionNamed("popScope"), th);
				break;
			}
			case 0x20:
			{
				//pushnull
				Builder.CreateCall(ex->FindFunctionNamed("pushNull"), th);
				break;
			}
			case 0x30:
			{
				//pushscope
				Builder.CreateCall(ex->FindFunctionNamed("pushScope"), th);
				break;
			}
			case 0x47:
			{
				//returnvoid
				Builder.CreateRetVoid();
				break;
			}
			case 0x58:
			{
				//newclass
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newClass"), th, constant);
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("findPropStrict"), th, constant);
				break;
			}
			case 0x60:
			{
				//getlex
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getLex"), th, constant);
				break;
			}
			case 0x65:
			{
				//getscopeobject
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getScopeObject"), th, constant);
				break;
			}
			case 0x68:
			{
				//initproperty
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("initProperty"), th, constant);
				break;
			}
			case 0xd0:
			{
				//getlocal_n
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode&3);
				Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), th, constant);
				break;
			}
			default:
				LOG(ERROR,"Not implemented instruction");
				u8 a,b,c;
				code >> a >> b >> c;
				LOG(ERROR,"dump " << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
						<< (unsigned int)b << ' ' << (unsigned int)c);
				Builder.CreateRetVoid();
				return m->f;
		}
	}
	return m->f;
}

void ABCVm::Run()
{
/*	for(int i=0;i<scripts.size();i++)
	{
		cout << "Script N: " << i << endl;
		for(int j=0;j<scripts[i].trait_count;j++)
			printTrait(&scripts[i].traits[j]);
	}*/

	//Take last script entry and run it
	method_info* m=get_method(scripts.back().init);
	cout << "Building entry script traits: " << scripts.back().trait_count << endl;
	for(int i=0;i<scripts.back().trait_count;i++)
		buildTrait(&scripts.back().traits[i]);
	printMethod(m);
	synt_method(m);
	//Set register 0 to Global
	registers[0]=SWFObject(&Global);

	if(m->f)
	{
		module.dump();
		void* f_ptr=ex->getPointerToFunction(m->f);
		void (*FP)() = (void (*)())f_ptr;
		FP();
	}
}

string ABCVm::getString(unsigned int s) const
{
	if(s)
		return constant_pool.strings[s];
	else
		return "<Anonymous>";
}

void ABCVm::buildTrait(const traits_info* t)
{
	switch(t->kind)
	{
		case traits_info::Class:
		{
			string name=getString(constant_pool.multinames[t->name].name);
			LOG(CALLS,"Registering trait " << name);
			Global.setVariableByName(STRING(name.c_str()), buildClass(t->classi));
			break;
		}
		default:
			LOG(ERROR,"Trait not supported");
	}
}

void ABCVm::printTrait(const traits_info* t) const
{
	switch(t->kind)
	{
		case traits_info::Slot:
			LOG(CALLS,"Slot trait");
			break;
		case traits_info::Method:
			LOG(CALLS,"Method trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Getter:
			LOG(CALLS,"Getter trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Setter:
			LOG(CALLS,"Setter trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Class:
			LOG(CALLS,"Class trait: slot "<< t->slot_id);
			printClass(t->classi);
			break;
		case traits_info::Function:
			LOG(CALLS,"Function trait");
			break;
		case traits_info::Const:
			LOG(CALLS,"Const trait");
			break;
	}
	printMultiname(t->name);
}

void ABCVm::printMultiname(int mi) const
{
	if(mi==0)
	{
		LOG(CALLS, "Any (*)");
		return;
	}
	const multiname_info* m=&constant_pool.multinames[mi];
	LOG(CALLS, "NameID: " << m->name );
	switch(m->kind)
	{
		case 0x07:
			LOG(CALLS, "QName: " << getString(m->name) );
			printNamespace(m->ns);
			break;
		case 0x0d:
			LOG(CALLS, "QNameA");
			break;
		case 0x0f:
			LOG(CALLS, "RTQName");
			break;
		case 0x10:
			LOG(CALLS, "RTQNameA");
			break;
		case 0x11:
			LOG(CALLS, "RTQNameL");
			break;
		case 0x12:
			LOG(CALLS, "RTQNameLA");
			break;
		case 0x09:
		{
			LOG(CALLS, "Multiname: " << getString(m->name));
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			printNamespaceSet(s);
			break;
		}
		case 0x0e:
			LOG(CALLS, "MultinameA");
			break;
		case 0x1b:
			LOG(CALLS, "MultinameL");
			break;
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;
	}
}

void ABCVm::printNamespace(int n) const
{
	if(n==0)
	{
		LOG(CALLS,"Any (*)");
		return;
	}

	const namespace_info* m=&constant_pool.namespaces[n];
	switch(m->kind)
	{
		case 0x08:
			LOG(CALLS, "Namespace " << getString(m->name));
			break;
		case 0x16:
			LOG(CALLS, "PackageNamespace " << getString(m->name));
			break;
		case 0x17:
			LOG(CALLS, "PackageInternalNs " << getString(m->name));
			break;
		case 0x18:
			LOG(CALLS, "ProtectedNamespace " << getString(m->name));
			break;
		case 0x19:
			LOG(CALLS, "ExplicitNamespace " << getString(m->name));
			break;
		case 0x1a:
			LOG(CALLS, "StaticProtectedNamespace " << getString(m->name));
			break;
		case 0x05:
			LOG(CALLS, "PrivateNamespace " << getString(m->name));
			break;
	}
}

void ABCVm::printNamespaceSet(const ns_set_info* m) const
{
	for(int i=0;i<m->count;i++)
	{
		printNamespace(m->ns[i]);
	}
}

SWFObject ABCVm::buildClass(int m) 
{
	if(valid_classes.find(m)==valid_classes.end())
	{
		const class_info* c=&classes[m];
		if(c->trait_count)
			LOG(NOT_IMPLEMENTED,"Should add class traits");

		valid_classes[m]=SWFObject(new ASObject);

/*		//Run class initialization
		method_info* mi=get_method(c->cinit);
		synt_method(mi);
		void* f_ptr=ex->getPointerToFunction(mi->f);
		void (*FP)() = (void (*)())f_ptr;
		LOG(CALLS,"Class init lenght" << mi->body->code_length);
		FP();*/
	}

	const instance_info* i=&instances[m];
	string name=getString(constant_pool.multinames[i->name].name);
	LOG(CALLS,"Building class " << name);
	if(i->supername)
	{
		string super=getString(constant_pool.multinames[i->supername].name);
		LOG(NOT_IMPLEMENTED,"Inheritance not supported: super " << super);
	}
	if(i->trait_count)
	{
		LOG(NOT_IMPLEMENTED,"Should add instance traits");
		for(int j=0;j<i->trait_count;j++)
		{
			printTrait(&i->traits[j]);
		}
	}
/*	//Run instance initialization
	method_info* mi=get_method(i->init);
	if(synt_method(mi))
	{
		void* f_ptr=ex->getPointerToFunction(mi->f);
		void (*FP)() = (void (*)())f_ptr;
		LOG(CALLS,"instance init lenght" << mi->body->code_length);
		FP();
	}*/
	return valid_classes[m]->clone();
}

void ABCVm::printClass(int m) const
{
	const instance_info* i=&instances[m];
	LOG(CALLS,"Class name:");
	printMultiname(i->name);
	LOG(CALLS,"Class supername:");
	printMultiname(i->supername);
	LOG(CALLS,"Flags " <<hex << i->flags);
	LOG(CALLS,"Instance traits n: " <<i->trait_count);
	for(int j=0;j<i->trait_count;j++)
		printTrait(&i->traits[j]);

	const class_info* c=&classes[m];
	LOG(CALLS,"Class traits n: " <<c->trait_count);
	for(int j=0;j<c->trait_count;j++)
		printTrait(&c->traits[j]);

}

void ABCVm::printMethod(const method_info* m) const
{
	string n=getString(m->name);
	LOG(CALLS,"Method " << n);
	LOG(CALLS,"Params n: " << m->param_count);
	LOG(CALLS,"Return " << m->return_type);
	LOG(CALLS,"Flags " << m->flags);
	LOG(CALLS,"Body Lenght " << m->body->code_length);
}

istream& operator>>(istream& in, u32& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i==35)
		{
			if(t>15)
				LOG(ERROR,"parsing u32");
			break;
		}
	}
	while(t2&0x80);
	return in;
}

istream& operator>>(istream& in, s32& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i==35)
		{
			if(t>15)
				LOG(ERROR,"parsing s32");
			break;
		}
	}
	while(t2&0x80);
	if(t2&0x40)
	{
		//Sign extend
		for(i;i<32;i++)
			v.val|=(1<<i);
	}
	return in;
}

istream& operator>>(istream& in, u30& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i>29)
			LOG(ERROR,"parsing u30");
	}
	while(t2&0x80);
	return in;
}

istream& operator>>(istream& in, u8& v)
{
	uint8_t t;
	in.read((char*)&t,1);
	v.val=t;
	return in;
}

istream& operator>>(istream& in, u16& v)
{
	uint16_t t;
	in.read((char*)&t,2);
	v.val=t;
	return in;
}

istream& operator>>(istream& in, d64& v)
{
	//Should check if this is right
	in.read((char*)&v.val,8);
	return in;
}

istream& operator>>(istream& in, string_info& v)
{
	in >> v.size;
	//TODO: String are expected to be UTF-8 encoded.
	//This temporary implementation assume ASCII, so fail if high bit is set
	uint8_t t;
	v.val.reserve(v.size);
	for(int i=0;i<v.size;i++)
	{
		in.read((char*)&t,1);
		v.val.push_back(t);
		if(t&0x80)
			LOG(NOT_IMPLEMENTED,"Multibyte not handled");
	}
	return in;
}

istream& operator>>(istream& in, namespace_info& v)
{
	in >> v.kind >> v.name;
	if(v.kind!=0x05 && v.kind!=0x08 && v.kind!=0x16 && v.kind!=0x17 && v.kind!=0x18 && v.kind!=0x19 && v.kind!=0x1a)
		LOG(ERROR,"Unexpected namespace kind");
	return in;
}

istream& operator>>(istream& in, method_body_info& v)
{
	in >> v.method >> v.max_stack >> v.local_count >> v.init_scope_depth >> v.max_scope_depth >> v.code_length;
	v.code.resize(v.code_length);
	for(int i=0;i<v.code_length;i++)
		in.read(&v.code[i],1);

	in >> v.exception_count;
	v.exceptions.resize(v.exception_count);
	for(int i=0;i<v.exception_count;i++)
		in >> v.exceptions[i];

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, ns_set_info& v)
{
	in >> v.count;

	v.ns.resize(v.count);
	for(int i=0;i<v.count;i++)
	{
		in >> v.ns[i];
		if(v.ns[i]==0)
			LOG(ERROR,"0 not allowed");
	}
	return in;
}

istream& operator>>(istream& in, multiname_info& v)
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
			break;
		case 0x11:
		case 0x12:
			break;
		case 0x09:
		case 0x0e:
			in >> v.name >> v.ns_set;
			break;
		case 0x1b:
		case 0x1c:
			in >> v.ns_set;
			break;
		default:
			LOG(ERROR,"Unexpected multiname kind");
			break;
	}
	return in;
}

istream& operator>>(istream& in, method_info& v)
{
	in >> v.param_count;
	in >> v.return_type;

	v.param_type.resize(v.param_count);
	for(int i=0;i<v.param_count;i++)
		in >> v.param_type[i];
	
	in >> v.name >> v.flags;
	if(v.flags&0x08)
	{
		in >> v.option_count;
		v.options.resize(v.option_count);
		for(int i=0;i<v.option_count;i++)
		{
			in >> v.options[i].val >> v.options[i].kind;
			if(v.options[i].kind>0x1a)
				LOG(ERROR,"Unexpected options type");
		}
	}
	if(v.flags&0x80)
	{
		LOG(ERROR,"Params names not supported");
		abort();
	}
	return in;
}

istream& operator>>(istream& in, script_info& v)
{
	in >> v.init >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, class_info& v)
{
	in >> v.cinit >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, metadata_info& v)
{
	in >> v.name;
	in >> v.item_count;

	v.items.resize(v.item_count);
	for(int i=0;i<v.item_count;i++)
	{
		in >> v.items[i].key >> v.items[i].value;
	}
	return in;
}

istream& operator>>(istream& in, traits_info& v)
{
	in >> v.name >> v.kind;
	switch(v.kind&0xf)
	{
		case traits_info::Slot:
		case traits_info::Const:
			in >> v.slot_id >> v.type_name >> v.vindex;
			if(v.vindex)
				in >> v.kind;
			break;
		case traits_info::Class:
			in >> v.slot_id >> v.classi;
			break;
		case traits_info::Function:
			in >> v.slot_id >> v.function;
			break;
		case traits_info::Method:
		case traits_info::Getter:
		case traits_info::Setter:
			in >> v.slot_id >> v.method;
			break;
		default:
			LOG(ERROR,"Unexpected kind " << v.kind);
			break;
	}

	if(v.kind&traits_info::Metadata)
	{
		in >> v.metadata_count;
		v.metadata.resize(v.metadata_count);
		for(int i=0;i<v.metadata_count;i++)
			in >> v.metadata[i];
	}
	return in;
}

istream& operator>>(istream& in, exception_info& v)
{
	in >> v.from >> v.to >> v.target >> v.exc_type >> v.var_name;
	return in;
}

istream& operator>>(istream& in, instance_info& v)
{
	in >> v.name >> v.supername >> v.flags;
	if(v.flags&instance_info::ClassProtectedNs)
		in >> v.protectedNs;

	in >> v.interface_count;
	v.interfaces.resize(v.interface_count);
	for(int i=0;i<v.interface_count;i++)
	{
		in >> v.interfaces[i];
		if(v.interfaces[i]==0)
			abort();
	}

	in >> v.init;

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, cpool_info& v)
{
	in >> v.int_count;
	v.integer.resize(v.int_count);
	for(int i=1;i<v.int_count;i++)
		in >> v.integer[i];

	in >> v.uint_count;
	v.uinteger.resize(v.uint_count);
	for(int i=1;i<v.uint_count;i++)
		in >> v.uinteger[i];

	in >> v.double_count;
	v.doubles.resize(v.double_count);
	for(int i=1;i<v.double_count;i++)
		in >> v.doubles[i];

	in >> v.string_count;
	v.strings.resize(v.string_count);
	for(int i=1;i<v.string_count;i++)
		in >> v.strings[i];

	in >> v.namespace_count;
	v.namespaces.resize(v.namespace_count);
	for(int i=1;i<v.namespace_count;i++)
		in >> v.namespaces[i];

	in >> v.ns_set_count;
	v.ns_sets.resize(v.ns_set_count);
	for(int i=1;i<v.ns_set_count;i++)
		in >> v.ns_sets[i];

	in >> v.multiname_count;
	v.multinames.resize(v.multiname_count);
	for(int i=1;i<v.multiname_count;i++)
		in >> v.multinames[i];

	return in;
}
