/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/PassManager.h>
#include <llvm/LLVMContext.h>
#include <llvm/Target/TargetData.h>
#ifdef HAVE_SUPPORT_TARGETSELECT_H
#include <llvm/Support/TargetSelect.h>
#else
#include <llvm/Target/TargetSelect.h>
#endif
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h> 
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include <limits>
#include <cmath>
#include "swf.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Math.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/XMLSocket.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/sensors/flashsensors.h"
#include "scripting/flash/utils/flashutils.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/external/ExternalInterface.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/xml/flashxml.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/text/flashtextengine.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "scripting/abc.h"

using namespace std;
using namespace lightspark;

static GStaticPrivate is_vm_thread = G_STATIC_PRIVATE_INIT; /* TLS */
bool lightspark::isVmThread()
{
	return g_static_private_get(&is_vm_thread);
}

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	LOG(LOG_CALLS,_("DoABCTag"));

	RootMovieClip* root=getParseThread()->getRootMovie();
	root->incRef();
	context=new ABCContext(_MR(root), in, getVm());

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCTag::execute(RootMovieClip*) const
{
	LOG(LOG_CALLS,_("ABC Exec"));
	/* currentVM will free the context*/
	getVm()->addEvent(NullRef,_MR(new (getSys()->unaccountedMemory) ABCContextInitEvent(context,false)));
}

DoABCDefineTag::DoABCDefineTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> Flags >> Name;
	LOG(LOG_CALLS,_("DoABCDefineTag Name: ") << Name);

	RootMovieClip* root=getParseThread()->getRootMovie();
	root->incRef();
	context=new ABCContext(_MR(root), in, getVm());

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCDefineTag::execute(RootMovieClip*) const
{
	LOG(LOG_CALLS,_("ABC Exec ") << Name);
	/* currentVM will free the context*/
	getVm()->addEvent(NullRef,_MR(new (getSys()->unaccountedMemory) ABCContextInitEvent(context,((int32_t)Flags)&1)));
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

void SymbolClassTag::execute(RootMovieClip* root) const
{
	LOG(LOG_TRACE,_("SymbolClassTag Exec"));

	for(int i=0;i<NumSymbols;i++)
	{
		LOG(LOG_CALLS,_("Binding ") << Tags[i] << ' ' << Names[i]);
		tiny_string className((const char*)Names[i],true);
		if(Tags[i]==0)
		{
			root->incRef();
			getVm()->addEvent(NullRef, _MR(new (getSys()->unaccountedMemory) BindClassEvent(_MR(root),className)));

		}
		else
		{
			root->addBinding(className, root->dictionaryLookup(Tags[i]));
		}
	}
}

void ScriptLimitsTag::execute(RootMovieClip* root) const
{
	getVm()->limits.max_recursion = MaxRecursionDepth;
	getVm()->limits.script_timeout = ScriptTimeoutSeconds;
}

void ABCVm::registerClasses()
{
	Global* builtin=Class<Global>::getInstanceS((ABCContext*)NULL, 0);
	//Register predefined types, ASObject are enough for not implemented classes
	builtin->registerBuiltin("Object","",Class<ASObject>::getRef());
	builtin->registerBuiltin("Class","",Class_object::getRef());
	builtin->registerBuiltin("Number","",Class<Number>::getRef());
	builtin->registerBuiltin("Boolean","",Class<Boolean>::getRef());
	builtin->registerBuiltin("NaN","",_MR(abstract_d(numeric_limits<double>::quiet_NaN())));
	builtin->registerBuiltin("Infinity","",_MR(abstract_d(numeric_limits<double>::infinity())));
	builtin->registerBuiltin("String","",Class<ASString>::getRef());
	builtin->registerBuiltin("Array","",Class<Array>::getRef());
	builtin->registerBuiltin("Function","",Class<IFunction>::getRef());
	builtin->registerBuiltin("undefined","",_MR(getSys()->getUndefinedRef()));
	builtin->registerBuiltin("Math","",Class<Math>::getRef());
	builtin->registerBuiltin("Namespace","",Class<Namespace>::getRef());
	builtin->registerBuiltin("AS3","",_MR(Class<Namespace>::getInstanceS(AS3)));
	builtin->registerBuiltin("Date","",Class<Date>::getRef());
	builtin->registerBuiltin("RegExp","",Class<RegExp>::getRef());
	builtin->registerBuiltin("QName","",Class<ASQName>::getRef());
	builtin->registerBuiltin("uint","",Class<UInteger>::getRef());
	builtin->registerBuiltin("Vector","__AS3__.vec",_MR(Template<Vector>::getTemplate()));
	//Some instances must be included, they are not created by AS3 code
	builtin->registerBuiltin("Vector$Object","__AS3__.vec",Template<Vector>::getTemplateInstance(Class<ASObject>::getClass()));
	builtin->registerBuiltin("Vector$Number","__AS3__.vec",Template<Vector>::getTemplateInstance(Class<Number>::getClass()));
	builtin->registerBuiltin("Error","",Class<ASError>::getRef());
	builtin->registerBuiltin("SecurityError","",Class<SecurityError>::getRef());
	builtin->registerBuiltin("ArgumentError","",Class<ArgumentError>::getRef());
	builtin->registerBuiltin("DefinitionError","",Class<DefinitionError>::getRef());
	builtin->registerBuiltin("EvalError","",Class<EvalError>::getRef());
	builtin->registerBuiltin("RangeError","",Class<RangeError>::getRef());
	builtin->registerBuiltin("ReferenceError","",Class<ReferenceError>::getRef());
	builtin->registerBuiltin("SyntaxError","",Class<SyntaxError>::getRef());
	builtin->registerBuiltin("TypeError","",Class<TypeError>::getRef());
	builtin->registerBuiltin("URIError","",Class<URIError>::getRef());
	builtin->registerBuiltin("UninitializedError","",Class<UninitializedError>::getRef());
	builtin->registerBuiltin("VerifyError","",Class<VerifyError>::getRef());
	builtin->registerBuiltin("XML","",Class<XML>::getRef());
	builtin->registerBuiltin("XMLList","",Class<XMLList>::getRef());
	builtin->registerBuiltin("int","",Class<Integer>::getRef());

	builtin->registerBuiltin("eval","",_MR(Class<IFunction>::getFunction(eval)));
	builtin->registerBuiltin("print","",_MR(Class<IFunction>::getFunction(print)));
	builtin->registerBuiltin("trace","",_MR(Class<IFunction>::getFunction(trace)));
	builtin->registerBuiltin("parseInt","",_MR(Class<IFunction>::getFunction(parseInt,2)));
	builtin->registerBuiltin("parseFloat","",_MR(Class<IFunction>::getFunction(parseFloat)));
	builtin->registerBuiltin("encodeURI","",_MR(Class<IFunction>::getFunction(encodeURI)));
	builtin->registerBuiltin("decodeURI","",_MR(Class<IFunction>::getFunction(decodeURI)));
	builtin->registerBuiltin("encodeURIComponent","",_MR(Class<IFunction>::getFunction(encodeURIComponent)));
	builtin->registerBuiltin("decodeURIComponent","",_MR(Class<IFunction>::getFunction(decodeURIComponent)));
	builtin->registerBuiltin("escape","",_MR(Class<IFunction>::getFunction(escape)));
	builtin->registerBuiltin("unescape","",_MR(Class<IFunction>::getFunction(unescape)));
	builtin->registerBuiltin("toString","",_MR(Class<IFunction>::getFunction(ASObject::_toString)));

	builtin->registerBuiltin("AccessibilityProperties","flash.accessibility",Class<AccessibilityProperties>::getRef());
	builtin->registerBuiltin("AccessibilityImplementation","flash.accessibility",Class<AccessibilityImplementation>::getRef());

	builtin->registerBuiltin("MovieClip","flash.display",Class<MovieClip>::getRef());
	builtin->registerBuiltin("DisplayObject","flash.display",Class<DisplayObject>::getRef());
	builtin->registerBuiltin("Loader","flash.display",Class<Loader>::getRef());
	builtin->registerBuiltin("LoaderInfo","flash.display",Class<LoaderInfo>::getRef());
	builtin->registerBuiltin("SimpleButton","flash.display",Class<SimpleButton>::getRef());
	builtin->registerBuiltin("InteractiveObject","flash.display",Class<InteractiveObject>::getRef());
	builtin->registerBuiltin("DisplayObjectContainer","flash.display",Class<DisplayObjectContainer>::getRef());
	builtin->registerBuiltin("Sprite","flash.display",Class<Sprite>::getRef());
	builtin->registerBuiltin("Shape","flash.display",Class<Shape>::getRef());
	builtin->registerBuiltin("Stage","flash.display",Class<Stage>::getRef());
	builtin->registerBuiltin("Graphics","flash.display",Class<Graphics>::getRef());
	builtin->registerBuiltin("GradientType","flash.display",Class<GradientType>::getRef());
	builtin->registerBuiltin("BlendMode","flash.display",Class<BlendMode>::getRef());
	builtin->registerBuiltin("LineScaleMode","flash.display",Class<LineScaleMode>::getRef());
	builtin->registerBuiltin("StageScaleMode","flash.display",Class<StageScaleMode>::getRef());
	builtin->registerBuiltin("StageAlign","flash.display",Class<StageAlign>::getRef());
	builtin->registerBuiltin("StageQuality","flash.display",Class<StageQuality>::getRef());
	builtin->registerBuiltin("StageDisplayState","flash.display",Class<StageDisplayState>::getRef());
	builtin->registerBuiltin("BitmapData","flash.display",Class<BitmapData>::getRef());
	builtin->registerBuiltin("Bitmap","flash.display",Class<Bitmap>::getRef());
	builtin->registerBuiltin("IBitmapDrawable","flash.display",InterfaceClass<IBitmapDrawable>::getRef());
	builtin->registerBuiltin("GraphicsGradientFill","flash.display",
			Class<ASObject>::getStubClass(QName("GraphicsGradientFill","flash.display")));
	builtin->registerBuiltin("GraphicsPath","flash.display",
			Class<ASObject>::getStubClass(QName("GraphicsPath","flash.display")));
	builtin->registerBuiltin("MorphShape","flash.display",Class<MorphShape>::getRef());
	builtin->registerBuiltin("SpreadMethod","flash.display",Class<SpreadMethod>::getRef());
	builtin->registerBuiltin("InterpolationMethod","flash.display",Class<InterpolationMethod>::getRef());
	builtin->registerBuiltin("FrameLabel","flash.display",Class<FrameLabel>::getRef());
	builtin->registerBuiltin("Scene","flash.display",Class<Scene>::getRef());
	builtin->registerBuiltin("AVM1Movie","flash.display",Class<AVM1Movie>::getRef());
	builtin->registerBuiltin("Shader","flash.display",Class<Shader>::getRef());

	builtin->registerBuiltin("BitmapFilter","flash.filters",Class<BitmapFilter>::getRef());
	builtin->registerBuiltin("DropShadowFilter","flash.filters",Class<DropShadowFilter>::getRef());
	builtin->registerBuiltin("GlowFilter","flash.filters",Class<GlowFilter>::getRef());
	builtin->registerBuiltin("BevelFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("BevelFilter","flash.filters")));
	builtin->registerBuiltin("ColorMatrixFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("ColorMatrixFilter","flash.filters")));
	builtin->registerBuiltin("BlurFilter","flash.filters",
			Class<ASObject>::getStubClass(QName("BlurFilter","flash.filters")));

	builtin->registerBuiltin("AntiAliasType","flash.text",Class<AntiAliasType>::getRef());
	builtin->registerBuiltin("Font","flash.text",Class<ASFont>::getRef());
	builtin->registerBuiltin("FontStyle","flash.text",Class<FontStyle>::getRef());
	builtin->registerBuiltin("FontType","flash.text",Class<FontType>::getRef());
	builtin->registerBuiltin("GridFitType","flash.text",Class<GridFitType>::getRef());
	builtin->registerBuiltin("StyleSheet","flash.text",Class<StyleSheet>::getRef());
	builtin->registerBuiltin("TextColorType","flash.text",Class<TextColorType>::getRef());
	builtin->registerBuiltin("TextDisplayMode","flash.text",Class<TextDisplayMode>::getRef());
	builtin->registerBuiltin("TextField","flash.text",Class<TextField>::getRef());
	builtin->registerBuiltin("TextFieldType","flash.text",Class<TextFieldType>::getRef());
	builtin->registerBuiltin("TextFieldAutoSize","flash.text",Class<TextFieldAutoSize>::getRef());
	builtin->registerBuiltin("TextFormat","flash.text",Class<TextFormat>::getRef());
	builtin->registerBuiltin("TextFormatAlign","flash.text",Class<TextFormatAlign>::getRef());
	builtin->registerBuiltin("StaticText","flash.text",Class<StaticText>::getRef());

	builtin->registerBuiltin("ContentElement","flash.text.engine",Class<ContentElement>::getRef());
	builtin->registerBuiltin("ElementFormat","flash.text.engine",Class<ElementFormat>::getRef());
	builtin->registerBuiltin("FontDescription","flash.text.engine",Class<FontDescription>::getRef());
	builtin->registerBuiltin("FontWeight","flash.text.engine",Class<FontWeight>::getRef());
	builtin->registerBuiltin("TextBlock","flash.text.engine",Class<TextBlock>::getRef());
	builtin->registerBuiltin("TextElement","flash.text.engine",Class<TextElement>::getRef());
	builtin->registerBuiltin("TextLine","flash.text.engine",Class<TextLine>::getRef());

	builtin->registerBuiltin("XMLDocument","flash.xml",Class<XMLDocument>::getRef());
	builtin->registerBuiltin("XMLNode","flash.xml",Class<XMLNode>::getRef());

	builtin->registerBuiltin("ExternalInterface","flash.external",Class<ExternalInterface>::getRef());

	builtin->registerBuiltin("Endian","flash.utils",Class<Endian>::getRef());
	builtin->registerBuiltin("ByteArray","flash.utils",Class<ByteArray>::getRef());
	builtin->registerBuiltin("Dictionary","flash.utils",Class<Dictionary>::getRef());
	builtin->registerBuiltin("Proxy","flash.utils",Class<Proxy>::getRef());
	builtin->registerBuiltin("Timer","flash.utils",Class<Timer>::getRef());
	builtin->registerBuiltin("getQualifiedClassName","flash.utils",
			_MR(Class<IFunction>::getFunction(getQualifiedClassName)));
	builtin->registerBuiltin("getQualifiedSuperclassName","flash.utils",
			_MR(Class<IFunction>::getFunction(getQualifiedSuperclassName)));
	builtin->registerBuiltin("getDefinitionByName","flash.utils",_MR(Class<IFunction>::getFunction(getDefinitionByName)));
	builtin->registerBuiltin("getTimer","flash.utils",_MR(Class<IFunction>::getFunction(getTimer)));
	builtin->registerBuiltin("setInterval","flash.utils",_MR(Class<IFunction>::getFunction(setInterval)));
	builtin->registerBuiltin("setTimeout","flash.utils",_MR(Class<IFunction>::getFunction(setTimeout)));
	builtin->registerBuiltin("clearInterval","flash.utils",_MR(Class<IFunction>::getFunction(clearInterval)));
	builtin->registerBuiltin("clearTimeout","flash.utils",_MR(Class<IFunction>::getFunction(clearTimeout)));
	builtin->registerBuiltin("describeType","flash.utils",_MR(Class<IFunction>::getFunction(describeType)));
	builtin->registerBuiltin("IExternalizable","flash.utils",InterfaceClass<IExternalizable>::getRef());
	builtin->registerBuiltin("IDataInput","flash.utils",InterfaceClass<IDataInput>::getRef());
	builtin->registerBuiltin("IDataOutput","flash.utils",InterfaceClass<IDataOutput>::getRef());

	builtin->registerBuiltin("ColorTransform","flash.geom",Class<ColorTransform>::getRef());
	builtin->registerBuiltin("Rectangle","flash.geom",Class<Rectangle>::getRef());
	builtin->registerBuiltin("Matrix","flash.geom",Class<Matrix>::getRef());
	builtin->registerBuiltin("Transform","flash.geom",Class<Transform>::getRef());
	builtin->registerBuiltin("Point","flash.geom",Class<Point>::getRef());
	builtin->registerBuiltin("Vector3D","flash.geom",Class<Vector3D>::getRef());
	builtin->registerBuiltin("Matrix3D","flash.geom",Class<ASObject>::getStubClass(QName("Matrix3D", "flash.geom")));

	builtin->registerBuiltin("EventDispatcher","flash.events",Class<EventDispatcher>::getRef());
	builtin->registerBuiltin("Event","flash.events",Class<Event>::getRef());
	builtin->registerBuiltin("EventPhase","flash.events",Class<EventPhase>::getRef());
	builtin->registerBuiltin("MouseEvent","flash.events",Class<MouseEvent>::getRef());
	builtin->registerBuiltin("ProgressEvent","flash.events",Class<ProgressEvent>::getRef());
	builtin->registerBuiltin("TimerEvent","flash.events",Class<TimerEvent>::getRef());
	builtin->registerBuiltin("IOErrorEvent","flash.events",Class<IOErrorEvent>::getRef());
	builtin->registerBuiltin("ErrorEvent","flash.events",Class<ErrorEvent>::getRef());
	builtin->registerBuiltin("SecurityErrorEvent","flash.events",Class<SecurityErrorEvent>::getRef());
	builtin->registerBuiltin("AsyncErrorEvent","flash.events",Class<AsyncErrorEvent>::getRef());
	builtin->registerBuiltin("FullScreenEvent","flash.events",Class<FullScreenEvent>::getRef());
	builtin->registerBuiltin("TextEvent","flash.events",Class<TextEvent>::getRef());
	builtin->registerBuiltin("IEventDispatcher","flash.events",InterfaceClass<IEventDispatcher>::getRef());
	builtin->registerBuiltin("FocusEvent","flash.events",Class<FocusEvent>::getRef());
	builtin->registerBuiltin("NetStatusEvent","flash.events",Class<NetStatusEvent>::getRef());
	builtin->registerBuiltin("HTTPStatusEvent","flash.events",Class<HTTPStatusEvent>::getRef());
	builtin->registerBuiltin("KeyboardEvent","flash.events",Class<KeyboardEvent>::getRef());
	builtin->registerBuiltin("StatusEvent","flash.events",Class<StatusEvent>::getRef());
	builtin->registerBuiltin("DataEvent","flash.events",Class<DataEvent>::getRef());
	builtin->registerBuiltin("DRMErrorEvent","flash.events",Class<DRMErrorEvent>::getRef());
	builtin->registerBuiltin("DRMStatusEvent","flash.events",Class<DRMStatusEvent>::getRef());
	builtin->registerBuiltin("StageVideoEvent","flash.events",Class<StageVideoEvent>::getRef());
	builtin->registerBuiltin("StageVideoAvailabilityEvent","flash.events",Class<StageVideoAvailabilityEvent>::getRef());

	builtin->registerBuiltin("sendToURL","flash.net",_MR(Class<IFunction>::getFunction(sendToURL)));
	builtin->registerBuiltin("LocalConnection","flash.net",Class<ASObject>::getStubClass(QName("LocalConnection","flash.net")));
	builtin->registerBuiltin("NetConnection","flash.net",Class<NetConnection>::getRef());
	builtin->registerBuiltin("NetStream","flash.net",Class<NetStream>::getRef());
	builtin->registerBuiltin("NetStreamPlayOptions","flash.net",Class<ASObject>::getStubClass(QName("NetStreamPlayOptions","flash.net")));
	builtin->registerBuiltin("URLLoader","flash.net",Class<URLLoader>::getRef());
	builtin->registerBuiltin("URLStream","flash.net",Class<URLStream>::getRef());
	builtin->registerBuiltin("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getRef());
	builtin->registerBuiltin("URLRequest","flash.net",Class<URLRequest>::getRef());
	builtin->registerBuiltin("URLRequestHeader","flash.net",Class<URLRequestHeader>::getRef());
	builtin->registerBuiltin("URLRequestMethod","flash.net",Class<URLRequestMethod>::getRef());
	builtin->registerBuiltin("URLVariables","flash.net",Class<URLVariables>::getRef());
	builtin->registerBuiltin("SharedObject","flash.net",Class<SharedObject>::getRef());
	builtin->registerBuiltin("ObjectEncoding","flash.net",Class<ObjectEncoding>::getRef());
	builtin->registerBuiltin("Socket","flash.net",Class<ASObject>::getStubClass(QName("Socket","flash.net")));
	builtin->registerBuiltin("Responder","flash.net",Class<Responder>::getRef());
	builtin->registerBuiltin("XMLSocket","flash.net",Class<XMLSocket>::getRef());
	builtin->registerBuiltin("registerClassAlias","flash.net",_MR(Class<IFunction>::getFunction(registerClassAlias)));
	builtin->registerBuiltin("getClassByAlias","flash.net",_MR(Class<IFunction>::getFunction(getClassByAlias)));

	builtin->registerBuiltin("fscommand","flash.system",_MR(Class<IFunction>::getFunction(fscommand)));
	builtin->registerBuiltin("Capabilities","flash.system",Class<Capabilities>::getRef());
	builtin->registerBuiltin("Security","flash.system",Class<Security>::getRef());
	builtin->registerBuiltin("ApplicationDomain","flash.system",Class<ApplicationDomain>::getRef());
	builtin->registerBuiltin("SecurityDomain","flash.system",Class<SecurityDomain>::getRef());
	builtin->registerBuiltin("LoaderContext","flash.system",Class<LoaderContext>::getRef());
	builtin->registerBuiltin("System","flash.system",Class<System>::getRef());

	builtin->registerBuiltin("SoundTransform","flash.media",Class<SoundTransform>::getRef());
	builtin->registerBuiltin("Video","flash.media",Class<Video>::getRef());
	builtin->registerBuiltin("Sound","flash.media",Class<Sound>::getRef());
	builtin->registerBuiltin("SoundLoaderContext","flash.media",Class<SoundLoaderContext>::getRef());
	builtin->registerBuiltin("SoundChannel","flash.media",Class<SoundChannel>::getRef());
	builtin->registerBuiltin("StageVideo","flash.media",Class<StageVideo>::getRef());
	builtin->registerBuiltin("StageVideoAvailability","flash.media",Class<StageVideoAvailability>::getRef());
	builtin->registerBuiltin("VideoStatus","flash.media",Class<VideoStatus>::getRef());

	builtin->registerBuiltin("Keyboard","flash.ui",Class<ASObject>::getStubClass(QName("Keyboard","flash.ui")));
	builtin->registerBuiltin("ContextMenu","flash.ui",Class<ASObject>::getStubClass(QName("ContextMenu","flash.ui")));
	builtin->registerBuiltin("ContextMenuItem","flash.ui",Class<ASObject>::getStubClass(QName("ContextMenuItem","flash.ui")));

	builtin->registerBuiltin("Accelerometer", "flash.sensors",Class<Accelerometer>::getRef());

	builtin->registerBuiltin("IOError","flash.errors",Class<IOError>::getRef());
	builtin->registerBuiltin("EOFError","flash.errors",Class<EOFError>::getRef());
	builtin->registerBuiltin("IllegalOperationError","flash.errors",Class<IllegalOperationError>::getRef());
	builtin->registerBuiltin("InvalidSWFError","flash.errors",Class<InvalidSWFError>::getRef());
	builtin->registerBuiltin("MemoryError","flash.errors",Class<MemoryError>::getRef());
	builtin->registerBuiltin("ScriptTimeoutError","flash.errors",Class<ScriptTimeoutError>::getRef());
	builtin->registerBuiltin("StackOverflowError","flash.errors",Class<StackOverflowError>::getRef());

	builtin->registerBuiltin("isNaN","",_MR(Class<IFunction>::getFunction(isNaN)));
	builtin->registerBuiltin("isFinite","",_MR(Class<IFunction>::getFunction(isFinite)));
	builtin->registerBuiltin("isXMLName","",_MR(Class<IFunction>::getFunction(_isXMLName)));

	//If needed add AIR definitions
	if(getSys()->flashMode==SystemState::AIR)
	{
		builtin->registerBuiltin("NativeApplication","flash.desktop",Class<NativeApplication>::getRef());

		builtin->registerBuiltin("InvokeEvent","flash.events",Class<InvokeEvent>::getRef());

		builtin->registerBuiltin("FileStream","flash.filesystem",
				Class<ASObject>::getStubClass(QName("FileStream","flash.filestream")));
	}

	getSys()->systemDomain->registerGlobalScope(builtin);
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
		m->cached=new (getVm()->vmDataMemory) multiname(getVm()->vmDataMemory);
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
					ret->ns.push_back(nsNameAndKind(th->context, s->ns[i]));
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
		m->cached=new (getVm()->vmDataMemory) multiname(getVm()->vmDataMemory);
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
					ret->ns.push_back(nsNameAndKind(th->context, s->ns[i]));
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
		m->cached=new (getVm()->vmDataMemory) multiname(getVm()->vmDataMemory);
		ret=m->cached;
		if(midx==0)
		{
			ret->name_s_id=getSys()->getUniqueStringId("any");
			ret->name_type=multiname::NAME_STRING;
			ret->ns.emplace_back(nsNameAndKind("",NAMESPACE));
			ret->isAttribute=false;
			return ret;
		}
		ret->isAttribute=m->isAttributeName();
		switch(m->kind)
		{
			case 0x07: //QName
			case 0x0D: //QNameA
			{
				ret->ns.push_back(nsNameAndKind(this, m->ns));

				ret->name_s_id=getSys()->getUniqueStringId(getString(m->name));
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
					ret->ns.push_back(nsNameAndKind(this, s->ns[i]));
				}
				sort(ret->ns.begin(),ret->ns.end());

				ret->name_s_id=getSys()->getUniqueStringId(getString(m->name));
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
					ret->ns.push_back(nsNameAndKind(this, s->ns[i]));
				}
				sort(ret->ns.begin(),ret->ns.end());
				break;
			}
			case 0x0f: //RTQName
			case 0x10: //RTQNameA
			{
				ret->name_type=multiname::NAME_STRING;
				ret->name_s_id=getSys()->getUniqueStringId(getString(m->name));
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
				ret->ns.push_back(nsNameAndKind(this, td->ns));
				ret->name_s_id=getSys()->getUniqueStringId(name);
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

ABCContext::ABCContext(_R<RootMovieClip> r, istream& in, ABCVm* vm):root(r),constant_pool(vm->vmDataMemory),
	methods(reporter_allocator<method_info>(vm->vmDataMemory)),
	metadata(reporter_allocator<metadata_info>(vm->vmDataMemory)),
	instances(reporter_allocator<instance_info>(vm->vmDataMemory)),
	classes(reporter_allocator<class_info>(vm->vmDataMemory)),
	scripts(reporter_allocator<script_info>(vm->vmDataMemory)),
	method_body(reporter_allocator<method_body_info>(vm->vmDataMemory))
{
	in >> minor >> major;
	LOG(LOG_CALLS,_("ABCVm version ") << major << '.' << minor);
	in >> constant_pool;

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
	getSys()->contextes.push_back(this);
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

/*
 * nextNamespaceBase is set to 1 since 0 is the empty namespace
 */
ABCVm::ABCVm(SystemState* s, MemoryAccount* m):m_sys(s),status(CREATED),shuttingdown(false),
	events_queue(reporter_allocator<eventType>(m)),nextNamespaceBase(1),currentCallContext(NULL),
	vmDataMemory(m),cur_recursion(0)
{
	limits.max_recursion = 256;
	limits.script_timeout = 20;
	m_sys=s;
}

void ABCVm::start()
{
	status=STARTED;
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	t = Thread::create(sigc::bind(&Run,this));
#else
	t = Thread::create(sigc::bind(&Run,this),true);
#endif
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
		t->join();
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
}

int ABCVm::getEventQueueSize()
{
	return events_queue.size();
}

void ABCVm::publicHandleEvent(_R<EventDispatcher> dispatcher, _R<Event> event)
{
	std::deque<_R<DisplayObject>> parents;
	//Only set the default target is it's not overridden
	if(event->target.isNull())
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
			{
				//no need to lock as this is the vm thread
				shuttingdown=true;
				break;
			}
			case FUNCTION:
			{
				FunctionEvent* ev=static_cast<FunctionEvent*>(e.second.getPtr());
				try
				{
					if(!ev->obj.isNull())
						ev->obj->incRef();
					ASObject* result = ev->f->call(ev->obj.getPtr(),ev->args,ev->numArgs);
					if(result)
						result->decRef();
				}
				catch(ASObject* exception)
				{
					//Exception unhandled, report up
					ev->done.signal();
					throw;
				}
				catch(LightsparkException& e)
				{
					//An internal error happended, sync and rethrow
					ev->done.signal();
					throw;
				}
				break;
			}
			case EXTERNAL_CALL:
			{
				ExternalCallEvent* ev=static_cast<ExternalCallEvent*>(e.second.getPtr());
				try
				{
					*(ev->result) = ev->f->call(getSys()->getNullRef(),ev->args,ev->numArgs);
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
					ev->done.signal();
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
			case INIT_FRAME:
			{
				InitFrameEvent* ev=static_cast<InitFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"INIT_FRAME");
				assert(!ev->clip.isNull());
				ev->clip->initFrame();
				break;
			}
			case ADVANCE_FRAME:
			{
				AdvanceFrameEvent* ev=static_cast<AdvanceFrameEvent*>(e.second.getPtr());
				LOG(LOG_CALLS,"ADVANCE_FRAME");
				m_sys->mainClip->getStage()->advanceFrame();
				ev->done.signal(); // Won't this signal twice, wrt to the signal() below?
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
			default:
				assert(false);
		}
	}

	/* If this was a waitable event, signal it */
	if(e.second->is<WaitableEvent>())
		e.second->as<WaitableEvent>()->done.signal();
}

/*! \brief enqueue an event, a reference is acquired
* * \param obj EventDispatcher that will receive the event
* * \param ev event that will be sent */
bool ABCVm::addEvent(_NR<EventDispatcher> obj ,_R<Event> ev)
{
	/* We have to run waitable events directly,
	 * because otherwise waiting on them in the vm thread
	 * will block the vm thread from executing them.
	 */
	if(isVmThread() && ev->is<WaitableEvent>())
	{
		handleEvent( make_pair(obj,ev) );
		return true;
	}


	Mutex::Lock l(event_queue_mutex);

	//If the system should terminate new events are not accepted
	if(shuttingdown)
		return false;

	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	sem_event_cond.signal();
	return true;
}

Class_inherit* ABCVm::findClassInherit(const string& s, RootMovieClip* root)
{
	LOG(LOG_CALLS,_("Setting class name to ") << s);
	ASObject* target;
	ASObject* derived_class=root->applicationDomain->getVariableByString(s,target);
	if(derived_class==NULL)
	{
		LOG(LOG_ERROR,_("Class ") << s << _(" not found in global for ")<<root->getOrigin());
		throw RunTimeException("Class not found in global");
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
	Class_inherit* derived_class_tmp = findClassInherit(s, base.getPtr());
	if(!derived_class_tmp)
		return;

	//Let's override the class
	base->setClass(derived_class_tmp);
	derived_class_tmp->bindToRoot();
}

void ABCVm::buildClassAndBindTag(const string& s, DictionaryTag* t)
{
	Class_inherit* derived_class_tmp = findClassInherit(s, t->loadedFrom);
	if(!derived_class_tmp)
		return;

	//It seems to be acceptable for the same base to be binded multiple times.
	//In such cases the first binding is bidirectional (instances created using PlaceObject
	//use the binded class and instances created using 'new' use the binded tag). Any other
	//bindings will be unidirectional (only instances created using new will use the binded tag)
	if(t->bindedTo==NULL)
		t->bindedTo=derived_class_tmp;

	derived_class_tmp->bindToTag(t);
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

	//TODO: Should check against multiname index being 0, not the name!
	if(name->qualifiedString() == "any")
		return true;
	
	ASObject* target;
	ASObject* ret=root->applicationDomain->getVariableAndTargetByMultiname(*name, target);
	if(!ret) //Could not retrieve type
	{
		LOG(LOG_ERROR,_("Cannot retrieve type"));
		return false;
	}

	ASObject* type=ret;
	bool real_ret=false;
	Class_base* objc=obj->classdef;
	Class_base* c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		real_ret=(c==Class<Integer>::getClass() || c==Class<Number>::getClass() || c==Class<UInteger>::getClass());
		LOG(LOG_CALLS,_("Numeric type is ") << ((real_ret)?"_(":")not _(") << ")subclass of " << c->class_name);
		return real_ret;
	}

	if(!objc)
	{
		real_ret=obj->getObjectType()==type->getObjectType();
		LOG(LOG_CALLS,_("isType on non classed object ") << real_ret);
		return real_ret;
	}

	assert_and_throw(type->getObjectType()==T_CLASS);
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
		Global* global=Class<Global>::getInstanceS(this, i);
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
		root->applicationDomain->registerGlobalScope(global);
	}
	//The last script entry has to be run
	LOG(LOG_CALLS, _("Last script (Entry Point)"));
	//Creating a new global for the last script
	Global* global=Class<Global>::getInstanceS(this, i);
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
	root->applicationDomain->registerGlobalScope(global);
	//the script init of the last script is the main entry point
	if(!lazy)
		runScriptInit(i, global);
	LOG(LOG_CALLS, _("End of Entry Point"));
}

void ABCContext::runScriptInit(unsigned int i, ASObject* g)
{
	LOG(LOG_CALLS, "Running script init for script " << i );

	assert(!hasRunScriptInit[i]);
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
	setTLSSys(th->m_sys);
	while(getVm()!=th);

	/* set TLS variable for isVmThread() */
        g_static_private_set(&is_vm_thread,(void*)1,NULL);

	if(th->m_sys->useJit)
	{
#ifdef LLVM_31
		llvm::TargetOptions Opts;
		Opts.JITExceptionHandling = true;
#else
		llvm::JITExceptionHandling = true;
#endif
#ifndef NDEBUG
#ifdef LLVM_31
		Opts.JITEmitDebugInfo = true;
#else
		llvm::JITEmitDebugInfo = true;
#endif
#endif
		llvm::InitializeNativeTarget();
		th->module=new llvm::Module(llvm::StringRef("abc jit"),th->llvm_context());
		llvm::EngineBuilder eb(th->module);
		eb.setEngineKind(llvm::EngineKind::JIT);
#ifdef LLVM_31
		eb.setTargetOptions(Opts);
#endif
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
				LOG(LOG_INFO,th->events_queue.size() << _(" events missing before exit"));
				firstMissingEvents = false;
			}
		}
		Chronometer chronometer;
		pair<_NR<EventDispatcher>,_R<Event>> e=th->events_queue.front();
		th->events_queue.pop_front();

		th->event_queue_mutex.unlock();
		try
		{
			//handle event without lock
			th->handleEvent(e);
			//Flush the invalidation queue
			th->m_sys->flushInvalidationQueue();
			profile->accountTime(chronometer.checkpoint());
#ifdef MEMORY_USAGE_PROFILING
			if((snapshotCount%100)==0)
				th->m_sys->saveMemoryUsageInformation(memoryProfile, snapshotCount);
			snapshotCount++;
#endif
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,_("Error in VM ") << e.cause);
			th->m_sys->setError(e.cause);
			/* do not allow any more event to be enqueued */
			th->shuttingdown = true;
			th->signalEventWaiters();
			break;
		}
		catch(ASObject*& e)
		{
			th->shuttingdown = true;
			if(e->getClass())
				LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM ") << e->toString());
			else
				LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM (no type)"));
			th->m_sys->setError(_("Unhandled ActionScript exception"));
			/* do not allow any more event to be enqueued */
			th->shuttingdown = true;
			th->signalEventWaiters();
			break;
		}
	}
	if(th->m_sys->useJit)
	{
		th->ex->clearAllGlobalMappings();
		delete th->module;
	}
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
			e.second->as<WaitableEvent>()->done.signal();
	}
}

void ABCVm::parseRPCMessage(_R<ByteArray> message, _NR<ASObject> client, _NR<Responder> responder)
{
	uint16_t version;
	if(!message->readShort(version))
		return;
	assert_and_throw(version==0x0);
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
		headerName.ns.push_back(nsNameAndKind("",NAMESPACE));
		tiny_string headerNameString;
		if(!message->readUTF(headerNameString))
			return;
		headerName.name_s_id=getSys()->getUniqueStringId(headerNameString);
		//Read the must understand flag
		uint8_t mustUnderstand;
		if(!message->readByte(mustUnderstand))
			return;
		//Read the header length, not really useful
		uint32_t headerLength;
		if(!message->readUnsignedInt(headerLength))
			return;

		uint8_t marker;
		if(!message->readByte(marker))
			return;
		assert_and_throw(marker==0x11);

		_R<ASObject> obj=_MR(ByteArray::readObject(message.getPtr(), NULL, 0));

		_NR<ASObject> callback;
		if(!client.isNull())
			callback = client->getVariableByMultiname(headerName);

		//If mustUnderstand is set there must be a suitable callback on the client
		if(mustUnderstand && (client.isNull() || callback.isNull() || callback->getObjectType()!=T_FUNCTION))
		{
			//TODO: use onStatus
			throw UnsupportedException("Unsupported header with mustUnderstand");
		}

		if(!callback.isNull())
		{
			obj->incRef();
			ASObject* const callbackArgs[1] {obj.getPtr()};
			client->incRef();
			callback->as<IFunction>()->call(client.getPtr(), callbackArgs, 1);
		}
	}
	uint16_t numMessage;
	if(!message->readShort(numMessage))
		return;
	assert_and_throw(numMessage==1);

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
	if(!message->readByte(marker))
		return;
	assert_and_throw(marker==0x11);
	_R<ASObject> ret=_MR(ByteArray::readObject(message.getPtr(), NULL, 0));

	if(!responder.isNull())
	{
		multiname onResultName(NULL);
		onResultName.name_type=multiname::NAME_STRING;
		onResultName.name_s_id=getSys()->getUniqueStringId("onResult");
		onResultName.ns.push_back(nsNameAndKind("",NAMESPACE));
		_NR<ASObject> callback = responder->getVariableByMultiname(onResultName);
		if(!callback.isNull() && callback->getObjectType() == T_FUNCTION)
		{
			ret->incRef();
			ASObject* const callbackArgs[1] {ret.getPtr()};
			responder->incRef();
			callback->as<IFunction>()->call(responder.getPtr(), callbackArgs, 1);
		}
	}
}

_R<ApplicationDomain> ABCVm::getCurrentApplicationDomain(call_context* th)
{
	return th->context->root->applicationDomain;
}

_R<SecurityDomain> ABCVm::getCurrentSecurityDomain(call_context* th)
{
	return th->context->root->securityDomain;
}

uint32_t ABCVm::getAndIncreaseNamespaceBase(uint32_t nsNum)
{
	return ATOMIC_ADD(nextNamespaceBase,nsNum)-nsNum;
}

tiny_string ABCVm::getDefaultXMLNamespace()
{
	return currentCallContext->defaultNamespaceUri;
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

	uint32_t nameId=mname.name_s_id;
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
			var = c->borrowedVariables.findObjVar(nameId,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && var->var)
			{
				assert_and_throw(var->var && var->var->getObjectType()==T_FUNCTION);

				IFunction* f=static_cast<IFunction*>(var->var);
				f->incRef();
				c->setDeclaredMethodByQName(nameId,mname.ns[0],f,NORMAL_METHOD,true);
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
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && var->getter)
			{
				assert_and_throw(var->getter);

				var->getter->incRef();
				c->setDeclaredMethodByQName(nameId,mname.ns[0],var->getter,GETTER_METHOD,true);
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
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && var->setter)
			{
				assert_and_throw(var->setter);

				var->setter->incRef();
				c->setDeclaredMethodByQName(nameId,mname.ns[0],var->setter,SETTER_METHOD,true);
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
			return getSys()->getUndefinedRef();
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
			return getSys()->getNullRef();
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
				QName className(getSys()->getStringFromUniqueId(mname->name_s_id),mname->ns[0].getImpl().name);

				MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(className.name);
				Class_inherit* ci=new (getSys()->unaccountedMemory) Class_inherit(className, memoryAccount);
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
					ci->initializeProtectedNamespace(getString(ns_info.name),ns_info);
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
			else
				ret=getSys()->getUndefinedRef();

			obj->setVariableByQName(mname->name_s_id,mname->ns[0],ret,DECLARED_TRAIT);

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
				if(isBorrowed)
				{
					obj->incRef();
					f->addToScope(scope_entry(_MR(obj),false));
				}
			}
			else
			{
				assert(scriptid != -1);
				obj->incRef();
				f->addToScope(scope_entry(_MR(obj),false));
			}
			//TODO: Avoid string lookup
			if(kind == traits_info::Getter)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,GETTER_METHOD,isBorrowed);
			else if(kind == traits_info::Setter)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,SETTER_METHOD,isBorrowed);
			else if(kind == traits_info::Method)
				obj->setDeclaredMethodByQName(mname->name_s_id,mname->ns[0],f,NORMAL_METHOD,isBorrowed);
			break;
		}
		case traits_info::Const:
		{
			//Check if this already defined in upper levels
			_NR<ASObject> tmpo=obj->getVariableByMultiname(*mname,ASObject::SKIP_IMPL);
			if(!tmpo.isNull())
				return;

			multiname* tname=getMultiname(t->type_name,NULL);
			ASObject* ret;
			//If the index is valid we set the constant
			if(t->vindex)
				ret=getConstant(t->vkind,t->vindex);
			else
				ret=getSys()->getUndefinedRef();

			LOG(LOG_CALLS,_("Const ") << *mname <<_(" type ")<< *tname<< " = " << ret->toDebugString());

			obj->initializeVariableByMultiname(*mname, ret, tname, this, CONSTANT_TRAIT);

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
				LOG(LOG_CALLS,_("Slot ") << t->slot_id << ' ' << *mname <<_(" type ")<<*tname<< " = " << ret->toDebugString() );
			}
			else
			{
				LOG(LOG_CALLS,_("Slot ")<< t->slot_id<<  _(" vindex 0 ") << *mname <<_(" type ")<<*tname);
				//The Undefined is coerced to the right type by the initializeVar..
				ret = getSys()->getUndefinedRef();
			}

			obj->initializeVariableByMultiname(*mname, ret, tname, this, DECLARED_TRAIT);

			if(t->slot_id)
				obj->initSlot(t->slot_id, *mname);

			break;
		}
		default:
			LOG(LOG_ERROR,_("Trait not supported ") << *mname << _(" ") << t->kind);
			obj->setVariableByMultiname(*mname, getSys()->getUndefinedRef(), ASObject::CONST_NOT_ALLOWED);
	}
}


ASObject* method_info::getOptional(unsigned int i)
{
	assert_and_throw(i<info.options.size());
	return context->getConstant(info.options[i].kind,info.options[i].val);
}

const multiname* method_info::paramTypeName(unsigned int i) const
{
	assert_and_throw(i<info.param_type.size());
	return context->getMultiname(info.param_type[i],NULL);
}

const multiname* method_info::returnTypeName() const
{
	return context->getMultiname(info.return_type,NULL);
}

istream& lightspark::operator>>(istream& in, method_info& v)
{
	return in >> v.info;
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
