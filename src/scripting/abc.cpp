/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h> 
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include <limits>
#include <cmath>
#include "swf.h"
#include "toplevel/Date.h"
#include "toplevel/Math.h"
#include "toplevel/Vector.h"
#include "flash/accessibility/flashaccessibility.h"
#include "flash/events/flashevents.h"
#include "flash/display/flashdisplay.h"
#include "flash/net/flashnet.h"
#include "flash/system/flashsystem.h"
#include "flash/sensors/flashsensors.h"
#include "flash/utils/flashutils.h"
#include "flash/geom/flashgeom.h"
#include "flash/external/ExternalInterface.h"
#include "flash/media/flashmedia.h"
#include "flash/xml/flashxml.h"
#include "class.h"
#include "exceptions.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

TLSDATA bool lightspark::isVmThread=false;

uint32_t ABCVm::cur_recursion = 0;
//these limits can be overwritten by a ScriptLimitsTag
ABCVm::abc_limits ABCVm::limits = { /*max_recursion=*/ 256, /*max_timeout=*/ 20 };

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	LOG(LOG_CALLS,_("DoABCTag"));

	context=new ABCContext(in);

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCTag::execute(RootMovieClip*)
{
	LOG(LOG_CALLS,_("ABC Exec"));
	/* currentVM will free the context*/
	sys->currentVm->addEvent(NullRef,_MR(new ABCContextInitEvent(context,false)));
}

DoABCDefineTag::DoABCDefineTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> Flags >> Name;
	LOG(LOG_CALLS,_("DoABCDefineTag Name: ") << Name);

	context=new ABCContext(in);

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCDefineTag::execute(RootMovieClip*)
{
	LOG(LOG_CALLS,_("ABC Exec ") << Name);
	/* currentVM will free the context*/
	sys->currentVm->addEvent(NullRef,_MR(new ABCContextInitEvent(context,((int32_t)Flags)&1)));
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
		tiny_string className((const char*)Names[i],true);
		if(Tags[i]==0)
		{
			//We have to bind this root movieclip itself, let's tell it.
			//This will be done later
			root->bindToName(className);
			root->incRef();
			sys->currentVm->addEvent(NullRef, _MR(new BindClassEvent(_MR(root),className)));

		}
		else
		{
			_R<DictionaryTag> t=root->dictionaryLookup(Tags[i]);
			_R<BindClassEvent> e(new BindClassEvent(t,className));
			sys->currentVm->addEvent(NullRef,e);
		}
	}
}

void ScriptLimitsTag::execute(RootMovieClip* root)
{
	ABCVm::limits.max_recursion = MaxRecursionDepth;
	ABCVm::limits.script_timeout = ScriptTimeoutSeconds;
}

void ABCVm::registerClasses()
{
	Global* builtin=Class<Global>::getInstanceS();
	//Register predefined types, ASObject are enough for not implemented classes
	builtin->setVariableByQName("Object","",Class<ASObject>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Class","",Class_object::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Number","",Class<Number>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Boolean","",Class<Boolean>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("NaN","",abstract_d(numeric_limits<double>::quiet_NaN()),DECLARED_TRAIT);
	builtin->setVariableByQName("Infinity","",abstract_d(numeric_limits<double>::infinity()),DECLARED_TRAIT);
	builtin->setVariableByQName("String","",Class<ASString>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Array","",Class<Array>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Function","",Class<IFunction>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("undefined","",new Undefined,DECLARED_TRAIT);
	builtin->setVariableByQName("Math","",Class<Math>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Namespace","",Class<Namespace>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("AS3","",Class<Namespace>::getInstanceS(AS3),DECLARED_TRAIT);
	builtin->setVariableByQName("Date","",Class<Date>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("RegExp","",Class<RegExp>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("QName","",Class<ASQName>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("uint","",Class<UInteger>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Vector","__AS3__.vec",Template<Vector>::getTemplate(),DECLARED_TRAIT);
	builtin->setVariableByQName("Error","",Class<ASError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SecurityError","",Class<SecurityError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ArgumentError","",Class<ArgumentError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("DefinitionError","",Class<DefinitionError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("EvalError","",Class<EvalError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("RangeError","",Class<RangeError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ReferenceError","",Class<ReferenceError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SyntaxError","",Class<SyntaxError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TypeError","",Class<TypeError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("URIError","",Class<URIError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("VerifyError","",Class<VerifyError>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("XML","",Class<XML>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("XMLList","",Class<XMLList>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("int","",Class<Integer>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("print","",Class<IFunction>::getFunction(print),DECLARED_TRAIT);
	builtin->setVariableByQName("trace","",Class<IFunction>::getFunction(trace),DECLARED_TRAIT);
	builtin->setVariableByQName("parseInt","",Class<IFunction>::getFunction(parseInt),DECLARED_TRAIT);
	builtin->setVariableByQName("parseFloat","",Class<IFunction>::getFunction(parseFloat),DECLARED_TRAIT);
	builtin->setVariableByQName("encodeURI","",Class<IFunction>::getFunction(encodeURI),DECLARED_TRAIT);
	builtin->setVariableByQName("decodeURI","",Class<IFunction>::getFunction(decodeURI),DECLARED_TRAIT);
	builtin->setVariableByQName("encodeURIComponent","",Class<IFunction>::getFunction(encodeURIComponent),DECLARED_TRAIT);
	builtin->setVariableByQName("decodeURIComponent","",Class<IFunction>::getFunction(decodeURIComponent),DECLARED_TRAIT);
	builtin->setVariableByQName("escape","",Class<IFunction>::getFunction(escape),DECLARED_TRAIT);
	builtin->setVariableByQName("unescape","",Class<IFunction>::getFunction(unescape),DECLARED_TRAIT);
	builtin->setVariableByQName("toString","",Class<IFunction>::getFunction(ASObject::_toString),DECLARED_TRAIT);

	builtin->setVariableByQName("AccessibilityProperties","flash.accessibility",
			Class<AccessibilityProperties>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("AccessibilityImplementation","flash.accessibility",
			Class<AccessibilityImplementation>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("MovieClip","flash.display",Class<MovieClip>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("DisplayObject","flash.display",Class<DisplayObject>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Loader","flash.display",Class<Loader>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("LoaderInfo","flash.display",Class<LoaderInfo>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SimpleButton","flash.display",Class<SimpleButton>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("InteractiveObject","flash.display",Class<InteractiveObject>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("DisplayObjectContainer","flash.display",Class<DisplayObjectContainer>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Sprite","flash.display",Class<Sprite>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Shape","flash.display",Class<Shape>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Stage","flash.display",Class<Stage>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Graphics","flash.display",Class<Graphics>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("GradientType","flash.display",Class<GradientType>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("BlendMode","flash.display",Class<BlendMode>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("LineScaleMode","flash.display",Class<LineScaleMode>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StageScaleMode","flash.display",Class<StageScaleMode>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StageAlign","flash.display",Class<StageAlign>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StageQuality","flash.display",Class<StageQuality>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StageDisplayState","flash.display",Class<StageDisplayState>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("IBitmapDrawable","flash.display",
			Class<ASObject>::getStubClass(QName("IBitmapDrawable","flash.display")),DECLARED_TRAIT);
	builtin->setVariableByQName("BitmapData","flash.display",
			Class<ASObject>::getStubClass(QName("BitmapData","flash.display")),DECLARED_TRAIT);
	builtin->setVariableByQName("Bitmap","flash.display",Class<Bitmap>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("GraphicsGradientFill","flash.display",
			Class<ASObject>::getStubClass(QName("GraphicsGradientFill","flash.display")),DECLARED_TRAIT);
	builtin->setVariableByQName("GraphicsPath","flash.display",
			Class<ASObject>::getStubClass(QName("GraphicsPath","flash.display")),DECLARED_TRAIT);
	builtin->setVariableByQName("MorphShape","flash.display",Class<MorphShape>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SpreadMethod","flash.display",Class<SpreadMethod>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("InterpolationMethod","flash.display",Class<InterpolationMethod>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("FrameLabel","flash.display",Class<FrameLabel>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Scene","flash.display",Class<Scene>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("AVM1Movie","flash.display",Class<AVM1Movie>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Shader","flash.display",Class<Shader>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("DropShadowFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("DropShadowFilter","flash.filters")),DECLARED_TRAIT);
	builtin->setVariableByQName("BitmapFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("BitmapFilter","flash.filters")),DECLARED_TRAIT);
	builtin->setVariableByQName("GlowFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("GlowFilter","flash.filters")),DECLARED_TRAIT);
	builtin->setVariableByQName("BevelFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("BevelFilter","flash.filters")),DECLARED_TRAIT);
	builtin->setVariableByQName("ColorMatrixFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("ColorMatrixFilter","flash.filters")),DECLARED_TRAIT);

	builtin->setVariableByQName("Font","flash.text",Class<Font>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StyleSheet","flash.text",Class<StyleSheet>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextField","flash.text",Class<TextField>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextFieldType","flash.text",Class<TextFieldType>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextFieldAutoSize","flash.text",Class<TextFieldAutoSize>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextFormat","flash.text",Class<TextFormat>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextFormatAlign","flash.text",Class<TextFormatAlign>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("StaticText","flash.text",Class<StaticText>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("XMLDocument","flash.xml",Class<XMLDocument>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("XMLNode","flash.xml",Class<XMLNode>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("ExternalInterface","flash.external",Class<ExternalInterface>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("ByteArray","flash.utils",Class<ByteArray>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Dictionary","flash.utils",Class<Dictionary>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Proxy","flash.utils",Class<Proxy>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Timer","flash.utils",Class<Timer>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("getQualifiedClassName","flash.utils",
			Class<IFunction>::getFunction(getQualifiedClassName),DECLARED_TRAIT);
	builtin->setVariableByQName("getQualifiedSuperclassName","flash.utils",
			Class<IFunction>::getFunction(getQualifiedSuperclassName),DECLARED_TRAIT);
	builtin->setVariableByQName("getDefinitionByName","flash.utils",Class<IFunction>::getFunction(getDefinitionByName),DECLARED_TRAIT);
	builtin->setVariableByQName("getTimer","flash.utils",Class<IFunction>::getFunction(getTimer),DECLARED_TRAIT);
	builtin->setVariableByQName("setInterval","flash.utils",Class<IFunction>::getFunction(setInterval),DECLARED_TRAIT);
	builtin->setVariableByQName("setTimeout","flash.utils",Class<IFunction>::getFunction(setTimeout),DECLARED_TRAIT);
	builtin->setVariableByQName("clearInterval","flash.utils",Class<IFunction>::getFunction(clearInterval),DECLARED_TRAIT);
	builtin->setVariableByQName("clearTimeout","flash.utils",Class<IFunction>::getFunction(clearTimeout),DECLARED_TRAIT);
	builtin->setVariableByQName("describeType","flash.utils",Class<IFunction>::getFunction(describeType),DECLARED_TRAIT);
	builtin->setVariableByQName("IExternalizable","flash.utils",Class<ASObject>::getStubClass(QName("IExternalizable","flash.utils")),DECLARED_TRAIT);

	builtin->setVariableByQName("ColorTransform","flash.geom",Class<ColorTransform>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Rectangle","flash.geom",Class<Rectangle>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Matrix","flash.geom",Class<Matrix>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Transform","flash.geom",Class<Transform>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Point","flash.geom",Class<Point>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Vector3D","flash.geom",Class<Vector3D>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Matrix3D","flash.geom",Class<ASObject>::getStubClass(QName("Matrix3D", "flash.geom")),DECLARED_TRAIT);

	builtin->setVariableByQName("EventDispatcher","flash.events",Class<EventDispatcher>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Event","flash.events",Class<Event>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("EventPhase","flash.events",Class<EventPhase>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("MouseEvent","flash.events",Class<MouseEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ProgressEvent","flash.events",Class<ProgressEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TimerEvent","flash.events",Class<TimerEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("IOErrorEvent","flash.events",Class<IOErrorEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ErrorEvent","flash.events",Class<ErrorEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SecurityErrorEvent","flash.events",Class<SecurityErrorEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("AsyncErrorEvent","flash.events",Class<AsyncErrorEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("FullScreenEvent","flash.events",Class<FullScreenEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("TextEvent","flash.events",Class<TextEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("IEventDispatcher","flash.events",Class<IEventDispatcher>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("FocusEvent","flash.events",Class<FocusEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("NetStatusEvent","flash.events",Class<NetStatusEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("HTTPStatusEvent","flash.events",Class<HTTPStatusEvent>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("KeyboardEvent","flash.events",Class<KeyboardEvent>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("sendToURL","flash.net",Class<IFunction>::getFunction(sendToURL),DECLARED_TRAIT);
	builtin->setVariableByQName("LocalConnection","flash.net",Class<ASObject>::getStubClass(QName("LocalConnection","flash.net")),DECLARED_TRAIT);
	builtin->setVariableByQName("NetConnection","flash.net",Class<NetConnection>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("NetStream","flash.net",Class<NetStream>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("NetStreamPlayOptions","flash.net",Class<ASObject>::getStubClass(QName("NetStreamPlayOptions","flash.net")),DECLARED_TRAIT);
	builtin->setVariableByQName("URLLoader","flash.net",Class<URLLoader>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("URLRequest","flash.net",Class<URLRequest>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("URLRequestMethod","flash.net",Class<URLRequestMethod>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("URLVariables","flash.net",Class<URLVariables>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SharedObject","flash.net",Class<SharedObject>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ObjectEncoding","flash.net",Class<ObjectEncoding>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Socket","flash.net",Class<ASObject>::getStubClass(QName("Socket","flash.net")),DECLARED_TRAIT);
	builtin->setVariableByQName("Responder","flash.net",Class<ASObject>::getStubClass(QName("Responder","flash.net")),DECLARED_TRAIT);

	builtin->setVariableByQName("fscommand","flash.system",Class<IFunction>::getFunction(fscommand),DECLARED_TRAIT);
	builtin->setVariableByQName("Capabilities","flash.system",Class<Capabilities>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Security","flash.system",Class<Security>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("ApplicationDomain","flash.system",Class<ApplicationDomain>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SecurityDomain","flash.system",Class<SecurityDomain>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("LoaderContext","flash.system",Class<ASObject>::getStubClass(QName("LoaderContext","flash.system")),DECLARED_TRAIT);

	builtin->setVariableByQName("SoundTransform","flash.media",Class<SoundTransform>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Video","flash.media",Class<Video>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("Sound","flash.media",Class<Sound>::getRef(),DECLARED_TRAIT);
	builtin->setVariableByQName("SoundLoaderContext","flash.media",Class<SoundLoaderContext>::getRef(),DECLARED_TRAIT);

	builtin->setVariableByQName("Keyboard","flash.ui",Class<ASObject>::getStubClass(QName("Keyboard","flash.ui")),DECLARED_TRAIT);
	builtin->setVariableByQName("ContextMenu","flash.ui",Class<ASObject>::getStubClass(QName("ContextMenu","flash.ui")),DECLARED_TRAIT);
	builtin->setVariableByQName("ContextMenuItem","flash.ui",Class<ASObject>::getStubClass(QName("ContextMenuItem","flash.ui")),DECLARED_TRAIT);

	builtin->setVariableByQName("Accelerometer", "flash.sensors", Class<Accelerometer>::getRef(), DECLARED_TRAIT);

	builtin->setVariableByQName("isNaN","",Class<IFunction>::getFunction(isNaN),DECLARED_TRAIT);
	builtin->setVariableByQName("isFinite","",Class<IFunction>::getFunction(isFinite),DECLARED_TRAIT);
	builtin->setVariableByQName("isXMLName","",Class<IFunction>::getFunction(_isXMLName),DECLARED_TRAIT);

	global->registerGlobalScope(builtin);
}

/* This function determines how many stack values are needed for
 * resolving the multiname at index mi
 */
int ABCContext::getMultinameRTData(int mi) const
{
	if(mi==0)
		return 0;

	const multiname_info* m=&constant_pool.multinames[mi];
	switch(m->kind)
	{
		case 0x07: //QName
		case 0x0d: //QNameA
		case 0x09: //Multiname
		case 0x0e: //MultinameA
		case 0x1d: //Templated name
			return 0;
		case 0x0f: //RTQName
		case 0x10: //RTQNameA
		case 0x1b: //MultinameL
		case 0x1c: //MultinameLA
			return 1;
		case 0x11: //RTQNameL
		case 0x12: //RTQNameLA
			return 2;
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
		ret->isAttribute=m->isAttributeName();
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

//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_i(call_context* th, uint32_t rti, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;
		ret->isAttribute=m->isAttributeName();
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

/*
 * Gets a multiname. May pop one value of the runtime stack
 * This is a helper called from interpreter.
 */
multiname* ABCContext::getMultiname(unsigned int n, call_context* context)
{
	int fromStack = getMultinameRTData(n);
	ASObject* rt1 = NULL;
	ASObject* rt2 = NULL;
	if(fromStack > 0)
		rt1 = context->runtime_stack_pop();
	if(fromStack > 1)
		rt2 = context->runtime_stack_pop();
	return getMultinameImpl(rt1,rt2,n);
}

/*
 * Gets a multiname without accessing the runtime stack.
 * The object from the top of the stack must be provided in 'n'
 * if getMultinameRTData(midx) returns 1 and the top two objects
 * must be provided if getMultinameRTData(midx) returns 2.
 * This is a helper used by codesynt.
 */
multiname* ABCContext::s_getMultiname(ABCContext* th, ASObject* n, ASObject* n2, int midx)
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
multiname* ABCContext::getMultinameImpl(ASObject* n, ASObject* n2, unsigned int midx)
{
	multiname* ret;
	multiname_info* m=&constant_pool.multinames[midx];

	/* If this multiname is not cached, resolve its static parts */
	if(m->cached==NULL)
	{
		m->cached=new multiname;
		ret=m->cached;
		if(midx==0)
		{
			ret->name_s="any";
			ret->name_type=multiname::NAME_STRING;
			ret->ns.emplace_back("",NAMESPACE);
			ret->isAttribute=false;
			return ret;
		}
		ret->isAttribute=m->isAttributeName();
		switch(m->kind)
		{
			case 0x07: //QName
			case 0x0D: //QNameA
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
			case 0x09: //Multiname
			case 0x0e: //MultinameA
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
			case 0x1b: //MultinameL
			case 0x1c: //MultinameLA
			{
				const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
				ret->ns.reserve(s->count);
				for(unsigned int i=0;i<s->count;i++)
				{
					const namespace_info* n=&constant_pool.namespaces[s->ns[i]];
					ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				}
				sort(ret->ns.begin(),ret->ns.end());
				break;
			}
			case 0x0f: //RTQName
			case 0x10: //RTQNameA
			{
				ret->name_type=multiname::NAME_STRING;
				ret->name_s=getString(m->name);
				break;
			}
			case 0x11: //RTQNameL
			case 0x12: //RTQNameLA
			{
				//Everything is dynamic
				break;
			}
			case 0x1d: //Template instance Name
			{
				multiname_info* td=&constant_pool.multinames[m->type_definition];
				//builds a name by concating the templateName$TypeName1$TypeName2...
				//this naming scheme is defined by the ABC compiler
				tiny_string name = getString(td->name);
				for(size_t i=0;i<m->param_types.size();++i)
				{
					multiname_info* p=&constant_pool.multinames[m->param_types[i]];
					name += "$";
					name += getString(p->name);
				}
				const namespace_info* n=&constant_pool.namespaces[td->ns];
				ret->ns.push_back(nsNameAndKind(getString(n->name),(NS_KIND)(int)n->kind));
				ret->name_s=name;
				ret->name_type=multiname::NAME_STRING;
				break;
			}
			default:
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
				throw UnsupportedException("Multiname to String not implemented");
		}
	}

	/* Now resolve its dynamic parts */
	ret=m->cached;
	if(midx==0)
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
			assert(!n && !n2);
			break;
		}
		case 0x1b: //MultinameL
		case 0x1c: //MultinameLA
		{
			assert(n && !n2);
			ret->setName(n);
			n->decRef();
			break;
		}
		case 0x0f: //RTQName
		case 0x10: //RTQNameA
		{
			assert(n && !n2);
			assert_and_throw(n->classdef==Class<Namespace>::getClass());
			Namespace* tmpns=static_cast<Namespace*>(n);
			//TODO: What is the right kind?
			ret->ns.clear();
			ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
			n->decRef();
			break;
		}
		case 0x11: //RTQNameL
		case 0x12: //RTQNameLA
		{
			assert(n && n2);
			assert_and_throw(n2->classdef==Class<Namespace>::getClass());
			Namespace* tmpns=static_cast<Namespace*>(n2);
			ret->ns.clear();
			ret->ns.push_back(nsNameAndKind(tmpns->uri,NAMESPACE));
			ret->setName(n);
			n->decRef();
			n2->decRef();
			break;
		}
		default:
			LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
			throw UnsupportedException("Multiname to String not implemented");
	}
	return ret;
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
		if(instances[i].isSealed())
			LOG(LOG_CALLS,_("\tSealed"));
		if(instances[i].isFinal())
			LOG(LOG_CALLS,_("\tFinal"));
		if(instances[i].isInterface())
			LOG(LOG_CALLS,_("\tInterface"));
		if(instances[i].isProtectedNs())
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

	hasRunScriptInit.resize(scripts.size(),false);
#ifdef PROFILING_SUPPORT
	sys->contextes.push_back(this);
#endif
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

ABCVm::ABCVm(SystemState* s):m_sys(s),status(CREATED),shuttingdown(false),curGlobalObj(NULL)
{
	sem_init(&event_queue_mutex,0,1);
	sem_init(&sem_event_count,0,0);
	m_sys=s;
	int_manager=new Manager(15);
	uint_manager=new Manager(15);
	number_manager=new Manager(15);
	global=new GlobalObject;
	LOG(LOG_INFO,_("Global is ") << global);
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

void ABCVm::finalize()
{
	//The event queue may be not empty if the VM has been been started
	if(status==CREATED && !events_queue.empty())
		LOG(LOG_ERROR, "Events queue is not empty as expected");
	events_queue.clear();
}


ABCVm::~ABCVm()
{
	for(size_t i=0;i<contexts.size();++i)
		delete contexts[i];

	sem_destroy(&sem_event_count);
	sem_destroy(&event_queue_mutex);
	delete int_manager;
	delete uint_manager;
	delete number_manager;
	delete global;
}

int ABCVm::getEventQueueSize()
{
	return events_queue.size();
}

void ABCVm::publicHandleEvent(_R<EventDispatcher> dispatcher, _R<Event> event)
{
	std::deque<_R<DisplayObject>> parents;
	assert_and_throw(!event->target);
	event->setTarget(dispatcher);
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
	if(dispatcher->classdef->isSubClass(Class<DisplayObject>::getClass()))
	{
		event->eventPhase = EventPhase::CAPTURING_PHASE;
		dispatcher->incRef();
		//We fetch the relatedObject in the case of rollOver/Out
		_NR<DisplayObject> rcur;
		if(event->type == "rollOver" || event->type == "rollOut")
		{
			event->incRef();
			_R<MouseEvent> mevent = _MR(dynamic_cast<MouseEvent*>(event.getPtr()));  
			if(mevent->relatedObject)
			{  
				mevent->relatedObject->incRef();
				rcur = mevent->relatedObject;
			}
		}
		//If the relObj is non null, we get its ancestors to build a truncated parents queue for the target 
		if(rcur)
		{
			std::vector<_NR<DisplayObject>> rparents;
			rparents.push_back(rcur);        
			while(true)
			{
				if(!rcur->getParent())
					break;
				rcur = rcur->getParent();
				rparents.push_back(rcur);
			}
			_R<DisplayObject> cur = _MR(dynamic_cast<DisplayObject*>(dispatcher.getPtr()));
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
			_R<DisplayObject> cur = _MR(dynamic_cast<DisplayObject*>(dispatcher.getPtr()));
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
			event->currentTarget=*i;
			(*i)->handleEvent(event);
		}
	}

	//Do target phase
	if(doTarget)
	{
		event->eventPhase = EventPhase::AT_TARGET;
		event->currentTarget=dispatcher;
		dispatcher->handleEvent(event);
	}

	//Do bubbling phase
	if(event->bubbles && !parents.empty())
	{
		event->eventPhase = EventPhase::BUBBLING_PHASE;
		auto i = parents.begin();
		for(;i!=parents.end();++i)
		{
			event->currentTarget=*i;
			(*i)->handleEvent(event);
		}
	}
	/* This must even be called if stop*Propagation has been called */
	if(!event->defaultPrevented)
		dispatcher->defaultEventBehavior(event);

	//Reset events so they might be recycled
	event->currentTarget=NullRef;
	event->setTarget(NullRef);
}

void ABCVm::handleEvent(std::pair<_NR<EventDispatcher>, _R<Event> > e)
{
	e.second->check();
	if(!e.first.isNull())
		publicHandleEvent(e.first, e.second);
	else
	{
		//Should be handled by the Vm itself
		switch(e.second->getEventType())
		{
			case BIND_CLASS:
			{
				BindClassEvent* ev=static_cast<BindClassEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,_("Binding of ") << ev->class_name);
				if(ev->tag)
					buildClassAndBindTag(ev->class_name.raw_buf(),ev->tag);
				else
					buildClassAndInjectBase(ev->class_name.raw_buf(),ev->base);
				LOG(LOG_CALLS,_("End of binding of ") << ev->class_name);
				break;
			}
			case SHUTDOWN:
				shuttingdown=true;
				break;
			case SYNC:
			{
				SynchronizationEvent* ev=static_cast<SynchronizationEvent*>(e.second.getPtr());
				ev->sync();
				break;
			}
			case FUNCTION:
			{
				FunctionEvent* ev=static_cast<FunctionEvent*>(e.second.getPtr());
				// We should catch exceptions and report them
				if(ev->exception != NULL)
				{
					//SyncEvents are only used in this code path
					try
					{
						ev->obj->incRef();
						ASObject* result = ev->f->call(ev->obj.getPtr(),ev->args,ev->numArgs);
						// We should report the function result
						if(ev->result != NULL)
							*(ev->result) = result;
					}
					catch(ASObject* exception)
					{
						// Report the exception
						*(ev->exception) = exception;
					}
					catch(LightsparkException& e)
					{
						//An internal error happended, sync and rethrow
						if(!ev->sync.isNull())
							ev->sync->sync();
						throw e;
					}
					// We should synchronize the passed SynchronizationEvent
					if(!ev->sync.isNull())
						ev->sync->sync();
				}
				// Exceptions aren't expected and shouldn't be ignored
				else
				{
					assert(ev->sync.isNull());
					if(!ev->obj.isNull())
						ev->obj->incRef();
					ASObject* result = ev->f->call(ev->obj.getPtr(),ev->args,ev->numArgs);
					// We should report the function result
					if(ev->result != NULL)
						*(ev->result) = result;
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
			case INIT_FRAME:
			{
				InitFrameEvent* ev=static_cast<InitFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"INIT_FRAME");
				if(!ev->clip.isNull())
					ev->clip->initFrame();
				else
					sys->getStage()->initFrame();
				break;
			}
			case ADVANCE_FRAME:
			{
				AdvanceFrameEvent* ev=static_cast<AdvanceFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"ADVANCE_FRAME");
				sys->getStage()->advanceFrame();
				ev->done.signal();
				break;
			}
			case FLUSH_INVALIDATION_QUEUE:
			{
				//Flush the invalidation queue
				m_sys->flushInvalidationQueue();
				break;
			}
			default:
				throw UnsupportedException("Not supported event");
		}
	}
}

/*! \brief enqueue an event, a reference is acquired
* * \param obj EventDispatcher that will receive the event
* * \param ev event that will be sent */
bool ABCVm::addEvent(_NR<EventDispatcher> obj ,_R<Event> ev)
{
	//If the system should terminate new events are not accepted
	if(m_sys->shouldTerminate())
		return false;
	//If the event is a synchronization and we are running in the VM context
	//we should handle it immediately to avoid deadlock
	if(isVmThread && (ev->getEventType()==SYNC))
	{
		assert(obj.isNull());
		handleEvent(make_pair(obj, ev));
		return true;
	}

	sem_wait(&event_queue_mutex);
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	sem_post(&event_queue_mutex);
	sem_post(&sem_event_count);
	return true;
}

Class_inherit* ABCVm::findClassInherit(const string& s)
{
	LOG(LOG_CALLS,_("Setting class name to ") << s);
	ASObject* target;
	ASObject* derived_class=global->getVariableByString(s,target);
	if(derived_class==NULL)
	{
		LOG(LOG_ERROR,_("Class ") << s << _(" not found in global"));
		throw RunTimeException("Class not found in global");
	}

	if(derived_class->is<Definable>())
	{
		LOG(LOG_CALLS,_("Class ") << s << _(" is not yet valid"));
		derived_class=derived_class->as<Definable>()->define();
		LOG(LOG_CALLS,_("End of deferred init of class ") << s);
		assert_and_throw(derived_class);
	}

	assert_and_throw(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	assert_and_throw(static_cast<Class_base*>(derived_class)->class_index!=-1);
	Class_inherit* derived_class_tmp=static_cast<Class_inherit*>(derived_class);
	if(derived_class_tmp->isBinded())
	{
		LOG(LOG_ERROR, "Class already binded to a tag. Not binding");
		return NULL;
	}
	return derived_class_tmp;
}

void ABCVm::buildClassAndInjectBase(const string& s, _R<RootMovieClip> base)
{
	Class_inherit* derived_class_tmp = findClassInherit(s);
	if(!derived_class_tmp)
		return;

	//Let's override the class
	base->setClass(derived_class_tmp);
	derived_class_tmp->bindToRoot();
}

void ABCVm::buildClassAndBindTag(const string& s, _R<DictionaryTag> t)
{
	Class_inherit* derived_class_tmp = findClassInherit(s);
	if(!derived_class_tmp)
		return;

	//It seems to be acceptable for the same base to be binded multiple times.
	//In such cases the first binding is bidirectional (instances created using PlaceObject
	//use the binded class and instances created using 'new' use the binded tag). Any other
	//bindings will be unidirectional (only instances created using new will use the binded tag)
	if(t->bindedTo==NULL)
		t->bindedTo=derived_class_tmp;

	derived_class_tmp->bindToTag(t.getPtr());
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

void call_context::runtime_stack_clear()
{
	while(stack_index > 0)
		stack[--stack_index]->decRef();
}

call_context::~call_context()
{
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

	for(uint32_t i=0;i<locals_size;i++)
	{
		if(locals[i])
			locals[i]->decRef();
	}
}

bool ABCContext::isinstance(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("isinstance ") << *name);

	if(name->qualifiedString() == "::any")
		return true;
	
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

	if(obj->classdef)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		objc=obj->classdef;
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
		real_ret=obj->getObjectType()==type->getObjectType();
		LOG(LOG_CALLS,_("isType on non classed object ") << real_ret);
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG(LOG_CALLS,_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	return real_ret;
}

/*
 * The ABC definitions (classes, scripts, etc) have been parsed in
 * ABCContext constructor. Now create the internal structures for them
 * and execute the main/init function.
 */
void ABCContext::exec(bool lazy)
{
	//Take script entries and declare their traits
	unsigned int i=0;

	for(;i<scripts.size()-1;i++)
	{
		LOG(LOG_CALLS, _("Script N: ") << i );

		//Creating a new global for this script
		Global* global=Class<Global>::getInstanceS();
#ifndef NDEBUG
		global->initialized=false;
#endif
		LOG(LOG_CALLS, _("Building script traits: ") << scripts[i].trait_count );


		for(unsigned int j=0;j<scripts[i].trait_count;j++)
		{
			buildTrait(global,&scripts[i].traits[j],false,i);
		}

#ifndef NDEBUG
		global->initialized=true;
#endif
		//Register it as one of the global scopes
		getGlobal()->registerGlobalScope(global);
	}
	//The last script entry has to be run
	LOG(LOG_CALLS, _("Last script (Entry Point)"));
	//Creating a new global for the last script
	Global* global=Class<Global>::getInstanceS();
#ifndef NDEBUG
		global->initialized=false;
#endif

	LOG(LOG_CALLS, _("Building entry script traits: ") << scripts[i].trait_count );
	for(unsigned int j=0;j<scripts[i].trait_count;j++)
	{
		buildTrait(global,&scripts[i].traits[j],false,i);
	}

#ifndef NDEBUG
		global->initialized=true;
#endif
	//Register it as one of the global scopes
	getGlobal()->registerGlobalScope(global);
	//the script init of the last script is the main entry point
	if(!lazy)
		runScriptInit(i, global);
	LOG(LOG_CALLS, _("End of Entry Point"));
}

void ABCContext::runScriptInit(unsigned int i, ASObject* g)
{
	LOG(LOG_CALLS, "Running script init for script " << i );

	if(hasRunScriptInit[i])
		throw Class<VerifyError>::getInstanceS("Script init did not define all DEFINABLEs");
	hasRunScriptInit[i] = true;

	method_info* m=get_method(scripts[i].init);
	SyntheticFunction* entry=Class<IFunction>::getSyntheticFunction(m);

	g->incRef();
	entry->addToScope(scope_entry(_MR(g),false));

	g->incRef();
	ASObject* ret=entry->call(g,NULL,0);

	if(ret)
		ret->decRef();

	entry->decRef();
	LOG(LOG_CALLS, "Finished script init for script " << i );
}

void ABCVm::Run(ABCVm* th)
{
	//Spin wait until the VM is aknowledged by the SystemState
	sys=th->m_sys;
	while(getVm()!=th);
	isVmThread=true;
	if(th->m_sys->useJit)
	{
		llvm::JITExceptionHandling = true;
#ifndef NDEBUG
		llvm::JITEmitDebugInfo = true;
#endif
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
				//	LOG(LOG_INFO,th->events_queue.size() << _(" events missing before exit"));
				else if(firstMissingEvents)
				{
					LOG(LOG_INFO,th->events_queue.size() << _(" events missing before exit"));
					firstMissingEvents = false;
				}
			}
			Chronometer chronometer;
			sem_wait(&th->event_queue_mutex);
			pair<_NR<EventDispatcher>,_R<Event>> e=th->events_queue.front();
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
			if(e->getClass())
				LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM ") << e->getClass()->class_name);
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

			variable* var=NULL;
			var = c->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,BORROWED_TRAIT);
			if(var && var->var)
			{
				assert_and_throw(var->var && var->var->getObjectType()==T_FUNCTION);

				IFunction* f=static_cast<IFunction*>(var->var);
				f->incRef();
				c->setDeclaredMethodByQName(name,mname.ns[0],f,NORMAL_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Method not linkable") << ": " << mname);
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

			variable* var=NULL;
			var=c->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,BORROWED_TRAIT);
			if(var && var->getter)
			{
				assert_and_throw(var->getter);

				var->getter->incRef();
				c->setDeclaredMethodByQName(name,mname.ns[0],var->getter,GETTER_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Getter not linkable") << ": " << mname);
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

			variable* var=NULL;
			var=c->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,BORROWED_TRAIT);
			if(var && var->setter)
			{
				assert_and_throw(var->setter);

				var->setter->incRef();
				c->setDeclaredMethodByQName(name,mname.ns[0],var->setter,SETTER_METHOD,true);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("Setter not linkable") << ": " << mname);
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

void ABCContext::buildTrait(ASObject* obj, const traits_info* t, bool isBorrowed, int scriptid)
{
	multiname* mname=getMultiname(t->name,NULL);
	//Should be a Qname
	assert_and_throw(mname->ns.size()==1 && mname->name_type==multiname::NAME_STRING);
	if(t->kind>>4)
		LOG(LOG_CALLS,_("Next slot has flags ") << (t->kind>>4));

	if(t->kind&traits_info::Metadata)
        {
		for(unsigned int i=0;i<t->metadata_count;i++)
		{
			metadata_info& minfo = metadata[t->metadata[i]];
			LOG(LOG_CALLS,"Metadata: " << getString(minfo.name));
			for(unsigned int j=0;j<minfo.item_count;++j)
				LOG(LOG_CALLS,"        : " << getString(minfo.items[j].key) << " " << getString(minfo.items[j].value));
		}
	}

	uint32_t kind = t->kind&0xf;
	switch(kind)
	{
		case traits_info::Class:
		{
			//Check if this already defined in upper levels
			_NR<ASObject> tmpo=obj->getVariableByMultiname(*mname,ASObject::SKIP_IMPL);
			if(!tmpo.isNull())
				return;
			ASObject* ret;

			//check if this class has the 'interface' flag, i.e. it is an interface
			if((instances[t->classi].flags)&0x04)
			{
				QName className(mname->name_s,mname->ns[0].name);

				// Should the new definition overwrite the old one?
				if(sys->classes.find(className)!=sys->classes.end())
				{
					LOG(LOG_TRACE, "Trying to re-define interface " << className.getQualifiedName());
					break;
				}

				Class_inherit* ci=new Class_inherit(className);
				ci->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
				LOG(LOG_CALLS,_("Building class traits"));
				for(unsigned int i=0;i<classes[t->classi].trait_count;i++)
					buildTrait(ci,&classes[t->classi].traits[i],false);
				//Add protected namespace if needed
				if((instances[t->classi].flags)&0x08)
				{
					ci->use_protected=true;
					int ns=instances[t->classi].protectedNs;
					const namespace_info& ns_info=constant_pool.namespaces[ns];
					ci->protected_ns=nsNameAndKind(getString(ns_info.name),(NS_KIND)(int)ns_info.kind);
				}
				LOG(LOG_CALLS,_("Adding immutable object traits to class"));
				//Class objects also contains all the methods/getters/setters declared for instances
				for(unsigned int i=0;i<instances[t->classi].trait_count;i++)
				{
					int kind=instances[t->classi].traits[i].kind&0xf;
					if(kind==traits_info::Method || kind==traits_info::Setter || kind==traits_info::Getter)
						buildTrait(ci,&instances[t->classi].traits[i],true);
				}

				//add implemented interfaces
				for(unsigned int i=0;i<instances[t->classi].interface_count;i++)
				{
					multiname* name=getMultiname(instances[t->classi].interfaces[i],NULL);
					ci->addImplementedInterface(*name);
				}

				ci->class_index=t->classi;
				ci->context = this;

				//can an interface derive from an other interface?
				//can an interface derive from an non interface class?
				assert(instances[t->classi].supername == 0);
				//do interfaces have cinit methods?
				//TODO: call them, set constructor property, do something
				if(classes[t->classi].cinit != 0)
					LOG(LOG_NOT_IMPLEMENTED,"Interface cinit (static)");
				if(instances[t->classi].init != 0)
					LOG(LOG_NOT_IMPLEMENTED,"Interface cinit (constructor)");
				ret = ci;
			}
			else if(scriptid != -1)
				//create Definable even for internal/private classes as they maybe used
				//as 'type' in a slot.
				ret=new Definable(this,scriptid,obj,mname);
			else
				ret=new Undefined;

			obj->setVariableByQName(mname->name_s,mname->ns[0],ret,DECLARED_TRAIT);

			LOG(LOG_CALLS,_("Class slot ")<< t->slot_id << _(" type Class name ") << *mname << _(" id ") << t->classi);
			if(t->slot_id)
				obj->initSlot(t->slot_id, *mname);
			break;
		}
		case traits_info::Getter:
		case traits_info::Setter:
		case traits_info::Method:
		{
			//methods can also be defined at toplevel (not only traits_info::Function!)
			if(kind == traits_info::Getter)
				LOG(LOG_CALLS,"Getter trait: " << *mname << _(" #") << t->method);
			else if(kind == traits_info::Setter)
				LOG(LOG_CALLS,"Setter trait: " << *mname << _(" #") << t->method);
			else if(kind == traits_info::Method)
				LOG(LOG_CALLS,"Method trait: " << *mname << _(" #") << t->method);
			method_info* m=&methods[t->method];
			SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(m);

#ifdef PROFILING_SUPPORT
			if(!m->validProfName)
			{
				m->profName=prot->class_name.name+"::"+mname->qualifiedString();
				m->validProfName=true;
			}
#endif
			//A script can also have a getter trait
			if(obj->is<Class_inherit>())
			{
				Class_inherit* prot = obj->as<Class_inherit>();
				f->inClass = prot;

				//Methods save a copy of the scope stack of the class
				f->acquireScope(prot->class_scope);
			}
			else
			{
				assert(scriptid != -1);
				obj->incRef();
				f->addToScope(scope_entry(_MR(obj),false));
			}
			if(kind == traits_info::Getter)
				obj->setDeclaredMethodByQName(mname->name_s,mname->ns[0],f,GETTER_METHOD,isBorrowed);
			else if(kind == traits_info::Setter)
				obj->setDeclaredMethodByQName(mname->name_s,mname->ns[0],f,SETTER_METHOD,isBorrowed);
			else if(kind == traits_info::Method)
				obj->setDeclaredMethodByQName(mname->name_s,mname->ns[0],f,NORMAL_METHOD,isBorrowed);
			break;
		}
		case traits_info::Const:
		{
			//Check if this already defined in upper levels
			_NR<ASObject> tmpo=obj->getVariableByMultiname(*mname,ASObject::SKIP_IMPL);
			if(!tmpo.isNull())
				return;

			ASObject* ret;
			//If the index is valid we set the constant
			if(t->vindex)
				ret=getConstant(t->vkind,t->vindex);
			else
				ret=new Undefined;

			LOG(LOG_CALLS,_("Const ") << *mname <<_(" type ")<< *getMultiname(t->type_name,NULL));
			obj->setVariableByQName(mname->name_s,mname->ns[0],ret,DECLARED_TRAIT);
			if(t->slot_id)
				obj->initSlot(t->slot_id, *mname);
			break;
		}
		case traits_info::Slot:
		{
			//Check if this already defined in upper levels
			_NR<ASObject> tmpo=obj->getVariableByMultiname(*mname,ASObject::SKIP_IMPL);
			if(!tmpo.isNull())
				return;

			multiname* tname=getMultiname(t->type_name,NULL);
			ASObject* ret;
			if(t->vindex)
			{
				ret=getConstant(t->vkind,t->vindex);
				LOG(LOG_CALLS,_("Slot ") << t->slot_id << ' ' << *mname <<_(" type ")<<*tname);
			}
			else
			{
				LOG(LOG_CALLS,_("Slot ")<< t->slot_id<<  _(" vindex 0 ") << *mname <<_(" type ")<<*tname);
				//The Undefined is coerced to the right type by the initializeVar..
				ret = new Undefined;
			}

			obj->initializeVariableByMultiname(*mname, ret, tname);

			if(t->slot_id)
				obj->initSlot(t->slot_id, *mname);

			break;
		}
		default:
			LOG(LOG_ERROR,_("Trait not supported ") << *mname << _(" ") << t->kind);
			obj->setVariableByMultiname(*mname, new Undefined);
	}
}


ASObject* method_info::getOptional(unsigned int i)
{
	assert_and_throw(i<options.size());
	return context->getConstant(options[i].kind,options[i].val);
}

const multiname* method_info::paramTypeName(unsigned int i) const
{
	assert_and_throw(i<param_type.size());
	return context->getMultiname(param_type[i],NULL);
}

const multiname* method_info::returnTypeName() const
{
	return context->getMultiname(return_type,NULL);
}

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
	v.val=LittleEndianToHost16(t);
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
	dummy.dump=LittleEndianToHost64(dummy.dump);
	v.val=dummy.value;
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
