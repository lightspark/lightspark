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

extern __thread SystemState* sys;
extern __thread ParseThread* pt;
__thread Manager* iManager=NULL;
__thread Manager* dManager=NULL;

using namespace std;

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> Flags >> Name;
	LOG(CALLS,"DoABCTag Name: " << Name);

	cout << "assign vm" << endl;
	context=new ABCContext(sys->currentVm,in);

	if(dest!=in.tellg())
		LOG(ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
}

void DoABCTag::execute()
{
	LOG(CALLS,"ABC Exec " << Name);
	sys->currentVm->addEvent(NULL,new ABCContextInitEvent(context));
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, istream& in):ControlTag(h,in)
{
	LOG(TRACE,"SymbolClassTag");
	in >> NumSymbols;

	Tags.resize(NumSymbols);
	Names.resize(NumSymbols);

	for(int i=0;i<NumSymbols;i++)
		in >> Tags[i] >> Names[i];
}

void SymbolClassTag::execute()
{
	LOG(TRACE,"SymbolClassTag Exec");

	for(int i=0;i<NumSymbols;i++)
	{
		LOG(CALLS,Tags[i] << ' ' << Names[i]);
		if(Tags[i]==0)
			sys->currentVm->addEvent(NULL,new BindClassEvent(sys,Names[i]));
		else
		{
			DictionaryTag* t=pt->root->dictionaryLookup(Tags[i]);
			ASObject* base=dynamic_cast<ASObject*>(t);
			if(base==NULL)
			{
				LOG(ERROR,"Base in not an ASObject");
				abort();
			}
			else
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
	Global.setVariableByName("Object",new ASObject);
//	Global.setVariableByName(".int",new ASObject);
//	Global.setVariableByName(".Boolean",new ASObject);
	Global.setVariableByName("Number",new ASObject);
	Global.setVariableByName("String",new ASString);
	Global.setVariableByName("Array",new ASArray);
	Global.setVariableByName("Function",new Function);
	Global.setVariableByName("undefined",new Undefined);
	Global.setVariableByName("Math",new Math);
	Global.setVariableByName("Date",new Date);

	Global.setVariableByName("print",new Function(print));
	Global.setVariableByName("trace",new Function(print));
	Global.setVariableByName("toString",new Function(ASObject::_toString));

	Global.setVariableByName(Qname("flash.display","MovieClip"),new MovieClip);
	Global.setVariableByName(Qname("flash.display","DisplayObject"),new DisplayObject);
	Global.setVariableByName(Qname("flash.display","Loader"),new Loader);
	Global.setVariableByName(Qname("flash.display","SimpleButton"),new ASObject);
	Global.setVariableByName(Qname("flash.display","InteractiveObject"),new ASObject),
	Global.setVariableByName(Qname("flash.display","DisplayObjectContainer"),new ASObject);
	Global.setVariableByName(Qname("flash.display","Sprite"),new Sprite);

	Global.setVariableByName(Qname("flash.text","TextField"),new ASObject);
	Global.setVariableByName(Qname("flash.text","TextFormat"),new ASObject);
	Global.setVariableByName(Qname("flash.text","TextFieldType"),new ASObject);

	Global.setVariableByName(Qname("flash.xml","XMLDocument"),new ASObject);

	Global.setVariableByName(Qname("flash.system","ApplicationDomain"),new ASObject);
	Global.setVariableByName(Qname("flash.system","LoaderContext"),new ASObject);

	Global.setVariableByName(Qname("flash.utils","ByteArray"),new ASObject);
	Global.setVariableByName(Qname("flash.utils","Dictionary"),new ASObject);
	Global.setVariableByName(Qname("flash.utils","Proxy"),new ASObject);
	Global.setVariableByName(Qname("flash.utils","Timer"),new ASObject);

	Global.setVariableByName(Qname("flash.geom","Rectangle"),new ASObject);

	Global.setVariableByName(Qname("flash.events","EventDispatcher"),new ASObject);
	Global.setVariableByName(Qname("flash.events","Event"),new Event(""));
	Global.setVariableByName(Qname("flash.events","MouseEvent"),new MouseEvent);
	Global.setVariableByName(Qname("flash.events","FocusEvent"),new FocusEvent);
	Global.setVariableByName(Qname("flash.events","KeyboardEvent"),new KeyboardEvent);
	Global.setVariableByName(Qname("flash.events","ProgressEvent"),new ASObject);
	Global.setVariableByName(Qname("flash.events","IOErrorEvent"),new IOErrorEvent);

	Global.setVariableByName(Qname("flash.net","LocalConnection"),new ASObject);
	Global.setVariableByName(Qname("flash.net","URLRequest"),new URLRequest);
	Global.setVariableByName(Qname("flash.net","URLVariables"),new ASObject);

	Global.setVariableByName(Qname("flash.system","Capabilities"),new Capabilities);
}

Qname ABCContext::getQname(unsigned int mi, call_context* th) const
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
			break;*/
		default:
			LOG(ERROR,"Not a Qname kind " << hex << m->kind);
			abort();
	}
}

int ABCContext::getMultinameRTData(int mi) const
{
	if(mi==0)
		return 0;

	const multiname_info* m=&constant_pool.multinames[mi];
	switch(m->kind)
	{
		case 0x07:
		case 0x09:
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
		case 0x0e:
			LOG(CALLS, "MultinameA");
			break;
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;*/
		default:
			LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
			abort();
	}
}

multiname* ABCContext::s_getMultiname(call_context* th, ISWFObject* rt1, int n)
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
				LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
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
				LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		return ret;
	}
}

//Pre: we already know that n is not zero from getMultinameRTData
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
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
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
				LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
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
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
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
				LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
				break;
		}
		return ret;
	}
}

multiname ABCContext::getMultiname(unsigned int mi, call_context* th) const
{
	multiname ret;
	if(mi==0)
	{
		ret.name_s="any";
		ret.name_type=multiname::NAME_STRING;
		return ret;
	}

	const multiname_info* m=&constant_pool.multinames[mi];
	switch(m->kind)
	{
		case 0x07:
		{
			const namespace_info* n=&constant_pool.namespaces[m->ns];
			if(n->name)
			{
				ret.ns.push_back(getString(n->name));
				ret.nskind.push_back(n->kind);
			}
			ret.name_s=getString(m->name);
			ret.name_type=multiname::NAME_STRING;
			break;
		}
		case 0x09:
		{
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			for(int i=0;i<s->count;i++)
			{
				const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
				ret.ns.push_back(getString(n->name));
				ret.nskind.push_back(n->kind);
			}
			ret.name_s=getString(m->name);
			ret.name_type=multiname::NAME_STRING;
			break;
		}
		case 0x1b:
		{
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			for(int i=0;i<s->count;i++)
			{
				const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
				ret.ns.push_back(getString(n->name));
				ret.nskind.push_back(n->kind);
			}
			if(th!=NULL)
			{
				ISWFObject* n=th->runtime_stack_pop();
				ret.name_s=n->toString();
				ret.name_type=multiname::NAME_STRING;
				n->decRef();
			}
			else
			{
				ret.name_s="<Invalid>";
				ret.name_type=multiname::NAME_STRING;
			}
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
			LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
			break;
	}
	return ret;
}

ABCContext::ABCContext(ABCVm* v,istream& in):vm(v),Global(&v->Global)
{
	in >> minor >> major;
	LOG(CALLS,"ABCVm version " << major << '.' << minor);
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
}

ABCVm::ABCVm(SystemState* s):shutdown(false),m_sys(s)
{
	sem_init(&mutex,0,1);
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
	sem_wait(&mutex);
	pair<EventDispatcher*,Event*> e=events_queue.front();
	events_queue.pop_front();
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
				arguments args(1);;
				args.incRef();
				//TODO: check
				args.set(0,new Null);
				if(ev->base->class_name=="SystemState")
				{
					MovieClip* m=static_cast<MovieClip*>(ev->base);
					m->initialize();
				}
				ISWFObject* o=last_context->buildNamedClass(ev->class_name,ev->base,&args);
				LOG(CALLS,"End of binding of " << ev->class_name);

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
			case FUNCTION:
			{
				FunctionEvent* ev=static_cast<FunctionEvent*>(e.second);
				//We hope the method is binded
				ev->f->call(NULL,NULL);
				break;
			}
			case CONTEXT:
			{
				ABCContextInitEvent* ev=static_cast<ABCContextInitEvent*>(e.second);
				last_context=ev->context;
				last_context->exec();
				break;
			}
			default:
				LOG(ERROR,"Not supported event");
				abort();
		}
	}
	sem_post(&mutex);
}

void ABCVm::addEvent(EventDispatcher* obj ,Event* ev)
{
	sem_wait(&mutex);
	events_queue.push_back(pair<EventDispatcher*,Event*>(obj, ev));
	sem_post(&sem_event_count);
	sem_post(&mutex);
}

ISWFObject* ABCContext::buildNamedClass(const string& s, ASObject* base,arguments* args)
{
	LOG(CALLS,"Setting class name to " << s);
	base->class_name=s;
	ISWFObject* owner;
	ISWFObject* r=Global->getVariableByString(s,owner);
	if(!owner)
	{
		LOG(ERROR,"Class " << s << " not found in global");
		abort();
	}
	if(r->getObjectType()==T_DEFINABLE)
	{
		LOG(CALLS,"Class " << s << " is not yet valid");
		Definable* d=dynamic_cast<Definable*>(r);
		d->define(Global);
		LOG(CALLS,"End of deferred init");
		r=Global->getVariableByString(s,owner);
		if(!owner)
		{
			LOG(ERROR,"Class " << s << " not found in global");
			abort();
		}
	}

	ASObject* ro=dynamic_cast<ASObject*>(r);
	if(ro==NULL)
	{
		LOG(ERROR,"Class is not as ASObject");
		abort();
	}
	base->prototype=ro;
	ro->incRef();

	if(r->class_index!=-1)
	{
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<instances[r->class_index].trait_count;i++)
			buildTrait(base,&instances[r->class_index].traits[i]);

		LOG(CALLS,"Calling Instance init on " << s);
		args->incRef();
		base->prototype->constructor->call(base,args);
	}
	return base;
}

inline method_info* ABCContext::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(ERROR,"Requested invalid method");
		return NULL;
	}
}

ISWFObject* ABCVm::increment_i(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED,"increment_i");
	abort();
	return o;
}

void ABCVm::isTypelate(call_context* th)
{
	LOG(NOT_IMPLEMENTED,"isTypelate");
}

void ABCVm::asTypelate(call_context* th)
{
	LOG(NOT_IMPLEMENTED,"asTypelate");
	ISWFObject* c=th->runtime_stack_pop();
	c->decRef();
//	ISWFObject* v=th->runtime_stack_pop();
//	th->runtime_stack_push(v);
}

ISWFObject* ABCVm::nextValue(ISWFObject* index, ISWFObject* obj)
{
	LOG(NOT_IMPLEMENTED,"nextValue");
	abort();
}

ISWFObject* ABCVm::nextName(ISWFObject* index, ISWFObject* obj)
{
	LOG(CALLS,"nextName");
	Integer* i=dynamic_cast<Integer*>(index);
	if(i==NULL)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}

	ISWFObject* ret=new ASString(obj->getNameAt(*i-1));
	obj->decRef();
	index->decRef();
	return ret;
}

void ABCVm::swap(call_context* th)
{
	LOG(CALLS,"swap");
}

ISWFObject* ABCVm::newActivation(call_context* th,method_info* info)
{
	LOG(CALLS,"newActivation");
	//TODO: Should create a real activation object
	//TODO: Should method traits be added to the activation context?
	ASObject* act=new ASObject;
	for(int i=0;i<info->body->trait_count;i++)
		th->context->buildTrait(act,&info->body->traits[i]);

	return act;
}

void ABCVm::popScope(call_context* th)
{
	LOG(CALLS,"popScope");
	th->scope_stack.back()->decRef();
	th->scope_stack.pop_back();
}

void ABCVm::constructProp(call_context* th, int n, int m)
{
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	multiname name=th->context->getMultiname(n);
	LOG(CALLS,"constructProp "<<name << ' ' << m);

	ISWFObject* obj=th->runtime_stack_pop();
	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(name,owner);
	if(!owner)
	{
		LOG(ERROR,"Could not resolve property");
		abort();
	}

	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(CALLS,"Deferred definition of property " << name);
		Definable* d=dynamic_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(name,owner);
		LOG(CALLS,"End of deferred definition of property " << name);
	}

	LOG(CALLS,"Constructing");
	//We get a shallow copy of the object, but clean out Variables
	//TODO: should be done in the copy constructor
	ASObject* ret=dynamic_cast<ASObject*>(o->clone());
	if(!ret->Variables.empty())
		abort();
	ret->Variables.clear();

	ASObject* aso=dynamic_cast<ASObject*>(o);
	ret->prototype=aso;
	aso->incRef();
	if(aso==NULL)
	{
		LOG(ERROR,"Class is not as ASObject");
		abort();
	}	

	if(o->class_index==-2)
	{
		//We have to build the method traits
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(ret);
		LOG(CALLS,"Building method traits");
		for(int i=0;i<sf->mi->body->trait_count;i++)
			th->context->buildTrait(ret,&sf->mi->body->traits[i]);
		sf->call(ret,&args);

	}
	else if(o->class_index==-1)
	{
		//The class is builtin
		LOG(CALLS,"Building a builtin class");
	}
	else
	{
		//The class is declared in the script and has an index
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<th->context->instances[o->class_index].trait_count;i++)
			th->context->buildTrait(ret,&th->context->instances[o->class_index].traits[i]);
	}

	if(o->constructor)
	{
		LOG(CALLS,"Calling Instance init");
		args.incRef();
		o->constructor->call(ret,&args);
//		args.decRef();
	}

	obj->decRef();
	LOG(CALLS,"End of constructing");
	th->runtime_stack_push(ret);
}

ISWFObject* ABCVm::hasNext2(call_context* th, int n, int m)
{
	LOG(CALLS,"hasNext2 " << n << ' ' << m);
	ISWFObject* obj=th->locals[n];
	int cur_index=th->locals[m]->toInt();
	if(cur_index+1<=obj->numVariables())
	{
//		th->locals[m]->decRef();
		th->locals[m]=new Integer(cur_index+1);
		return new Boolean(true);
	}
	else
	{
//		obj->decRef();
		th->locals[n]=new Null;
//		th->locals[m]->decRef();
		th->locals[m]=new Integer(0);
		return new Boolean(false);
	}
}

void ABCVm::callSuper(call_context* th, int n, int m)
{
	multiname name=th->context->getMultiname(n); 
	LOG(NOT_IMPLEMENTED,"callSuper " << name << ' ' << m);
	th->runtime_stack_push(new Undefined);
}

void ABCVm::callSuperVoid(call_context* th, int n, int m)
{
	multiname name=th->context->getMultiname(n); 
	LOG(NOT_IMPLEMENTED,"callSuperVoid " << name << ' ' << m);
}

bool ABCVm::ifFalse(ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifFalse " << offset);

	bool ret=!Boolean_concrete(obj1);
	obj1->decRef();
	return ret;
}

//We follow the Boolean() algorithm, but return a concrete result, not a Boolean object
bool Boolean_concrete(ISWFObject* obj)
{
	if(obj->getObjectType()==T_STRING)
	{
		LOG(CALLS,"String to bool");
		string s=obj->toString();
		if(s.empty())
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_BOOLEAN)
	{
		LOG(CALLS,"Boolean to bool");
		Boolean* b=static_cast<Boolean*>(obj);
		return b->val;
	}
	else if(obj->getObjectType()==T_OBJECT)
	{
		LOG(CALLS,"Object to bool");
		return true;
	}
	else if(obj->getObjectType()==T_UNDEFINED)
	{
		LOG(CALLS,"Undefined to bool");
		return false;
	}
	else
		return false;
}

bool ABCVm::ifStrictNE(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(NOT_IMPLEMENTED,"ifStrictNE " << offset);
	abort();
}

bool ABCVm::ifNLE(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNLE " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifGE " << offset);
	abort();
}

bool ABCVm::ifNGE(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNGE " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isGreater(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifEq(ISWFObject* obj1, ISWFObject* obj2, int offset)
{
	LOG(CALLS,"ifEq " << offset);

	//Real comparision demanded to object
	bool ret=obj1->isEqual(obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGT(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifGT " << offset);

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNGT " << offset);

	//Real comparision demanded to object
	bool ret= !(obj1->isGreater(obj2));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNLT(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNLT " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

ISWFObject* ABCVm::lessThan(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(CALLS,"lessThan");

	//Real comparision demanded to object
	bool ret=obj1->isLess(obj2);
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

ISWFObject* ABCVm::greaterThan(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(CALLS,"greaterThan");

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

bool ABCVm::ifStrictEq(ISWFObject* obj1, ISWFObject* obj2, int offset)
{
	LOG(CALLS,"ifStrictEq " << offset);
	abort();

	//CHECK types

	//Real comparision demanded to object
	if(obj1->isEqual(obj2))
		return true;
	else
		return false;
}

void ABCVm::deleteProperty(call_context* th, int n)
{
	LOG(NOT_IMPLEMENTED,"deleteProperty " << n);
}

void ABCVm::call(call_context* th, int m)
{
	LOG(CALLS,"call " << m);
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	ISWFObject* obj=th->runtime_stack_pop();
	IFunction* f=th->runtime_stack_pop()->toFunction();

	if(f==NULL)
	{
		LOG(ERROR,"Not a function");
		abort();
	}

	ISWFObject* ret=f->call(obj,&args);
	th->runtime_stack_push(ret);
	obj->decRef();
	f->decRef();
}

void ABCVm::coerce(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n); 
	LOG(NOT_IMPLEMENTED,"coerce " << name);
}

ISWFObject* ABCVm::newCatch(call_context* th, int n)
{
	LOG(NOT_IMPLEMENTED,"newCatch " << n);
	return new Undefined;
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n); 

	LOG(NOT_IMPLEMENTED,"getSuper " << name);

	ISWFObject* obj=th->runtime_stack_pop();
	ASObject* o2=dynamic_cast<ASObject*>(obj);
	if(o2 && o2->super)
		obj=o2->super;

	ISWFObject* owner;
	ISWFObject* ret=obj->getVariableByMultiname(name,owner);

	if(owner)
		th->runtime_stack_push(ret);
	else
		th->runtime_stack_push(new Undefined);
}

void ABCVm::setSuper(call_context* th, int n)
{
	ISWFObject* value=th->runtime_stack_pop();
	multiname name=th->context->getMultiname(n); 

	LOG(NOT_IMPLEMENTED,"setSuper " << name);

	ISWFObject* obj=th->runtime_stack_pop();
	obj->setVariableByName((const char*)name.name_s,value);
}

ISWFObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG(CALLS,"newFunction " << n);

	method_info* m=&th->context->methods[n];
	SyntheticFunction* f=new SyntheticFunction(m);
	f->func_scope=th->scope_stack;
	return f;
}

void ABCVm::newObject(call_context* th, int n)
{
	LOG(CALLS,"newObject " << n);
	ISWFObject* ret=new ASObject;
	for(int i=0;i<n;i++)
	{
		ISWFObject* value=th->runtime_stack_pop();
		ISWFObject* name=th->runtime_stack_pop();
		ret->setVariableByName(name->toString(),value);
		name->decRef();
	}

	th->runtime_stack_push(ret);
}

void ABCVm::not_impl(int n)
{
	LOG(CALLS, "not implement opcode 0x" << hex << n );
	abort();
}

void ABCVm::_throw(call_context* th)
{
	LOG(NOT_IMPLEMENTED, "throw" );
	abort();
}

ISWFObject* ABCVm::strictEquals(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(NOT_IMPLEMENTED, "strictEquals" );
	abort();
}

ISWFObject* ABCVm::in(ISWFObject* val2, ISWFObject* val1)
{
	LOG(NOT_IMPLEMENTED, "in" );
	abort();
	return new Boolean(false);
}

ISWFObject* ABCVm::pushUndefined(call_context* th)
{
	LOG(CALLS, "pushUndefined" );
	return new Undefined;
}

ISWFObject* ABCVm::pushNull(call_context* th)
{
	LOG(CALLS, "pushNull" );
	return new Null;
}

void ABCVm::pushWith(call_context* th)
{
	ISWFObject* t=th->runtime_stack_pop();
	LOG(CALLS, "pushWith " << t );
	th->scope_stack.push_back(t);
}

void ABCVm::pushScope(call_context* th)
{
	ISWFObject* t=th->runtime_stack_pop();
	LOG(CALLS, "pushScope " << t );
	th->scope_stack.push_back(t);
}

ISWFObject* ABCVm::pushDouble(call_context* th, int n)
{
	d64 d=th->context->constant_pool.doubles[n];
	LOG(CALLS, "pushDouble [" << dec << n << "] " << d);
	return new Number(d);
}

void ABCVm::constructSuper(call_context* th, int n)
{
	LOG(CALLS, "constructSuper " << n);
	arguments args(n);
	for(int i=0;i<n;i++)
		args.set(n-i-1,th->runtime_stack_pop());

	ASObject* obj=dynamic_cast<ASObject*>(th->runtime_stack_pop());

	if(obj==NULL)
	{
		LOG(CALLS,"Not an ASObject. Aborting");
		abort();
	}

	if(obj->prototype==NULL)
	{
		LOG(CALLS,"No prototype. Returning");
		return;
	}

	//The prototype of the new super instance is the super of the prototype
	ASObject* super=obj->prototype->super;

	//Check if the super is the one we expect
	int super_name=th->context->instances[obj->prototype->class_index].supername;
	if(super_name)
	{
		Qname sname=th->context->getQname(super_name);
		ISWFObject* owner;
		ISWFObject* real_super=th->context->Global->getVariableByName(sname,owner);
		if(owner)
		{
			if(real_super==super)
			{
				LOG(CALLS,"Same super");
			}
			else
			{
				LOG(CALLS,"Changing super");
				super=dynamic_cast<ASObject*>(real_super);
			}
		}
		else
		{
			LOG(ERROR,"Super not found");
			abort();
		}
	}
	else
	{
		LOG(ERROR,"No super");
		abort();
	}

	if(super->class_index!=-1)
	{
		obj->super=new ASObject;
		obj->super->prototype=super;
		super->incRef();

		multiname name=th->context->getMultiname(th->context->instances[super->class_index].name,th);
		LOG(CALLS,"Constructing " << name);
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<th->context->instances[super->class_index].trait_count;i++)
			th->context->buildTrait(obj->super,&th->context->instances[super->class_index].traits[i]);
		LOG(CALLS,"Calling Instance init");
		//args.incRef();
		super->constructor->call(obj->super,&args);
		//args.decRef();

		LOG(CALLS,"End of constructing " << name);
	}
	else
	{
		LOG(CALLS,"Builtin super");
		obj->super=dynamic_cast<ASObject*>(super->clone());
		if(!obj->super->Variables.empty())
			abort();
		obj->super->Variables.clear();
		LOG(CALLS,"Calling Instance init");
		//args.incRef();
		if(super->constructor)
			super->constructor->call(obj->super,&args);
		//args.decRef();
	}
	LOG(CALLS,"End super construct ");
	obj->decRef();
}

void ABCVm::findProperty(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n);
	LOG(CALLS, "findProperty " << name );

	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ISWFObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByMultiname(name,owner);
		if(owner)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(owner);
			owner->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, pushing global" );
		th->runtime_stack_push(&th->context->vm->Global);
		th->context->vm->Global.incRef();
	}
}

void ABCVm::findPropStrict(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n);
	LOG(CALLS, "findPropStrict " << name );

	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ISWFObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByMultiname(name,owner);
		if(owner)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(owner);
			owner->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, trying Global" );
		//TODO: to multiname
		th->context->vm->Global.getVariableByName((const char*)name.name_s,owner);
		if(owner)
		{
			th->runtime_stack_push(owner);
			owner->incRef();
		}
		else
		{
			LOG(CALLS, "NOT found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}
}

void ABCVm::initProperty(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n);
	LOG(CALLS, "initProperty " << name );
	ISWFObject* value=th->runtime_stack_pop();

	ISWFObject* obj=th->runtime_stack_pop();

	obj->setVariableByName((const char*)name.name_s,value);
	obj->decRef();
}

void ABCVm::newArray(call_context* th, int n)
{
	LOG(CALLS, "newArray " << n );
	ASArray* ret=new ASArray;
	ret->resize(n);
	for(int i=0;i<n;i++)
		ret->set(n-i-1,th->runtime_stack_pop());

	ret->_constructor(ret,NULL);
	th->runtime_stack_push(ret);
}

void ABCVm::newClass(call_context* th, int n)
{
	LOG(CALLS, "newClass " << n );
	ASObject* ret=new ASObject;
	ret->super=dynamic_cast<ASObject*>(th->runtime_stack_pop());

	method_info* m=&th->context->methods[th->context->classes[n].cinit];
	IFunction* cinit=new SyntheticFunction(m);
	LOG(CALLS,"Building class traits");
	for(int i=0;i<th->context->classes[n].trait_count;i++)
		th->context->buildTrait(ret,&th->context->classes[n].traits[i]);

	//add Constructor the the class methods
	method_info* constructor=&th->context->methods[th->context->instances[n].init];
	ret->constructor=new SyntheticFunction(constructor);
	ret->class_index=n;

	LOG(CALLS,"Calling Class init");
	cinit->call(ret,NULL);
	th->runtime_stack_push(ret);
}

ISWFObject* ABCVm::getScopeObject(call_context* th, int n)
{
	ISWFObject* ret=th->scope_stack[n];
	ret->incRef();
	LOG(CALLS, "getScopeObject: " << ret );
	return ret;
}

void ABCVm::getLex(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n);
	LOG(CALLS, "getLex: " << name );
	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ISWFObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		ISWFObject* o=(*it)->getVariableByMultiname(name,owner);
		if(owner)
		{
			//If we are getting a function object attach the the current scope
			if(o->getObjectType()==T_FUNCTION)
			{
				LOG(CALLS,"Attaching this to function " << name);
				IFunction* f=o->toFunction();
				//f->closure_this=th->locals[0];
				f->closure_this=(*it);
				f->bind();
			}
			else if(o->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << name);
				Definable* d=dynamic_cast<Definable*>(o);
				d->define(*it);
				o=(*it)->getVariableByMultiname(name,owner);
				LOG(CALLS,"End of deferred definition of property " << name);
			}
			th->runtime_stack_push(o);
			o->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, trying Global" );
		ISWFObject* o2=th->context->vm->Global.getVariableByMultiname(name,owner);
		if(owner)
		{
			if(o2->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << name);
				Definable* d=dynamic_cast<Definable*>(o2);
				d->define(th->context->Global);
				o2=th->context->Global->getVariableByMultiname(name,owner);
				LOG(CALLS,"End of deferred definition of property " << name);
			}

			th->runtime_stack_push(o2);
			o2->incRef();
		}
		else
		{
			LOG(CALLS, "NOT found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}
}

ISWFObject* ABCVm::pushString(call_context* th, int n)
{
	string s=th->context->getString(n); 
	LOG(CALLS, "pushString " << s );
	return new ASString(s);
}

void call_context::runtime_stack_push(ISWFObject* s)
{
	stack[stack_index++]=s;
}

ISWFObject* call_context::runtime_stack_pop()
{
	if(stack_index==0)
	{
		LOG(ERROR,"Empty stack");
		abort();
	}
	ISWFObject* ret=stack[--stack_index];
	return ret;
}

ISWFObject* call_context::runtime_stack_peek()
{
	if(stack_index==0)
	{
		LOG(ERROR,"Empty stack");
		return NULL;
	}
	return stack[stack_index-1];
}

call_context::call_context(method_info* th)
{
	locals=new ISWFObject*[th->body->local_count];
	locals_size=th->body->local_count;
	memset(locals,0,sizeof(ISWFObject*)*locals_size);
	//TODO: We add a 3x safety margin because not implemented instruction do not clean the stack as they should
	stack=new ISWFObject*[th->body->max_stack*3];
	stack_index=0;
	context=th->context;
}

call_context::~call_context()
{
	if(stack_index!=0)
		LOG(NOT_IMPLEMENTED,"Should clean stack of " << stack_index);

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
		LOG(CALLS, "Script N: " << i );
		method_info* m=get_method(scripts[i].init);

		LOG(CALLS, "Building script traits: " << scripts[i].trait_count );
		for(int j=0;j<scripts[i].trait_count;j++)
			buildTrait(Global,&scripts[i].traits[j],new SyntheticFunction(m));
	}
	//Before the entry point we run early events
//	while(sem_trywait(&th->sem_event_count)==0)
//		th->handleEvent();
	//The last script entry has to be run
	LOG(CALLS, "Last script (Entry Point)");
	method_info* m=get_method(scripts[i].init);
	IFunction* entry=new SyntheticFunction(m);
	LOG(CALLS, "Building entry script traits: " << scripts[i].trait_count );
	for(int j=0;j<scripts[i].trait_count;j++)
		buildTrait(Global,&scripts[i].traits[j]);
	entry->call(Global,NULL);
	LOG(CALLS, "End of Entry Point");

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
	//th->FPM->add(llvm::createVerifierPass());
	th->FPM->add(llvm::createPromoteMemoryToRegisterPass());
	th->FPM->add(llvm::createReassociatePass());
	th->FPM->add(llvm::createCFGSimplificationPass());
	th->FPM->add(llvm::createGVNPass());
	th->FPM->add(llvm::createInstructionCombiningPass());
	th->FPM->add(llvm::createLICMPass());
	th->FPM->add(llvm::createDeadStoreEliminationPass());

	th->registerFunctions();
	th->registerClasses();
	th->Global.class_name="Global";

	while(1)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		sem_wait(&th->sem_event_count);
		th->handleEvent();
		sys->fps_prof->event_count++;
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->event_time+=timeDiff(ts,td);
		if(th->shutdown)
			break;
	}
	mp.releaseModule();
}

string ABCContext::getString(unsigned int s) const
{
	if(s)
		return constant_pool.strings[s];
	else
		return "";
}

void ABCContext::buildTrait(ISWFObject* obj, const traits_info* t, IFunction* deferred_initialization)
{
	Qname name=getQname(t->name);
	switch(t->kind&0xf)
	{
		case traits_info::Class:
		{
			ISWFObject* ret;
			if(deferred_initialization)
			{
				ret=new ScriptDefinable(deferred_initialization);
				obj->setVariableByName(name, ret);
			}
			else
			{
				ret=new Undefined;
				obj->setVariableByName(name, ret);
				/*ret=new ASObject;
				//Should chek super
				//ret->super=dynamic_cast<ASObject*>(th->runtime_stack_pop());

				method_info* m=&methods[classes[t->classi].cinit];
				IFunction* cinit=new SyntheticFunction(m);
				LOG(CALLS,"Building class traits");
				for(int i=0;i<classes[t->classi].trait_count;i++)
					buildTrait(ret,&classes[t->classi].traits[i]);

				//add Constructor the the class methods
				method_info* constructor=&methods[instances[t->classi].init];
				ret->constructor=new SyntheticFunction(constructor);
				ret->class_index=t->classi;

				LOG(CALLS,"Calling Class init");
				cinit->call(ret,NULL);
				delete cinit;
				ret=obj->setVariableByName(name, ret);*/
			}
			
			LOG(CALLS,"Slot "<< t->slot_id << " type Class name " << name << " id " << t->classi);
			if(t->slot_id)
				obj->initSlot(t->slot_id, ret, name);
			break;
		}
		case traits_info::Getter:
		{
			LOG(CALLS,"Getter trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			obj->setGetterByName(name, new SyntheticFunction(m));
			LOG(CALLS,"End Getter trait: " << name);
			break;
		}
		case traits_info::Setter:
		{
			LOG(CALLS,"Setter trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			obj->setSetterByName(name, new SyntheticFunction(m));
			LOG(CALLS,"End Setter trait: " << name);
			break;
		}
		case traits_info::Method:
		{
			LOG(CALLS,"Method trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			obj->setVariableByName(name, new SyntheticFunction(m));
			break;
		}
		case traits_info::Const:
		{
			//TODO: Not so const right now
			if(deferred_initialization)
				obj->setVariableByName(name, new ScriptDefinable(deferred_initialization));
			else
				obj->setVariableByName(name, new Undefined);
			break;
		}
		case traits_info::Slot:
		{
			if(t->vindex)
			{
				switch(t->vkind)
				{
					case 0x01: //String
					{
						ISWFObject* ret=new ASString(constant_pool.strings[t->vindex]);
						obj->setVariableByName(name, ret);
						if(t->slot_id)
							obj->initSlot(t->slot_id, ret, name);
						break;
					}
					case 0x03: //Int
					{
						ISWFObject* ret=new Integer(constant_pool.integer[t->vindex]);
						obj->setVariableByName(name, ret);
						if(t->slot_id)
							obj->initSlot(t->slot_id, ret, name);
						break;
					}
					case 0x0a: //False
					{
						ISWFObject* ret=new Boolean(false);
						obj->setVariableByName(name, ret);
						if(t->slot_id)
							obj->initSlot(t->slot_id, ret, name);
						break;
					}
					case 0x0b: //True
					{
						ISWFObject* ret=new Boolean(true);
						obj->setVariableByName(name, ret);
						if(t->slot_id)
							obj->initSlot(t->slot_id, ret, name);
						break;
					}
					case 0x0c: //Null
					{
						ISWFObject* ret=new Null;
						obj->setVariableByName(name, ret);
						if(t->slot_id)
							obj->initSlot(t->slot_id, ret, name);
						break;
					}
					default:
					{
						//fallthrough
						LOG(ERROR,"Slot kind " << hex << t->vkind);
						LOG(ERROR,"Trait not supported " << name << " " << t->kind);
						abort();
						obj->setVariableByName(name, new Undefined);
						return;
					}
				}
				multiname type=getMultiname(t->type_name);
				LOG(CALLS,"Slot "<<name<<" type "<<type);
				break;
			}
			else
			{
				//else fallthrough
				multiname type=getMultiname(t->type_name);
				LOG(CALLS,"Slot "<< t->slot_id<<  " vindex 0 "<<name<<" type "<<type);
				ISWFObject* owner;
				ISWFObject* ret=obj->getVariableByName(name,owner);
				if(!owner)
				{
					if(deferred_initialization)
					{
						ret=new ScriptDefinable(deferred_initialization);
						obj->setVariableByName(name, ret);
					}
					else
					{
						ret=new Undefined;
						obj->setVariableByName(name, ret);
					}
				}
				else
				{
					LOG(CALLS,"Not resetting variable " << name);
//					if(ret->constructor)
//						ret->constructor->call(ret,NULL);
				}
				if(t->slot_id)
					obj->initSlot(t->slot_id, ret, name);
				break;
			}
		}
		default:
			LOG(ERROR,"Trait not supported " << name << " " << t->kind);
			obj->setVariableByName(name, new Undefined);
	}
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

istream& operator>>(istream& in, s24& v)
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
	if(v.val&0xc0000000)
			LOG(ERROR,"parsing u30");
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
	{
		in >> v.traits[i];
	}
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

ISWFObject* parseInt(ISWFObject* obj,arguments* args)
{
	return new Integer(0);
}

