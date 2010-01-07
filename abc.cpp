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

//#define __STDC_LIMIT_MACROS
#include "abc.h"
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/LLVMContext.h>
#include <llvm/ModuleProvider.h> 
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h> 
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include "swf.h"
#include "flashevents.h"
#include "flashdisplay.h"
#include "flashnet.h"
#include "flashsystem.h"
#include "flashutils.h"
#include "flashgeom.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA ParseThread* pt;
TLSDATA Manager* iManager=NULL;
TLSDATA Manager* dManager=NULL;

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> Flags >> Name;
	LOG(LOG_CALLS,"DoABCTag Name: " << Name);

	cout << "assign vm" << endl;
	context=new ABCContext(sys->currentVm,in);

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
		abort();
	}
}

void DoABCTag::execute()
{
	LOG(LOG_CALLS,"ABC Exec " << Name);
	sys->currentVm->addEvent(NULL,new ABCContextInitEvent(context));
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, istream& in):ControlTag(h,in)
{
	LOG(LOG_TRACE,"SymbolClassTag");
	in >> NumSymbols;

	Tags.resize(NumSymbols);
	Names.resize(NumSymbols);

	for(int i=0;i<NumSymbols;i++)
		in >> Tags[i] >> Names[i];
}

void SymbolClassTag::execute()
{
	LOG(LOG_TRACE,"SymbolClassTag Exec");

	for(int i=0;i<NumSymbols;i++)
	{
		LOG(LOG_CALLS,Tags[i] << ' ' << Names[i]);
		if(Tags[i]==0)
		{
			sys->currentVm->addEvent(NULL,new BindClassEvent(sys,Names[i]));
		}
		else
		{
			DictionaryTag* t=pt->root->dictionaryLookup(Tags[i]);
			IInterface* base=dynamic_cast<IInterface*>(t);
			assert(base!=NULL);
			sys->currentVm->addEvent(NULL,new BindClassEvent(base,Names[i]));
		}
	}
}

ASFUNCTIONBODY(ABCVm,print)
{
	cerr << args->at(0)->toString() << endl;
	return new Null;
}

void ABCVm::registerClasses()
{
	//Register predefined types, ASObject are enough for not implemented classes
	Global.setVariableByQName("Object","",Class<IInterface>::getClass());
	Global.setVariableByQName("Number","",new Number(0.0));
	Global.setVariableByQName("String","",new ASString);
	Global.setVariableByQName("Array","",Class<Array>::getClass());
	Global.setVariableByQName("Function","",new Function);
	Global.setVariableByQName("undefined","",new Undefined);
	Global.setVariableByQName("Math","",new Math);
	Global.setVariableByQName("Date","",Class<Date>::getClass());
	Global.setVariableByQName("RegExp","",Class<RegExp>::getClass());
	Global.setVariableByQName("QName","",Class<ASQName>::getClass());

	Global.setVariableByQName("print","",new Function(print));
	Global.setVariableByQName("trace","",new Function(print));
	Global.setVariableByQName("parseInt","",new Function(parseInt));
	Global.setVariableByQName("toString","",new Function(ASObject::_toString));

	Global.setVariableByQName("MovieClip","flash.display",Class<MovieClip>::getClass());
	Global.setVariableByQName("DisplayObject","flash.display",Class<DisplayObject>::getClass());
	Global.setVariableByQName("Loader","flash.display",Class<Loader>::getClass());
	Global.setVariableByQName("SimpleButton","flash.display",new ASObject);
	Global.setVariableByQName("InteractiveObject","flash.display",new ASObject),
	Global.setVariableByQName("DisplayObjectContainer","flash.display",Class<DisplayObjectContainer>::getClass());
	Global.setVariableByQName("Sprite","flash.display",Class<Sprite>::getClass());
	Global.setVariableByQName("Shape","flash.display",Class<Shape>::getClass());
	Global.setVariableByQName("IBitmapDrawable","flash.display",Class<IInterface>::getClass("IBitmapDrawable"));

	Global.setVariableByQName("TextField","flash.text",Class<IInterface>::getClass("TextField"));
	Global.setVariableByQName("TextFormat","flash.text",Class<IInterface>::getClass("TextFormat"));
	Global.setVariableByQName("TextFieldType","flash.text",Class<IInterface>::getClass("TextFieldType"));

	Global.setVariableByQName("XMLDocument","flash.xml",new ASObject);

	Global.setVariableByQName("ApplicationDomain","flash.system",Class<IInterface>::getClass("ApplicationDomain"));
	Global.setVariableByQName("LoaderContext","flash.system",Class<IInterface>::getClass("LoaderContext"));

	Global.setVariableByQName("ByteArray","flash.utils",Class<ByteArray>::getClass());
	Global.setVariableByQName("Dictionary","flash.utils",Class<IInterface>::getClass("Dictionary"));
	Global.setVariableByQName("Proxy","flash.utils",Class<IInterface>::getClass("Proxy"));
	Global.setVariableByQName("Timer","flash.utils",Class<Timer>::getClass());
	Global.setVariableByQName("getQualifiedClassName","flash.utils",new Function(getQualifiedClassName));

	Global.setVariableByQName("ColorTransform","flash.geom",Class<ColorTransform>::getClass());
	Global.setVariableByQName("Rectangle","flash.geom",Class<Rectangle>::getClass());
	Global.setVariableByQName("Matrix","flash.geom",Class<IInterface>::getClass("Matrix"));
	Global.setVariableByQName("Point","flash.geom",Class<IInterface>::getClass("Point"));

	Global.setVariableByQName("EventDispatcher","flash.events",Class<EventDispatcher>::getClass());
	Global.setVariableByQName("Event","flash.events",Class<Event>::getClass());
	Global.setVariableByQName("MouseEvent","flash.events",Class<MouseEvent>::getClass());
	Global.setVariableByQName("ProgressEvent","flash.events",Class<ProgressEvent>::getClass());
	Global.setVariableByQName("TimerEvent","flash.events",Class<TimerEvent>::getClass());
	Global.setVariableByQName("IOErrorEvent","flash.events",Class<IOErrorEvent>::getClass());
	Global.setVariableByQName("SecurityErrorEvent","flash.events",Class<FakeEvent>::getClass("SecurityErrorEvent"));
	Global.setVariableByQName("TextEvent","flash.events",Class<FakeEvent>::getClass("TextEvent"));
	Global.setVariableByQName("ErrorEvent","flash.events",Class<FakeEvent>::getClass("ErrorEvent"));
	Global.setVariableByQName("IEventDispatcher","flash.events",Class<IInterface>::getClass("IEventDispatcher"));
//	Global.setVariableByQName("FocusEvent","flash.events",new FocusEvent);
//	Global.setVariableByQName("KeyboardEvent","flash.events",new KeyboardEvent);
//	Global.setVariableByQName("ProgressEvent","flash.events",new Event("ProgressEvent"));

	Global.setVariableByQName("LocalConnection","flash.net",new ASObject);
	Global.setVariableByQName("URLLoader","flash.net",Class<URLLoader>::getClass());
	Global.setVariableByQName("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getClass());
	Global.setVariableByQName("URLRequest","flash.net",Class<URLRequest>::getClass());
	Global.setVariableByQName("URLVariables","flash.net",new ASObject);

	Global.setVariableByQName("Capabilities","flash.system",new Capabilities);

	Global.setVariableByQName("ContextMenu","flash.ui",Class<IInterface>::getClass("ContextMenu"));
	Global.setVariableByQName("ContextMenuItem","flash.ui",Class<IInterface>::getClass("ContextMenuItem"));

	Global.setVariableByQName("isNaN","",new Function(isNaN));
}

/*Qname ABCContext::getQname(unsigned int mi, call_context* th) const
{
	if(mi==0)
	{
		LOG(ERROR,"Not a Qname");
		abort();
	}

	const multiname_info* m=&constant_pool.multinames[mi];
	switch(m->kind)
	{
		case 0x07:
		{
			Qname ret(getString(m->name));
			const namespace_info* n=&constant_pool.namespaces[m->ns];
			if(n->name)
				ret.ns=getString(n->name);
			ret.nskind=n->kind;

			return ret;
		}
/*		case 0x0d:
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
		case 0x0e:
			LOG(CALLS, "MultinameA");
			break;
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;*
		default:
			LOG(ERROR,"Not a Qname kind " << hex << m->kind);
			abort();
	}
}*/

int ABCContext::getMultinameRTData(int mi) const
{
	if(mi==0)
		return 0;

	const multiname_info* m=&constant_pool.multinames[mi];
	switch(m->kind)
	{
		case 0x07:
		case 0x09:
		case 0x0e:
			return 0;
		case 0x1b:
			return 1;
/*		case 0x0d:
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
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;*/
		default:
			LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
			abort();
	}
}

//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_d(call_context* th, number_t rtd, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;
		switch(m->kind)
		{
			case 0x1b:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(th->context->getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				ret->name_d=rtd;
				ret->name_type=multiname::NAME_NUMBER;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
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
				break;
		}
		return ret;
	}
}

multiname* ABCContext::s_getMultiname(call_context* th, ASObject* rt1, int n)
{
	//We are allowe to access only the ABCContext, as the stack is not synced
	multiname* ret;
	if(n==0)
	{
		ret=new multiname;
		ret->name_s="any";
		ret->name_type=multiname::NAME_STRING;
		cout << "allocating any count " << ret->count << endl;
		return ret;
	}

	multiname_info* m=&th->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;
		switch(m->kind)
		{
			case 0x07:
			{
				const namespace_info* n=&th->context->constant_pool.namespaces[m->ns];
				if(n->name)
				{
					ret->ns.push_back(th->context->getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				ret->name_s=th->context->getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x09:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(th->context->getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				ret->name_s=th->context->getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x1b:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(th->context->getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				if(rt1->getObjectType()==T_INTEGER)
				{
					Integer* o=static_cast<Integer*>(rt1);
					ret->name_i=o->val;
					ret->name_type=multiname::NAME_INT;
				}
				else if(rt1->getObjectType()==T_NUMBER)
				{
					Number* o=static_cast<Number*>(rt1);
					ret->name_d=o->val;
					ret->name_type=multiname::NAME_NUMBER;
				}
				else if(rt1->getObjectType()==T_QNAME)
				{
					ASQName* qname=static_cast<ASQName*>(rt1->implementation);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					ret->name_s=rt1->toString();
					ret->name_type=multiname::NAME_STRING;
				}
				rt1->decRef();
				break;
			}
	/*		case 0x0d:
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
			case 0x0e:
				LOG(CALLS, "MultinameA");
				break;
			case 0x1c:
				LOG(CALLS, "MultinameLA");
				break;*/
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		return ret;
	}
	else
	{
		ret=m->cached;
		switch(m->kind)
		{
			case 0x07:
			case 0x09:
			{
				//Nothing to do, the cached value is enough
				break;
			}
			case 0x1b:
			{
				if(rt1->getObjectType()==T_INTEGER)
				{
					Integer* o=static_cast<Integer*>(rt1);
					ret->name_i=o->val;
					ret->name_type=multiname::NAME_INT;
				}
				else if(rt1->getObjectType()==T_NUMBER)
				{
					Number* o=static_cast<Number*>(rt1);
					ret->name_d=o->val;
					ret->name_type=multiname::NAME_NUMBER;
				}
				else if(rt1->getObjectType()==T_QNAME)
				{
					ASQName* qname=static_cast<ASQName*>(rt1->implementation);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					ret->name_s=rt1->toString();
					ret->name_type=multiname::NAME_STRING;
				}
				rt1->decRef();
				break;
			}
	/*		case 0x0d:
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
			case 0x0e:
				LOG(CALLS, "MultinameA");
				break;
			case 0x1c:
				LOG(CALLS, "MultinameLA");
				break;*/
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		return ret;
	}
}

//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_i(call_context* th, uintptr_t rti, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;
		switch(m->kind)
		{
			case 0x1b:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(th->context->getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
				break;
			}
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
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
				break;
		}
		return ret;
	}
}

multiname* ABCContext::getMultiname(unsigned int n, call_context* th)
{
	multiname* ret;
	multiname_info* m=&constant_pool.multinames[n];

	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;

		if(n==0)
		{
			ret->name_s="any";
			ret->name_type=multiname::NAME_STRING;
			return ret;
		}
		switch(m->kind)
		{
			case 0x07:
			{
				const namespace_info* n=&constant_pool.namespaces[m->ns];
				ret->ns.push_back(getString(n->name));
				ret->nskind.push_back(n->kind);
				ret->name_s=getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				ret->name_s.len();
				break;
			}
			case 0x09:
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				//Should also sort the kinds
				sort(ret->ns.begin(),ret->ns.end());

				ret->name_s=getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				ret->name_s.len();
				break;
			}
			case 0x1b:
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				ret->nskind.reserve(s->count);
				for(int i=0;i<s->count;i++)
				{
					const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(getString(n->name));
					ret->nskind.push_back(n->kind);
				}
				//Should also sort the kinds
				sort(ret->ns.begin(),ret->ns.end());

				ASObject* n=th->runtime_stack_pop();
				ret->name_s=n->toString();
				ret->name_type=multiname::NAME_STRING;
				n->decRef();
				ret->name_s.len();
				break;
			}
	/*		case 0x0d:
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
			case 0x0e:
				LOG(CALLS, "MultinameA");
				break;
			case 0x1c:
				LOG(CALLS, "MultinameLA");
				break;*/
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		return ret;
	}
	else
	{
		ret=m->cached;
		if(n==0)
			return ret;
		switch(m->kind)
		{
			case 0x07:
			case 0x09:
			{
				//Nothing to do, the cached value is enough
				break;
			}
			case 0x1b:
			{
				ASObject* n=th->runtime_stack_pop();
				ret->name_s=n->toString();
				ret->name_type=multiname::NAME_STRING;
				n->decRef();
				break;
			}
	/*		case 0x0d:
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
			case 0x0e:
				LOG(CALLS, "MultinameA");
				break;
			case 0x1c:
				LOG(CALLS, "MultinameLA");
				break;*/
			default:
				LOG(LOG_ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		ret->name_s.len();
		return ret;
	}
}

ABCContext::ABCContext(ABCVm* v,istream& in):vm(v),Global(&v->Global)
{
	in >> minor >> major;
	LOG(LOG_CALLS,"ABCVm version " << major << '.' << minor);
	in >> constant_pool;

	in >> method_count;
	methods.resize(method_count);
	for(int i=0;i<method_count;i++)
	{
		in >> methods[i];
		methods[i].context=this;
	}

	in >> metadata_count;
	metadata.resize(metadata_count);
	for(int i=0;i<metadata_count;i++)
		in >> metadata[i];

	in >> class_count;
	instances.resize(class_count);
	for(int i=0;i<class_count;i++)
	{
		in >> instances[i];
		cout << "Class " << *getMultiname(instances[i].name,NULL) << endl;
		if(instances[i].supername)
			cout << "Super " << *getMultiname(instances[i].supername,NULL) << endl;
		if(instances[i].interface_count)
		{
			cout << "Implements" << endl; 
			for(int j=0;j<instances[i].interfaces.size();j++)
			{
				cout << "\t" << *getMultiname(instances[i].interfaces[j],NULL) << endl;
			}
		}
	}
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
			LOG(LOG_ERROR,"Duplicate body assigment")
		else
			methods[method_body[i].method].body=&method_body[i];
	}
}

ABCVm::ABCVm(SystemState* s):shutdown(false),m_sys(s)
{
	sem_init(&event_queue_mutex,0,1);
	sem_init(&sem_event_count,0,0);
	m_sys=s;
	int_manager=new Manager;
	number_manager=new Manager;
	pthread_create(&t,NULL,(thread_worker)Run,this);
}

ABCVm::~ABCVm()
{
	pthread_cancel(t);
	pthread_join(t,NULL);
	//delete ex;
	//delete module;
}

void ABCVm::wait()
{
	pthread_join(t,NULL);
}

void ABCVm::handleEvent()
{
	sem_wait(&event_queue_mutex);
	pair<EventDispatcher*,Event*> e=events_queue.front();
	events_queue.pop_front();
	sem_post(&event_queue_mutex);
	if(e.first)
		e.first->handleEvent(e.second);
	else
	{
		//Should be handled by the Vm itself
		switch(e.second->getEventType())
		{
			case BIND_CLASS:
			{
				BindClassEvent* ev=static_cast<BindClassEvent*>(e.second);
				arguments args(0);
/*				args.incRef();
				//TODO: check
				args.set(0,new Null);*/
				bool construct_instance=false;
				if(ev->base==sys)
				{
					MovieClip* m=static_cast<MovieClip*>(ev->base);
					m->initialize();
					construct_instance=true;
				}
				LOG(LOG_CALLS,"Binding of " << ev->class_name);
				last_context->buildClassAndInjectBase(ev->class_name,ev->base,&args,construct_instance);
				LOG(LOG_CALLS,"End of binding of " << ev->class_name);
				break;
			}
			case SHUTDOWN:
				shutdown=true;
				break;
			case SYNC:
			{
				SynchronizationEvent* ev=static_cast<SynchronizationEvent*>(e.second);
				ev->sync();
				break;
			}
			/*case FUNCTION:
			{
				FunctionEvent* ev=static_cast<FunctionEvent*>(e.second);
				//We hope the method is binded
				ev->f->call(NULL,NULL);
				break;
			}*/
			case CONTEXT_INIT:
			{
				ABCContextInitEvent* ev=static_cast<ABCContextInitEvent*>(e.second);
				last_context=ev->context;
				last_context->exec();
				break;
			}
			case CONSTRUCT_OBJECT:
			{
				ConstructObjectEvent* ev=static_cast<ConstructObjectEvent*>(e.second);
				LOG(LOG_CALLS,"Building instance traits");
				last_context->buildClassTraits(ev->obj, ev->_class->class_index);

				LOG(LOG_CALLS,"Calling Instance init");
				ev->_class->constructor->call(ev->obj,NULL,ev->obj->max_level);
				ev->sync();
				break;
			}
			default:
				LOG(LOG_ERROR,"Not supported event");
				abort();
		}
	}
}

void ABCVm::addEvent(EventDispatcher* obj ,Event* ev)
{
	sem_wait(&event_queue_mutex);
	events_queue.push_back(pair<EventDispatcher*,Event*>(obj, ev));
	sem_post(&sem_event_count);
	sem_post(&event_queue_mutex);
}

void ABCContext::buildClassAndInjectBase(const string& s, IInterface* base,arguments* args, bool construct_instance)
{
	LOG(LOG_CALLS,"Setting class name to " << s);
	ASObject* owner;
	ASObject* derived_class=Global->getVariableByString(s,owner);
	if(!owner)
	{
		LOG(LOG_ERROR,"Class " << s << " not found in global");
		abort();
	}
	if(derived_class->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,"Class " << s << " is not yet valid");
		Definable* d=static_cast<Definable*>(derived_class);
		d->define(Global);
		LOG(LOG_CALLS,"End of deferred init of class " << s);
		derived_class=Global->getVariableByString(s,owner);
		if(!owner)
		{
			LOG(LOG_ERROR,"Class " << s << " not found in global");
			abort();
		}
	}

	assert(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	Class_base* derived_class_tmp=static_cast<Class_base*>(derived_class);
	assert(derived_class_tmp->class_index!=-1);

	assert(base->obj==NULL);
/*	//It's now possible to actually build an instance
	ASObject* obj=derived_class_tmp->getInstance()->obj;
	//Acquire the base interface in the object
	obj->acquireInterface(base);
	__asm__("int $3");*/
	base->obj=derived_class_tmp;

	if(construct_instance)
	{
		assert(derived_class_tmp->class_index>0);

		ASObject* tmp=new ASObject;
		base->obj=tmp;
		tmp->max_level=derived_class_tmp->max_level;
		tmp->prototype=derived_class_tmp;
		tmp->actualPrototype=derived_class_tmp;
		tmp->implementation=base;

		LOG(LOG_CALLS,"Building instance traits");
		buildClassTraits(base->obj, derived_class_tmp->class_index);

		LOG(LOG_CALLS,"Calling Instance init on " << s);
		//args->incRef();
		derived_class_tmp->constructor->call(base->obj,args,base->obj->max_level);
	}
}

inline method_info* ABCContext::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(LOG_ERROR,"Requested invalid method");
		return NULL;
	}
}

//We follow the Boolean() algorithm, but return a concrete result, not a Boolean object
bool lightspark::Boolean_concrete(ASObject* obj)
{
	if(obj->getObjectType()==T_STRING)
	{
		LOG(LOG_CALLS,"String to bool");
		tiny_string s=obj->toString();
		if(s.len()==0)
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_BOOLEAN)
	{
		LOG(LOG_CALLS,"Boolean to bool");
		Boolean* b=static_cast<Boolean*>(obj);
		return b->val;
	}
	else if(obj->getObjectType()==T_OBJECT)
	{
		LOG(LOG_CALLS,"Object to bool");
		return true;
	}
	else if(obj->getObjectType()==T_UNDEFINED)
	{
		LOG(LOG_CALLS,"Undefined to bool");
		return false;
	}
	else
		return false;
}

void ABCVm::deleteProperty(call_context* th, int n)
{
	LOG(LOG_NOT_IMPLEMENTED,"deleteProperty " << n);
}

void ABCVm::call(call_context* th, int m)
{
	LOG(LOG_CALLS,"call " << m);
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	ASObject* obj=th->runtime_stack_pop();
	IFunction* f=th->runtime_stack_pop()->toFunction();

	if(f==NULL)
	{
		LOG(LOG_ERROR,"Not a function");
		abort();
	}

	//This seems wrong
	ASObject* ret=f->call(obj,&args,obj->max_level);
	th->runtime_stack_push(ret);
	obj->decRef();
	f->decRef();
}

void ABCVm::coerce(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,NULL); 
	LOG(LOG_NOT_IMPLEMENTED,"coerce " << *name);
}

ASObject* ABCVm::newCatch(call_context* th, int n)
{
	LOG(LOG_NOT_IMPLEMENTED,"newCatch " << n);
	return new Undefined;
}

ASObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG(LOG_CALLS,"newFunction " << n);

	method_info* m=&th->context->methods[n];
	SyntheticFunction* f=new SyntheticFunction(m);
	f->func_scope=th->scope_stack;
	return f;
}

void ABCVm::not_impl(int n)
{
	LOG(LOG_NOT_IMPLEMENTED, "not implement opcode 0x" << hex << n );
	abort();
}

ASObject* ABCVm::strictEquals(ASObject* obj1, ASObject* obj2)
{
	LOG(LOG_NOT_IMPLEMENTED, "strictEquals" );
	abort();
	return NULL;
}

void ABCVm::newArray(call_context* th, int n)
{
	LOG(LOG_CALLS, "newArray " << n );
	Array* ret=Class<Array>::getInstanceS();
	ret->resize(n);
	for(int i=0;i<n;i++)
		ret->set(n-i-1,th->runtime_stack_pop());

	ret->_constructor(ret->obj,NULL);
	th->runtime_stack_push(ret->obj);
}

ASObject* ABCVm::getScopeObject(call_context* th, int n)
{
	ASObject* ret=th->scope_stack[n];
	ret->incRef();
	LOG(LOG_CALLS, "getScopeObject: " << ret );
	return ret;
}

ASObject* ABCVm::pushString(call_context* th, int n)
{
	tiny_string s=th->context->getString(n); 
	LOG(LOG_CALLS, "pushString " << s );
	return new ASString(s);
}

void call_context::runtime_stack_push(ASObject* s)
{
	stack[stack_index++]=s;
}

ASObject* call_context::runtime_stack_pop()
{
	if(stack_index==0)
	{
		LOG(LOG_ERROR,"Empty stack");
		abort();
	}
	ASObject* ret=stack[--stack_index];
	return ret;
}

ASObject* call_context::runtime_stack_peek()
{
	if(stack_index==0)
	{
		LOG(LOG_ERROR,"Empty stack");
		return NULL;
	}
	return stack[stack_index-1];
}

call_context::call_context(method_info* th, int level, ASObject** args, int num_args):cur_level_of_this(level)
{
	locals=new ASObject*[th->body->local_count+1];
	locals_size=th->body->local_count;
	memset(locals,0,sizeof(ASObject*)*locals_size);
	if(args)
		memcpy(locals+1,args,num_args*sizeof(ASObject*));
	//TODO: We add a 3x safety margin because not implemented instruction do not clean the stack as they should
	stack=new ASObject*[th->body->max_stack*3];
	stack_index=0;
	context=th->context;
}

call_context::~call_context()
{
	if(stack_index!=0)
		LOG(LOG_NOT_IMPLEMENTED,"Should clean stack of " << stack_index);

	for(int i=0;i<locals_size;i++)
	{
		if(locals[i])
			locals[i]->decRef();
	}
	delete[] locals;
	delete[] stack;

	for(int i=0;i<scope_stack.size();i++)
		scope_stack[i]->decRef();
}

void ABCContext::exec()
{
	//Take script entries and declare their traits
	int i=0;
	for(i;i<scripts.size()-1;i++)
	{
		LOG(LOG_CALLS, "Script N: " << i );
		method_info* m=get_method(scripts[i].init);

		LOG(LOG_CALLS, "Building script traits: " << scripts[i].trait_count );
		SyntheticFunction* mf=new SyntheticFunction(m);
		for(int j=0;j<scripts[i].trait_count;j++)
			buildTrait(Global,&scripts[i].traits[j],mf);
	}
	//Before the entry point we run early events
//	while(sem_trywait(&th->sem_event_count)==0)
//		th->handleEvent();
	//The last script entry has to be run
	LOG(LOG_CALLS, "Last script (Entry Point)");
	method_info* m=get_method(scripts[i].init);
	IFunction* entry=new SyntheticFunction(m);
	LOG(LOG_CALLS, "Building entry script traits: " << scripts[i].trait_count );
	for(int j=0;j<scripts[i].trait_count;j++)
		buildTrait(Global,&scripts[i].traits[j]);
	entry->call(Global,NULL,Global->max_level);
	LOG(LOG_CALLS, "End of Entry Point");

}

void ABCVm::Run(ABCVm* th)
{
	sys=th->m_sys;
	iManager=th->int_manager;
	dManager=th->number_manager;
	llvm::InitializeNativeTarget();
	th->module=new llvm::Module(llvm::StringRef("abc jit"),th->llvm_context);
	llvm::ExistingModuleProvider mp(th->module);
	llvm::EngineBuilder eb(&mp);
	eb.setEngineKind(llvm::EngineKind::JIT);
	eb.setOptLevel(llvm::CodeGenOpt::Default);
	th->ex=eb.create();

	th->FPM=new llvm::FunctionPassManager(&mp);
       
	th->FPM->add(new llvm::TargetData(*th->ex->getTargetData()));
//	th->FPM->add(llvm::createVerifierPass());
	th->FPM->add(llvm::createPromoteMemoryToRegisterPass());
	th->FPM->add(llvm::createReassociatePass());
	th->FPM->add(llvm::createCFGSimplificationPass());
	th->FPM->add(llvm::createGVNPass());
	th->FPM->add(llvm::createInstructionCombiningPass());
	th->FPM->add(llvm::createLICMPass());
	th->FPM->add(llvm::createDeadStoreEliminationPass());

	th->registerFunctions();
	th->registerClasses();

	while(1)
	{
#ifndef WIN32
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
#endif
		sem_wait(&th->sem_event_count);
		th->handleEvent();
		sys->fps_prof->event_count++;
#ifndef WIN32
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->event_time+=timeDiff(ts,td);
#endif
		if(th->shutdown)
			break;
	}
	mp.releaseModule();
}

tiny_string ABCContext::getString(unsigned int s) const
{
	if(s)
		return constant_pool.strings[s];
	else
		return "";
}

inline void ABCContext::buildClassTraits(ASObject* obj, int class_index)
{
	for(int i=0;i<instances[class_index].trait_count;i++)
		buildTrait(obj,&instances[class_index].traits[i]);
}

void ABCContext::linkInterface(const multiname& interface_name, ASObject* obj)
{
	ASObject* owner;
	ASObject* interface_obj=Global->getVariableByMultiname(interface_name,owner).obj;
	assert(owner && interface_obj->getObjectType()==T_CLASS);
	Class_base* inter=static_cast<Class_base*>(interface_obj);

	//TODO: implement builtin interfaces
	if(inter->class_index==-1)
		return;

	//Recursively link interfaces implemented by this interface
	for(int i=0;i<inter->interfaces.size();i++)
		linkInterface(inter->interfaces[i],obj);

	//Link traits of this interface
	for(int j=0;j<instances[inter->class_index].trait_count;j++)
	{
		traits_info* t=&instances[inter->class_index].traits[j];
		linkTrait(obj,t);
	}

	if(inter->constructor)
	{
		LOG(LOG_CALLS,"Calling interface init for " << inter->class_name);
		inter->constructor->call(obj,NULL,obj->max_level);
	}
}

void ABCContext::linkTrait(ASObject* obj, const traits_info* t)
{
	const multiname* mname=getMultiname(t->name,NULL);
	//Should be a Qname
	assert(mname->ns.size()==1);

	const tiny_string& name=mname->name_s;
	const tiny_string& ns=mname->ns[0];
	if(t->kind>>4)
		cout << "Next slot has flags " << (t->kind>>4) << endl;
	switch(t->kind&0xf)
	{
		//Link the methods to the implementations
		case traits_info::Method:
		{
			LOG(LOG_CALLS,"Method trait: " << ns << "::" << name << " #" << t->method);
			method_info* m=&methods[t->method];
			assert(m->body==0);
			int level=obj->max_level;
			obj_var* var=NULL;
			do
			{
				var=obj->Variables.findObjVar(name,"",level,false,false);
				level--;
			}
			while(var==NULL && level>=0);
			if(var)
			{
				assert(var);
				assert(var->var);

				var->var->incRef();
				obj->setVariableByQName(name,ns,var->var);
			}

			LOG(LOG_CALLS,"End Method trait: " << ns << "::" << name);
			break;
		}
		case traits_info::Getter:
		{
			LOG(LOG_CALLS,"Getter trait: " << ns << "::" << name);
			method_info* m=&methods[t->method];
			assert(m->body==0);
			int level=obj->max_level;
			obj_var* var=NULL;
			do
			{
				var=obj->Variables.findObjVar(name,"",level,false,false);
				level--;
			}
			while((var==NULL || var->getter==NULL) && level>=0);
			if(var)
			{
				assert(var);
				assert(var->getter);

				var->getter->incRef();
				obj->setGetterByQName(name,ns,var->getter);
			}
			
			LOG(LOG_CALLS,"End Getter trait: " << ns << "::" << name);
			break;
		}
		case traits_info::Setter:
		{
			LOG(LOG_CALLS,"Setter trait: " << ns << "::" << name << " #" << t->method);
			method_info* m=&methods[t->method];
			assert(m->body==0);
			int level=obj->max_level;
			obj_var* var=NULL;
			do
			{
				var=obj->Variables.findObjVar(name,"",level,false,false);
				level--;
			}
			while((var==NULL || var->setter==NULL) && level>=0);
			if(var && var->setter)
			{
				assert(var);
				assert(var->setter);

				var->setter->incRef();
				obj->setSetterByQName(name,ns,var->setter);
			}
			
			LOG(LOG_CALLS,"End Setter trait: " << ns << "::" << name);
			break;
		}
//		case traits_info::Class:
//		case traits_info::Const:
//		case traits_info::Slot:
		default:
			LOG(LOG_ERROR,"Trait not supported " << name << " " << t->kind);
			abort();
			//obj->setVariableByQName(name, ns, new Undefined);
	}
}

ASObject* ABCContext::getConstant(int kind, int index)
{
	switch(kind)
	{
		case 0x01: //String
			return new ASString(constant_pool.strings[index]);
		case 0x03: //Int
			return new Integer(constant_pool.integer[index]);
		case 0x06: //Double
			return new Number(constant_pool.doubles[index]);
		case 0x08: //Namespace
			assert(constant_pool.namespaces[index].name);
			return Class<Namespace>::getInstanceS(false,getString(constant_pool.namespaces[index].name))->obj;
		case 0x0a: //False
			return new Boolean(false);
		case 0x0b: //True
			return new Boolean(true);
		case 0x0c: //Null
			return new Null;
		default:
		{
			LOG(LOG_ERROR,"Constant kind " << hex << kind);
			abort();
		}
	}
}

void ABCContext::buildTrait(ASObject* obj, const traits_info* t, IFunction* deferred_initialization)
{
	const multiname* mname=getMultiname(t->name,NULL);
	//Should be a Qname
	assert(mname->ns.size()==1);

	const tiny_string& name=mname->name_s;
	const tiny_string& ns=mname->ns[0];

	if(t->kind>>4)
		cout << "Next slot has flags " << (t->kind>>4) << endl;
	switch(t->kind&0xf)
	{
		case traits_info::Class:
		{
			ASObject* ret;
			if(deferred_initialization)
			{
				ret=new ScriptDefinable(deferred_initialization);
				obj->setVariableByMultiname(*mname, ret);
			}
			else
			{
				ret=new Undefined;
				obj->setVariableByMultiname(*mname, ret);
			}
			
			LOG(LOG_CALLS,"Slot "<< t->slot_id << " type Class name " << ns << "::" << name << " id " << t->classi);
			if(t->slot_id)
				obj->initSlot(t->slot_id, ret, name, ns);
			break;
		}
		case traits_info::Getter:
		{
			LOG(LOG_CALLS,"Getter trait: " << ns << "::" << name << " #" << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			IFunction* f=new SyntheticFunction(m);

			obj->setGetterByQName(name,ns,f);
			//We should inherit the setter from the hierarchy
			obj_var* var=obj->Variables.findObjVar(name,ns,obj->max_level-1,false,true);
			if(var && var->setter) //Ok, also set the inherited setter
				obj->setSetterByQName(name,ns,var->setter);
			
			LOG(LOG_CALLS,"End Getter trait: " << ns << "::" << name);
			break;
		}
		case traits_info::Setter:
		{
			LOG(LOG_CALLS,"Setter trait: " << ns << "::" << name << " #" << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];

			IFunction* f=new SyntheticFunction(m);

			obj->setSetterByQName(name,ns,f);
			//We should inherit the setter from the hierarchy
			obj_var* var=obj->Variables.findObjVar(name,ns,obj->max_level-1,false,true);
			if(var && var->getter) //Ok, also set the inherited setter
				obj->setGetterByQName(name,ns,var->getter);
			
			LOG(LOG_CALLS,"End Setter trait: " << ns << "::" << name);
			break;
		}
		case traits_info::Method:
		{
			LOG(LOG_CALLS,"Method trait: " << ns << "::" << name << " #" << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			IFunction* f=new SyntheticFunction(m);

			/*//If overriding we lookup if this name is already defined
			//If the namespace where is defined is different usually we dont't care
			//but, if it's the name of a super, then we have to take over also that
			//ns:name combination
			if(t->kind&0x20)
			{
				obj_var* var=obj->Variables.findObjVar(name,ns,
			}*/

			obj->setVariableByQName(name,ns,f);
			LOG(LOG_CALLS,"End Method trait: " << ns << "::" << name);
			break;
		}
		case traits_info::Const:
		{
			//The index has to be valid
			assert(t->vindex);

			ASObject* ret=getConstant(t->vkind,t->vindex);
			obj->setVariableByQName(name, ns, ret);
			if(t->slot_id)
				obj->initSlot(t->slot_id, ret, name, ns);

			multiname* type=getMultiname(t->type_name,NULL);
			LOG(LOG_CALLS,"Const "<<name<<" type "<<*type);
			break;
		}
		case traits_info::Slot:
		{
			if(t->vindex)
			{
				ASObject* ret=getConstant(t->vkind,t->vindex);
				obj->setVariableByQName(name, ns, ret);
				if(t->slot_id)
					obj->initSlot(t->slot_id, ret, name, ns);

				multiname* type=getMultiname(t->type_name,NULL);
				LOG(LOG_CALLS,"Slot " << t->slot_id << ' ' <<name<<" type "<<*type);
				break;
			}
			else
			{
				//else fallthrough
				multiname* type=getMultiname(t->type_name,NULL);
				LOG(LOG_CALLS,"Slot "<< t->slot_id<<  " vindex 0 "<<name<<" type "<<*type);
				ASObject* owner;
				ASObject* ret=obj->getVariableByQName(name,ns,owner).obj;
				if(!owner)
				{
					if(deferred_initialization)
					{
						ret=new ScriptDefinable(deferred_initialization);
						obj->setVariableByQName(name, ns, ret);
					}
					else
					{
						ret=new Undefined;
						obj->setVariableByQName(name, ns, ret);
					}
				}
				else
				{
					LOG(LOG_CALLS,"Not resetting variable " << name);
//					if(ret->constructor)
//						ret->constructor->call(ret,NULL);
				}
				if(t->slot_id)
					obj->initSlot(t->slot_id, ret, name,ns );
				break;
			}
		}
		default:
			LOG(LOG_ERROR,"Trait not supported " << name << " " << t->kind);
			obj->setVariableByQName(name, ns, new Undefined);
	}
}


ASObject* method_info::getOptional(int i)
{
	assert(i<options.size());
	return context->getConstant(options[i].kind,options[i].val);
}

istream& lightspark::operator>>(istream& in, u32& v)
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
				LOG(LOG_ERROR,"parsing u32");
			break;
		}
	}
	while(t2&0x80);
	return in;
}

istream& lightspark::operator>>(istream& in, s32& v)
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
				LOG(LOG_ERROR,"parsing s32");
			break;
		}
	}
	while(t2&0x80);
/*	//Sign extension usage not clear at all
	if(t2&0x40)
	{
		//Sign extend
		for(i;i<32;i++)
			v.val|=(1<<i);
	}*/
	return in;
}

istream& lightspark::operator>>(istream& in, s24& v)
{
	int i=0;
	v.val=0;
	uint8_t t;
	for(i=0;i<24;i+=8)
	{
		in.read((char*)&t,1);
		v.val|=(t<<i);
	}

	if(t&0x80)
	{
		//Sign extend
		for(i;i<32;i++)
			v.val|=(1<<i);
	}
	return in;
}

istream& lightspark::operator>>(istream& in, u30& v)
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
			LOG(LOG_ERROR,"parsing u30");
	}
	while(t2&0x80);
	if(v.val&0xc0000000)
			LOG(LOG_ERROR,"parsing u30");
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
	v.val=t;
	return in;
}

istream& lightspark::operator>>(istream& in, d64& v)
{
	//Should check if this is right
	in.read((char*)&v.val,8);
	return in;
}

istream& lightspark::operator>>(istream& in, string_info& v)
{
	in >> v.size;
	//TODO: String are expected to be UTF-8 encoded.
	//This temporary implementation assume ASCII, so fail if high bit is set
	uint8_t t;
	string tmp;
	tmp.reserve(v.size);
	for(int i=0;i<v.size;i++)
	{
		in.read((char*)&t,1);
		tmp.push_back(t);
		if(t&0x80)
			LOG(LOG_NOT_IMPLEMENTED,"Multibyte not handled");
	}
	v.val=tmp.c_str();
	return in;
}

istream& lightspark::operator>>(istream& in, namespace_info& v)
{
	in >> v.kind >> v.name;
	if(v.kind!=0x05 && v.kind!=0x08 && v.kind!=0x16 && v.kind!=0x17 && v.kind!=0x18 && v.kind!=0x19 && v.kind!=0x1a)
	{
		LOG(LOG_ERROR,"Unexpected namespace kind");
		abort();
	}
	return in;
}

istream& lightspark::operator>>(istream& in, method_body_info& v)
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

istream& lightspark::operator >>(istream& in, ns_set_info& v)
{
	in >> v.count;

	v.ns.resize(v.count);
	for(int i=0;i<v.count;i++)
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
		case 0x1d:
		{
			in >> v.type_definition;
			u8 num_params;
			in >> num_params;
			v.param_types.resize(num_params);
			for(int i=0;i<num_params;i++)
			{
				u30 t;
				in >> t;
				v.param_types[i]=t;
			}
			break;
		}
		default:
			LOG(LOG_ERROR,"Unexpected multiname kind " << hex << v.kind);
			abort();
			break;
	}
	return in;
}

istream& lightspark::operator>>(istream& in, method_info& v)
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
				LOG(LOG_ERROR,"Unexpected options type");
		}
	}
	if(v.flags&0x80)
	{
		v.param_names.resize(v.param_count);
		for(int i=0;i<v.param_count;i++)
			in >> v.param_names[i];
	}
	return in;
}

istream& lightspark::operator>>(istream& in, script_info& v)
{
	in >> v.init >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& lightspark::operator>>(istream& in, class_info& v)
{
	in >> v.cinit >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
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
	for(int i=0;i<v.item_count;i++)
	{
		in >> v.items[i].key >> v.items[i].value;
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
		for(int i=0;i<v.metadata_count;i++)
			in >> v.metadata[i];
	}
	return in;
}

istream& lightspark::operator >>(istream& in, exception_info& v)
{
	in >> v.from >> v.to >> v.target >> v.exc_type >> v.var_name;
	return in;
}

istream& lightspark::operator>>(istream& in, instance_info& v)
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

istream& lightspark::operator>>(istream& in, cpool_info& v)
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

ASObject* lightspark::parseInt(ASObject* obj,arguments* args)
{
	if(args->at(0)->getObjectType()==T_UNDEFINED)
		return new Undefined;
	else
	{
		return new Integer(atoi(args->at(0)->toString().raw_buf()));
	}
}

intptr_t ABCVm::s_toInt(ASObject* o)
{
	if(o->getObjectType()!=T_INTEGER)
		abort();
	intptr_t ret=o->toInt();
	o->decRef();
	return ret;
}

ASObject* lightspark::isNaN(ASObject* obj,arguments* args)
{
	if(args->at(0)->getObjectType()==T_UNDEFINED)
		return abstract_b(true);
	else if(args->at(0)->getObjectType()==T_INTEGER)
		return abstract_b(false);
	else if(args->at(0)->getObjectType()==T_NUMBER)
		return abstract_b(false);
	else
		abort();
}

