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

#ifdef LLVM_28
#define alignof alignOf
#endif

#include "compat.h"
#include <algorithm>

#ifdef LLVM_ENABLED
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#ifdef LLVM_70
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#endif
#ifndef LLVM_36
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/PassManager.h>
#else
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#endif
#ifdef HAVE_IR_DATALAYOUT_H
#  include <llvm/IR/Module.h>
#  include <llvm/IR/LLVMContext.h>
#else
#  include <llvm/Module.h>
#  include <llvm/LLVMContext.h>
#endif
#ifdef HAVE_IR_DATALAYOUT_H
#  include <llvm/IR/DataLayout.h>
#elif defined HAVE_DATALAYOUT_H
#  include <llvm/DataLayout.h>
#else
#  include <llvm/Target/TargetData.h>
#endif
#ifdef HAVE_SUPPORT_TARGETSELECT_H
#include <llvm/Support/TargetSelect.h>
#else
#include <llvm/Target/TargetSelect.h>
#endif
#include <llvm/Target/TargetOptions.h>
#ifdef HAVE_IR_VERIFIER_H
#  include <llvm/IR/Verifier.h>
#else
#  include <llvm/Analysis/Verifier.h>
#endif
#include <llvm/Transforms/Scalar.h> 
#ifdef HAVE_TRANSFORMS_SCALAR_GVN_H
#  include <llvm/Transforms/Scalar/GVN.h>
#endif
#endif
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include <limits>
#include <cmath>
#include "swf.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "scripting/abc.h"
#include "backends/rendering.h"
#include "parsing/tags.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/net/flashnet.h"

using namespace std;
using namespace lightspark;

DEFINE_AND_INITIALIZE_TLS(is_vm_thread);
#ifndef NDEBUG
bool inStartupOrClose=true;
#endif
bool lightspark::isVmThread()
{
#ifndef NDEBUG
	return inStartupOrClose || GPOINTER_TO_INT(tls_get(is_vm_thread));
#else
	return GPOINTER_TO_INT(tls_get(is_vm_thread));
#endif
}


void ABCVm::registerClasses()
{
	Global* builtin=Class<Global>::getInstanceS(m_sys->worker,(ABCContext*)nullptr, 0,false);
	builtin->setRefConstant();
	//Register predefined types, ASObject are enough for not implemented classes
	registerClassesToplevel(builtin);
	registerClassesFlashAccessibility(builtin);
	registerClassesFlashConcurrent(builtin);
	registerClassesFlashCrypto(builtin);
	registerClassesFlashSecurity(builtin);
	registerClassesFlashDisplay(builtin);
	registerClassesFlashDisplay3D(builtin);
	registerClassesFlashFilters(builtin);
	registerClassesFlashText(builtin);
	registerClassesFlashXML(builtin);
	registerClassesFlashExternal(builtin);
	registerClassesFlashUtils(builtin);
	registerClassesFlashGeom(builtin);
	registerClassesFlashEvents(builtin);
	registerClassesFlashNet(builtin);
	registerClassesFlashSampler(builtin);
	registerClassesFlashSystem(builtin);
	registerClassesFlashMedia(builtin);
	registerClassesFlashUI(builtin);
	registerClassesFlashSensors(builtin);
	registerClassesFlashErrors(builtin);
	registerClassesFlashPrinting(builtin);
	registerClassesFlashGlobalization(builtin);
	registerClassesFlashDesktop(builtin);
	registerClassesFlashFilesystem(builtin);
	registerClassesAvmplus(builtin);

	Class_object::getRef(m_sys)->getClass(m_sys)->prototype = _MNR(new_objectPrototype(m_sys->worker));
	Class_object::getRef(m_sys)->getClass(m_sys)->initStandardProps();

	m_sys->systemDomain->registerGlobalScope(builtin);
}

//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_d(call_context* th, number_t rtd, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->mi->context->constant_pool.multinames[n];
	if(m->cached==nullptr)
	{
		m->cached=new (getVm(th->sys)->vmDataMemory) multiname(getVm(th->sys)->vmDataMemory);
		ret=m->cached;
		ret->isAttribute=m->isAttributeName();
		switch(m->kind)
		{
			case 0x1b:
			{
				const ns_set_info* s=&th->mi->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					ret->ns.emplace_back(th->mi->context, s->ns[i]);
				}
				sort(ret->ns.begin(),ret->ns.end());
				ret->name_d=rtd;
				ret->name_type=multiname::NAME_NUMBER;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
		return ret;
	}
	else
	{
		ret=m->cached;
		switch(m->kind)
		{
			case 0x1b:
			{
				ret->name_d=rtd;
				ret->name_type=multiname::NAME_NUMBER;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
		return ret;
	}
}

//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_i(call_context* th, uint32_t rti, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->mi->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new (getVm(th->sys)->vmDataMemory) multiname(getVm(th->sys)->vmDataMemory);
		ret=m->cached;
		ret->isAttribute=m->isAttributeName();
		switch(m->kind)
		{
			case 0x1b:
			{
				const ns_set_info* s=&th->mi->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					ret->ns.emplace_back(th->mi->context, s->ns[i]);
				}
				sort(ret->ns.begin(),ret->ns.end());
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
		return ret;
	}
	else
	{
		ret=m->cached;
		switch(m->kind)
		{
			case 0x1b:
			{
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
		return ret;
	}
}

/*
 * Gets a multiname. May pop one value of the runtime stack
 * This is a helper called from interpreter.
 */
multiname* ABCContext::getMultiname(unsigned int n, call_context* context)
{
	int fromStack = 0;
	if(n!=0)
	{
		const multiname_info* m=&constant_pool.multinames[n];
		if (m->cached && m->cached->isStatic)
			return m->cached;
		fromStack = m->runtimeargs;
	}
	
	asAtom rt1=asAtomHandler::invalidAtom;
	ASObject* rt2 = nullptr;
	if(fromStack > 0)
	{
		if(!context)
			return nullptr;
		RUNTIME_STACK_POP(context,rt1);
	}
	if(fromStack > 1)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,rt2);
	}
	return getMultinameImpl(rt1,rt2,n);
}

/*
 * Gets a multiname without accessing the runtime stack.
 * The object from the top of the stack must be provided in 'n'
 * if getMultinameRTData(midx) returns 1 and the top two objects
 * must be provided if getMultinameRTData(midx) returns 2.
 * This is a helper used by codesynt.
 */
multiname* ABCContext::s_getMultiname(ABCContext* th, asAtom& n, ASObject* n2, int midx)
{
	return th->getMultinameImpl(n,n2,midx);
}

/*
 * Gets a multiname without accessing the runtime stack.
 * If getMultinameRTData(midx) return 1 then the object
 * from the top of the stack must be provided in 'n'.
 * If getMultinameRTData(midx) return 2 then the two objects
 * from the top of the stack must be provided in 'n' and 'n2'.
 *
 * ATTENTION: The returned multiname may change its value
 * with the next invocation of getMultinameImpl if
 * getMultinameRTData(midx) != 0.
 */
multiname* ABCContext::getMultinameImpl(asAtom& n, ASObject* n2, unsigned int midx,bool isrefcounted)
{
	if (constant_pool.multiname_count == 0)
		return NULL;
	multiname* ret;
	multiname_info* m=&constant_pool.multinames[midx];

	/* If this multiname is not cached, resolve its static parts */
	if(m->cached==NULL)
	{
		m->cached=new (getVm(root->getSystemState())->vmDataMemory) multiname(getVm(root->getSystemState())->vmDataMemory);
		ret=m->cached;
		if(midx==0)
		{
			ret->name_s_id=BUILTIN_STRINGS::ANY;
			ret->name_type=multiname::NAME_STRING;
			ret->ns.emplace_back(root->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			ret->isAttribute=false;
			ret->hasEmptyNS=true;
			ret->hasBuiltinNS=false;
			ret->hasGlobalNS=true;
			ret->isInteger=false;
			return ret;
		}
		ret->isAttribute=m->isAttributeName();
		ret->hasEmptyNS = false;
		ret->hasBuiltinNS=false;
		ret->hasGlobalNS=false;
		ret->isInteger=false;
		switch(m->kind)
		{
			case 0x07: //QName
			case 0x0D: //QNameA
			{
				if (constant_pool.namespaces[m->ns].kind == 0)
				{
					ret->hasGlobalNS=true;
				}
				else
				{
					ret->ns.emplace_back(constant_pool.namespaces[m->ns].getNS(this,m->ns));
					ret->hasEmptyNS = (ret->ns.begin()->hasEmptyName());
					ret->hasBuiltinNS=(ret->ns.begin()->hasBuiltinName());
					ret->hasGlobalNS=(ret->ns.begin()->kind == NAMESPACE);
				}
				if (m->name)
				{
					ret->name_s_id=getString(m->name);
					ret->name_type=multiname::NAME_STRING;
					ret->isInteger=Array::isIntegerWithoutLeadingZeros(root->getSystemState()->getStringFromUniqueId(ret->name_s_id));
				}
				break;
			}
			case 0x09: //Multiname
			case 0x0e: //MultinameA
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					nsNameAndKind ns = constant_pool.namespaces[s->ns[i]].getNS(this,s->ns[i]);
					if (ns.hasEmptyName())
						ret->hasEmptyNS = true;
					if (ns.hasBuiltinName())
						ret->hasBuiltinNS = true;
					if (ns.kind == NAMESPACE)
						ret->hasGlobalNS = true;
					ret->ns.emplace_back(ns);
				}

				if (m->name)
				{
					ret->name_s_id=getString(m->name);
					ret->name_type=multiname::NAME_STRING;
					ret->isInteger=Array::isIntegerWithoutLeadingZeros(root->getSystemState()->getStringFromUniqueId(ret->name_s_id));
				}
				break;
			}
			case 0x1b: //MultinameL
			case 0x1c: //MultinameLA
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->isStatic = false;
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					nsNameAndKind ns = constant_pool.namespaces[s->ns[i]].getNS(this,s->ns[i]);
					if (ns.hasEmptyName())
						ret->hasEmptyNS = true;
					if (ns.hasBuiltinName())
						ret->hasBuiltinNS = true;
					if (ns.kind == NAMESPACE)
						ret->hasGlobalNS = true;
					ret->ns.emplace_back(ns);
				}
				break;
			}
			case 0x0f: //RTQName
			case 0x10: //RTQNameA
			{
				ret->isStatic = false;
				ret->name_type=multiname::NAME_STRING;
				ret->name_s_id=getString(m->name);
				ret->isInteger=Array::isIntegerWithoutLeadingZeros(root->getSystemState()->getStringFromUniqueId(ret->name_s_id));
				break;
			}
			case 0x11: //RTQNameL
			case 0x12: //RTQNameLA
			{
				//Everything is dynamic
				ret->isStatic = false;
				break;
			}
			case 0x1d: //Template instance Name
			{
				multiname_info* td=&constant_pool.multinames[m->type_definition];
				//builds a name by concating the templateName$TypeName1$TypeName2...
				//this naming scheme is defined by the ABC compiler
				tiny_string name = root->getSystemState()->getStringFromUniqueId(getString(td->name));
				for(size_t i=0;i<m->param_types.size();++i)
				{
					ret->templateinstancenames.push_back(getMultiname(m->param_types[i],nullptr));
					multiname_info* p=&constant_pool.multinames[m->param_types[i]];
					name += "$";
					tiny_string nsname;
					if (p->ns < constant_pool.namespaces.size())
					{
						// TODO there's no documentation about how to handle derived classes
						// We just prepend the namespace to the template class, so we can find it when needed
						namespace_info nsi = constant_pool.namespaces[p->ns];
						nsname = root->getSystemState()->getStringFromUniqueId(getString(nsi.name));
						if (nsname != "")
							name += nsname+"$";
					}
					name += root->getSystemState()->getStringFromUniqueId(getString(p->name));
				}
				ret->ns.emplace_back(constant_pool.namespaces[td->ns].getNS(this,td->ns));
				ret->hasEmptyNS = (ret->ns.begin()->hasEmptyName());
				ret->hasBuiltinNS=(ret->ns.begin()->hasBuiltinName());
				ret->hasGlobalNS=(ret->ns.begin()->kind == NAMESPACE);
				ret->name_s_id=root->getSystemState()->getUniqueStringId(name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
	}

	/* Now resolve its dynamic parts */
	ret=m->cached;
	if(midx==0 || ret->isStatic)
		return ret;
	switch(m->kind)
	{
		case 0x1d: //Template instance name
		case 0x07: //QName
		case 0x0d: //QNameA
		case 0x09: //Multiname
		case 0x0e: //MultinameA
		{
			//Nothing to do, everything is static
			assert(!n2);
			break;
		}
		case 0x1b: //MultinameL
		case 0x1c: //MultinameLA
		{
			assert(asAtomHandler::isValid(n) && !n2);

			//Testing shows that the namespace from a
			//QName is used even in MultinameL
			if (asAtomHandler::isInteger(n))
			{
				ret->name_i=asAtomHandler::toInt(n);
				ret->name_type = multiname::NAME_INT;
				ret->name_s_id = UINT32_MAX;
				ret->isInteger=true;
				if (isrefcounted)
				{
					ASATOM_DECREF(n);
				}
				asAtomHandler::applyProxyProperty(n,root->getSystemState(),*ret);
				break;
			}
			else if (asAtomHandler::isQName(n))
			{
				ASQName *qname = asAtomHandler::as<ASQName>(n);
				// don't overwrite any static parts
				if (!m->dynamic)
					m->dynamic=new (getVm(root->getSystemState())->vmDataMemory) multiname(getVm(root->getSystemState())->vmDataMemory);
				ret=m->dynamic;
				ret->isAttribute=m->cached->isAttribute;
				ret->ns.clear();
				ret->ns.emplace_back(root->getSystemState(),qname->getURI(),NAMESPACE);
				ret->hasEmptyNS = (ret->ns.begin()->hasEmptyName());
				ret->hasBuiltinNS=(ret->ns.begin()->hasBuiltinName());
				ret->hasGlobalNS=(ret->ns.begin()->kind == NAMESPACE);
				ret->isStatic = false;
			}
			else
				asAtomHandler::applyProxyProperty(n,root->getSystemState(),*ret);
			ret->setName(n,root->getInstanceWorker());
			if (isrefcounted)
			{
				ASATOM_DECREF(n);
			}
			break;
		}
		case 0x0f: //RTQName
		case 0x10: //RTQNameA
		{
			assert(asAtomHandler::isValid(n) && !n2);
			assert_and_throw(asAtomHandler::isNamespace(n));
			Namespace* tmpns=asAtomHandler::as<Namespace>(n);
			ret->ns.clear();
			ret->ns.emplace_back(root->getSystemState(),tmpns->uri,tmpns->nskind);
			ret->hasEmptyNS = (ret->ns.begin()->hasEmptyName());
			ret->hasBuiltinNS=(ret->ns.begin()->hasBuiltinName());
			ret->hasGlobalNS=(ret->ns.begin()->kind == NAMESPACE);
			if (isrefcounted)
			{
				ASATOM_DECREF(n);
			}
			break;
		}
		case 0x11: //RTQNameL
		case 0x12: //RTQNameLA
		{
			assert(asAtomHandler::isValid(n) && n2);
			assert_and_throw(n2->classdef==Class<Namespace>::getClass(n2->getSystemState()));
			Namespace* tmpns=static_cast<Namespace*>(n2);
			ret->ns.clear();
			ret->ns.emplace_back(root->getSystemState(),tmpns->uri,tmpns->nskind);
			ret->hasEmptyNS = (ret->ns.begin()->hasEmptyName());
			ret->hasBuiltinNS=(ret->ns.begin()->hasBuiltinName());
			ret->hasGlobalNS=(ret->ns.begin()->kind == NAMESPACE);
			ret->setName(n,root->getInstanceWorker());
			if (isrefcounted)
			{
				ASATOM_DECREF(n);
				n2->decRef();
			}
			break;
		}
		default:
			LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
			throw UnsupportedException("Multiname to String not implemented");
	}
	return ret;
}
ABCContext::ABCContext(RootMovieClip* r, istream& in, ABCVm* vm):scriptsdeclared(false),root(r),constant_pool(vm->vmDataMemory),
	methods(reporter_allocator<method_info>(vm->vmDataMemory)),
	metadata(reporter_allocator<metadata_info>(vm->vmDataMemory)),
	instances(reporter_allocator<instance_info>(vm->vmDataMemory)),
	classes(reporter_allocator<class_info>(vm->vmDataMemory)),
	scripts(reporter_allocator<script_info>(vm->vmDataMemory)),
	method_body(reporter_allocator<method_body_info>(vm->vmDataMemory))
{
	in >> minor >> major;
	LOG(LOG_CALLS,"ABCVm version " << major << '.' << minor);
	in >> constant_pool;

	constantAtoms_integer.resize(constant_pool.integer.size());
	for (uint32_t i = 0; i < constant_pool.integer.size(); i++)
	{
		constantAtoms_integer[i] = asAtomHandler::fromInt(constant_pool.integer[i]);
		if (asAtomHandler::getObject(constantAtoms_integer[i]))
			asAtomHandler::getObject(constantAtoms_integer[i])->setRefConstant();
	}
	constantAtoms_uinteger.resize(constant_pool.uinteger.size());
	for (uint32_t i = 0; i < constant_pool.uinteger.size(); i++)
	{
		constantAtoms_uinteger[i] = asAtomHandler::fromUInt(constant_pool.uinteger[i]);
		if (asAtomHandler::getObject(constantAtoms_uinteger[i]))
			asAtomHandler::getObject(constantAtoms_uinteger[i])->setRefConstant();
	}
	constantAtoms_doubles.resize(constant_pool.doubles.size());
	for (uint32_t i = 0; i < constant_pool.doubles.size(); i++)
	{
		ASObject* res = abstract_d_constant(root->getInstanceWorker(),constant_pool.doubles[i]);
		constantAtoms_doubles[i] = asAtomHandler::fromObject(res);
	}
	constantAtoms_strings.resize(constant_pool.strings.size());
	for (uint32_t i = 0; i < constant_pool.strings.size(); i++)
	{
		constantAtoms_strings[i] = asAtomHandler::fromStringID(constant_pool.strings[i]);
	}
	constantAtoms_namespaces.resize(constant_pool.namespaces.size());
	for (uint32_t i = 0; i < constant_pool.namespaces.size(); i++)
	{
		Namespace* res = Class<Namespace>::getInstanceS(root->getInstanceWorker(),getString(constant_pool.namespaces[i].name),BUILTIN_STRINGS::EMPTY,(NS_KIND)(int)constant_pool.namespaces[i].kind);
		if (constant_pool.namespaces[i].kind != 0)
			res->nskind =(NS_KIND)(int)(constant_pool.namespaces[i].kind);
		res->setRefConstant();
		constantAtoms_namespaces[i] = asAtomHandler::fromObject(res);
	}
	constantAtoms_byte.resize(0x100);
	for (uint32_t i = 0; i < 0x100; i++)
		constantAtoms_byte[i] = asAtomHandler::fromInt((int32_t)(int8_t)i);
	constantAtoms_short.resize(0x10000);
	for (int32_t i = 0; i < 0x10000; i++)
		constantAtoms_short[i] = asAtomHandler::fromInt((int32_t)(int16_t)i);
	atomsCachedMaxID=0;
	
	namespaceBaseId=vm->getAndIncreaseNamespaceBase(constant_pool.namespaces.size());

	in >> method_count;
	methods.resize(method_count);
	for(unsigned int i=0;i<method_count;i++)
	{
		in >> methods[i];
		methods[i].context=this;
	}

	in >> metadata_count;
	metadata.resize(metadata_count);
	for(unsigned int i=0;i<metadata_count;i++)
		in >> metadata[i];

	in >> class_count;
	instances.resize(class_count);
	for(unsigned int i=0;i<class_count;i++)
	{
		in >> instances[i];

		if(instances[i].supername)
		{
			multiname* supermname = getMultiname(instances[i].supername,nullptr);
			QName superclassName(supermname->name_s_id,supermname->ns[0].nsNameId);
			auto it = root->customclassoverriddenmethods.find(superclassName);
			if (it == root->customclassoverriddenmethods.end())
			{
				// super class is builtin class
				instances[i].overriddenmethods = new std::unordered_set<uint32_t>();
			}
			else
			{
				instances[i].overriddenmethods = it->second;
			}
		}
		else
			instances[i].overriddenmethods = new std::unordered_set<uint32_t>();

		if (instances[i].overriddenmethods && !instances[i].isInterface())
		{
			for (uint32_t j = 0; j < instances[i].trait_count; j++)
			{
				if (instances[i].traits[j].kind&traits_info::Override)
				{
					const multiname_info* m=&constant_pool.multinames[instances[i].traits[j].name];
					instances[i].overriddenmethods->insert(getString(m->name));
				}
			}
		}
		multiname* mname = getMultiname(instances[i].name,nullptr);
		QName className(mname->name_s_id,mname->ns[0].nsNameId);
		root->customclassoverriddenmethods.insert(make_pair(className,instances[i].overriddenmethods));

		LOG(LOG_TRACE,"Class " << *getMultiname(instances[i].name,nullptr));
		LOG(LOG_TRACE,"Flags:");
		if(instances[i].isSealed())
			LOG(LOG_TRACE,"\tSealed");
		if(instances[i].isFinal())
			LOG(LOG_TRACE,"\tFinal");
		if(instances[i].isInterface())
			LOG(LOG_TRACE,"\tInterface");
		if(instances[i].isProtectedNs())
			LOG(LOG_TRACE,"\tProtectedNS " << root->getSystemState()->getStringFromUniqueId(getString(constant_pool.namespaces[instances[i].protectedNs].name)));
		if(instances[i].supername)
			LOG(LOG_TRACE,"Super " << *getMultiname(instances[i].supername,nullptr));
		if(instances[i].interface_count)
		{
			LOG(LOG_TRACE,"Implements");
			for(unsigned int j=0;j<instances[i].interfaces.size();j++)
			{
				LOG(LOG_TRACE,"\t" << *getMultiname(instances[i].interfaces[j],nullptr));
			}
		}
		LOG(LOG_TRACE,endl);
	}
	classes.resize(class_count);
	for(unsigned int i=0;i<class_count;i++)
		in >> classes[i];

	in >> script_count;
	scripts.resize(script_count);
	for(unsigned int i=0;i<script_count;i++)
		in >> scripts[i];

	in >> method_body_count;
	method_body.resize(method_body_count);
	for(unsigned int i=0;i<method_body_count;i++)
	{
		in >> method_body[i];

		//Link method body with method signature
		if(methods[method_body[i].method].body!=nullptr)
			throw ParseException("Duplicated body for function");
		else
			methods[method_body[i].method].body=&method_body[i];
	}

	hasRunScriptInit.resize(scripts.size(),false);
#ifdef PROFILING_SUPPORT
	root->getSystemState()->contextes.push_back(this);
#endif
}

ABCContext::~ABCContext()
{
}

#ifdef PROFILING_SUPPORT
void ABCContext::dumpProfilingData(ostream& f) const
{
	for(uint32_t i=0;i<methods.size();i++)
	{
		if(!methods[i].profTime.empty()) //The function was executed at least once
		{
			if(methods[i].validProfName)
				f << "fn=" << methods[i].profName << endl;
			else
				f << "fn=" << &methods[i] << endl;
			for(uint32_t j=0;j<methods[i].profTime.size();j++)
			{
				//Only output instructions that have been actually executed
				if(methods[i].profTime[j]!=0)
					f << j << ' ' << methods[i].profTime[j] << endl;
			}
			auto it=methods[i].profCalls.begin();
			for(;it!=methods[i].profCalls.end();it++)
			{
				if(it->first->validProfName)
					f << "cfn=" << it->first->profName << endl;
				else
					f << "cfn=" << it->first << endl;
				f << "calls=1 1" << endl;
				f << "1 " << it->second << endl;
			}
		}
	}
}
#endif

/*
 * nextNamespaceBase is set to 2 since 0 is the empty namespace and 1 is the AS3 namespace
 */
ABCVm::ABCVm(SystemState* s, MemoryAccount* m):m_sys(s),status(CREATED),isIdle(true),canFlushInvalidationQueue(true),shuttingdown(false),
	events_queue(reporter_allocator<eventType>(m)),idleevents_queue(reporter_allocator<eventType>(m)),nextNamespaceBase(2),
	vmDataMemory(m)
{
	m_sys=s;
}

void ABCVm::start()
{
	t = SDL_CreateThread(Run,"ABCVm",this);
}

void ABCVm::shutdown()
{
	if(status==STARTED)
	{
		//Signal the Vm thread
		event_queue_mutex.lock();
		shuttingdown=true;
		sem_event_cond.signal();
		event_queue_mutex.unlock();
		//Wait for the vm thread
		SDL_WaitThread(t,0);
		status=TERMINATED;
		signalEventWaiters();
	}
}

void ABCVm::addDeletableObject(ASObject *obj)
{
	Locker l(deletable_objects_mutex);
	deletableObjects.push_back(obj);
}

void ABCVm::finalize()
{
	//The event queue may be not empty if the VM has been been started
	if(status==CREATED && !events_queue.empty())
		LOG(LOG_ERROR, "Events queue is not empty as expected");
	events_queue.clear();
}


ABCVm::~ABCVm()
{
	std::unordered_set<std::unordered_set<uint32_t>*> overriddenmethods;
	for(size_t i=0;i<contexts.size();++i)
	{
		for(size_t j=0;j<contexts[i]->class_count;j++)
		{
			if (contexts[i]->instances[j].overriddenmethods)
				overriddenmethods.insert(contexts[i]->instances[j].overriddenmethods);
		}
		delete contexts[i];
	}
	auto it = overriddenmethods.begin();
	while(it != overriddenmethods.end())
	{
		delete (*it);
		it++;
	}
}

int ABCVm::getEventQueueSize()
{
	return events_queue.size();
}

void ABCVm::publicHandleEvent(EventDispatcher* dispatcher, _R<Event> event)
{
	if (dispatcher && dispatcher->is<DisplayObject>() && event->type == "enterFrame" && (
				(dispatcher->is<RootMovieClip>() && dispatcher->as<RootMovieClip>()->isWaitingForParser()) || // RootMovieClip is not yet completely parsed
				(dispatcher->as<DisplayObject>()->legacy && !dispatcher->as<DisplayObject>()->isOnStage()))) // it seems that enterFrame event is only executed for DisplayObjects that are on stage or added from ActionScript
		return;
	if (event->is<ProgressEvent>())
	{
		event->as<ProgressEvent>()->accesmutex.lock();
		if (dispatcher->is<LoaderInfo>()) // ensure that the LoaderInfo reports the same number as the ProgressEvent for bytesLoaded
			dispatcher->as<LoaderInfo>()->setBytesLoadedPublic(event->as<ProgressEvent>()->bytesLoaded);
	}

	std::deque<DisplayObject*> parents;
	//Only set the default target is it's not overridden
	if(asAtomHandler::isInvalid(event->target))
		event->setTarget(asAtomHandler::fromObject(dispatcher));
	/** rollOver/Out are special: according to spec 
	http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/flash/display/InteractiveObject.html?  		
	filter_flash=cs5&filter_flashplayer=10.2&filter_air=2.6#event:rollOver 
	*
	*   The relatedObject is the object that was previously under the pointing device. 
	*   The rollOver events are dispatched consecutively down the parent chain of the object, 
	*   starting with the highest parent that is neither the root nor an ancestor of the relatedObject
	*   and ending with the object.
	*
	*   So no bubbling phase, a truncated capture phase, and sometimes no target phase (when the target is an ancestor 
	*	of the relatedObject).
	*/
	//This is to take care of rollOver/Out
	bool doTarget = true;
	//capture phase
	if(dispatcher->classdef && dispatcher->classdef->isSubClass(Class<DisplayObject>::getClass(dispatcher->getSystemState())))
	{
		event->eventPhase = EventPhase::CAPTURING_PHASE;
		//We fetch the relatedObject in the case of rollOver/Out
		DisplayObject* rcur=nullptr;
		if(event->type == "rollOver" || event->type == "rollOut")
		{
			event->incRef();
			_R<MouseEvent> mevent = _MR(event->as<MouseEvent>());
			if(mevent->relatedObject)
			{
				rcur = mevent->relatedObject.getPtr();
			}
			dispatcher->as<DisplayObject>()->handleMouseCursor(event->type == "rollOver");
		}
		//If the relObj is non null, we get its ancestors to build a truncated parents queue for the target 
		if(rcur)
		{
			std::vector<DisplayObject*> rparents;
			rparents.push_back(rcur);
			while(true)
			{
				if(!rcur->getParent())
					break;
				rcur = rcur->getParent();
				rparents.push_back(rcur);
			}
			DisplayObject* cur = dispatcher->as<DisplayObject>();
			//Check if cur is an ancestor of rcur
			auto i = rparents.begin();
			for(;i!=rparents.end();++i)
			{
				if((*i) == cur)
				{
					doTarget = false;//If the dispatcher is an ancestor of the relatedObject, no target phase
					break;
				}
			}
			//Get the parents of cur that are not ancestors of rcur
			while(true && doTarget)
			{
				if(!cur->getParent())
					break;
				cur = cur->getParent();
				auto i = rparents.begin();
				bool stop = false;
				for(;i!=rparents.end();++i)
				{
					if((*i) == cur) 
					{
						stop = true;
						break;
					}
				}
				if (stop) break;
				parents.push_back(cur);
			}
		}
		//The standard behavior
		else
		{
			DisplayObject* cur = dispatcher->as<DisplayObject>();
			while(true)
			{
				if(!cur->getParent())
					break;
				cur = cur->getParent();
				parents.push_back(cur);
			}
		}
		auto i = parents.rbegin();
		for(;i!=parents.rend();++i)
		{
			if (event->immediatePropagationStopped || event->propagationStopped)
				break;
			(*i)->incRef();
			event->currentTarget=_MNR(*i);
			(*i)->handleEvent(event);
			event->currentTarget=NullRef;
		}
	}

	//Do target phase
	if(doTarget)
	{
		event->eventPhase = EventPhase::AT_TARGET;
		if (!event->immediatePropagationStopped && !event->propagationStopped)
		{
			dispatcher->incRef();
			event->currentTarget=_MNR(dispatcher);
			dispatcher->handleEvent(event);
			event->currentTarget=NullRef;
		}
	}
	
	bool stagehandled=false;
	//Do bubbling phase
	if(event->bubbles && !parents.empty())
	{
		event->eventPhase = EventPhase::BUBBLING_PHASE;
		auto i = parents.begin();
		for(;i!=parents.end();++i)
		{
			if ((*i)->is<Stage>())
				stagehandled=true;
			if (event->immediatePropagationStopped || event->propagationStopped)
				break;
			(*i)->incRef();
			event->currentTarget=_MNR(*i);
			(*i)->handleEvent(event);
			event->currentTarget=NullRef;
		}
	}
	// ensure that keyboard events are also handled for the stage
	if (event->is<KeyboardEvent>() && !stagehandled && !dispatcher->is<Stage>() && dispatcher->is<DisplayObject>())
	{
		dispatcher->getSystemState()->stage->incRef();
		event->currentTarget=_MR(dispatcher->getSystemState()->stage);
		dispatcher->getSystemState()->stage->handleEvent(event);
		event->currentTarget=NullRef;
		if(!event->defaultPrevented)
			dispatcher->getSystemState()->stage->defaultEventBehavior(event);
	}
	if (event->type == "mouseDown" && dispatcher->is<InteractiveObject>())
	{
		// only set new focus target if the current focus is not a child of the dispatcher
		InteractiveObject* f = dispatcher->getSystemState()->stage->getFocusTarget().getPtr();
		while (f && f != dispatcher)
			f=f->getParent();
		if (!f)
		{
			dispatcher->incRef();
			dispatcher->getSystemState()->stage->setFocusTarget(_MNR(dispatcher->as<InteractiveObject>()));
		}
	}
	if (dispatcher->is<DisplayObject>() || dispatcher->is<LoaderInfo>() || dispatcher->is<URLLoader>())
		dispatcher->getSystemState()->stage->AVM1HandleEvent(dispatcher,event.getPtr());
	else
		dispatcher->AVM1HandleEvent(dispatcher,event.getPtr());
	
	/* This must even be called if stop*Propagation has been called */
	if(!event->defaultPrevented)
		dispatcher->defaultEventBehavior(event);
	
	//Reset events so they might be recycled
	event->currentTarget=NullRef;
	event->setTarget(asAtomHandler::invalidAtom);
	if (event->is<ProgressEvent>())
		event->as<ProgressEvent>()->accesmutex.unlock();
}
void ABCVm::handleEvent(std::pair<_NR<EventDispatcher>, _R<Event> > e)
{
	//LOG(LOG_INFO,"handleEvent:"<<e.second->type);
	e.second->check();
	if(!e.first.isNull())
		publicHandleEvent(e.first.getPtr(), e.second);
	else
	{
		//Should be handled by the Vm itself
		switch(e.second->getEventType())
		{
			case BIND_CLASS:
			{
				BindClassEvent* ev=static_cast<BindClassEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"Binding of " << ev->class_name);
				if(ev->tag)
					buildClassAndBindTag(ev->class_name.raw_buf(),ev->tag);
				else
					buildClassAndInjectBase(ev->class_name.raw_buf(),ev->base);
				LOG(LOG_CALLS,"End of binding of " << ev->class_name);
				break;
			}
			case SHUTDOWN:
			{
				//no need to lock as this is the vm thread
				shuttingdown=true;
				getSys()->signalTerminated();
				break;
			}
			case FUNCTION:
			{
				FunctionEvent* ev=static_cast<FunctionEvent*>(e.second.getPtr());
				try
				{
					asAtom result=asAtomHandler::invalidAtom;
					if (asAtomHandler::is<AVM1Function>(ev->f))
						asAtomHandler::as<AVM1Function>(ev->f)->call(&result,&ev->obj,ev->args,ev->numArgs);
					else
						asAtomHandler::callFunction(ev->f,getWorker(),result,ev->obj,ev->args,ev->numArgs,true);
					ASATOM_DECREF(result);
				}
				catch(ASObject* exception)
				{
					//Exception unhandled, report up
					ev->signal();
					throw;
				}
				catch(LightsparkException& e)
				{
					//An internal error happended, sync and rethrow
					ev->signal();
					throw;
				}
				break;
			}
			case FUNCTION_ASYNC:
			{
				FunctionAsyncEvent* ev=static_cast<FunctionAsyncEvent*>(e.second.getPtr());
				asAtom result=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<AVM1Function>(ev->f))
					asAtomHandler::as<AVM1Function>(ev->f)->call(&result,&ev->obj,ev->args,ev->numArgs);
				else
					asAtomHandler::callFunction(ev->f,getWorker(),result,ev->obj,ev->args,ev->numArgs,true);
				ASATOM_DECREF(result);
				break;
			}
			case EXTERNAL_CALL:
			{
				ExternalCallEvent* ev=static_cast<ExternalCallEvent*>(e.second.getPtr());
				try
				{
					asAtom* newArgs=nullptr;
					if (ev->numArgs > 0)
					{
						newArgs=g_newa(asAtom, ev->numArgs);
						for (uint32_t i = 0; i < ev->numArgs; i++)
						{
							newArgs[i] = asAtomHandler::fromObject(ev->args[i]);
						}
					}
					asAtom res=asAtomHandler::invalidAtom;
					asAtomHandler::callFunction(ev->f,ev->getInstanceWorker(),res,asAtomHandler::nullAtom,newArgs,ev->numArgs,true);
					*(ev->result)=asAtomHandler::toObject(res,ev->getInstanceWorker());
				}
				catch(ASObject* exception)
				{
					// Report the exception
					*(ev->exception) = exception->toString();
					*(ev->thrown) = true;
				}
				catch(LightsparkException& e)
				{
					//An internal error happended, sync and rethrow
					ev->signal();
					throw;
				}
				break;
			}
			case CONTEXT_INIT:
			{
				ABCContextInitEvent* ev=static_cast<ABCContextInitEvent*>(e.second.getPtr());
				ev->context->exec(ev->lazy);
				contexts.push_back(ev->context);
				break;
			}
			case AVM1INITACTION_EVENT:
			{
				LOG(LOG_CALLS,"AVM1INITACTION:"<<e.second->toDebugString());
				AVM1InitActionEvent* ev=static_cast<AVM1InitActionEvent*>(e.second.getPtr());
				ev->executeActions();
				LOG(LOG_CALLS,"AVM1INITACTION done");
				break;
			}
			case INIT_FRAME:
			{
				InitFrameEvent* ev=static_cast<InitFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"INIT_FRAME");
				assert(!ev->clip.isNull());
				ev->clip->initFrame();
				break;
			}
			case EXECUTE_FRAMESCRIPT:
			{
				ExecuteFrameScriptEvent* ev=static_cast<ExecuteFrameScriptEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"EXECUTE_FRAMESCRIPT");
				assert(!ev->clip.isNull());
				ev->clip->executeFrameScript();
				if (ev->clip == m_sys->stage)
					m_sys->swapAsyncDrawJobQueue();
				break;
			}
			case ADVANCE_FRAME:
			{
				Locker l(m_sys->getRenderThread()->mutexRendering);
				AdvanceFrameEvent* ev=static_cast<AdvanceFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"ADVANCE_FRAME");
				if (ev->clip)
					ev->clip->advanceFrame(true);
				else
					m_sys->stage->advanceFrame(true);
				break;
			}
			case ROOTCONSTRUCTEDEVENT:
			{
				RootConstructedEvent* ev=static_cast<RootConstructedEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"RootConstructedEvent");
				ev->clip->constructionComplete();
				break;
			}
			case IDLE_EVENT:
			{
				m_sys->worker->processGarbageCollection(false);
				// DisplayObjects that are removed from the display list keep their Parent set until all removedFromStage events are handled
				// see http://www.senocular.com/flash/tutorials/orderofoperations/#ObjectDestruction
				m_sys->resetParentList();
				{
					Locker l(event_queue_mutex);
					while (!idleevents_queue.empty())
					{
						events_queue.push_back(idleevents_queue.front());
						idleevents_queue.pop_front();
					}
					isIdle = true;
#ifndef NDEBUG
//					if (getEventQueueSize() == 0)
//						ASObject::dumpObjectCounters(100);
#endif
				}
				break;
			}
			case FLUSH_INVALIDATION_QUEUE:
			{
				//Flush the invalidation queue
				m_sys->flushInvalidationQueue();
				break;
			}
			case PARSE_RPC_MESSAGE:
			{
				ParseRPCMessageEvent* ev=static_cast<ParseRPCMessageEvent*>(e.second.getPtr());
				try
				{
					parseRPCMessage(ev->message, ev->client, ev->responder);
				}
				catch(ASObject* exception)
				{
					LOG(LOG_ERROR, "Exception while parsing RPC message");
				}
				catch(LightsparkException& e)
				{
					LOG(LOG_ERROR, "Internal exception while parsing RPC message");
				}
				break;
			}
			case TEXTINPUT_EVENT:
			{
				TextInputEvent* ev=static_cast<TextInputEvent*>(e.second.getPtr());
				if (ev->target)
					ev->target->textInputChanged(ev->text);
				break;
			}
			default:
				assert(false);
		}
	}

	/* If this was a waitable event, signal it */
	if(e.second->is<WaitableEvent>())
		e.second->as<WaitableEvent>()->signal();
	RELEASE_WRITE(e.second->queued,false);
	//LOG(LOG_INFO,"handleEvent done:"<<e.second->type);
}

bool ABCVm::prependEvent(_NR<EventDispatcher> obj ,_R<Event> ev, bool force)
{
	/* We have to run waitable events directly,
	 * because otherwise waiting on them in the vm thread
	 * will block the vm thread from executing them.
	 */
	if(isVmThread() && ev->is<WaitableEvent>())
	{
		handleEvent( make_pair(obj,ev) );
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return true;
	}

	Locker l(event_queue_mutex);

	//If the system should terminate new events are not accepted
	if(shuttingdown)
		return false;

	if (!obj.isNull())
		obj->onNewEvent(ev.getPtr());

	if (isIdle || force)
		events_queue.push_front(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	else
		events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	sem_event_cond.signal();
	return true;
}

/*! \brief enqueue an event, a reference is acquired
* * \param obj EventDispatcher that will receive the event
* * \param ev event that will be sent */
bool ABCVm::addEvent(_NR<EventDispatcher> obj ,_R<Event> ev, bool isGlobalMessage)
{
	if (!isGlobalMessage && !obj.isNull() && ev->getInstanceWorker() && !ev->getInstanceWorker()->isPrimordial)
	{
		return ev->getInstanceWorker()->addEvent(obj,ev);
	}
	/* We have to run waitable events directly,
	 * because otherwise waiting on them in the vm thread
	 * will block the vm thread from executing them.
	 */
	if(isVmThread() && ev->is<WaitableEvent>())
	{
		RELEASE_WRITE(ev->queued,true);
		handleEvent( make_pair(obj,ev) );
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return true;
	}


	Locker l(event_queue_mutex);

	//If the system should terminate new events are not accepted
	if(shuttingdown)
	{
		if (ev->is<WaitableEvent>())
			ev->as<WaitableEvent>()->signal();
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return false;
	}
	if (!obj.isNull())
		obj->onNewEvent(ev.getPtr());
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	sem_event_cond.signal();
	if (isGlobalMessage)
	{
		m_sys->addEventToBackgroundWorkers(obj,ev);
	}
	return true;
}
bool ABCVm::addIdleEvent(_NR<EventDispatcher> obj ,_R<Event> ev, bool removeprevious)
{
	Locker l(event_queue_mutex);
	//If the system should terminate new events are not accepted
	if(shuttingdown)
	{
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return false;
	}
	if (!obj.isNull() && ev->getInstanceWorker() && !ev->getInstanceWorker()->isPrimordial)
		return ev->getInstanceWorker()->addEvent(obj,ev);
	if (removeprevious) 
	{
		for (auto it = idleevents_queue.begin(); it != idleevents_queue.end(); it++)
		{
			// if an idle event with the same dispatcher and same type was added previously, it is removed
			// this avoids flooding the event queue with too many "similar" events (e.g. mouseMove)
			if ((*it).first == obj && (*it).second->type == ev->type)
			{
				idleevents_queue.erase(it);
				break;
			}
		}
	}
	idleevents_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	return true;
}

Class_inherit* ABCVm::findClassInherit(const string& s, RootMovieClip* root)
{
	LOG(LOG_CALLS,"Setting class name to " << s);
	ASObject* target;
	ASObject* derived_class=root->applicationDomain->getVariableByString(s,target);
	if(derived_class==nullptr)
	{
		//LOG(LOG_ERROR,"Class " << s << " not found in global for "<<root->getOrigin());
		//throw RunTimeException("Class not found in global");
		return nullptr;
	}

	assert_and_throw(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	assert_and_throw(static_cast<Class_base*>(derived_class)->class_index!=-1);
	Class_inherit* derived_class_tmp=static_cast<Class_inherit*>(derived_class);
	if(derived_class_tmp->isBinded())
	{
		//LOG(LOG_ERROR, "Class already binded to a tag. Not binding:"<<s<< " class:"<<derived_class_tmp->getQualifiedClassName());
		return nullptr;
	}
	return derived_class_tmp;
}

void ABCVm::buildClassAndInjectBase(const string& s, _R<RootMovieClip> base)
{
	Class_inherit* derived_class_tmp = findClassInherit(s, base.getPtr());
	if(!derived_class_tmp)
		return;

	//Let's override the class
	base->setClass(derived_class_tmp);
	// ensure that traits are initialized for movies loaded from actionscript
	base->setIsInitialized(false);
	derived_class_tmp->bindToRoot();
	// the root movie clip may have it's own constructor, so we make sure it is called
	asAtom r = asAtomHandler::fromObject(base.getPtr());
	derived_class_tmp->handleConstruction(r,nullptr,0,true);
	base->setConstructorCallComplete();
}

bool ABCVm::buildClassAndBindTag(const string& s, DictionaryTag* t, Class_inherit* derived_cls)
{
	Class_inherit* derived_class_tmp = derived_cls? derived_cls : findClassInherit(s, t->loadedFrom);
	if(!derived_class_tmp)
		return false;
	derived_class_tmp->checkScriptInit();
	//It seems to be acceptable for the same base to be binded multiple times.
	//In such cases the first binding is bidirectional (instances created using PlaceObject
	//use the binded class and instances created using 'new' use the binded tag). Any other
	//bindings will be unidirectional (only instances created using new will use the binded tag)
	if(t->bindedTo==nullptr)
		t->bindedTo=derived_class_tmp;

	derived_class_tmp->bindToTag(t);
	return true;
}
void ABCVm::checkExternalCallEvent()
{
	if (shuttingdown)
		return;
	event_queue_mutex.lock();
	if (events_queue.size() == 0)
	{
		event_queue_mutex.unlock();
		return;
	}
	pair<_NR<EventDispatcher>,_R<Event>> e=events_queue.front();
	if (e.first.isNull() && e.second->getEventType() == EXTERNAL_CALL)
		handleFrontEvent();
	else
		event_queue_mutex.unlock();
}
void ABCVm::handleFrontEvent()
{
	pair<_NR<EventDispatcher>,_R<Event>> e=events_queue.front();
	events_queue.pop_front();

	event_queue_mutex.unlock();
	try
	{
		//handle event without lock
		handleEvent(e);
		if (e.second->getEventType() == INIT_FRAME) // don't flush between initFrame and executeFrameScript
			canFlushInvalidationQueue=false;
		if (e.second->getEventType() == EXECUTE_FRAMESCRIPT) // don't flush between initFrame and executeFrameScript
			canFlushInvalidationQueue=true;
		//Flush the invalidation queue
		if (canFlushInvalidationQueue && (!e.first.isNull() || 
				(e.second->getEventType() != EXTERNAL_CALL)
				))
			m_sys->flushInvalidationQueue();
		if (!e.first.isNull())
			e.first->afterHandleEvent(e.second.getPtr());
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Error in VM " << e.cause);
		m_sys->setError(e.cause);
		/* do not allow any more event to be enqueued */
		signalEventWaiters();
	}
	catch(ASObject*& e)
	{
		if(e->getClass())
			LOG(LOG_ERROR,"Unhandled ActionScript exception in VM " << e->toString());
		else
			LOG(LOG_ERROR,"Unhandled ActionScript exception in VM (no type)");
		if (e->is<ASError>())
		{
			LOG(LOG_ERROR,"Unhandled ActionScript exception in VM " << e->as<ASError>()->getStackTraceString());
			if (m_sys->ignoreUnhandledExceptions)
				return;
			m_sys->setError(e->as<ASError>()->getStackTraceString());
		}
		else
			m_sys->setError("Unhandled ActionScript exception");
		if (!m_sys->isShuttingDown())
		{
			/* do not allow any more event to be enqueued */
			shuttingdown = true;
			signalEventWaiters();
		}
	}
}

method_info* ABCContext::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(LOG_ERROR,"Requested invalid method");
		return NULL;
	}
}

void ABCVm::not_impl(int n)
{
	LOG(LOG_NOT_IMPLEMENTED, "not implement opcode 0x" << hex << n );
	throw UnsupportedException("Not implemented opcode");
}



void call_context::handleError(int errorcode)
{
	createError<ASError>(getWorker(),errorcode);
}

bool ABCContext::isinstance(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, "isinstance " << *name);

	if(name->name_s_id == BUILTIN_STRINGS::ANY)
		return true;
	
	ASObject* target;
	asAtom ret=asAtomHandler::invalidAtom;
	root->applicationDomain->getVariableAndTargetByMultiname(ret,*name, target,root->getInstanceWorker());
	if(asAtomHandler::isInvalid(ret)) //Could not retrieve type
	{
		LOG(LOG_ERROR,"isInstance: Cannot retrieve type:"<<*name);
		return false;
	}

	ASObject* type=asAtomHandler::toObject(ret,obj->getInstanceWorker());
	bool real_ret=false;
	Class_base* objc=obj->classdef;
	Class_base* c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		real_ret=(c==Class<Integer>::getClass(obj->getSystemState()) || c==Class<Number>::getClass(obj->getSystemState()) || c==Class<UInteger>::getClass(obj->getSystemState()));
		LOG(LOG_CALLS,"Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		return real_ret;
	}

	if(!objc)
	{
		real_ret=obj->getObjectType()==type->getObjectType();
		LOG(LOG_CALLS,"isType on non classed object " << real_ret);
		return real_ret;
	}

	assert_and_throw(type->getObjectType()==T_CLASS);
	real_ret=objc->isSubClass(c);
	LOG(LOG_CALLS,"Type " << objc->class_name << " is " << ((real_ret)?"":"not ") 
			<< "subclass of " << c->class_name);
	return real_ret;
}

void ABCContext::declareScripts()
{
	if (scriptsdeclared)
		return;
	//Take script entries and declare their traits
	unsigned int i=0;

	for(;i<scripts.size();i++)
	{
		LOG(LOG_CALLS, "Script N: " << i );

		//Creating a new global for this script
		Global* global=Class<Global>::getInstanceS(root->getInstanceWorker(),this, i,false);
		global->setRefConstant();
#ifndef NDEBUG
		global->initialized=false;
#endif
		LOG(LOG_CALLS, "Building script traits: " << scripts[i].trait_count );


		std::vector<multiname*> additionalslots;
		for(unsigned int j=0;j<scripts[i].trait_count;j++)
		{
			buildTrait(global,additionalslots,&scripts[i].traits[j],false,i);
		}
		global->initAdditionalSlots(additionalslots);

#ifndef NDEBUG
		global->initialized=true;
#endif
		//Register it as one of the global scopes
		root->applicationDomain->registerGlobalScope(global);
	}
	scriptsdeclared=true;
}
/*
 * The ABC definitions (classes, scripts, etc) have been parsed in
 * ABCContext constructor. Now create the internal structures for them
 * and execute the main/init function.
 */
void ABCContext::exec(bool lazy)
{
	declareScripts();
	//The last script entry has to be run
	LOG(LOG_CALLS, "Last script (Entry Point)");
	//Creating a new global for the last script
	Global* global=root->applicationDomain->getLastGlobalScope();

	//the script init of the last script is the main entry point
	if(!lazy)
	{
		int lastscript = scripts.size()-1;
		asAtom g = asAtomHandler::fromObject(global);
		runScriptInit(lastscript, g);
	}
	LOG(LOG_CALLS, "End of Entry Point");
}

void ABCContext::runScriptInit(unsigned int i, asAtom &g)
{
	LOG(LOG_CALLS, "Running script init for script " << i );

	assert(!hasRunScriptInit[i]);
	hasRunScriptInit[i] = true;

	method_info* m=get_method(scripts[i].init);
	SyntheticFunction* entry=Class<IFunction>::getSyntheticFunction(this->root->getInstanceWorker(),m,m->numArgs());
	entry->fromNewFunction=true;
	
	entry->addToScope(scope_entry(g,false));

	asAtom ret=asAtomHandler::invalidAtom;
	asAtom f =asAtomHandler::fromObject(entry);
	asAtomHandler::callFunction(f,this->root->getInstanceWorker(),ret,g,nullptr,0,false);

	ASATOM_DECREF(ret);

	entry->decRef();
	LOG(LOG_CALLS, "Finished script init for script " << i );
}

int ABCVm::Run(void* d)
{
	ABCVm* th = (ABCVm*)d;
	//Spin wait until the VM is aknowledged by the SystemState
	setTLSSys(th->m_sys);
	setTLSWorker(th->m_sys->worker);
	while(getVm(th->m_sys)!=th)
		;

	/* set TLS variable for isVmThread() */
	tls_set(is_vm_thread, GINT_TO_POINTER(1));
#ifndef NDEBUG
	inStartupOrClose= false;
#endif
	if(th->m_sys->useJit)
	{
#ifdef LLVM_ENABLED
#ifdef LLVM_31
		llvm::TargetOptions Opts;
#ifndef LLVM_34
		Opts.JITExceptionHandling = true;
#endif
#else
		llvm::JITExceptionHandling = true;
#endif
#if defined(NDEBUG) && !defined(LLVM_37)
#ifdef LLVM_31
		Opts.JITEmitDebugInfo = true;
#else
		llvm::JITEmitDebugInfo = true;
#endif
#endif
		llvm::InitializeNativeTarget();
#ifdef LLVM_34
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
#endif
		th->module=new llvm::Module(llvm::StringRef("abc jit"),th->llvm_context());
#ifdef LLVM_36
		llvm::EngineBuilder eb(std::unique_ptr<llvm::Module>(th->module));
#else
		llvm::EngineBuilder eb(th->module);
#endif
		eb.setEngineKind(llvm::EngineKind::JIT);
		std::string errStr;
		eb.setErrorStr(&errStr);
#ifdef LLVM_31
		eb.setTargetOptions(Opts);
#endif
		eb.setOptLevel(llvm::CodeGenOpt::Default);
		th->ex=eb.create();
		if (th->ex == NULL)
			LOG(LOG_ERROR,"could not create llvm engine:"<<errStr);
		assert_and_throw(th->ex);

#ifdef LLVM_36
		th->FPM=new llvm::legacy::FunctionPassManager(th->module);
#ifndef LLVM_37
		th->FPM->add(new llvm::DataLayoutPass());
#endif
#else
		th->FPM=new llvm::FunctionPassManager(th->module);
#ifdef LLVM_35
		th->FPM->add(new llvm::DataLayoutPass(*th->ex->getDataLayout()));
#elif defined HAVE_DATALAYOUT_H || defined HAVE_IR_DATALAYOUT_H
		th->FPM->add(new llvm::DataLayout(*th->ex->getDataLayout()));
#else
		th->FPM->add(new llvm::TargetData(*th->ex->getTargetData()));
#endif
#endif
#ifdef EXPENSIVE_DEBUG
		//This is pretty heavy, do not enable in release
		th->FPM->add(llvm::createVerifierPass());
#endif
		th->FPM->add(llvm::createPromoteMemoryToRegisterPass());
		th->FPM->add(llvm::createReassociatePass());
		th->FPM->add(llvm::createCFGSimplificationPass());
		th->FPM->add(llvm::createGVNPass());
		th->FPM->add(llvm::createInstructionCombiningPass());
		th->FPM->add(llvm::createLICMPass());
		th->FPM->add(llvm::createDeadStoreEliminationPass());

		th->registerFunctions();
#endif
	}
	th->registerClasses();
	if (!th->m_sys->mainClip->usesActionScript3)
		th->registerClassesAVM1();
	th->status=STARTED;

	ThreadProfile* profile=th->m_sys->allocateProfiler(RGB(0,200,0));
	profile->setTag("VM");
	//When aborting execution remaining events should be handled
	bool firstMissingEvents=true;

#ifdef MEMORY_USAGE_PROFILING
	string memoryProfileFile="lightspark.massif.";
	memoryProfileFile+=th->m_sys->mainClip->getOrigin().getPathFile().raw_buf();
	ofstream memoryProfile(memoryProfileFile, ios_base::out | ios_base::trunc);
	int snapshotCount = 0;
	memoryProfile << "desc: (none) \ncmd: lightspark\ntime_unit: i" << endl;
#endif
	while(true)
	{
		th->deletable_objects_mutex.lock();
		for (auto it = th->deletableObjects.begin(); it != th->deletableObjects.end(); it++)
			(*it)->decRef();
		th->deletableObjects.clear();
		th->deletable_objects_mutex.unlock();
		th->event_queue_mutex.lock();
		while(th->events_queue.empty() && !th->shuttingdown)
			th->sem_event_cond.wait(th->event_queue_mutex);
		if(th->shuttingdown)
		{
			//If the queue is empty stop immediately
			if(th->events_queue.empty())
			{
				th->event_queue_mutex.unlock();
				break;
			}
			else if(firstMissingEvents)
			{
				LOG(LOG_INFO,th->events_queue.size() << " events missing before exit");
				firstMissingEvents = false;
			}
		}
		Chronometer chronometer;

		th->handleFrontEvent();
		profile->accountTime(chronometer.checkpoint());
#ifdef MEMORY_USAGE_PROFILING
		if((snapshotCount%100)==0)
			th->m_sys->saveMemoryUsageInformation(memoryProfile, snapshotCount);
		snapshotCount++;
#endif
	}
#ifdef LLVM_ENABLED
	if(th->m_sys->useJit)
	{
		th->ex->clearAllGlobalMappings();
		delete th->module;
	}
#endif
#ifndef NDEBUG
	inStartupOrClose= true;
#endif
	return 0;
}

/* This breaks the lock on all enqueued events to prevent deadlocking */
void ABCVm::signalEventWaiters()
{
	assert(shuttingdown);
	//we do not need a lock because th->shuttingdown keeps other events from being enqueued
	while(!events_queue.empty())
	{
		pair<_NR<EventDispatcher>,_R<Event>> e=events_queue.front();
		events_queue.pop_front();
		if(e.second->is<WaitableEvent>())
			e.second->as<WaitableEvent>()->signal();
	}
}

void ABCVm::parseRPCMessage(_R<ByteArray> message, _NR<ASObject> client, _NR<Responder> responder)
{
	uint16_t version;
	if(!message->readShort(version))
		return;
	message->setCurrentObjectEncoding(version == 3 ? OBJECT_ENCODING::AMF3 : OBJECT_ENCODING::AMF0);
	uint16_t numHeaders;
	if(!message->readShort(numHeaders))
		return;
	for(uint32_t i=0;i<numHeaders;i++)
	{
		//Read the header name
		//header names are method that must be
		//invoked on the client object
		multiname headerName(NULL);
		headerName.name_type=multiname::NAME_STRING;
		headerName.ns.emplace_back(m_sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
		tiny_string headerNameString;
		if(!message->readUTF(headerNameString))
			return;
		headerName.name_s_id=m_sys->getUniqueStringId(headerNameString);
		//Read the must understand flag
		uint8_t mustUnderstand;
		if(!message->readByte(mustUnderstand))
			return;
		//Read the header length, not really useful
		uint32_t headerLength;
		if(!message->readUnsignedInt(headerLength))
			return;

		uint8_t marker;
		if(!message->peekByte(marker))
			return;
		if (marker == 0x11 && message->getCurrentObjectEncoding() != OBJECT_ENCODING::AMF3) // switch to AMF3
		{
			message->setCurrentObjectEncoding(OBJECT_ENCODING::AMF3);
			message->readByte(marker);
		}
		asAtom v = asAtomHandler::fromObject(message.getPtr());
		asAtom obj=asAtomHandler::invalidAtom;
		ByteArray::readObject(obj,message->getInstanceWorker(), v, nullptr, 0);

		asAtom callback=asAtomHandler::invalidAtom;
		if(!client.isNull())
			client->getVariableByMultiname(callback,headerName,GET_VARIABLE_OPTION::NONE,client->getInstanceWorker());

		//If mustUnderstand is set there must be a suitable callback on the client
		if(mustUnderstand && (client.isNull() || !asAtomHandler::isFunction(callback)))
		{
			//TODO: use onStatus
			throw UnsupportedException("Unsupported header with mustUnderstand");
		}

		if(asAtomHandler::isFunction(callback))
		{
			ASATOM_INCREF(obj);
			asAtom callbackArgs[1] { obj };
			asAtom v = asAtomHandler::fromObject(client.getPtr());
			asAtom r=asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(callback,client->getInstanceWorker(),r,v, callbackArgs, 1,true);
		}
	}
	uint16_t numMessage;
	if(!message->readShort(numMessage))
		return;
	for(uint32_t i=0;i<numMessage;i++)
	{
	
		tiny_string target;
		if(!message->readUTF(target))
			return;
		tiny_string response;
		if(!message->readUTF(response))
			return;
	
		//TODO: Really use the response to map request/responses and detect errors
		uint32_t objLen;
		if(!message->readUnsignedInt(objLen))
			return;
		uint8_t marker;
		if(!message->peekByte(marker))
			return;
		if (marker == 0x11 && message->getCurrentObjectEncoding() != OBJECT_ENCODING::AMF3) // switch to AMF3
		{
			message->setCurrentObjectEncoding(OBJECT_ENCODING::AMF3);
			message->readByte(marker);
		}
		asAtom v = asAtomHandler::fromObject(message.getPtr());
		asAtom ret=asAtomHandler::invalidAtom;
		ByteArray::readObject(ret,message->getInstanceWorker(),v, nullptr, 0);
	
		if(!responder.isNull())
		{
			multiname onResultName(nullptr);
			onResultName.name_type=multiname::NAME_STRING;
			onResultName.name_s_id=m_sys->getUniqueStringId("onResult");
			onResultName.ns.emplace_back(m_sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
			asAtom callback=asAtomHandler::invalidAtom;
			responder->getVariableByMultiname(callback,onResultName,GET_VARIABLE_OPTION::NONE,responder->getInstanceWorker());
			if(asAtomHandler::isFunction(callback))
			{
				ASATOM_INCREF(ret);
				asAtom callbackArgs[1] { ret };
				asAtom v = asAtomHandler::fromObject(responder.getPtr());
				asAtom r=asAtomHandler::invalidAtom;
				asAtomHandler::callFunction(callback,responder->getInstanceWorker(),r,v, callbackArgs, 1,true);
			}
		}
	}
}

_R<ApplicationDomain> ABCVm::getCurrentApplicationDomain(call_context* th)
{
	return th->mi->context->root->applicationDomain;
}

_R<SecurityDomain> ABCVm::getCurrentSecurityDomain(call_context* th)
{
	return th->mi->context->root->securityDomain;
}

uint32_t ABCVm::getAndIncreaseNamespaceBase(uint32_t nsNum)
{
	return ATOMIC_ADD(nextNamespaceBase,nsNum)-nsNum;
}

uint32_t ABCContext::getString(unsigned int s) const
{
	return constant_pool.strings[s];
}

void ABCContext::buildInstanceTraits(ASObject* obj, int class_index)
{
	if(class_index==-1)
		return;

	//Build only the traits that has not been build in the class
	std::vector<multiname*> additionalslots;
	for(unsigned int i=0;i<instances[class_index].trait_count;i++)
	{
		int kind=instances[class_index].traits[i].kind&0xf;
		if(kind==traits_info::Slot || kind==traits_info::Class ||
			kind==traits_info::Function || kind==traits_info::Const)
		{
			buildTrait(obj,additionalslots,&instances[class_index].traits[i],false);
		}
	}
	obj->initAdditionalSlots(additionalslots);
}

void ABCContext::linkTrait(Class_base* c, const traits_info* t)
{
	const multiname& mname=*getMultiname(t->name,nullptr);
	//Should be a Qname
	assert_and_throw(mname.ns.size()==1 && mname.name_type==multiname::NAME_STRING);

	uint32_t nameId=mname.name_s_id;
	if(t->kind>>4)
		LOG(LOG_CALLS,"Next slot has flags " << (t->kind>>4));
	switch(t->kind&0xf)
	{
		//Link the methods to the implementations
		case traits_info::Method:
		{
			LOG(LOG_CALLS,"Method trait: " << mname << " #" << t->method);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			variable* var=NULL;
			var = c->borrowedVariables.findObjVar(nameId,nsNameAndKind(),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->var))
			{
				assert_and_throw(asAtomHandler::isFunction(var->var));

				ASATOM_INCREF(var->var);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->var,NORMAL_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"Method not linkable" << ": " << mname);
			}

			LOG(LOG_TRACE,"End Method trait: " << mname);
			break;
		}
		case traits_info::Getter:
		{
			LOG(LOG_CALLS,"Getter trait: " << mname);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			variable* var=NULL;
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind(),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->getter))
			{
				ASATOM_INCREF(var->getter);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->getter,GETTER_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"Getter not linkable" << ": " << mname);
			}

			LOG(LOG_TRACE,"End Getter trait: " << mname);
			break;
		}
		case traits_info::Setter:
		{
			LOG(LOG_CALLS,"Setter trait: " << mname << " #" << t->method);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			variable* var=NULL;
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind(),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->setter))
			{
				ASATOM_INCREF(var->setter);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->setter,SETTER_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"Setter not linkable" << ": " << mname);
			}
			
			LOG(LOG_TRACE,"End Setter trait: " << mname);
			break;
		}
//		case traits_info::Class:
//		case traits_info::Const:
//		case traits_info::Slot:
		default:
			LOG(LOG_ERROR,"Trait not supported " << mname << " " << t->kind);
			throw UnsupportedException("Trait not supported");
	}
}

void ABCContext::getConstant(asAtom &ret, int kind, int index)
{
	switch(kind)
	{
		case 0x00: //Undefined
			asAtomHandler::setUndefined(ret);
			break;
		case 0x01: //String
			ret = constantAtoms_strings[index];
			break;
		case 0x03: //Int
			ret = constantAtoms_integer[index];
			break;
		case 0x04: //UInt
			ret = constantAtoms_uinteger[index];
			break;
		case 0x06: //Double
			ret = constantAtoms_doubles[index];
			break;
		case 0x08: //Namespace
			ret = constantAtoms_namespaces[index];
			break;
		case 0x0a: //False
			asAtomHandler::setBool(ret,false);
			break;
		case 0x0b: //True
			asAtomHandler::setBool(ret,true);
			break;
		case 0x0c: //Null
			asAtomHandler::setNull(ret);
			break;
		default:
		{
			LOG(LOG_ERROR,"Constant kind " << hex << kind);
			throw UnsupportedException("Constant trait not supported");
		}
	}
}

asAtom* ABCContext::getConstantAtom(OPERANDTYPES kind, int index)
{
	asAtom* ret = NULL;
	switch(kind)
	{
		case OP_UNDEFINED:
			ret = &asAtomHandler::undefinedAtom;
			break;
		case OP_STRING:
			ret = &constantAtoms_strings[index];
			break;
		case OP_INTEGER: //Int
			ret = &constantAtoms_integer[index];
			break;
		case OP_UINTEGER:
			ret = &constantAtoms_uinteger[index];
			break;
		case OP_DOUBLE:
			ret = &constantAtoms_doubles[index];
			break;
		case OP_NAMESPACE:
			ret = &constantAtoms_namespaces[index];
			break;
		case OP_FALSE:
			ret = &asAtomHandler::falseAtom;
			break;
		case OP_TRUE:
			ret = &asAtomHandler::trueAtom;
			break;
		case OP_NULL:
			ret = &asAtomHandler::nullAtom;
			break;
		case OP_NAN:
			ret = &this->root->getSystemState()->nanAtom;
			break;
		case OP_BYTE:
			ret = &constantAtoms_byte[index];
			break;
		case OP_SHORT:
			ret = &constantAtoms_short[index];
			break;
		case OP_CACHED_CONSTANT:
			ret =&constantAtoms_cached[index];
			break;
		default:
		{
			LOG(LOG_ERROR,"Constant kind " << hex << kind);
			throw UnsupportedException("Constant trait not supported");
		}
	}
	return ret;
}

uint32_t ABCContext::addCachedConstantAtom(asAtom a)
{
	++atomsCachedMaxID;
	constantAtoms_cached[atomsCachedMaxID]=a;
	return atomsCachedMaxID;
}

void ABCContext::buildTrait(ASObject* obj,std::vector<multiname*>& additionalslots, const traits_info* t, bool isBorrowed, int scriptid, bool checkExisting)
{
	multiname* mname=getMultiname(t->name,nullptr);
	//Should be a Qname
	assert_and_throw(mname->name_type==multiname::NAME_STRING);
#ifndef NDEBUG
	if(t->kind>>4)
		LOG(LOG_CALLS,"Next slot has flags " << (t->kind>>4));
	if(t->kind&traits_info::Metadata)
	{
		for(unsigned int i=0;i<t->metadata_count;i++)
		{
			metadata_info& minfo = metadata[t->metadata[i]];
			LOG(LOG_CALLS,"Metadata: " << root->getSystemState()->getStringFromUniqueId(getString(minfo.name)));
			for(unsigned int j=0;j<minfo.item_count;++j)
				LOG(LOG_CALLS,"        : " << root->getSystemState()->getStringFromUniqueId(getString(minfo.items[j].key)) << " " << root->getSystemState()->getStringFromUniqueId(getString(minfo.items[j].value)));
		}
	}
#endif
	bool isenumerable = true;
	if(t->kind&traits_info::Metadata)
	{
		for(unsigned int i=0;i<t->metadata_count;i++)
		{
			metadata_info& minfo = metadata[t->metadata[i]];
			tiny_string name = root->getSystemState()->getStringFromUniqueId(getString(minfo.name));
			if (name == "Transient")
 				isenumerable = false;
			if (name == "SWF")
			{
				// it seems that settings from the SWF metadata tag override the entries in the swf header
				uint32_t w = UINT32_MAX;
				uint32_t h = UINT32_MAX;
				for(unsigned int j=0;j<minfo.item_count;++j)
				{
					tiny_string key =root->getSystemState()->getStringFromUniqueId(getString(minfo.items[j].key));

					if (key =="width")
						w = atoi(root->getSystemState()->getStringFromUniqueId(getString(minfo.items[j].value)).raw_buf());
					if (key =="height")
						h = atoi(root->getSystemState()->getStringFromUniqueId(getString(minfo.items[j].value)).raw_buf());
				}
				if (w != UINT32_MAX || h != UINT32_MAX)
				{
					RenderThread* rt=root->getSystemState()->getRenderThread();
					rt->requestResize(w == UINT32_MAX ? rt->windowWidth : w, h == UINT32_MAX ? rt->windowHeight : h, true);
				}
			}
		}
	}
	uint32_t kind = t->kind&0xf;
	switch(kind)
	{
		case traits_info::Class:
		{
			//Check if this already defined in upper levels
			if(obj->hasPropertyByMultiname(*mname,false,false,root->getInstanceWorker()))
				return;

			//Check if this already defined in parent applicationdomains
			ASObject* target;
			asAtom oldDefinition=asAtomHandler::invalidAtom;
			root->applicationDomain->getVariableAndTargetByMultiname(oldDefinition,*mname, target,root->getInstanceWorker());
			if(asAtomHandler::isClass(oldDefinition))
			{
				return;
			}
			
			ASObject* ret;

			QName className(mname->name_s_id,mname->ns[0].nsNameId);
			//check if this class has the 'interface' flag, i.e. it is an interface
			if((instances[t->classi].flags)&0x04)
			{

				MemoryAccount* m = obj->getSystemState()->allocateMemoryAccount(className.getQualifiedName(obj->getSystemState()));
				Class_inherit* ci=new (m) Class_inherit(className, m,t,obj->is<Global>() ? obj->as<Global>() : nullptr);
				root->customClasses.insert(make_pair(mname->name_s_id,ci));
				ci->isInterface = true;
				Function* f = Class<IFunction>::getFunction(obj->getSystemState(),Class_base::_toString);
				f->setRefConstant();
				ci->setDeclaredMethodByQName("toString",AS3,f,NORMAL_METHOD,false);
				LOG(LOG_CALLS,"Building class traits");
				for(unsigned int i=0;i<classes[t->classi].trait_count;i++)
					buildTrait(ci,additionalslots,&classes[t->classi].traits[i],false);
				//Add protected namespace if needed
				if((instances[t->classi].flags)&0x08)
				{
					ci->use_protected=true;
					int ns=instances[t->classi].protectedNs;
					const namespace_info& ns_info=constant_pool.namespaces[ns];
					ci->initializeProtectedNamespace(getString(ns_info.name),ns_info,root);
				}
				LOG(LOG_CALLS,"Adding immutable object traits to class");
				//Class objects also contains all the methods/getters/setters declared for instances
				for(unsigned int i=0;i<instances[t->classi].trait_count;i++)
				{
					int kind=instances[t->classi].traits[i].kind&0xf;
					if(kind==traits_info::Method || kind==traits_info::Setter || kind==traits_info::Getter)
						buildTrait(ci,additionalslots,&instances[t->classi].traits[i],true);
				}

				//add implemented interfaces
				for(unsigned int i=0;i<instances[t->classi].interface_count;i++)
				{
					multiname* name=getMultiname(instances[t->classi].interfaces[i],nullptr);
					ci->addImplementedInterface(*name);
				}

				ci->class_index=t->classi;
				ci->context = this;
				ci->setWorker(root->getInstanceWorker());

				//can an interface derive from an other interface?
				//can an interface derive from an non interface class?
				assert(instances[t->classi].supername == 0);
				//do interfaces have cinit methods?
				//TODO: call them, set constructor property, do something
				if(classes[t->classi].cinit != 0)
				{
					method_info* m=&methods[classes[t->classi].cinit];
					if (m->body)
						LOG(LOG_NOT_IMPLEMENTED,"Interface cinit (static):"<<className);
				}
				if(instances[t->classi].init != 0)
				{
					method_info* m=&methods[instances[t->classi].init];
					if (m->body)
						LOG(LOG_NOT_IMPLEMENTED,"Interface cinit (constructor):"<<className);
				}
				ret = ci;
				ci->setIsInitialized();
			}
			else
			{
				MemoryAccount* m = obj->getSystemState()->allocateMemoryAccount(className.getQualifiedName(obj->getSystemState()));
				Class_inherit* c=new (m) Class_inherit(className, m,t,obj->is<Global>() ? obj->as<Global>() : nullptr);
				c->context = this;
				c->setWorker(root->getInstanceWorker());
				root->customClasses.insert(make_pair(mname->name_s_id,c));

				if(instances[t->classi].supername)
				{
					// set superclass for classes that are not instantiated by newClass opcode (e.g. buttons)
					multiname mnsuper = *getMultiname(instances[t->classi].supername,nullptr);
					ASObject* superclass=root->applicationDomain->getVariableByMultinameOpportunistic(mnsuper,root->getInstanceWorker());
					if(superclass && superclass->is<Class_base>() && !superclass->is<Class_inherit>())
					{
						superclass->incRef();
						c->setSuper(_MR(superclass->as<Class_base>()));
					}
				}
				root->applicationDomain->classesBeingDefined.insert(make_pair(mname, c));
				ret=c;
				c->setIsInitialized();
				assert(mname->isStatic);
			}
			// the variable on the Definition object is set to null now (it will be set to the real value after the class init function was executed in newclass opcode)
			// testing for class==null in actionscript code is used to determine if the class initializer function has been called
			variable* v = obj->is<Global>() ? obj->setVariableAtomByQName(mname->name_s_id,mname->ns[0], asAtomHandler::nullAtom,DECLARED_TRAIT)
											: obj->setVariableByQName(mname->name_s_id,mname->ns[0], ret,DECLARED_TRAIT);

			LOG(LOG_CALLS,"Class slot "<< t->slot_id << " type Class name " << *mname << " id " << t->classi);
			if(t->slot_id)
				obj->initSlot(t->slot_id, v);
			else
				additionalslots.push_back(mname);
			break;
		}
		case traits_info::Getter:
		case traits_info::Setter:
		case traits_info::Method:
		{
			//methods can also be defined at toplevel (not only traits_info::Function!)
			if(kind == traits_info::Getter)
				LOG(LOG_CALLS,"Getter trait: " << *mname << " #" << t->method);
			else if(kind == traits_info::Setter)
				LOG(LOG_CALLS,"Setter trait: " << *mname << " #" << t->method);
			else if(kind == traits_info::Method)
				LOG(LOG_CALLS,"Method trait: " << *mname << " #" << t->method);
			method_info* m=&methods[t->method];
			SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(obj->getInstanceWorker(),m,m->numArgs());

#ifdef PROFILING_SUPPORT
			if(!m->validProfName)
			{
				m->profName=obj->getClassName()+"::"+mname->qualifiedString(obj->getSystemState());
				m->validProfName=true;
			}
#endif
			//A script can also have a getter trait
			if(obj->is<Class_inherit>())
			{
				Class_inherit* prot = obj->as<Class_inherit>();
				f->inClass = prot;
				f->isStatic = !isBorrowed;
				f->setRefConstant();


				//Methods save a copy of the scope stack of the class
				f->acquireScope(prot->class_scope);
				if(isBorrowed)
				{
					f->addToScope(scope_entry(asAtomHandler::fromObject(obj),false));
				}
			}
			else
			{
				assert(scriptid != -1);
				if (obj->getConstant())
					f->setRefConstant();
				f->addToScope(scope_entry(asAtomHandler::fromObject(obj),false));
			}
			if(kind == traits_info::Getter)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,GETTER_METHOD,isBorrowed,isenumerable);
			else if(kind == traits_info::Setter)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,SETTER_METHOD,isBorrowed,false);
			else if(kind == traits_info::Method)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,NORMAL_METHOD,isBorrowed,false);
			break;
		}
		case traits_info::Const:
		{
			//Check if this already defined in upper levels
			if(checkExisting && obj->hasPropertyByMultiname(*mname,false,false,root->getInstanceWorker()))
				return;

			multiname* tname=getMultiname(t->type_name,nullptr);
			asAtom ret=asAtomHandler::invalidAtom;
			//If the index is valid we set the constant
			if(t->vindex)
				getConstant(ret,t->vkind,t->vindex);
			else if(tname->name_type == multiname::NAME_STRING && tname->name_s_id==BUILTIN_STRINGS::ANY
					&& tname->ns.size() == 1 && tname->hasEmptyNS)
				asAtomHandler::setUndefined(ret);
			else
				asAtomHandler::setNull(ret);

			LOG_CALL("Const " << *mname <<" type "<< *tname<< " = " << asAtomHandler::toDebugString(ret));

			obj->initializeVariableByMultiname(*mname, ret, tname, this, CONSTANT_TRAIT,t->slot_id,isenumerable);
			break;
		}
		case traits_info::Slot:
		{
			//Check if this already defined in upper levels
			if(checkExisting && obj->hasPropertyByMultiname(*mname,false,false,root->getInstanceWorker()))
				return;

			multiname* tname=getMultiname(t->type_name,nullptr);
			asAtom ret=asAtomHandler::invalidAtom;
			if(t->vindex)
			{
				getConstant(ret,t->vkind,t->vindex);
				LOG_CALL("Slot " << t->slot_id << ' ' << *mname <<" type "<<*tname<< " = " << asAtomHandler::toDebugString(ret) <<" "<<isBorrowed);
			}
			else
			{
				LOG_CALL("Slot "<< t->slot_id<<  " vindex 0 " << *mname <<" type "<<*tname<<" "<<isBorrowed);
				ret = asAtomHandler::invalidAtom;
			}

			obj->initializeVariableByMultiname(*mname, ret, tname, this, isBorrowed ? INSTANCE_TRAIT : DECLARED_TRAIT,t->slot_id,isenumerable);
			if (t->slot_id == 0)
				additionalslots.push_back(mname);
			break;
		}
		default:
			LOG(LOG_ERROR,"Trait not supported " << *mname << " " << t->kind);
			obj->setVariableByMultiname(*mname, asAtomHandler::undefinedAtom, ASObject::CONST_NOT_ALLOWED,nullptr,root->getInstanceWorker());
			break;
	}
}


void method_info::getOptional(asAtom& ret, unsigned int i)
{
	assert_and_throw(i<info.options.size());
	context->getConstant(ret,info.options[i].kind,info.options[i].val);
}

multiname* method_info::paramTypeName(unsigned int i) const
{
	assert_and_throw(i<info.param_type.size());
	return context->getMultiname(info.param_type[i],nullptr);
}

multiname* method_info::returnTypeName() const
{
	return context->getMultiname(info.return_type,nullptr);
}

istream& lightspark::operator>>(istream& in, method_info& v)
{
	return in >> v.info;
}

/* Multiname types that end in 'A' are attributes names */
bool multiname_info::isAttributeName() const
{
	switch(kind)
	{
		case 0x0d: //QNameA
		case 0x10: //RTQNameA
		case 0x12: //RTQNameLA
		case 0x0e: //MultinameA
		case 0x1c: //MultinameLA
			return true;
		default:
			return false;
	}
}
