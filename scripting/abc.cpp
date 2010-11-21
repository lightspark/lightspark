/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/LLVMContext.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h> 
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include <limits>
#include <math.h>
#include "swf.h"
#include "flashevents.h"
#include "flashdisplay.h"
#include "flashnet.h"
#include "flashsystem.h"
#include "flashutils.h"
#include "flashgeom.h"
#include "flashexternal.h"
#include "flashmedia.h"
#include "flashxml.h"
#include "class.h"
#include "exceptions.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

TLSDATA bool isVmThread=false;

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> Flags >> Name;
	LOG(LOG_CALLS,_("DoABCTag Name: ") << Name);

	context=new ABCContext(in);

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

DoABCTag::~DoABCTag()
{
	delete context;
}

void DoABCTag::execute(RootMovieClip*)
{
	LOG(LOG_CALLS,_("ABC Exec ") << Name);
	sys->currentVm->addEvent(NULL,new ABCContextInitEvent(context));
	SynchronizationEvent* se=new SynchronizationEvent;
	bool added=sys->currentVm->addEvent(NULL,se);
	if(!added)
	{
		se->decRef();
		throw RunTimeException("Could not add event");
	}
	se->wait();
	se->decRef();
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, istream& in):ControlTag(h)
{
	LOG(LOG_TRACE,_("SymbolClassTag"));
	in >> NumSymbols;

	Tags.resize(NumSymbols);
	Names.resize(NumSymbols);

	for(int i=0;i<NumSymbols;i++)
		in >> Tags[i] >> Names[i];
}

void SymbolClassTag::execute(RootMovieClip* root)
{
	LOG(LOG_TRACE,_("SymbolClassTag Exec"));

	for(int i=0;i<NumSymbols;i++)
	{
		LOG(LOG_CALLS,_("Binding ") << Tags[i] << ' ' << Names[i]);
		if(Tags[i]==0)
		{
			//We have to bind this root movieclip itself, let's tell it.
			//This will be done later
			root->bindToName((const char*)Names[i]);
		}
		else
		{
			DictionaryTag* t=root->dictionaryLookup(Tags[i]);
			ASObject* base=dynamic_cast<ASObject*>(t);
			assert_and_throw(base!=NULL);
			BindClassEvent* e=new BindClassEvent(base,(const char*)Names[i],BindClassEvent::NONROOT);
			sys->currentVm->addEvent(NULL,e);
			e->decRef();
		}
	}
}

void ABCVm::pushObjAndLevel(ASObject* o, int l)
{
	method_this_stack.push_back(thisAndLevel(o,l));
}

thisAndLevel ABCVm::popObjAndLevel()
{
	thisAndLevel ret=method_this_stack.back();
	method_this_stack.pop_back();
	return ret;
}

void ABCVm::registerClasses()
{
	ASObject* builtin=Class<ASObject>::getInstanceS();
	//Register predefined types, ASObject are enough for not implemented classes
	builtin->setVariableByQName("Object","",Class<ASObject>::getClass());
	builtin->setVariableByQName("Class","",Class_object::getClass());
	builtin->setVariableByQName("Number","",Class<Number>::getClass());
	builtin->setVariableByQName("Boolean","",Class<Boolean>::getClass());
	builtin->setVariableByQName("NaN","",abstract_d(numeric_limits<double>::quiet_NaN()));
	builtin->setVariableByQName("Infinity","",abstract_d(numeric_limits<double>::infinity()));
	builtin->setVariableByQName("String","",Class<ASString>::getClass());
	builtin->setVariableByQName("Array","",Class<Array>::getClass());
	builtin->setVariableByQName("Function","",Class_function::getClass());
	builtin->setVariableByQName("undefined","",new Undefined);
	builtin->setVariableByQName("Math","",Class<Math>::getClass());
	builtin->setVariableByQName("Date","",Class<Date>::getClass());
	builtin->setVariableByQName("RegExp","",Class<RegExp>::getClass());
	builtin->setVariableByQName("QName","",Class<ASQName>::getClass());
	builtin->setVariableByQName("uint","",Class<UInteger>::getClass());
	builtin->setVariableByQName("Error","",Class<ASError>::getClass());
	builtin->setVariableByQName("SecurityError","",Class<SecurityError>::getClass());
	builtin->setVariableByQName("ArgumentError","",Class<ArgumentError>::getClass());
	builtin->setVariableByQName("DefinitionError","",Class<DefinitionError>::getClass());
	builtin->setVariableByQName("EvalError","",Class<EvalError>::getClass());
	builtin->setVariableByQName("RangeError","",Class<RangeError>::getClass());
	builtin->setVariableByQName("ReferenceError","",Class<ReferenceError>::getClass());
	builtin->setVariableByQName("SyntaxError","",Class<SyntaxError>::getClass());
	builtin->setVariableByQName("TypeError","",Class<TypeError>::getClass());
	builtin->setVariableByQName("URIError","",Class<URIError>::getClass());
	builtin->setVariableByQName("VerifyError","",Class<VerifyError>::getClass());
	builtin->setVariableByQName("XML","",Class<ASObject>::getClass(QName("XML","")));
	builtin->setVariableByQName("XMLList","",Class<ASObject>::getClass(QName("XMLList","")));
	builtin->setVariableByQName("int","",Class<Integer>::getClass());

	builtin->setVariableByQName("print","",Class<IFunction>::getFunction(print));
	builtin->setVariableByQName("trace","",Class<IFunction>::getFunction(trace));
	builtin->setVariableByQName("parseInt","",Class<IFunction>::getFunction(parseInt));
	builtin->setVariableByQName("parseFloat","",Class<IFunction>::getFunction(parseFloat));
	builtin->setVariableByQName("encodeURI","",Class<IFunction>::getFunction(encodeURI));
	builtin->setVariableByQName("decodeURI","",Class<IFunction>::getFunction(decodeURI));
	builtin->setVariableByQName("encodeURIComponent","",Class<IFunction>::getFunction(encodeURIComponent));
	builtin->setVariableByQName("decodeURIComponent","",Class<IFunction>::getFunction(decodeURIComponent));
	builtin->setVariableByQName("escape","",Class<IFunction>::getFunction(escape));
	builtin->setVariableByQName("unescape","",Class<IFunction>::getFunction(unescape));
	builtin->setVariableByQName("toString","",Class<IFunction>::getFunction(ASObject::_toString));

	builtin->setVariableByQName("AccessibilityProperties","flash.accessibility",
					Class<ASObject>::getClass(QName("AccessibilityProperties","flash.accessibility")));

	builtin->setVariableByQName("MovieClip","flash.display",Class<MovieClip>::getClass());
	builtin->setVariableByQName("DisplayObject","flash.display",Class<DisplayObject>::getClass());
	builtin->setVariableByQName("Loader","flash.display",Class<Loader>::getClass());
	builtin->setVariableByQName("LoaderInfo","flash.display",Class<LoaderInfo>::getClass());
	builtin->setVariableByQName("SimpleButton","flash.display",Class<ASObject>::getClass(QName("SimpleButton","flash.display")));
	builtin->setVariableByQName("InteractiveObject","flash.display",Class<InteractiveObject>::getClass());
	builtin->setVariableByQName("DisplayObjectContainer","flash.display",Class<DisplayObjectContainer>::getClass());
	builtin->setVariableByQName("Sprite","flash.display",Class<Sprite>::getClass());
	builtin->setVariableByQName("Shape","flash.display",Class<Shape>::getClass());
	builtin->setVariableByQName("Stage","flash.display",Class<Stage>::getClass());
	builtin->setVariableByQName("Graphics","flash.display",Class<Graphics>::getClass());
	builtin->setVariableByQName("LineScaleMode","flash.display",Class<LineScaleMode>::getClass());
	builtin->setVariableByQName("StageScaleMode","flash.display",Class<StageScaleMode>::getClass());
	builtin->setVariableByQName("StageAlign","flash.display",Class<StageAlign>::getClass());
	builtin->setVariableByQName("StageQuality","flash.display",Class<StageQuality>::getClass());
	builtin->setVariableByQName("IBitmapDrawable","flash.display",Class<ASObject>::getClass(QName("IBitmapDrawable","flash.display")));
	builtin->setVariableByQName("BitmapData","flash.display",Class<ASObject>::getClass(QName("BitmapData","flash.display")));
	builtin->setVariableByQName("Bitmap","flash.display",Class<Bitmap>::getClass());

	builtin->setVariableByQName("DropShadowFilter","flash.filters",Class<ASObject>::getClass(QName("DropShadowFilter","flash.filters")));
	builtin->setVariableByQName("BitmapFilter","flash.filters",Class<ASObject>::getClass(QName("BitmapFilter","flash.filters")));

	builtin->setVariableByQName("Font","flash.text",Class<Font>::getClass());
	builtin->setVariableByQName("StyleSheet","flash.text",Class<StyleSheet>::getClass());
	builtin->setVariableByQName("TextField","flash.text",Class<TextField>::getClass());
	builtin->setVariableByQName("TextFieldType","flash.text",Class<ASObject>::getClass(QName("TextFieldType","flash.text")));
	builtin->setVariableByQName("TextFormat","flash.text",Class<ASObject>::getClass(QName("TextFormat","flash.text")));

	builtin->setVariableByQName("XMLDocument","flash.xml",Class<XMLDocument>::getClass());

	builtin->setVariableByQName("ExternalInterface","flash.external",Class<ExternalInterface>::getClass());

	builtin->setVariableByQName("ByteArray","flash.utils",Class<ByteArray>::getClass());
	builtin->setVariableByQName("Dictionary","flash.utils",Class<Dictionary>::getClass());
	builtin->setVariableByQName("Proxy","flash.utils",Class<Proxy>::getClass());
	builtin->setVariableByQName("Timer","flash.utils",Class<Timer>::getClass());
	builtin->setVariableByQName("getQualifiedClassName","flash.utils",Class<IFunction>::getFunction(getQualifiedClassName));
	builtin->setVariableByQName("getQualifiedSuperclassName","flash.utils",Class<IFunction>::getFunction(getQualifiedSuperclassName));
	builtin->setVariableByQName("getDefinitionByName","flash.utils",Class<IFunction>::getFunction(getDefinitionByName));
	builtin->setVariableByQName("getTimer","flash.utils",Class<IFunction>::getFunction(getTimer));
	builtin->setVariableByQName("setInterval","flash.utils",Class<IFunction>::getFunction(setInterval));
	builtin->setVariableByQName("setTimeout","flash.utils",Class<IFunction>::getFunction(setTimeout));
	builtin->setVariableByQName("clearInterval","flash.utils",Class<IFunction>::getFunction(clearInterval));
	builtin->setVariableByQName("clearTimeout","flash.utils",Class<IFunction>::getFunction(clearTimeout));
	builtin->setVariableByQName("IExternalizable","flash.utils",Class<ASObject>::getClass(QName("IExternalizable","flash.utils")));

	builtin->setVariableByQName("ColorTransform","flash.geom",Class<ColorTransform>::getClass());
	builtin->setVariableByQName("Rectangle","flash.geom",Class<Rectangle>::getClass());
	builtin->setVariableByQName("Matrix","flash.geom",Class<Matrix>::getClass());
	builtin->setVariableByQName("Transform","flash.geom",Class<Transform>::getClass());
	builtin->setVariableByQName("Point","flash.geom",Class<Point>::getClass());

	builtin->setVariableByQName("EventDispatcher","flash.events",Class<EventDispatcher>::getClass());
	builtin->setVariableByQName("Event","flash.events",Class<Event>::getClass());
	builtin->setVariableByQName("MouseEvent","flash.events",Class<MouseEvent>::getClass());
	builtin->setVariableByQName("ProgressEvent","flash.events",Class<ProgressEvent>::getClass());
	builtin->setVariableByQName("TimerEvent","flash.events",Class<TimerEvent>::getClass());
	builtin->setVariableByQName("IOErrorEvent","flash.events",Class<IOErrorEvent>::getClass());
	builtin->setVariableByQName("ErrorEvent","flash.events",Class<ErrorEvent>::getClass());
	builtin->setVariableByQName("SecurityErrorEvent","flash.events",Class<SecurityErrorEvent>::getClass());
	builtin->setVariableByQName("AsyncErrorEvent","flash.events",Class<AsyncErrorEvent>::getClass());
	builtin->setVariableByQName("FullScreenEvent","flash.events",Class<FullScreenEvent>::getClass());
	builtin->setVariableByQName("TextEvent","flash.events",Class<TextEvent>::getClass());
	builtin->setVariableByQName("IEventDispatcher","flash.events",Class<IEventDispatcher>::getClass());
	builtin->setVariableByQName("FocusEvent","flash.events",Class<FocusEvent>::getClass());
	builtin->setVariableByQName("NetStatusEvent","flash.events",Class<NetStatusEvent>::getClass());
	builtin->setVariableByQName("HTTPStatusEvent","flash.events",Class<HTTPStatusEvent>::getClass());
	builtin->setVariableByQName("KeyboardEvent","flash.events",Class<KeyboardEvent>::getClass());

	builtin->setVariableByQName("sendToURL","flash.net",Class<IFunction>::getFunction(sendToURL));
	builtin->setVariableByQName("LocalConnection","flash.net",Class<ASObject>::getClass(QName("LocalConnection","flash.net")));
	builtin->setVariableByQName("NetConnection","flash.net",Class<NetConnection>::getClass());
	builtin->setVariableByQName("NetStream","flash.net",Class<NetStream>::getClass());
	builtin->setVariableByQName("URLLoader","flash.net",Class<URLLoader>::getClass());
	builtin->setVariableByQName("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getClass());
	builtin->setVariableByQName("URLRequest","flash.net",Class<URLRequest>::getClass());
	builtin->setVariableByQName("URLVariables","flash.net",Class<URLVariables>::getClass());
	builtin->setVariableByQName("SharedObject","flash.net",Class<SharedObject>::getClass());
	builtin->setVariableByQName("ObjectEncoding","flash.net",Class<ObjectEncoding>::getClass());

	builtin->setVariableByQName("fscommand","flash.system",Class<IFunction>::getFunction(fscommand));
	builtin->setVariableByQName("Capabilities","flash.system",Class<Capabilities>::getClass());
	builtin->setVariableByQName("Security","flash.system",Class<Security>::getClass());
	builtin->setVariableByQName("ApplicationDomain","flash.system",Class<ApplicationDomain>::getClass());
	builtin->setVariableByQName("SecurityDomain","flash.system",Class<SecurityDomain>::getClass());
	builtin->setVariableByQName("LoaderContext","flash.system",Class<ASObject>::getClass(QName("LoaderContext","flash.system")));

	builtin->setVariableByQName("SoundTransform","flash.media",Class<SoundTransform>::getClass());
	builtin->setVariableByQName("Video","flash.media",Class<Video>::getClass());
	builtin->setVariableByQName("Sound","flash.media",Class<Sound>::getClass());

	builtin->setVariableByQName("ContextMenu","flash.ui",Class<ASObject>::getClass(QName("ContextMenu","flash.ui")));
	builtin->setVariableByQName("ContextMenuItem","flash.ui",Class<ASObject>::getClass(QName("ContextMenuItem","flash.ui")));

	builtin->setVariableByQName("isNaN","",Class<IFunction>::getFunction(isNaN));
	builtin->setVariableByQName("isFinite","",Class<IFunction>::getFunction(isFinite));

	Global->registerGlobalScope(builtin);
}

//This function is used at compile time
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
		case 0x0f:
		case 0x1b:
			return 1;
/*		case 0x0d:
			LOG(CALLS, _("QNameA"));
			break;
		case 0x10:
			LOG(CALLS, _("RTQNameA"));
			break;
		case 0x11:
			LOG(CALLS, _("RTQNameL"));
			break;
		case 0x12:
			LOG(CALLS, _("RTQNameLA"));
			break;
		case 0x1c:
			LOG(CALLS, _("MultinameLA"));
			break;*/
		default:
			LOG(LOG_ERROR,_("getMultinameRTData not yet implemented for this kind ") << hex << m->kind);
			throw UnsupportedException("kind not implemented for getMultinameRTData");
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
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(th->context->getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());
				ret->name_d=rtd;
				ret->name_type=multiname::NAME_NUMBER;
				break;
			}
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
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
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
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
				assert_and_throw(n->name);
				ret->ns.push_back(nsNameAndKind(th->context->getString(n->name),(NS_KIND)(int)n->kind));

				ret->name_s=th->context->getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x09:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(th->context->getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());
				ret->name_s=th->context->getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x1b:
			{
				const ns_set_info* s=&th->context->constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(th->context->getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());
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
					ASQName* qname=static_cast<ASQName*>(rt1);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else if(rt1->getObjectType()==T_OBJECT 
						|| rt1->getObjectType()==T_CLASS
						|| rt1->getObjectType()==T_FUNCTION)
				{
					ret->name_o=rt1;
					ret->name_type=multiname::NAME_OBJECT;
					rt1->incRef();
				}
				else if(rt1->getObjectType()==T_STRING)
				{
					ASString* o=static_cast<ASString*>(rt1);
					ret->name_s=o->data;
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					
					throw UnsupportedException("Multiname to String not implemented");
					//ret->name_s=rt1->toString();
					//ret->name_type=multiname::NAME_STRING;
				}
				rt1->decRef();
				break;
			}
			case 0x0f: //RTQName
			{
				assert_and_throw(rt1->prototype==Class<Namespace>::getClass());
				Namespace* tmpns=static_cast<Namespace*>(rt1);
				//TODO: What is the right ns kind?
				ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
				ret->name_type=multiname::NAME_STRING;
				ret->name_s=th->context->getString(m->name);
				rt1->decRef();
				break;
			}
	/*		case 0x0d:
				LOG(CALLS, _("QNameA"));
				break;
			case 0x10:
				LOG(CALLS, _("RTQNameA"));
				break;
			case 0x11:
				LOG(CALLS, _("RTQNameL"));
				break;
			case 0x12:
				LOG(CALLS, _("RTQNameLA"));
				break;
			case 0x0e:
				LOG(CALLS, _("MultinameA"));
				break;
			case 0x1c:
				LOG(CALLS, _("MultinameLA"));
				break;*/
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
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
					ASQName* qname=static_cast<ASQName*>(rt1);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else if(rt1->getObjectType()==T_OBJECT 
						|| rt1->getObjectType()==T_CLASS
						|| rt1->getObjectType()==T_FUNCTION)
				{
					ret->name_o=rt1;
					ret->name_type=multiname::NAME_OBJECT;
					rt1->incRef();
				}
				else if(rt1->getObjectType()==T_STRING)
				{
					ASString* o=static_cast<ASString*>(rt1);
					ret->name_s=o->data;
					ret->name_type=multiname::NAME_STRING;
				}
				else if(rt1->getObjectType()==T_UNDEFINED ||
					rt1->getObjectType()==T_NULL)
				{
					ret->name_s="undefined";
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					throw UnsupportedException("getMultiname not completely implemented");
					//ret->name_s=rt1->toString();
					//ret->name_type=multiname::NAME_STRING;
				}
				rt1->decRef();
				break;
			}
			case 0x0f: //RTQName
			{
				//Reset the namespaces
				ret->ns.clear();

				assert_and_throw(rt1->prototype==Class<Namespace>::getClass());
				Namespace* tmpns=static_cast<Namespace*>(rt1);
				//TODO: What is the right ns kind?
				ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
				rt1->decRef();
				break;
			}
	/*		case 0x0d:
				LOG(CALLS, _("QNameA"));
				break;
			case 0x10:
				LOG(CALLS, _("RTQNameA"));
				break;
			case 0x11:
				LOG(CALLS, _("RTQNameL"));
				break;
			case 0x12:
				LOG(CALLS, _("RTQNameLA"));
				break;
			case 0x0e:
				LOG(CALLS, _("MultinameA"));
				break;
			case 0x1c:
				LOG(CALLS, _("MultinameLA"));
				break;*/
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
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
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&th->context->constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(th->context->getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());
				ret->name_i=rti;
				ret->name_type=multiname::NAME_INT;
				break;
			}
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
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
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
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
			ret->ns.emplace_back("",NAMESPACE);
			return ret;
		}
		switch(m->kind)
		{
			case 0x07:
			{
				const namespace_info* n=&constant_pool.namespaces[m->ns];
				if(n->name)
					ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				else
					ret->ns.push_back(nsNameAndKind("",(NS_KIND)(int)n->kind));

				ret->name_s=getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x09:
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());

				ret->name_s=getString(m->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			case 0x1b:
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());

				ASObject* n=th->runtime_stack_pop();
				if(n->getObjectType()==T_INTEGER)
				{
					Integer* o=static_cast<Integer*>(n);
					ret->name_i=o->val;
					ret->name_type=multiname::NAME_INT;
				}
				else if(n->getObjectType()==T_NUMBER)
				{
					Number* o=static_cast<Number*>(n);
					ret->name_d=o->val;
					ret->name_type=multiname::NAME_NUMBER;
				}
				else if(n->getObjectType()==T_QNAME)
				{
					ASQName* qname=static_cast<ASQName*>(n);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else if(n->getObjectType()==T_OBJECT ||
						n->getObjectType()==T_CLASS ||
						n->getObjectType()==T_FUNCTION)
				{
					ret->name_o=n;
					ret->name_type=multiname::NAME_OBJECT;
					n->incRef();
				}
				else if(n->getObjectType()==T_STRING)
				{
					ASString* o=static_cast<ASString*>(n);
					ret->name_s=o->data;
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					ret->name_s=n->toString();
					ret->name_type=multiname::NAME_STRING;
				}
				n->decRef();
				break;
			}
			case 0x0f: //RTQName
			{
				ASObject* n=th->runtime_stack_pop();
				assert_and_throw(n->prototype==Class<Namespace>::getClass());
				Namespace* tmpns=static_cast<Namespace*>(n);
				//TODO: What is the right ns kind?
				ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
				ret->name_type=multiname::NAME_STRING;
				ret->name_s=getString(m->name);
				n->decRef();
				break;
			}
			case 0x1d:
			{
				assert_and_throw(m->param_types.size()==1);
				multiname_info* td=&constant_pool.multinames[m->type_definition];
				//multiname_info* p=&constant_pool.multinames[m->param_types[0]];
				const namespace_info* n=&constant_pool.namespaces[td->ns];
				ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				ret->name_s=getString(td->name);
				ret->name_type=multiname::NAME_STRING;
				break;
			}
	/*		case 0x0d:
				LOG(CALLS, _("QNameA"));
				break;
			case 0x10:
				LOG(CALLS, _("RTQNameA"));
				break;
			case 0x11:
				LOG(CALLS, _("RTQNameL"));
				break;
			case 0x12:
				LOG(CALLS, _("RTQNameLA"));
				break;
			case 0x0e:
				LOG(CALLS, _("MultinameA"));
				break;
			case 0x1c:
				LOG(CALLS, _("MultinameLA"));
				break;*/
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
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
			case 0x1d: //Generics, still not implemented
			case 0x07:
			case 0x09:
			{
				//Nothing to do, the cached value is enough
				break;
			}
			case 0x1b:
			{
				ASObject* n=th->runtime_stack_pop();
				if(n->getObjectType()==T_INTEGER)
				{
					Integer* o=static_cast<Integer*>(n);
					ret->name_i=o->val;
					ret->name_type=multiname::NAME_INT;
				}
				else if(n->getObjectType()==T_NUMBER)
				{
					Number* o=static_cast<Number*>(n);
					ret->name_d=o->val;
					ret->name_type=multiname::NAME_NUMBER;
				}
				else if(n->getObjectType()==T_QNAME)
				{
					ASQName* qname=static_cast<ASQName*>(n);
					ret->name_s=qname->local_name;
					ret->name_type=multiname::NAME_STRING;
				}
				else if(n->getObjectType()==T_OBJECT 
						|| n->getObjectType()==T_CLASS
						|| n->getObjectType()==T_FUNCTION)
				{
					ret->name_o=n;
					ret->name_type=multiname::NAME_OBJECT;
					n->incRef();
				}
				else if(n->getObjectType()==T_STRING)
				{
					ASString* o=static_cast<ASString*>(n);
					ret->name_s=o->data;
					ret->name_type=multiname::NAME_STRING;
				}
				else
				{
					ret->name_s=n->toString();
					ret->name_type=multiname::NAME_STRING;
				}
				n->decRef();
				break;
			}
			case 0x0f: //RTQName
			{
				ASObject* n=th->runtime_stack_pop();
				//Reset the namespaces
				ret->ns.clear();

				assert_and_throw(n->prototype==Class<Namespace>::getClass());
				Namespace* tmpns=static_cast<Namespace*>(n);
				//TODO: What is the right kind?
				ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
				n->decRef();
				break;
			}
	/*		case 0x0d:
				LOG(CALLS, _("QNameA"));
				break;
			case 0x10:
				LOG(CALLS, _("RTQNameA"));
				break;
			case 0x11:
				LOG(CALLS, _("RTQNameL"));
				break;
			case 0x12:
				LOG(CALLS, _("RTQNameLA"));
				break;
			case 0x0e:
				LOG(CALLS, _("MultinameA"));
				break;
			case 0x1c:
				LOG(CALLS, _("MultinameLA"));
				break;*/
			default:
				LOG(LOG_ERROR,_("dMultiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
		ret->name_s.len();
		return ret;
	}
}

ABCContext::ABCContext(istream& in)
{
	in >> minor >> major;
	LOG(LOG_CALLS,_("ABCVm version ") << major << '.' << minor);
	in >> constant_pool;

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
		LOG(LOG_CALLS,_("Class ") << *getMultiname(instances[i].name,NULL));
		LOG(LOG_CALLS,_("Flags:"));
		if((instances[i].flags)&0x01)
			LOG(LOG_CALLS,_("\tSealed"));
		if((instances[i].flags)&0x02)
			LOG(LOG_CALLS,_("\tFinal"));
		if((instances[i].flags)&0x04)
			LOG(LOG_CALLS,_("\tInterface"));
		if((instances[i].flags)&0x08)
			LOG(LOG_CALLS,_("\tProtectedNS ") << getString(constant_pool.namespaces[instances[i].protectedNs].name));
		if(instances[i].supername)
			LOG(LOG_CALLS,_("Super ") << *getMultiname(instances[i].supername,NULL));
		if(instances[i].interface_count)
		{
			LOG(LOG_CALLS,_("Implements"));
			for(unsigned int j=0;j<instances[i].interfaces.size();j++)
			{
				LOG(LOG_CALLS,_("\t") << *getMultiname(instances[i].interfaces[j],NULL));
			}
		}
		LOG(LOG_CALLS,endl);
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
		if(methods[method_body[i].method].body!=NULL)
			throw ParseException("Duplicated body for function");
		else
			methods[method_body[i].method].body=&method_body[i];
	}
}

ABCVm::ABCVm(SystemState* s):m_sys(s),status(CREATED),shuttingdown(false)
{
	sem_init(&event_queue_mutex,0,1);
	sem_init(&sem_event_count,0,0);
	m_sys=s;
	int_manager=new Manager(15);
	number_manager=new Manager(15);
	Global=new GlobalObject;
	LOG(LOG_NO_INFO,_("Global is ") << Global);
	//Push a dummy default context
	pushObjAndLevel(Class<ASObject>::getInstanceS(),0);
}

void ABCVm::start()
{
	status=STARTED;
	pthread_create(&t,NULL,(thread_worker)Run,this);
}

void ABCVm::shutdown()
{
	if(status==STARTED)
	{
		//signal potentially blocking semaphores
		shuttingdown=true;
		sem_post(&sem_event_count);
		if(pthread_join(t,NULL)!=0)
		{
			LOG(LOG_ERROR,_("pthread_join in ABCVm failed"));
		}
		status=TERMINATED;
	}
}

ABCVm::~ABCVm()
{
	sem_destroy(&sem_event_count);
	sem_destroy(&event_queue_mutex);
	delete int_manager;
	delete number_manager;
	delete Global;
}

int ABCVm::getEventQueueSize()
{
	return events_queue.size();
}

void ABCVm::handleEvent(pair<EventDispatcher*,Event*> e)
{
	e.second->check();
	if(e.first)
	{
		//TODO: implement capture phase
		//Do target phase
		Event* event=e.second;
		assert_and_throw(event->target==NULL);
		event->target=e.first;
		event->currentTarget=e.first;
		e.first->handleEvent(event);
		//Do bubbling phase
		if(event->bubbles && e.first->prototype->isSubClass(Class<DisplayObject>::getClass()))
		{
			DisplayObjectContainer* cur=static_cast<DisplayObject*>(e.first)->parent;
			while(cur)
			{
				event->currentTarget=cur;
				cur->handleEvent(event);
				cur=cur->parent;
			}
		}
		//Reset events so they might be recycled
		event->currentTarget=NULL;
		event->target=NULL;
		e.first->decRef();
	}
	else
	{
		//Should be handled by the Vm itself
		switch(e.second->getEventType())
		{
			case BIND_CLASS:
			{
				BindClassEvent* ev=static_cast<BindClassEvent*>(e.second);
				LOG(LOG_CALLS,_("Binding of ") << ev->class_name);
				buildClassAndInjectBase(ev->class_name.raw_buf(),ev->base,NULL,0,ev->isRoot);
				LOG(LOG_CALLS,_("End of binding of ") << ev->class_name);
				break;
			}
			case SHUTDOWN:
				shuttingdown=true;
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
				ev->f->call(ev->obj,ev->args,ev->numArgs,ev->thisOverride);
				break;
			}
			case CONTEXT_INIT:
			{
				ABCContextInitEvent* ev=static_cast<ABCContextInitEvent*>(e.second);
				ev->context->exec();
				break;
			}
			case CONSTRUCT_OBJECT:
			{
				ConstructObjectEvent* ev=static_cast<ConstructObjectEvent*>(e.second);
				LOG(LOG_CALLS,_("Building instance traits"));
				try
				{
					ev->_class->handleConstruction(ev->_obj,NULL,0,true);
				}
				catch(LightsparkException& e)
				{
					//Sync anyway and rethrow
					ev->sync();
					throw;
				}
				ev->sync();
				break;
			}
			case CHANGE_FRAME:
			{
				FrameChangeEvent* ev=static_cast<FrameChangeEvent*>(e.second);
				ev->movieClip->state.next_FP=ev->frame;
				ev->movieClip->state.explicit_FP=true;
				break;
			}
			default:
				throw UnsupportedException("Not supported event");
		}
	}
	e.second->decRef();
}

/*! \brief enqueue an event, a reference is acquired
* * \param obj EventDispatcher that will receive the event
* * \param ev event that will be sent */
bool ABCVm::addEvent(EventDispatcher* obj ,Event* ev)
{
	//If the system should terminate new events are not accepted
	if(m_sys->shouldTerminate())
		return false;
	//If the event is a synchronization and we are running in the VM context
	//we should handle it immidiately to avoid deadlock
	if(isVmThread && (ev->getEventType()==SYNC || ev->getEventType()==CONSTRUCT_OBJECT))
	{
		assert(obj==NULL);
		ev->incRef();
		handleEvent(make_pair<EventDispatcher*>(NULL, ev));
		return true;
	}

	sem_wait(&event_queue_mutex);
	if(obj)
		obj->incRef();
	ev->incRef();
	events_queue.push_back(pair<EventDispatcher*,Event*>(obj, ev));
	sem_post(&event_queue_mutex);
	sem_post(&sem_event_count);
	return true;
}

void ABCVm::buildClassAndInjectBase(const string& s, ASObject* base, ASObject* const* args, const unsigned int argslen, bool isRoot)
{
	//It seems to be acceptable for the same base to be binded multiple times,
	//We refuse to do it, as it's an undocumented behaviour, but we warn the user
	//I've seen this behaviour only for youtube
	DictionaryTag* t=dynamic_cast<DictionaryTag*>(base);
	if(t && t->bindedTo && !isRoot)
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Multiple binding on ") << s << _(". Not binding"));
		return;
	}

	LOG(LOG_CALLS,_("Setting class name to ") << s);
	ASObject* target;
	ASObject* derived_class=Global->getVariableByString(s,target);
	if(derived_class==NULL)
	{
		LOG(LOG_ERROR,_("Class ") << s << _(" not found in global"));
		throw RunTimeException("Class not found in global");
	}

	if(derived_class->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,_("Class ") << s << _(" is not yet valid"));
		Definable* d=static_cast<Definable*>(derived_class);
		d->define(target);
		LOG(LOG_CALLS,_("End of deferred init of class ") << s);
		derived_class=Global->getVariableByString(s,target);
		assert_and_throw(derived_class);
	}

	assert_and_throw(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	assert_and_throw(static_cast<Class_base*>(derived_class)->class_index!=-1);
	Class_inherit* derived_class_tmp=static_cast<Class_inherit*>(derived_class);

	if(isRoot)
	{
		assert_and_throw(base);
		//Let's override the class
		base->setPrototype(derived_class_tmp);
		getVm()->pushObjAndLevel(base,derived_class_tmp->max_level);
		derived_class_tmp->handleConstruction(base,args,argslen,true);
		thisAndLevel tl=getVm()->popObjAndLevel();
		assert_and_throw(tl.cur_this==base);
	}
	else
	{
		//If this is not a root movie clip, then the base has to be a DictionaryTag
		assert_and_throw(t);

		t->bindedTo=derived_class_tmp;
		derived_class_tmp->bindTag(t);
	}
}

inline method_info* ABCContext::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(LOG_ERROR,_("Requested invalid method"));
		return NULL;
	}
}

void ABCVm::not_impl(int n)
{
	LOG(LOG_NOT_IMPLEMENTED, _("not implement opcode 0x") << hex << n );
	throw UnsupportedException("Not implemented opcode");
}

void call_context::runtime_stack_push(ASObject* s)
{
	if(stack_index>=mi->body->max_stack)
		throw RunTimeException("Stack overflow");
	stack[stack_index++]=s;
}

void call_context::runtime_stack_clear()
{
	while(stack_index > 0)
		stack[--stack_index]->decRef();
}

ASObject* call_context::runtime_stack_pop()
{
	if(stack_index==0)
		throw RunTimeException("Empty stack");

	ASObject* ret=stack[--stack_index];
	return ret;
}

ASObject* call_context::runtime_stack_peek()
{
	if(stack_index==0)
	{
		LOG(LOG_ERROR,_("Empty stack"));
		return NULL;
	}
	return stack[stack_index-1];
}

call_context::call_context(method_info* th, int level, ASObject* const* args, const unsigned int num_args)
{
	mi=th;
	locals=new ASObject*[th->body->local_count+1];
	locals_size=th->body->local_count+1;
	memset(locals,0,sizeof(ASObject*)*locals_size);
	if(args)
		memcpy(locals+1,args,num_args*sizeof(ASObject*));
	stack=new ASObject*[th->body->max_stack];
	stack_index=0;
	context=th->context;
}

call_context::~call_context()
{
	//assert_and_throw(stack_index==0);
	//The stack may be not clean, is this a programmer/compiler error?
	if(stack_index)
	{
		LOG(LOG_ERROR,_("Stack not clean at the end of function"));
		for(uint32_t i=0;i<stack_index;i++)
		{
			if(stack[i]) //Values might be NULL when using callproperty to call a void returning function
				stack[i]->decRef();
		}
	}

	for(int i=0;i<locals_size;i++)
	{
		if(locals[i])
			locals[i]->decRef();
	}
	delete[] locals;
	delete[] stack;

	for(unsigned int i=0;i<scope_stack.size();i++)
		scope_stack[i]->decRef();
}

bool ABCContext::isinstance(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("isinstance ") << *name);

	ASObject* target;
	ASObject* ret=getGlobal()->getVariableAndTargetByMultiname(*name, target);
	if(!ret) //Could not retrieve type
	{
		LOG(LOG_ERROR,_("Cannot retrieve type"));
		return false;
	}

	ASObject* type=ret;
	bool real_ret=false;
	Class_base* objc=NULL;
	Class_base* c=NULL;
	c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		real_ret=(c==Class<Integer>::getClass() || c==Class<Number>::getClass() || c==Class<UInteger>::getClass());
		LOG(LOG_CALLS,_("Numeric type is ") << ((real_ret)?"_(":")not _(") << ")subclass of " << c->class_name);
		return real_ret;
	}

	if(obj->prototype)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		objc=obj->prototype;
	}
	else if(obj->getObjectType()==T_CLASS)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		//Special case for Class
		if(c->class_name.name=="Class" && c->class_name.ns=="")
			return true;
		else
			return false;
	}
	else
	{
		//Special cases
		if(obj->getObjectType()==T_FUNCTION && type==Class_function::getClass())
			return true;

		real_ret=obj->getObjectType()==type->getObjectType();
		LOG(LOG_CALLS,_("isType on non classed object ") << real_ret);
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG(LOG_CALLS,_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	return real_ret;
}

void ABCContext::exec()
{
	//Take script entries and declare their traits
	unsigned int i=0;
	for(;i<scripts.size()-1;i++)
	{
		LOG(LOG_CALLS, _("Script N: ") << i );
		method_info* m=get_method(scripts[i].init);

		//Creating a new global for this script
		ASObject* global=Class<ASObject>::getInstanceS();
#ifndef NDEBUG
		global->initialized=false;
#endif
		LOG(LOG_CALLS, _("Building script traits: ") << scripts[i].trait_count );
		SyntheticFunction* mf=Class<IFunction>::getSyntheticFunction(m);

		for(unsigned int j=0;j<scripts[i].trait_count;j++)
			buildTrait(global,&scripts[i].traits[j],false,mf);

#ifndef NDEBUG
		global->initialized=true;
#endif
		//Register it as one of the global scopes
		getGlobal()->registerGlobalScope(global);
	}
	//The last script entry has to be run
	LOG(LOG_CALLS, _("Last script (Entry Point)"));
	method_info* m=get_method(scripts[i].init);
	SyntheticFunction* entry=Class<IFunction>::getSyntheticFunction(m);
	//Creating a new global for the last script
	ASObject* global=Class<ASObject>::getInstanceS();
#ifndef NDEBUG
		global->initialized=false;
#endif

	LOG(LOG_CALLS, _("Building entry script traits: ") << scripts[i].trait_count );
	for(unsigned int j=0;j<scripts[i].trait_count;j++)
		buildTrait(global,&scripts[i].traits[j],false,entry);

#ifndef NDEBUG
		global->initialized=true;
#endif
	//Register it as one of the global scopes
	getGlobal()->registerGlobalScope(global);

	ASObject* ret=entry->call(global,NULL,0);
	if(ret)
		ret->decRef();
	LOG(LOG_CALLS, _("End of Entry Point"));
}

void ABCVm::Run(ABCVm* th)
{
	//Spin wait until the VM is aknowledged by the SystemState
	sys=th->m_sys;
	while(getVm()!=th);
	isVmThread=true;
	if(th->m_sys->useJit)
	{
		llvm::InitializeNativeTarget();
		th->module=new llvm::Module(llvm::StringRef("abc jit"),th->llvm_context);
		llvm::EngineBuilder eb(th->module);
		eb.setEngineKind(llvm::EngineKind::JIT);
		eb.setOptLevel(llvm::CodeGenOpt::Default);
		th->ex=eb.create();
		assert_and_throw(th->ex);

		th->FPM=new llvm::FunctionPassManager(th->module);
	      
		th->FPM->add(new llvm::TargetData(*th->ex->getTargetData()));
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
	}
	th->registerClasses();

	ThreadProfile* profile=th->m_sys->allocateProfiler(RGB(0,200,0));
	profile->setTag("VM");
	//When aborting execution remaining events should be handled
	bool bailOut=false;
	bool firstMissingEvents=true;
	//bailout is used to keep the vm running. When bailout is true only process events until the queue is empty
	while(!bailOut || !th->events_queue.empty())
	{
		try
		{
			sem_wait(&th->sem_event_count);
			if(th->shuttingdown)
				bailOut=true;
			if(bailOut)
			{
				//If the queue is empty stop immediately
				if(th->events_queue.empty())
					break;
				//else
				//	LOG(LOG_NO_INFO,th->events_queue.size() << _(" events missing before exit"));
				else if(firstMissingEvents)
				{
					LOG(LOG_NO_INFO,th->events_queue.size() << _(" events missing before exit"));
					firstMissingEvents = false;
				}
			}
			Chronometer chronometer;
			sem_wait(&th->event_queue_mutex);
			pair<EventDispatcher*,Event*> e=th->events_queue.front();
			th->events_queue.pop_front();
			sem_post(&th->event_queue_mutex);
			th->handleEvent(e);
			if(th->shuttingdown)
				bailOut=true;
			profile->accountTime(chronometer.checkpoint());
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,_("Error in VM ") << e.cause);
			th->m_sys->setError(e.cause);
			bailOut=true;
		}
		catch(ASObject*& e)
		{
			if(e->getPrototype())
				LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM ") << e->getPrototype()->class_name);
			else
				LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM (no type)"));
			th->m_sys->setError(_("Unhandled ActionScript exception"));
			bailOut=true;
		}
	}
	if(th->m_sys->useJit)
	{
		th->ex->clearAllGlobalMappings();
		delete th->module;
	}
}

const tiny_string& ABCContext::getString(unsigned int s) const
{
	return constant_pool.strings[s];
}

void ABCContext::buildInstanceTraits(ASObject* obj, int class_index)
{
	if(class_index==-1)
		return;

	//Build only the traits that has not been build in the class
	for(unsigned int i=0;i<instances[class_index].trait_count;i++)
	{
		int kind=instances[class_index].traits[i].kind&0xf;
		if(kind==traits_info::Slot || kind==traits_info::Class ||
			kind==traits_info::Function || kind==traits_info::Const)
		{
			buildTrait(obj,&instances[class_index].traits[i],false);
		}
	}
}

void ABCContext::linkTrait(Class_base* c, const traits_info* t)
{
	const multiname& mname=*getMultiname(t->name,NULL);
	//Should be a Qname
	assert_and_throw(mname.ns.size()==1 && mname.name_type==multiname::NAME_STRING);

	const tiny_string& name=mname.name_s;
	if(t->kind>>4)
		LOG(LOG_CALLS,_("Next slot has flags ") << (t->kind>>4));
	switch(t->kind&0xf)
	{
		//Link the methods to the implementations
		case traits_info::Method:
		{
			LOG(LOG_CALLS,_("Method trait: ") << mname << _(" #") << t->method);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			obj_var* var=NULL;
			Class_base* cur=c;
			while(cur)
			{
				var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),false,true);
				if(var)
					break;
				cur=cur->super;
			}
			if(var)
			{
				assert_and_throw(var->var);

				var->var->incRef();
				c->setVariableByMultiname(mname,var->var);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Method not linkable"));
			}

			LOG(LOG_TRACE,_("End Method trait: ") << mname);
			break;
		}
		case traits_info::Getter:
		{
			LOG(LOG_CALLS,_("Getter trait: ") << mname);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			obj_var* var=NULL;
			Class_base* cur=c;
			while(cur)
			{
				var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),false,true);
				if(var && var->getter)
					break;
				cur=cur->super;
			}
			if(var)
			{
				assert_and_throw(var->getter);

				var->getter->incRef();
				c->setGetterByQName(name,mname.ns[0],var->getter,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Getter not linkable"));
			}
			
			LOG(LOG_TRACE,_("End Getter trait: ") << mname);
			break;
		}
		case traits_info::Setter:
		{
			LOG(LOG_CALLS,_("Setter trait: ") << mname << _(" #") << t->method);
			method_info* m=&methods[t->method];
			if(m->body!=NULL)
				throw ParseException("Interface trait has to be a NULL body");

			obj_var* var=NULL;
			Class_base* cur=c;
			while(cur)
			{
				var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),false,true);
				if(var && var->setter)
					break;
				cur=cur->super;
			}
			if(var)
			{
				assert_and_throw(var->setter);

				var->setter->incRef();
				c->setSetterByQName(name,mname.ns[0],var->setter,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Setter not linkable"));
			}
			
			LOG(LOG_TRACE,_("End Setter trait: ") << mname);
			break;
		}
//		case traits_info::Class:
//		case traits_info::Const:
//		case traits_info::Slot:
		default:
			LOG(LOG_ERROR,_("Trait not supported ") << mname << _(" ") << t->kind);
			throw UnsupportedException("Trait not supported");
	}
}

ASObject* ABCContext::getConstant(int kind, int index)
{
	switch(kind)
	{
		case 0x00: //Undefined
			return new Undefined;
		case 0x01: //String
			return Class<ASString>::getInstanceS(constant_pool.strings[index]);
		case 0x03: //Int
			return abstract_i(constant_pool.integer[index]);
		case 0x06: //Double
			return abstract_d(constant_pool.doubles[index]);
		case 0x08: //Namespace
			assert_and_throw(constant_pool.namespaces[index].name);
			return Class<Namespace>::getInstanceS(getString(constant_pool.namespaces[index].name));
		case 0x0a: //False
			return abstract_b(false);
		case 0x0b: //True
			return abstract_b(true);
		case 0x0c: //Null
			return new Null;
		default:
		{
			LOG(LOG_ERROR,_("Constant kind ") << hex << kind);
			throw UnsupportedException("Constant trait not supported");
		}
	}
}

void ABCContext::buildTrait(ASObject* obj, const traits_info* t, bool isBorrowed, IFunction* deferred_initialization)
{
	const multiname& mname=*getMultiname(t->name,NULL);
	//Should be a Qname
	assert_and_throw(mname.ns.size()==1 && mname.name_type==multiname::NAME_STRING);
	if(t->kind>>4)
		LOG(LOG_CALLS,_("Next slot has flags ") << (t->kind>>4));
	switch(t->kind&0xf)
	{
		case traits_info::Class:
		{
			//Check if this already defined in upper levels
			ASObject* tmpo=obj->getVariableByMultiname(mname,true);
			if(tmpo)
				return;

			ASObject* ret;
			if(deferred_initialization)
				ret=new ScriptDefinable(deferred_initialization);
			else
				ret=new Undefined;

			obj->setVariableByMultiname(mname, ret);
			
			LOG(LOG_CALLS,_("Class slot ")<< t->slot_id << _(" type Class name ") << mname << _(" id ") << t->classi);
			if(t->slot_id)
				obj->initSlot(t->slot_id, mname);
			break;
		}
		case traits_info::Getter:
		{
			LOG(LOG_CALLS,_("Getter trait: ") << mname << _(" #") << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(m);

			//We have to override if there is a method with the same name,
			//even if the namespace are different, if both are protected
			assert_and_throw(obj->getObjectType()==T_CLASS);
			Class_inherit* prot=static_cast<Class_inherit*>(obj);
			assert(prot);
			if(t->kind&0x20 && prot->use_protected && mname.ns[0]==prot->protected_ns)
			{
				//Walk the super chain and find variables to override
				Class_base* cur=prot->super;
				while(cur)
				{
					if(cur->use_protected)
					{
						obj_var* var=cur->Variables.findObjVar(mname.name_s,cur->protected_ns,false,isBorrowed);
						if(var)
						{
							assert(var->getter);
							//A superclass defined a protected method that we have to override.
							f->incRef();
							obj->setGetterByQName(mname.name_s,cur->protected_ns,f,isBorrowed);
						}
					}
					cur=cur->super;
				}
			}

			f->bindLevel(obj->getLevel());
			obj->setGetterByQName(mname.name_s,mname.ns[0],f,isBorrowed);
			
			LOG(LOG_TRACE,_("End Getter trait: ") << mname);
			break;
		}
		case traits_info::Setter:
		{
			LOG(LOG_CALLS,_("Setter trait: ") << mname << _(" #") << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];

			IFunction* f=Class<IFunction>::getSyntheticFunction(m);

			//We have to override if there is a method with the same name,
			//even if the namespace are different, if both are protected
			assert_and_throw(obj->getObjectType()==T_CLASS);
			Class_base* prot=static_cast<Class_base*>(obj);
			assert(prot);
			if(t->kind&0x20 && prot->use_protected && mname.ns[0]==prot->protected_ns)
			{
				//Walk the super chain and find variables to override
				Class_base* cur=prot->super;
				while(cur)
				{
					if(cur->use_protected)
					{
						obj_var* var=cur->Variables.findObjVar(mname.name_s,cur->protected_ns,false,isBorrowed);
						if(var)
						{
							assert(var->setter);
							//A superclass defined a protected method that we have to override.
							f->incRef();
							obj->setSetterByQName(mname.name_s,cur->protected_ns,f,isBorrowed);
						}
					}
					cur=cur->super;
				}
			}

			f->bindLevel(obj->getLevel());
			obj->setSetterByQName(mname.name_s,mname.ns[0],f,isBorrowed);
			
			LOG(LOG_TRACE,_("End Setter trait: ") << mname);
			break;
		}
		case traits_info::Method:
		{
			LOG(LOG_CALLS,_("Method trait: ") << mname << _(" #") << t->method);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(m);

			if(obj->getObjectType()==T_CLASS)
			{
				//Class method

				//We have to override if there is a method with the same name,
				//even if the namespace are different, if both are protected
				Class_inherit* prot=static_cast<Class_inherit*>(obj);
				assert(prot);
				if(t->kind&0x20 && prot->use_protected && mname.ns[0]==prot->protected_ns)
				{
					//Walk the super chain and find variables to override
					Class_base* cur=prot->super;
					while(cur)
					{
						if(cur->use_protected)
						{
							obj_var* var=cur->Variables.findObjVar(mname.name_s,cur->protected_ns,false,isBorrowed);
							if(var)
							{
								assert(var->var);
								//A superclass defined a protected method that we have to override.
								f->incRef();
								obj->setMethodByQName(mname.name_s,cur->protected_ns,f,isBorrowed);
							}
						}
						cur=cur->super;
					}
				}
				//Methods save inside the scope stack of the class
				f->acquireScope(prot->class_scope);
			}
			else if(deferred_initialization)
			{
				//Script method
				f->addToScope(obj);
			}
			else //TODO: transform in a simple assert
				assert_and_throw(obj->getObjectType()==T_CLASS || deferred_initialization);
			
			f->bindLevel(obj->getLevel());
			obj->setMethodByQName(mname.name_s,mname.ns[0],f,isBorrowed);

			LOG(LOG_TRACE,_("End Method trait: ") << mname);
			break;
		}
		case traits_info::Const:
		{
			//Check if this already defined in upper levels
			ASObject* tmpo=obj->getVariableByMultiname(mname,true);
			if(tmpo)
				return;

			ASObject* ret;
			//If the index is valid we set the constant
			if(t->vindex)
			{
				ret=getConstant(t->vkind,t->vindex);
				obj->setVariableByMultiname(mname, ret);
				if(t->slot_id)
					obj->initSlot(t->slot_id, mname);
			}
			else
			{
				ret=obj->getVariableByMultiname(mname);
				assert_and_throw(ret==NULL);
				
				if(deferred_initialization)
					ret=new ScriptDefinable(deferred_initialization);
				else
					ret=new Undefined;

				obj->setVariableByMultiname(mname, ret);
			}
			LOG(LOG_CALLS,_("Const ") << mname <<_(" type ")<< *getMultiname(t->type_name,NULL));
			if(t->slot_id)
				obj->initSlot(t->slot_id, mname);
			break;
		}
		case traits_info::Slot:
		{
			//Check if this already defined in upper levels
			ASObject* tmpo=obj->getVariableByMultiname(mname,true);
			if(tmpo)
				return;

			multiname* type=getMultiname(t->type_name,NULL);
			if(t->vindex)
			{
				ASObject* ret=getConstant(t->vkind,t->vindex);
				obj->setVariableByMultiname(mname, ret);
				if(t->slot_id)
					obj->initSlot(t->slot_id, mname);

				LOG(LOG_CALLS,_("Slot ") << t->slot_id << ' ' << mname <<_(" type ")<<*type);
				break;
			}
			else
			{
				//else fallthrough
				LOG(LOG_CALLS,_("Slot ")<< t->slot_id<<  _(" vindex 0 ") << mname <<_(" type ")<<*type);
				ASObject* previous_definition=obj->getVariableByMultiname(mname);
				assert_and_throw(!previous_definition);

				ASObject* ret;
				if(deferred_initialization)
					ret=new ScriptDefinable(deferred_initialization);
				else
				{
					//TODO: find nice way to handle default construction
					if(type->name_type==multiname::NAME_STRING && 
							type->ns.size()==1 && type->ns[0].name=="")
					{
						if(type->name_s=="int" || type->name_s=="uint" )
							ret=abstract_i(0);
						else if(type->name_s=="Number")
							ret=abstract_d(numeric_limits<double>::quiet_NaN());
						else
							ret=new Undefined;
					}
					else
						ret=new Undefined;
				}
				obj->setVariableByMultiname(mname, ret);

				if(t->slot_id)
					obj->initSlot(t->slot_id, mname);
				break;
			}
		}
		default:
			LOG(LOG_ERROR,_("Trait not supported ") << mname << _(" ") << t->kind);
			obj->setVariableByMultiname(mname, new Undefined);
	}
}


ASObject* method_info::getOptional(unsigned int i)
{
	assert_and_throw(i<options.size());
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
				LOG(LOG_ERROR,_("parsing u32 ") << (int)t);
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
			{
				LOG(LOG_ERROR,_("parsing s32"));
			}
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
		for(;i<32;i++)
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
			LOG(LOG_ERROR,_("parsing u30"));
	}
	while(t2&0x80);
	if(v.val&0xc0000000)
			LOG(LOG_ERROR,_("parsing u30"));
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
	for(unsigned int i=0;i<v.size;i++)
	{
		in.read((char*)&t,1);
		tmp.push_back(t);
		if(t&0x80)
			LOG(LOG_NOT_IMPLEMENTED,_("Multibyte not handled"));
	}
	v.val=tmp;
	return in;
}

istream& lightspark::operator>>(istream& in, namespace_info& v)
{
	in >> v.kind >> v.name;
	if(v.kind!=0x05 && v.kind!=0x08 && v.kind!=0x16 && v.kind!=0x17 && v.kind!=0x18 && v.kind!=0x19 && v.kind!=0x1a)
		throw UnsupportedException("Unexpected namespace kind");

	return in;
}

istream& lightspark::operator>>(istream& in, method_body_info& v)
{
	in >> v.method >> v.max_stack >> v.local_count >> v.init_scope_depth >> v.max_scope_depth >> v.code_length;
	v.code.resize(v.code_length);
	for(unsigned int i=0;i<v.code_length;i++)
		in.read(&v.code[i],1);

	in >> v.exception_count;
	v.exceptions.resize(v.exception_count);
	for(unsigned int i=0;i<v.exception_count;i++)
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
			LOG(LOG_ERROR,_("0 not allowed"));
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
			for(unsigned int i=0;i<num_params;i++)
			{
				u30 t;
				in >> t;
				v.param_types[i]=t;
			}
			break;
		}
		default:
			LOG(LOG_ERROR,_("Unexpected multiname kind ") << hex << v.kind);
			throw UnsupportedException("Unexpected namespace kind");
	}
	return in;
}

istream& lightspark::operator>>(istream& in, method_info& v)
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
				LOG(LOG_ERROR,_("Unexpected options type"));
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
	for(unsigned int i=0;i<v.item_count;i++)
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
			LOG(LOG_ERROR,_("Unexpected kind ") << v.kind);
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

ASFUNCTIONBODY(lightspark,undefinedFunction)
{
	LOG(LOG_NOT_IMPLEMENTED,_("Function not implemented"));
	return NULL;
}
