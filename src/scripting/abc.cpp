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
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/JSON.h"
#include "scripting/toplevel/Math.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/concurrent/Mutex.h"
#include "scripting/flash/concurrent/Condition.h"
#include "scripting/flash/crypto/flashcrypto.h"
#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/desktop/clipboardformats.h"
#include "scripting/flash/desktop/clipboardtransfermode.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/bitmapencodingcolorspace.h"
#include "scripting/flash/display/colorcorrection.h"
#include "scripting/flash/display/ColorCorrectionSupport.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/display/GraphicsBitmapFill.h"
#include "scripting/flash/display/GraphicsEndFill.h"
#include "scripting/flash/display/GraphicsGradientFill.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/flash/display/GraphicsShaderFill.h"
#include "scripting/flash/display/GraphicsSolidFill.h"
#include "scripting/flash/display/GraphicsStroke.h"
#include "scripting/flash/display/GraphicsTrianglePath.h"
#include "scripting/flash/display/IGraphicsData.h"
#include "scripting/flash/display/IGraphicsFill.h"
#include "scripting/flash/display/IGraphicsPath.h"
#include "scripting/flash/display/IGraphicsStroke.h"
#include "scripting/flash/display/jpegencoderoptions.h"
#include "scripting/flash/display/jpegxrencoderoptions.h"
#include "scripting/flash/display/pngencoderoptions.h"
#include "scripting/flash/display/shaderparametertype.h"
#include "scripting/flash/display/shaderprecision.h"
#include "scripting/flash/display/swfversion.h"
#include "scripting/flash/display/triangleculling.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/display3d/flashdisplay3dtextures.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/filesystem/flashfilesystem.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/XMLSocket.h"
#include "scripting/flash/net/NetStreamInfo.h"
#include "scripting/flash/net/NetStreamPlayOptions.h"
#include "scripting/flash/net/NetStreamPlayTransitions.h"
#include "scripting/flash/printing/flashprinting.h"
#include "scripting/flash/sampler/flashsampler.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/sensors/flashsensors.h"
#include "scripting/flash/utils/flashutils.h"
#include "scripting/flash/utils/CompressionAlgorithm.h"
#include "scripting/flash/utils/Dictionary.h"
#include "scripting/flash/utils/Proxy.h"
#include "scripting/flash/utils/Timer.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/orientation3d.h"
#include "scripting/flash/globalization/collator.h"
#include "scripting/flash/globalization/datetimeformatter.h"
#include "scripting/flash/globalization/datetimestyle.h"
#include "scripting/flash/globalization/collator.h"
#include "scripting/flash/globalization/collatormode.h"
#include "scripting/flash/globalization/datetimenamecontext.h"
#include "scripting/flash/globalization/datetimenamestyle.h"
#include "scripting/flash/globalization/nationaldigitstype.h"
#include "scripting/flash/globalization/lastoperationstatus.h"
#include "scripting/flash/globalization/localeid.h"
#include "scripting/flash/globalization/currencyformatter.h"
#include "scripting/flash/globalization/numberformatter.h"
#include "scripting/flash/globalization/stringtools.h"
#include "scripting/flash/external/ExternalInterface.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/media/audiodecoder.h"
#include "scripting/flash/media/audiooutputchangereason.h"
#include "scripting/flash/media/avnetworkingparams.h"
#include "scripting/flash/media/h264level.h"
#include "scripting/flash/media/h264profile.h"
#include "scripting/flash/media/microphoneenhancedmode.h"
#include "scripting/flash/media/microphoneenhancedoptions.h"
#include "scripting/flash/media/soundcodec.h"
#include "scripting/flash/media/stagevideoavailabilityreason.h"
#include "scripting/flash/media/videodecoder.h"
#include "scripting/flash/media/avtagdata.h"
#include "scripting/flash/media/id3info.h"
#include "scripting/flash/net/netgroupreceivemode.h"
#include "scripting/flash/net/netgroupreplicationstrategy.h"
#include "scripting/flash/net/netgroupsendmode.h"
#include "scripting/flash/net/netgroupsendresult.h"
#include "scripting/flash/security/certificatestatus.h"
#include "scripting/flash/system/messagechannelstate.h"
#include "scripting/flash/system/securitypanel.h"
#include "scripting/flash/system/systemupdater.h"
#include "scripting/flash/system/systemupdatertype.h"
#include "scripting/flash/system/touchscreentype.h"
#include "scripting/flash/system/ime.h"
#include "scripting/flash/system/jpegloadercontext.h"
#include "scripting/flash/xml/flashxml.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/text/csmsettings.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/text/flashtextengine.h"
#include "scripting/flash/text/textrenderer.h"
#include "scripting/flash/ui/Keyboard.h"
#include "scripting/flash/ui/Mouse.h"
#include "scripting/flash/ui/Multitouch.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/ContextMenuItem.h"
#include "scripting/flash/ui/ContextMenuBuiltInItems.h"
#include "scripting/flash/ui/gameinput.h"
#include "scripting/avmplus/avmplus.h"
#include "scripting/avm1/avm1key.h"
#include "scripting/avm1/avm1sound.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1media.h"
#include "scripting/avm1/avm1net.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/avm1/avm1xml.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "scripting/abc.h"
#include"backends/rendering.h"

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

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	LOG(LOG_CALLS,_("DoABCTag"));

	RootMovieClip* root=getParseThread()->getRootMovie();
	root->incRef();
	context=new ABCContext(_MR(root), in, getVm(root->getSystemState()));

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCTag::execute(RootMovieClip* root) const
{
	LOG(LOG_CALLS,_("ABC Exec"));
	/* currentVM will free the context*/
	if (!getWorker() || getWorker()->isPrimordial)
		getVm(root->getSystemState())->addEvent(NullRef,_MR(new (root->getSystemState()->unaccountedMemory) ABCContextInitEvent(context,false)));
	else
		context->exec(false);
}

DoABCDefineTag::DoABCDefineTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> Flags >> Name;
	LOG(LOG_CALLS,_("DoABCDefineTag Name: ") << Name);

	RootMovieClip* root=getParseThread()->getRootMovie();
	root->incRef();
	context=new ABCContext(_MR(root), in, getVm(root->getSystemState()));

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,_("Corrupted ABC data: missing ") << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCDefineTag::execute(RootMovieClip* root) const
{
	LOG(LOG_CALLS,_("ABC Exec ") << Name);
	/* currentVM will free the context*/
	// some swf files have multiple abc tags without the "lazy" flag.
	// if the swf file also has a SymbolClass, we just ignore them and execute all abc tags lazy.
	// the real start of the main class is done when the symbol with id 0 is detected in SymbolClass tag
	bool lazy = root->hasSymbolClass || ((int32_t)Flags)&1;
	if (root == root->getSystemState()->mainClip && (!getWorker() || getWorker()->isPrimordial))
		getVm(root->getSystemState())->addEvent(NullRef,_MR(new (root->getSystemState()->unaccountedMemory) ABCContextInitEvent(context,lazy)));
	else
		context->exec(lazy);
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
			root->hasMainClass=true;
			root->incRef();
			getVm(root->getSystemState())->addEvent(NullRef, _MR(new (root->getSystemState()->unaccountedMemory) BindClassEvent(_MR(root),className)));
		}
		else
		{
			root->addBinding(className, root->dictionaryLookup(Tags[i]));
		}
	}
}

void ScriptLimitsTag::execute(RootMovieClip* root) const
{
	if (root != root->getSystemState()->mainClip)
		return;
	if (MaxRecursionDepth > getVm(root->getSystemState())->limits.max_recursion)
	{
		delete[] getVm(root->getSystemState())->stacktrace;
		getVm(root->getSystemState())->stacktrace = new ABCVm::stacktrace_entry[MaxRecursionDepth];
	}
	getVm(root->getSystemState())->limits.max_recursion = MaxRecursionDepth;
	getVm(root->getSystemState())->limits.script_timeout = ScriptTimeoutSeconds;
}

void ABCVm::registerClassesAVM1()
{
	Global* builtinavm1 = Class<Global>::getInstanceS(m_sys,(ABCContext*)nullptr, 0);

	registerClassesToplevel(builtinavm1);

	if (!m_sys->mainClip->usesActionScript3)
	{
		Class<ASObject>::getRef(m_sys)->setDeclaredMethodByQName("addProperty","",Class<IFunction>::getFunction(m_sys,ASObject::addProperty),NORMAL_METHOD,true);
		Class<ASObject>::getRef(m_sys)->prototype->setVariableByQName("addProperty","",Class<IFunction>::getFunction(m_sys,ASObject::addProperty),DYNAMIC_TRAIT);
		Class<ASObject>::getRef(m_sys)->setDeclaredMethodByQName("registerClass","",Class<IFunction>::getFunction(m_sys,ASObject::registerClass),NORMAL_METHOD,false);
		Class<ASObject>::getRef(m_sys)->prototype->setVariableByQName("registerClass","",Class<IFunction>::getFunction(m_sys,ASObject::registerClass),DYNAMIC_TRAIT);
	}

	builtinavm1->registerBuiltin("ASSetPropFlags","",_MR(Class<IFunction>::getFunction(m_sys,AVM1_ASSetPropFlags)));
	builtinavm1->registerBuiltin("setInterval","",_MR(Class<IFunction>::getFunction(m_sys,setInterval)));
	builtinavm1->registerBuiltin("clearInterval","",_MR(Class<IFunction>::getFunction(m_sys,clearInterval)));

	builtinavm1->registerBuiltin("object","",Class<ASObject>::getRef(m_sys));
	builtinavm1->registerBuiltin("Button","",Class<SimpleButton>::getRef(m_sys));
	builtinavm1->registerBuiltin("Color","",Class<AVM1Color>::getRef(m_sys));
	builtinavm1->registerBuiltin("Mouse","",Class<AVM1Mouse>::getRef(m_sys));
	builtinavm1->registerBuiltin("Sound","",Class<AVM1Sound>::getRef(m_sys));
	builtinavm1->registerBuiltin("MovieClip","",Class<AVM1MovieClip>::getRef(m_sys));
	builtinavm1->registerBuiltin("MovieClipLoader","",Class<AVM1MovieClipLoader>::getRef(m_sys));
	builtinavm1->registerBuiltin("Key","",Class<AVM1Key>::getRef(m_sys));
	builtinavm1->registerBuiltin("Stage","",Class<AVM1Stage>::getRef(m_sys));
	builtinavm1->registerBuiltin("SharedObject","",Class<AVM1SharedObject>::getRef(m_sys));
	builtinavm1->registerBuiltin("ContextMenu","",Class<ContextMenu>::getRef(m_sys));
	builtinavm1->registerBuiltin("ContextMenuItem","",Class<ContextMenuItem>::getRef(m_sys));
	builtinavm1->registerBuiltin("TextField","",Class<AVM1TextField>::getRef(m_sys));
	builtinavm1->registerBuiltin("TextFormat","",Class<TextFormat>::getRef(m_sys));
	builtinavm1->registerBuiltin("XML","",Class<AVM1XMLDocument>::getRef(m_sys));
	builtinavm1->registerBuiltin("XMLNode","",Class<XMLNode>::getRef(m_sys));
	builtinavm1->registerBuiltin("NetConnection","",Class<NetConnection>::getRef(m_sys));
	builtinavm1->registerBuiltin("NetStream","",Class<NetStream>::getRef(m_sys));
	builtinavm1->registerBuiltin("Video","",Class<AVM1Video>::getRef(m_sys));
	builtinavm1->registerBuiltin("AsBroadcaster","",Class<AVM1Broadcaster>::getRef(m_sys));

	if (m_sys->getSwfVersion() >= 8 && !m_sys->mainClip->usesActionScript3)
	{
		ASObject* systempackage = Class<ASObject>::getInstanceS(m_sys);
		builtinavm1->setVariableByQName("System",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),systempackage,CONSTANT_TRAIT);
		
		systempackage->setVariableByQName("security","System",Class<Security>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);

		ASObject* flashpackage = Class<ASObject>::getInstanceS(m_sys);
		builtinavm1->setVariableByQName("flash",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashpackage,CONSTANT_TRAIT);

		ASObject* flashdisplaypackage = Class<ASObject>::getInstanceS(m_sys);
		flashpackage->setVariableByQName("display",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashdisplaypackage,CONSTANT_TRAIT);

		flashdisplaypackage->setVariableByQName("BitmapData","flash.display",Class<BitmapData>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);

		ASObject* flashfilterspackage = Class<ASObject>::getInstanceS(m_sys);
		flashpackage->setVariableByQName("filters",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashfilterspackage,CONSTANT_TRAIT);

		flashfilterspackage->setVariableByQName("BitmapFilter","flash.filters",Class<BitmapFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("DropShadowFilter","flash.filters",Class<DropShadowFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("GlowFilter","flash.filters",Class<GlowFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("GradientGlowFilter","flash.filters",Class<GradientGlowFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("BevelFilter","flash.filters",Class<BevelFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("ColorMatrixFilter","flash.filters",Class<ColorMatrixFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("BlurFilter","flash.filters",Class<BlurFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("ConvolutionFilter","flash.filters",Class<ConvolutionFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("DisplacementMapFilter","flash.filters",Class<DisplacementMapFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashfilterspackage->setVariableByQName("GradientBevelFilter","flash.filters",Class<GradientBevelFilter>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);

		ASObject* flashgeompackage = Class<ASObject>::getInstanceS(m_sys);
		flashpackage->setVariableByQName("geom",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashgeompackage,CONSTANT_TRAIT);

		flashgeompackage->setVariableByQName("Matrix","flash.geom",Class<Matrix>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("ColorTransform","flash.geom",Class<ColorTransform>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Transform","flash.geom",Class<ColorTransform>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Rectangle","flash.geom",Class<Rectangle>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Point","flash.geom",Class<Point>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
	}
	m_sys->avm1global=builtinavm1;
}

void ABCVm::registerClassesToplevel(Global* builtin)
{
	builtin->registerBuiltin("Object","",Class<ASObject>::getRef(m_sys));
	builtin->registerBuiltin("Class","",Class_object::getRef(m_sys));
	builtin->registerBuiltin("Number","",Class<Number>::getRef(m_sys));
	builtin->registerBuiltin("Boolean","",Class<Boolean>::getRef(m_sys));
	builtin->setVariableAtomByQName("NaN",nsNameAndKind(),asAtomHandler::fromNumber(m_sys,numeric_limits<double>::quiet_NaN(),true),CONSTANT_TRAIT);
	builtin->setVariableAtomByQName("Infinity",nsNameAndKind(),asAtomHandler::fromNumber(m_sys,numeric_limits<double>::infinity(),true),CONSTANT_TRAIT);
	builtin->registerBuiltin("String","",Class<ASString>::getRef(m_sys));
	builtin->registerBuiltin("Array","",Class<Array>::getRef(m_sys));
	builtin->registerBuiltin("Function","",Class<IFunction>::getRef(m_sys));
	builtin->registerBuiltin("undefined","",_MR(m_sys->getUndefinedRef()));
	builtin->registerBuiltin("Math","",Class<Math>::getRef(m_sys));
	builtin->registerBuiltin("Namespace","",Class<Namespace>::getRef(m_sys));
	builtin->registerBuiltin("AS3","",_MR(Class<Namespace>::getInstanceS(m_sys,BUILTIN_STRINGS::STRING_AS3NS)));
	builtin->registerBuiltin("Date","",Class<Date>::getRef(m_sys));
	builtin->registerBuiltin("JSON","",Class<JSON>::getRef(m_sys));
	builtin->registerBuiltin("RegExp","",Class<RegExp>::getRef(m_sys));
	builtin->registerBuiltin("QName","",Class<ASQName>::getRef(m_sys));
	builtin->registerBuiltin("uint","",Class<UInteger>::getRef(m_sys));
	builtin->registerBuiltin("Vector","__AS3__.vec",_MR(Template<Vector>::getTemplate(m_sys)));
	builtin->registerBuiltin("Error","",Class<ASError>::getRef(m_sys));
	builtin->registerBuiltin("SecurityError","",Class<SecurityError>::getRef(m_sys));
	builtin->registerBuiltin("ArgumentError","",Class<ArgumentError>::getRef(m_sys));
	builtin->registerBuiltin("DefinitionError","",Class<DefinitionError>::getRef(m_sys));
	builtin->registerBuiltin("EvalError","",Class<EvalError>::getRef(m_sys));
	builtin->registerBuiltin("RangeError","",Class<RangeError>::getRef(m_sys));
	builtin->registerBuiltin("ReferenceError","",Class<ReferenceError>::getRef(m_sys));
	builtin->registerBuiltin("SyntaxError","",Class<SyntaxError>::getRef(m_sys));
	builtin->registerBuiltin("TypeError","",Class<TypeError>::getRef(m_sys));
	builtin->registerBuiltin("URIError","",Class<URIError>::getRef(m_sys));
	builtin->registerBuiltin("UninitializedError","",Class<UninitializedError>::getRef(m_sys));
	builtin->registerBuiltin("VerifyError","",Class<VerifyError>::getRef(m_sys));
	if (m_sys->mainClip->usesActionScript3)
	{
		builtin->registerBuiltin("XML","",Class<XML>::getRef(m_sys));
		builtin->registerBuiltin("XMLList","",Class<XMLList>::getRef(m_sys));
	}
	builtin->registerBuiltin("int","",Class<Integer>::getRef(m_sys));

	builtin->registerBuiltin("eval","",_MR(Class<IFunction>::getFunction(m_sys,eval)));
	builtin->registerBuiltin("print","",_MR(Class<IFunction>::getFunction(m_sys,print)));
	builtin->registerBuiltin("trace","",_MR(Class<IFunction>::getFunction(m_sys,trace)));
	builtin->registerBuiltin("parseInt","",_MR(Class<IFunction>::getFunction(m_sys,parseInt,2)));
	builtin->registerBuiltin("parseFloat","",_MR(Class<IFunction>::getFunction(m_sys,parseFloat,1)));
	builtin->registerBuiltin("encodeURI","",_MR(Class<IFunction>::getFunction(m_sys,encodeURI)));
	builtin->registerBuiltin("decodeURI","",_MR(Class<IFunction>::getFunction(m_sys,decodeURI)));
	builtin->registerBuiltin("encodeURIComponent","",_MR(Class<IFunction>::getFunction(m_sys,encodeURIComponent)));
	builtin->registerBuiltin("decodeURIComponent","",_MR(Class<IFunction>::getFunction(m_sys,decodeURIComponent)));
	builtin->registerBuiltin("escape","",_MR(Class<IFunction>::getFunction(m_sys,escape,1)));
	builtin->registerBuiltin("unescape","",_MR(Class<IFunction>::getFunction(m_sys,unescape,1)));
	builtin->registerBuiltin("toString","",_MR(Class<IFunction>::getFunction(m_sys,ASObject::_toString)));

	builtin->registerBuiltin("isNaN","",_MR(Class<IFunction>::getFunction(m_sys,isNaN,1)));
	builtin->registerBuiltin("isFinite","",_MR(Class<IFunction>::getFunction(m_sys,isFinite,1)));
	builtin->registerBuiltin("isXMLName","",_MR(Class<IFunction>::getFunction(m_sys,_isXMLName)));
}

void ABCVm::registerClasses()
{
	Global* builtin=Class<Global>::getInstanceS(m_sys,(ABCContext*)nullptr, 0);
	//Register predefined types, ASObject are enough for not implemented classes
	registerClassesToplevel(builtin);

	builtin->registerBuiltin("AccessibilityProperties","flash.accessibility",Class<AccessibilityProperties>::getRef(m_sys));
	builtin->registerBuiltin("AccessibilityImplementation","flash.accessibility",Class<AccessibilityImplementation>::getRef(m_sys));
	builtin->registerBuiltin("Accessibility","flash.accessibility",Class<Accessibility>::getRef(m_sys));

	builtin->registerBuiltin("Mutex","flash.concurrent",Class<ASMutex>::getRef(m_sys));
	builtin->registerBuiltin("Condition","flash.concurrent",Class<ASCondition>::getRef(m_sys));

	builtin->registerBuiltin("generateRandomBytes","flash.crypto",_MR(Class<IFunction>::getFunction(m_sys,generateRandomBytes)));

	builtin->registerBuiltin("ActionScriptVersion","flash.display",Class<ActionScriptVersion>::getRef(m_sys));
	builtin->registerBuiltin("MovieClip","flash.display",Class<MovieClip>::getRef(m_sys));
	builtin->registerBuiltin("DisplayObject","flash.display",Class<DisplayObject>::getRef(m_sys));
	builtin->registerBuiltin("Loader","flash.display",Class<Loader>::getRef(m_sys));
	builtin->registerBuiltin("LoaderInfo","flash.display",Class<LoaderInfo>::getRef(m_sys));
	builtin->registerBuiltin("SimpleButton","flash.display",Class<SimpleButton>::getRef(m_sys));
	builtin->registerBuiltin("InteractiveObject","flash.display",Class<InteractiveObject>::getRef(m_sys));
	builtin->registerBuiltin("JPEGEncoderOptions","flash.display",Class<JPEGEncoderOptions>::getRef(m_sys));
	builtin->registerBuiltin("JPEGXREncoderOptions","flash.display",Class<JPEGXREncoderOptions>::getRef(m_sys));
	builtin->registerBuiltin("DisplayObjectContainer","flash.display",Class<DisplayObjectContainer>::getRef(m_sys));
	builtin->registerBuiltin("PNGEncoderOptions","flash.display",Class<PNGEncoderOptions>::getRef(m_sys));
	builtin->registerBuiltin("Sprite","flash.display",Class<Sprite>::getRef(m_sys));
	builtin->registerBuiltin("Shape","flash.display",Class<Shape>::getRef(m_sys));
	builtin->registerBuiltin("Stage","flash.display",Class<Stage>::getRef(m_sys));
	builtin->registerBuiltin("Stage3D","flash.display",Class<Stage3D>::getRef(m_sys));
	builtin->registerBuiltin("Graphics","flash.display",Class<Graphics>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsBitmapFill","flash.display",Class<GraphicsBitmapFill>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsEndFill","flash.display",Class<GraphicsEndFill>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsGradientFill","flash.display",Class<GraphicsGradientFill>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsPath","flash.display",Class<GraphicsPath>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsPathCommand","flash.display",Class<GraphicsPathCommand>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsPathWinding","flash.display",Class<GraphicsPathWinding>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsShaderFill","flash.display",Class<GraphicsShaderFill>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsSolidFill","flash.display",Class<GraphicsSolidFill>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsStroke","flash.display",Class<GraphicsStroke>::getRef(m_sys));
	builtin->registerBuiltin("GraphicsTrianglePath","flash.display",Class<GraphicsTrianglePath>::getRef(m_sys));
	builtin->registerBuiltin("IGraphicsData","flash.display",InterfaceClass<IGraphicsData>::getRef(m_sys));
	builtin->registerBuiltin("IGraphicsFill","flash.display",InterfaceClass<IGraphicsFill>::getRef(m_sys));
	builtin->registerBuiltin("IGraphicsPath","flash.display",InterfaceClass<IGraphicsPath>::getRef(m_sys));
	builtin->registerBuiltin("IGraphicsStroke","flash.display",InterfaceClass<IGraphicsStroke>::getRef(m_sys));
	builtin->registerBuiltin("GradientType","flash.display",Class<GradientType>::getRef(m_sys));
	builtin->registerBuiltin("BlendMode","flash.display",Class<BlendMode>::getRef(m_sys));
	builtin->registerBuiltin("LineScaleMode","flash.display",Class<LineScaleMode>::getRef(m_sys));
	builtin->registerBuiltin("StageScaleMode","flash.display",Class<StageScaleMode>::getRef(m_sys));
	builtin->registerBuiltin("StageAlign","flash.display",Class<StageAlign>::getRef(m_sys));
	builtin->registerBuiltin("StageQuality","flash.display",Class<StageQuality>::getRef(m_sys));
	builtin->registerBuiltin("StageDisplayState","flash.display",Class<StageDisplayState>::getRef(m_sys));
	builtin->registerBuiltin("BitmapData","flash.display",Class<BitmapData>::getRef(m_sys));
	builtin->registerBuiltin("Bitmap","flash.display",Class<Bitmap>::getRef(m_sys));
	builtin->registerBuiltin("BitmapEncodingColorSpace","flash.display",Class<BitmapEncodingColorSpace>::getRef(m_sys));
	builtin->registerBuiltin("ColorCorrection","flash.display",Class<ColorCorrection>::getRef(m_sys));
	builtin->registerBuiltin("ColorCorrectionSupport","flash.display",Class<ColorCorrectionSupport>::getRef(m_sys));
	builtin->registerBuiltin("ShaderParameterType","flash.display",Class<ShaderParameterType>::getRef(m_sys));
	builtin->registerBuiltin("ShaderPrecision","flash.display",Class<ShaderPrecision>::getRef(m_sys));
	builtin->registerBuiltin("SWFVersion","flash.display",Class<SWFVersion>::getRef(m_sys));
	builtin->registerBuiltin("TriangleCulling","flash.display",Class<TriangleCulling>::getRef(m_sys));
	builtin->registerBuiltin("IBitmapDrawable","flash.display",InterfaceClass<IBitmapDrawable>::getRef(m_sys));
	builtin->registerBuiltin("MorphShape","flash.display",Class<MorphShape>::getRef(m_sys));
	builtin->registerBuiltin("SpreadMethod","flash.display",Class<SpreadMethod>::getRef(m_sys));
	builtin->registerBuiltin("InterpolationMethod","flash.display",Class<InterpolationMethod>::getRef(m_sys));
	builtin->registerBuiltin("FrameLabel","flash.display",Class<FrameLabel>::getRef(m_sys));
	builtin->registerBuiltin("Scene","flash.display",Class<Scene>::getRef(m_sys));
	builtin->registerBuiltin("AVM1Movie","flash.display",Class<AVM1Movie>::getRef(m_sys));
	builtin->registerBuiltin("Shader","flash.display",Class<Shader>::getRef(m_sys));
	builtin->registerBuiltin("BitmapDataChannel","flash.display",Class<BitmapDataChannel>::getRef(m_sys));
	builtin->registerBuiltin("PixelSnapping","flash.display",Class<PixelSnapping>::getRef(m_sys));
	builtin->registerBuiltin("CapsStyle","flash.display",Class<CapsStyle>::getRef(m_sys));
	builtin->registerBuiltin("JointStyle","flash.display",Class<JointStyle>::getRef(m_sys));

	builtin->registerBuiltin("Context3D","flash.display3D",Class<Context3D>::getRef(m_sys));
	builtin->registerBuiltin("Context3DBlendFactor","flash.display3D",Class<Context3DBlendFactor>::getRef(m_sys));
	builtin->registerBuiltin("Context3DBufferUsage","flash.display3D",Class<Context3DBufferUsage>::getRef(m_sys));
	builtin->registerBuiltin("Context3DClearMask","flash.display3D",Class<Context3DClearMask>::getRef(m_sys));
	builtin->registerBuiltin("Context3DCompareMode","flash.display3D",Class<Context3DCompareMode>::getRef(m_sys));
	builtin->registerBuiltin("Context3DMipFilter","flash.display3D",Class<Context3DMipFilter>::getRef(m_sys));
	builtin->registerBuiltin("Context3DProfile","flash.display3D",Class<Context3DProfile>::getRef(m_sys));
	builtin->registerBuiltin("Context3DProgramType","flash.display3D",Class<Context3DProgramType>::getRef(m_sys));
	builtin->registerBuiltin("Context3DRenderMode","flash.display3D",Class<Context3DRenderMode>::getRef(m_sys));
	builtin->registerBuiltin("Context3DStencilAction","flash.display3D",Class<Context3DStencilAction>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTextureFilter","flash.display3D",Class<Context3DTextureFilter>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTextureFormat","flash.display3D",Class<Context3DTextureFormat>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTriangleFace","flash.display3D",Class<Context3DTriangleFace>::getRef(m_sys));
	builtin->registerBuiltin("Context3DVertexBufferFormat","flash.display3D",Class<Context3DVertexBufferFormat>::getRef(m_sys));
	builtin->registerBuiltin("Context3DWrapMode","flash.display3D",Class<Context3DWrapMode>::getRef(m_sys));
	builtin->registerBuiltin("IndexBuffer3D","flash.display3D",Class<IndexBuffer3D>::getRef(m_sys));
	builtin->registerBuiltin("Program3D","flash.display3D",Class<Program3D>::getRef(m_sys));
	builtin->registerBuiltin("VertexBuffer3D","flash.display3D",Class<VertexBuffer3D>::getRef(m_sys));

	builtin->registerBuiltin("TextureBase","flash.display3D.textures",Class<TextureBase>::getRef(m_sys));
	builtin->registerBuiltin("Texture","flash.display3D.textures",Class<Texture>::getRef(m_sys));
	builtin->registerBuiltin("CubeTexture","flash.display3D.textures",Class<CubeTexture>::getRef(m_sys));
	builtin->registerBuiltin("RectangleTexture","flash.display3D.textures",Class<RectangleTexture>::getRef(m_sys));
	builtin->registerBuiltin("VideoTexture","flash.display3D.textures",Class<VideoTexture>::getRef(m_sys));

	builtin->registerBuiltin("BitmapFilter","flash.filters",Class<BitmapFilter>::getRef(m_sys));
	builtin->registerBuiltin("BitmapFilterQuality","flash.filters",Class<BitmapFilterQuality>::getRef(m_sys));
	builtin->registerBuiltin("BitmapFilterType","flash.filters",Class<BitmapFilterType>::getRef(m_sys));
	builtin->registerBuiltin("DropShadowFilter","flash.filters",Class<DropShadowFilter>::getRef(m_sys));
	builtin->registerBuiltin("GlowFilter","flash.filters",Class<GlowFilter>::getRef(m_sys));
	builtin->registerBuiltin("GradientGlowFilter","flash.filters",Class<GradientGlowFilter>::getRef(m_sys));
	builtin->registerBuiltin("BevelFilter","flash.filters",Class<BevelFilter>::getRef(m_sys));
	builtin->registerBuiltin("ColorMatrixFilter","flash.filters",Class<ColorMatrixFilter>::getRef(m_sys));
	builtin->registerBuiltin("BlurFilter","flash.filters",Class<BlurFilter>::getRef(m_sys));
	builtin->registerBuiltin("ConvolutionFilter","flash.filters",Class<ConvolutionFilter>::getRef(m_sys));
	builtin->registerBuiltin("DisplacementMapFilter","flash.filters",Class<DisplacementMapFilter>::getRef(m_sys));
	builtin->registerBuiltin("DisplacementMapFilterMode","flash.filters",Class<DisplacementMapFilterMode>::getRef(m_sys));
	builtin->registerBuiltin("GradientBevelFilter","flash.filters",Class<GradientBevelFilter>::getRef(m_sys));
	builtin->registerBuiltin("ShaderFilter","flash.filters",Class<ShaderFilter>::getRef(m_sys));

	builtin->registerBuiltin("AntiAliasType","flash.text",Class<AntiAliasType>::getRef(m_sys));
	builtin->registerBuiltin("CSMSettings","flash.text",Class<CSMSettings>::getRef(m_sys));
	builtin->registerBuiltin("Font","flash.text",Class<ASFont>::getRef(m_sys));
	builtin->registerBuiltin("FontStyle","flash.text",Class<FontStyle>::getRef(m_sys));
	builtin->registerBuiltin("FontType","flash.text",Class<FontType>::getRef(m_sys));
	builtin->registerBuiltin("GridFitType","flash.text",Class<GridFitType>::getRef(m_sys));
	builtin->registerBuiltin("StyleSheet","flash.text",Class<StyleSheet>::getRef(m_sys));
	builtin->registerBuiltin("TextColorType","flash.text",Class<TextColorType>::getRef(m_sys));
	builtin->registerBuiltin("TextDisplayMode","flash.text",Class<TextDisplayMode>::getRef(m_sys));
	builtin->registerBuiltin("TextField","flash.text",Class<TextField>::getRef(m_sys));
	builtin->registerBuiltin("TextFieldType","flash.text",Class<TextFieldType>::getRef(m_sys));
	builtin->registerBuiltin("TextFieldAutoSize","flash.text",Class<TextFieldAutoSize>::getRef(m_sys));
	builtin->registerBuiltin("TextFormat","flash.text",Class<TextFormat>::getRef(m_sys));
	builtin->registerBuiltin("TextFormatAlign","flash.text",Class<TextFormatAlign>::getRef(m_sys));
	builtin->registerBuiltin("TextLineMetrics","flash.text",Class<TextLineMetrics>::getRef(m_sys));
	builtin->registerBuiltin("TextInteractionMode","flash.text",Class<TextInteractionMode>::getRef(m_sys));
	builtin->registerBuiltin("TextRenderer","flash.text",Class<TextRenderer>::getRef(m_sys));
	builtin->registerBuiltin("StaticText","flash.text",Class<StaticText>::getRef(m_sys));

	builtin->registerBuiltin("BreakOpportunity","flash.text.engine",Class<BreakOpportunity>::getRef(m_sys));
	builtin->registerBuiltin("CFFHinting","flash.text.engine",Class<CFFHinting>::getRef(m_sys));
	builtin->registerBuiltin("ContentElement","flash.text.engine",Class<ContentElement>::getRef(m_sys));
	builtin->registerBuiltin("DigitCase","flash.text.engine",Class<DigitCase>::getRef(m_sys));
	builtin->registerBuiltin("DigitWidth","flash.text.engine",Class<DigitWidth>::getRef(m_sys));
	builtin->registerBuiltin("EastAsianJustifier","flash.text.engine",Class<EastAsianJustifier>::getRef(m_sys));
	builtin->registerBuiltin("ElementFormat","flash.text.engine",Class<ElementFormat>::getRef(m_sys));
	builtin->registerBuiltin("FontDescription","flash.text.engine",Class<FontDescription>::getRef(m_sys));
	builtin->registerBuiltin("FontMetrics","flash.text.engine",Class<FontMetrics>::getRef(m_sys));
	builtin->registerBuiltin("FontLookup","flash.text.engine",Class<FontLookup>::getRef(m_sys));
	builtin->registerBuiltin("FontPosture","flash.text.engine",Class<FontPosture>::getRef(m_sys));
	builtin->registerBuiltin("FontWeight","flash.text.engine",Class<FontWeight>::getRef(m_sys));
	builtin->registerBuiltin("GroupElement","flash.text.engine",Class<GroupElement>::getRef(m_sys));
	builtin->registerBuiltin("JustificationStyle","flash.text.engine",Class<JustificationStyle>::getRef(m_sys));
	builtin->registerBuiltin("Kerning","flash.text.engine",Class<Kerning>::getRef(m_sys));
	builtin->registerBuiltin("LigatureLevel","flash.text.engine",Class<LigatureLevel>::getRef(m_sys));
	builtin->registerBuiltin("LineJustification","flash.text.engine",Class<LineJustification>::getRef(m_sys));
	builtin->registerBuiltin("RenderingMode","flash.text.engine",Class<RenderingMode>::getRef(m_sys));
	builtin->registerBuiltin("SpaceJustifier","flash.text.engine",Class<SpaceJustifier>::getRef(m_sys));
	builtin->registerBuiltin("TabAlignment","flash.text.engine",Class<TabAlignment>::getRef(m_sys));
	builtin->registerBuiltin("TabStop","flash.text.engine",Class<TabStop>::getRef(m_sys));
	builtin->registerBuiltin("TextBaseline","flash.text.engine",Class<TextBaseline>::getRef(m_sys));
	builtin->registerBuiltin("TextBlock","flash.text.engine",Class<TextBlock>::getRef(m_sys));
	builtin->registerBuiltin("TextElement","flash.text.engine",Class<TextElement>::getRef(m_sys));
	builtin->registerBuiltin("TextLine","flash.text.engine",Class<TextLine>::getRef(m_sys));
	builtin->registerBuiltin("TextLineValidity","flash.text.engine",Class<TextLineValidity>::getRef(m_sys));
	builtin->registerBuiltin("TextRotation","flash.text.engine",Class<TextRotation>::getRef(m_sys));
	builtin->registerBuiltin("TextJustifier","flash.text.engine",Class<TextJustifier>::getRef(m_sys));
	
	builtin->registerBuiltin("XMLDocument","flash.xml",Class<XMLDocument>::getRef(m_sys));
	builtin->registerBuiltin("XMLNode","flash.xml",Class<XMLNode>::getRef(m_sys));

	builtin->registerBuiltin("ExternalInterface","flash.external",Class<ExternalInterface>::getRef(m_sys));

	builtin->registerBuiltin("Endian","flash.utils",Class<Endian>::getRef(m_sys));
	builtin->registerBuiltin("ByteArray","flash.utils",Class<ByteArray>::getRef(m_sys));
	builtin->registerBuiltin("CompressionAlgorithm","flash.utils",Class<CompressionAlgorithm>::getRef(m_sys));
	builtin->registerBuiltin("Dictionary","flash.utils",Class<Dictionary>::getRef(m_sys));
	builtin->registerBuiltin("Proxy","flash.utils",Class<Proxy>::getRef(m_sys));
	builtin->registerBuiltin("Timer","flash.utils",Class<Timer>::getRef(m_sys));
	builtin->registerBuiltin("getQualifiedClassName","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,getQualifiedClassName)));
	builtin->registerBuiltin("getQualifiedSuperclassName","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,getQualifiedSuperclassName)));
	builtin->registerBuiltin("getDefinitionByName","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,getDefinitionByName)));
	builtin->registerBuiltin("getTimer","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,getTimer)));
	builtin->registerBuiltin("setInterval","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,setInterval)));
	builtin->registerBuiltin("setTimeout","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,setTimeout)));
	builtin->registerBuiltin("clearInterval","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,clearInterval)));
	builtin->registerBuiltin("clearTimeout","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,clearTimeout)));
	builtin->registerBuiltin("describeType","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,describeType)));
	builtin->registerBuiltin("escapeMultiByte","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,escapeMultiByte)));
	builtin->registerBuiltin("unescapeMultiByte","flash.utils",_MR(Class<IFunction>::getFunction(m_sys,unescapeMultiByte)));
	builtin->registerBuiltin("IExternalizable","flash.utils",InterfaceClass<IExternalizable>::getRef(m_sys));
	builtin->registerBuiltin("IDataInput","flash.utils",InterfaceClass<IDataInput>::getRef(m_sys));
	builtin->registerBuiltin("IDataOutput","flash.utils",InterfaceClass<IDataOutput>::getRef(m_sys));

	builtin->registerBuiltin("ColorTransform","flash.geom",Class<ColorTransform>::getRef(m_sys));
	builtin->registerBuiltin("Rectangle","flash.geom",Class<Rectangle>::getRef(m_sys));
	builtin->registerBuiltin("Matrix","flash.geom",Class<Matrix>::getRef(m_sys));
	builtin->registerBuiltin("Transform","flash.geom",Class<Transform>::getRef(m_sys));
	builtin->registerBuiltin("Point","flash.geom",Class<Point>::getRef(m_sys));
	builtin->registerBuiltin("Vector3D","flash.geom",Class<Vector3D>::getRef(m_sys));
	builtin->registerBuiltin("Matrix3D","flash.geom",Class<Matrix3D>::getRef(m_sys));
	builtin->registerBuiltin("Orientation3D","flash.geom",Class<Orientation3D>::getRef(m_sys));
	builtin->registerBuiltin("PerspectiveProjection","flash.geom",Class<PerspectiveProjection>::getRef(m_sys));

	builtin->registerBuiltin("EventDispatcher","flash.events",Class<EventDispatcher>::getRef(m_sys));
	builtin->registerBuiltin("Event","flash.events",Class<Event>::getRef(m_sys));
	builtin->registerBuiltin("EventPhase","flash.events",Class<EventPhase>::getRef(m_sys));
	builtin->registerBuiltin("MouseEvent","flash.events",Class<MouseEvent>::getRef(m_sys));
	builtin->registerBuiltin("ProgressEvent","flash.events",Class<ProgressEvent>::getRef(m_sys));
	builtin->registerBuiltin("TimerEvent","flash.events",Class<TimerEvent>::getRef(m_sys));
	builtin->registerBuiltin("IOErrorEvent","flash.events",Class<IOErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("ErrorEvent","flash.events",Class<ErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("SecurityErrorEvent","flash.events",Class<SecurityErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("AsyncErrorEvent","flash.events",Class<AsyncErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("FullScreenEvent","flash.events",Class<FullScreenEvent>::getRef(m_sys));
	builtin->registerBuiltin("TextEvent","flash.events",Class<TextEvent>::getRef(m_sys));
	builtin->registerBuiltin("IEventDispatcher","flash.events",InterfaceClass<IEventDispatcher>::getRef(m_sys));
	builtin->registerBuiltin("FocusEvent","flash.events",Class<FocusEvent>::getRef(m_sys));
	builtin->registerBuiltin("NetStatusEvent","flash.events",Class<NetStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("HTTPStatusEvent","flash.events",Class<HTTPStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("KeyboardEvent","flash.events",Class<KeyboardEvent>::getRef(m_sys));
	builtin->registerBuiltin("StatusEvent","flash.events",Class<StatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("DataEvent","flash.events",Class<DataEvent>::getRef(m_sys));
	builtin->registerBuiltin("DRMErrorEvent","flash.events",Class<DRMErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("DRMStatusEvent","flash.events",Class<DRMStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoEvent","flash.events",Class<StageVideoEvent>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailabilityEvent","flash.events",Class<StageVideoAvailabilityEvent>::getRef(m_sys));
	builtin->registerBuiltin("TouchEvent","flash.events",Class<TouchEvent>::getRef(m_sys));
	builtin->registerBuiltin("GameInputEvent","flash.events",Class<GameInputEvent>::getRef(m_sys));
	builtin->registerBuiltin("GestureEvent","flash.events",Class<GestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("PressAndTapGestureEvent","flash.events",Class<PressAndTapGestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("TransformGestureEvent","flash.events",Class<TransformGestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuEvent","flash.events",Class<ContextMenuEvent>::getRef(m_sys));
	builtin->registerBuiltin("UncaughtErrorEvent","flash.events",Class<UncaughtErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("UncaughtErrorEvents","flash.events",Class<UncaughtErrorEvents>::getRef(m_sys));
	builtin->registerBuiltin("VideoEvent","flash.events",Class<VideoEvent>::getRef(m_sys));
	builtin->registerBuiltin("SampleDataEvent","flash.events",Class<SampleDataEvent>::getRef(m_sys));
	builtin->registerBuiltin("ThrottleEvent","flash.events",Class<ThrottleEvent>::getRef(m_sys));
	builtin->registerBuiltin("ThrottleType","flash.events",Class<ThrottleType>::getRef(m_sys));
	
	builtin->registerBuiltin("navigateToURL","flash.net",_MR(Class<IFunction>::getFunction(m_sys,navigateToURL)));
	builtin->registerBuiltin("sendToURL","flash.net",_MR(Class<IFunction>::getFunction(m_sys,sendToURL)));
	builtin->registerBuiltin("DynamicPropertyOutput","flash.net",Class<DynamicPropertyOutput>::getRef(m_sys));
	builtin->registerBuiltin("FileFilter","flash.net",Class<FileFilter>::getRef(m_sys));
	builtin->registerBuiltin("FileReference","flash.net",Class<FileReference>::getRef(m_sys));
	builtin->registerBuiltin("IDynamicPropertyWriter","flash.net",InterfaceClass<IDynamicPropertyWriter>::getRef(m_sys));
	builtin->registerBuiltin("IDynamicPropertyOutput","flash.net",InterfaceClass<IDynamicPropertyOutput>::getRef(m_sys));
	builtin->registerBuiltin("LocalConnection","flash.net",Class<LocalConnection>::getRef(m_sys));
	builtin->registerBuiltin("NetConnection","flash.net",Class<NetConnection>::getRef(m_sys));
	builtin->registerBuiltin("NetGroup","flash.net",Class<NetGroup>::getRef(m_sys));
	builtin->registerBuiltin("NetStream","flash.net",Class<NetStream>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamAppendBytesAction","flash.net",Class<NetStreamAppendBytesAction>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamInfo","flash.net",Class<NetStreamInfo>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamPlayOptions","flash.net",Class<NetStreamPlayOptions>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamPlayTransitions","flash.net",Class<NetStreamPlayTransitions>::getRef(m_sys));
	builtin->registerBuiltin("URLLoader","flash.net",Class<URLLoader>::getRef(m_sys));
	builtin->registerBuiltin("URLStream","flash.net",Class<URLStream>::getRef(m_sys));
	builtin->registerBuiltin("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getRef(m_sys));
	builtin->registerBuiltin("URLRequest","flash.net",Class<URLRequest>::getRef(m_sys));
	builtin->registerBuiltin("URLRequestHeader","flash.net",Class<URLRequestHeader>::getRef(m_sys));
	builtin->registerBuiltin("URLRequestMethod","flash.net",Class<URLRequestMethod>::getRef(m_sys));
	builtin->registerBuiltin("URLVariables","flash.net",Class<URLVariables>::getRef(m_sys));
	builtin->registerBuiltin("SharedObject","flash.net",Class<SharedObject>::getRef(m_sys));
	builtin->registerBuiltin("SharedObjectFlushStatus","flash.net",Class<SharedObjectFlushStatus>::getRef(m_sys));
	builtin->registerBuiltin("ObjectEncoding","flash.net",Class<ObjectEncoding>::getRef(m_sys));
	builtin->registerBuiltin("Socket","flash.net",Class<ASSocket>::getRef(m_sys));
	builtin->registerBuiltin("Responder","flash.net",Class<Responder>::getRef(m_sys));
	builtin->registerBuiltin("XMLSocket","flash.net",Class<XMLSocket>::getRef(m_sys));
	builtin->registerBuiltin("registerClassAlias","flash.net",_MR(Class<IFunction>::getFunction(m_sys,registerClassAlias)));
	builtin->registerBuiltin("getClassByAlias","flash.net",_MR(Class<IFunction>::getFunction(m_sys,getClassByAlias)));
	builtin->registerBuiltin("NetGroupReceiveMode","flash.net",Class<NetGroupReceiveMode>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupReplicationStrategy","flash.net",Class<NetGroupReplicationStrategy>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupSendMode","flash.net",Class<NetGroupSendMode>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupSendResult","flash.net",Class<NetGroupSendResult>::getRef(m_sys));

	builtin->registerBuiltin("DRMManager","flash.net.drm",Class<DRMManager>::getRef(m_sys));

	builtin->registerBuiltin("clearSamples","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,clearSamples)));
	builtin->registerBuiltin("getGetterInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getGetterInvocationCount)));
	builtin->registerBuiltin("getInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getInvocationCount)));
	builtin->registerBuiltin("getLexicalScopes","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getLexicalScopes)));
	builtin->registerBuiltin("getMasterString","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getMasterString)));
	builtin->registerBuiltin("getMemberNames","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getMemberNames)));
	builtin->registerBuiltin("getSampleCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSampleCount)));
	builtin->registerBuiltin("getSamples","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSamples)));
	builtin->registerBuiltin("getSavedThis","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSavedThis)));
	builtin->registerBuiltin("getSetterInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSetterInvocationCount)));
	builtin->registerBuiltin("getSize","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSize)));
	builtin->registerBuiltin("isGetterSetter","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,isGetterSetter)));
	builtin->registerBuiltin("pauseSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,pauseSampling)));
	builtin->registerBuiltin("sampleInternalAllocs","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,sampleInternalAllocs)));
	builtin->registerBuiltin("setSamplerCallback","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,setSamplerCallback)));
	builtin->registerBuiltin("startSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,startSampling)));
	builtin->registerBuiltin("stopSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,stopSampling)));
	builtin->registerBuiltin("DeleteObjectSample","flash.sampler",Class<DeleteObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("NewObjectSample","flash.sampler",Class<NewObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("Sample","flash.sampler",Class<Sample>::getRef(m_sys));
	builtin->registerBuiltin("StackFrame","flash.sampler",Class<StackFrame>::getRef(m_sys));

	builtin->registerBuiltin("CertificateStatus","flash.security",Class<CertificateStatus>::getRef(m_sys));

	builtin->registerBuiltin("fscommand","flash.system",_MR(Class<IFunction>::getFunction(m_sys,fscommand)));
	builtin->registerBuiltin("Capabilities","flash.system",Class<Capabilities>::getRef(m_sys));
	builtin->registerBuiltin("Security","flash.system",Class<Security>::getRef(m_sys));
	builtin->registerBuiltin("ApplicationDomain","flash.system",Class<ApplicationDomain>::getRef(m_sys));
	builtin->registerBuiltin("SecurityDomain","flash.system",Class<SecurityDomain>::getRef(m_sys));
	builtin->registerBuiltin("LoaderContext","flash.system",Class<LoaderContext>::getRef(m_sys));
	builtin->registerBuiltin("System","flash.system",Class<System>::getRef(m_sys));
	builtin->registerBuiltin("Worker","flash.system",Class<ASWorker>::getRef(m_sys));
	builtin->registerBuiltin("WorkerDomain","flash.system",Class<WorkerDomain>::getRef(m_sys));
	builtin->registerBuiltin("WorkerState","flash.system",Class<WorkerState>::getRef(m_sys));
	builtin->registerBuiltin("ImageDecodingPolicy","flash.system",Class<ImageDecodingPolicy>::getRef(m_sys));
	builtin->registerBuiltin("IMEConversionMode","flash.system",Class<IMEConversionMode>::getRef(m_sys));
	builtin->registerBuiltin("MessageChannelState","flash.system",Class<MessageChannelState>::getRef(m_sys));
	builtin->registerBuiltin("SecurityPanel","flash.system",Class<SecurityPanel>::getRef(m_sys));
        builtin->registerBuiltin("SystemUpdater","flash.system",Class<SystemUpdater>::getRef(m_sys));
	builtin->registerBuiltin("SystemUpdaterType","flash.system",Class<SystemUpdaterType>::getRef(m_sys));
	builtin->registerBuiltin("TouchscreenType","flash.system",Class<TouchscreenType>::getRef(m_sys));
	builtin->registerBuiltin("IME","flash.system",Class<IME>::getRef(m_sys));
	builtin->registerBuiltin("JPEGLoaderContext","flash.system",Class<JPEGLoaderContext>::getRef(m_sys));

	builtin->registerBuiltin("AudioDecoder","flash.media",Class<MediaAudioDecoder>::getRef(m_sys));
	builtin->registerBuiltin("AudioOutputChangeReason","flash.media",Class<AudioOutputChangeReason>::getRef(m_sys));
    builtin->registerBuiltin("AVNetworkingParams","flash.media",Class<AVNetworkingParams>::getRef(m_sys));
	builtin->registerBuiltin("H264Level","flash.media",Class<H264Level>::getRef(m_sys));
	builtin->registerBuiltin("H264Profile","flash.media",Class<H264Profile>::getRef(m_sys));
	builtin->registerBuiltin("MicrophoneEnhancedMode","flash.media",Class<MicrophoneEnhancedMode>::getRef(m_sys));
	builtin->registerBuiltin("MicrophoneEnhancedOptions","flash.media",Class<MicrophoneEnhancedOptions>::getRef(m_sys));
	builtin->registerBuiltin("SoundCodec","flash.media",Class<SoundCodec>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailabilityReason","flash.media",Class<StageVideoAvailabilityReason>::getRef(m_sys));
	builtin->registerBuiltin("VideoCodec","flash.media",Class<MediaVideoDecoder>::getRef(m_sys));
	builtin->registerBuiltin("SoundTransform","flash.media",Class<SoundTransform>::getRef(m_sys));
	builtin->registerBuiltin("Video","flash.media",Class<Video>::getRef(m_sys));
	builtin->registerBuiltin("Sound","flash.media",Class<Sound>::getRef(m_sys));
	builtin->registerBuiltin("SoundLoaderContext","flash.media",Class<SoundLoaderContext>::getRef(m_sys));
	builtin->registerBuiltin("SoundChannel","flash.media",Class<SoundChannel>::getRef(m_sys));
	builtin->registerBuiltin("SoundMixer","flash.media",Class<SoundMixer>::getRef(m_sys));
	builtin->registerBuiltin("StageVideo","flash.media",Class<StageVideo>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailability","flash.media",Class<StageVideoAvailability>::getRef(m_sys));
	builtin->registerBuiltin("VideoStatus","flash.media",Class<VideoStatus>::getRef(m_sys));
	builtin->registerBuiltin("Microphone","flash.media",Class<Microphone>::getRef(m_sys));
	builtin->registerBuiltin("Camera","flash.media",Class<Camera>::getRef(m_sys));
	builtin->registerBuiltin("VideoStreamSettings","flash.media",Class<VideoStreamSettings>::getRef(m_sys));
	builtin->registerBuiltin("H264VideoStreamSettings","flash.media",Class<H264VideoStreamSettings>::getRef(m_sys));
	builtin->registerBuiltin("AVTagData","flash.media",Class<AVTagData>::getRef(m_sys));
	builtin->registerBuiltin("ID3Info","flash.media",Class<ID3Info>::getRef(m_sys));

	builtin->registerBuiltin("Keyboard","flash.ui",Class<Keyboard>::getRef(m_sys));
	builtin->registerBuiltin("KeyboardType","flash.ui",Class<KeyboardType>::getRef(m_sys));
	builtin->registerBuiltin("KeyLocation","flash.ui",Class<KeyLocation>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenu","flash.ui",Class<ContextMenu>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuItem","flash.ui",Class<ContextMenuItem>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuBuiltInItems","flash.ui",Class<ContextMenuBuiltInItems>::getRef(m_sys));
	builtin->registerBuiltin("Mouse","flash.ui",Class<Mouse>::getRef(m_sys));
	builtin->registerBuiltin("MouseCursor","flash.ui",Class<MouseCursor>::getRef(m_sys));
	builtin->registerBuiltin("MouseCursorData","flash.ui",Class<MouseCursorData>::getRef(m_sys));
	builtin->registerBuiltin("Multitouch","flash.ui",Class<Multitouch>::getRef(m_sys));
	builtin->registerBuiltin("MultitouchInputMode","flash.ui",Class<MultitouchInputMode>::getRef(m_sys));

	builtin->registerBuiltin("Accelerometer", "flash.sensors",Class<Accelerometer>::getRef(m_sys));

	builtin->registerBuiltin("IOError","flash.errors",Class<IOError>::getRef(m_sys));
	builtin->registerBuiltin("EOFError","flash.errors",Class<EOFError>::getRef(m_sys));
	builtin->registerBuiltin("IllegalOperationError","flash.errors",Class<IllegalOperationError>::getRef(m_sys));
	builtin->registerBuiltin("InvalidSWFError","flash.errors",Class<InvalidSWFError>::getRef(m_sys));
	builtin->registerBuiltin("MemoryError","flash.errors",Class<MemoryError>::getRef(m_sys));
	builtin->registerBuiltin("ScriptTimeoutError","flash.errors",Class<ScriptTimeoutError>::getRef(m_sys));
	builtin->registerBuiltin("StackOverflowError","flash.errors",Class<StackOverflowError>::getRef(m_sys));

	builtin->registerBuiltin("PrintJob","flash.printing",Class<PrintJob>::getRef(m_sys));
	builtin->registerBuiltin("PrintJobOptions","flash.printing",Class<PrintJobOptions>::getRef(m_sys));
	builtin->registerBuiltin("PrintJobOrientation","flash.printing",Class<PrintJobOrientation>::getRef(m_sys));

	builtin->registerBuiltin("Collator","flash.globalization",Class<Collator>::getRef(m_sys));
	builtin->registerBuiltin("StringTools","flash.globalization",Class<StringTools>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeFormatter","flash.globalization",Class<DateTimeFormatter>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeStyle","flash.globalization",Class<DateTimeStyle>::getRef(m_sys));
	builtin->registerBuiltin("LastOperationStatus","flash.globalization",Class<LastOperationStatus>::getRef(m_sys));
	builtin->registerBuiltin("CurrencyFormatter","flash.globalization",Class<CurrencyFormatter>::getRef(m_sys));
	builtin->registerBuiltin("NumberFormatter","flash.globalization",Class<NumberFormatter>::getRef(m_sys));
	builtin->registerBuiltin("Collator","flash.globalization",Class<Collator>::getRef(m_sys));
	builtin->registerBuiltin("CollatorMode","flash.globalization",Class<CollatorMode>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeNameContext","flash.globalization",Class<DateTimeNameContext>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeNameStyle","flash.globalization",Class<DateTimeNameStyle>::getRef(m_sys));
	builtin->registerBuiltin("NationalDigitsType","flash.globalization",Class<NationalDigitsType>::getRef(m_sys));
	builtin->registerBuiltin("LocaleID","flash.globalization",Class<LocaleID>::getRef(m_sys));
	
	builtin->registerBuiltin("ClipboardFormats","flash.desktop",Class<ClipboardFormats>::getRef(m_sys));
	builtin->registerBuiltin("ClipboardTransferMode","flash.desktop",Class<ClipboardTransferMode>::getRef(m_sys));


	// avm intrinsics, not documented, but implemented in avmplus
	builtin->registerBuiltin("casi32","avm2.intrinsics.memory",_MR(Class<IFunction>::getFunction(m_sys,casi32,3)));

	//If needed add AIR definitions
	if(m_sys->flashMode==SystemState::AIR)
	{
		builtin->registerBuiltin("NativeApplication","flash.desktop",Class<NativeApplication>::getRef(m_sys));
		builtin->registerBuiltin("NativeDragManager","flash.desktop",Class<NativeDragManager>::getRef(m_sys));
		
		builtin->registerBuiltin("NativeMenuItem","flash.display",Class<NativeMenuItem>::getRef(m_sys));

		builtin->registerBuiltin("InvokeEvent","flash.events",Class<InvokeEvent>::getRef(m_sys));
		builtin->registerBuiltin("NativeDragEvent","flash.events",Class<NativeDragEvent>::getRef(m_sys));

		builtin->registerBuiltin("File","flash.filesystem",Class<ASFile>::getRef(m_sys));
		builtin->registerBuiltin("FileStream","flash.filesystem",Class<FileStream>::getRef(m_sys));

		builtin->registerBuiltin("GameInput","flash.ui",Class<GameInput>::getRef(m_sys));
		builtin->registerBuiltin("GameInputDevice","flash.ui",Class<GameInputDevice>::getRef(m_sys));
	}

	// if needed add AVMPLUS definitions
	if(m_sys->flashMode==SystemState::AVMPLUS)
	{
		builtin->registerBuiltin("getQualifiedClassName","avmplus",_MR(Class<IFunction>::getFunction(m_sys,getQualifiedClassName)));
		builtin->registerBuiltin("getQualifiedSuperclassName","avmplus",_MR(Class<IFunction>::getFunction(m_sys,getQualifiedSuperclassName)));
		builtin->registerBuiltin("getTimer","",_MR(Class<IFunction>::getFunction(m_sys,getTimer)));
		builtin->registerBuiltin("FLASH10_FLAGS","avmplus",_MR(abstract_ui(m_sys,0x7FF)));
		builtin->registerBuiltin("HIDE_NSURI_METHODS","avmplus",_MR(abstract_ui(m_sys,0x0001)));
		builtin->registerBuiltin("INCLUDE_BASES","avmplus",_MR(abstract_ui(m_sys,0x0002)));
		builtin->registerBuiltin("INCLUDE_INTERFACES","avmplus",_MR(abstract_ui(m_sys,0x0004)));
		builtin->registerBuiltin("INCLUDE_VARIABLES","avmplus",_MR(abstract_ui(m_sys,0x0008)));
		builtin->registerBuiltin("INCLUDE_ACCESSORS","avmplus",_MR(abstract_ui(m_sys,0x0010)));
		builtin->registerBuiltin("INCLUDE_METHODS","avmplus",_MR(abstract_ui(m_sys,0x0020)));
		builtin->registerBuiltin("INCLUDE_METADATA","avmplus",_MR(abstract_ui(m_sys,0x0040)));
		builtin->registerBuiltin("INCLUDE_CONSTRUCTOR","avmplus",_MR(abstract_ui(m_sys,0x0080)));
		builtin->registerBuiltin("INCLUDE_TRAITS","avmplus",_MR(abstract_ui(m_sys,0x0100)));
		builtin->registerBuiltin("USE_ITRAITS","avmplus",_MR(abstract_ui(m_sys,0x0200)));
		builtin->registerBuiltin("HIDE_OBJECT","avmplus",_MR(abstract_ui(m_sys,0x0400)));
		builtin->registerBuiltin("describeType","avmplus",_MR(Class<IFunction>::getFunction(m_sys,describeType)));
		builtin->registerBuiltin("describeTypeJSON","avmplus",_MR(Class<IFunction>::getFunction(m_sys,describeTypeJSON)),PACKAGE_INTERNAL_NAMESPACE);

		builtin->registerBuiltin("System","avmplus",Class<avmplusSystem>::getRef(m_sys));
		builtin->registerBuiltin("Domain","avmplus",Class<avmplusDomain>::getRef(m_sys));
		builtin->registerBuiltin("File","avmplus",Class<avmplusFile>::getRef(m_sys));

		builtin->registerBuiltin("AbstractBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("AbstractRestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("NativeBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("NativeBaseAS3","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("NativeSubclassOfAbstractBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("NativeSubclassOfAbstractRestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("NativeSubclassOfRestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("RestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("SubclassOfAbstractBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("SubclassOfAbstractRestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
		builtin->registerBuiltin("SubclassOfRestrictedBase","avmshell",Class<ASObject>::getRef(m_sys));
	}

	Class_object::getRef(m_sys)->getClass(m_sys)->prototype = _MNR(new_objectPrototype(m_sys));
	Class_object::getRef(m_sys)->getClass(m_sys)->initStandardProps();

	m_sys->systemDomain->registerGlobalScope(builtin);
}

void ABCVm::loadFloat(call_context *th)
{
	RUNTIME_STACK_POP_CREATE(th,arg1);
	float addr=asAtomHandler::toNumber(*arg1);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	number_t ret=appDomain->readFromDomainMemory<float>(addr);
	ASATOM_DECREF_POINTER(arg1);
	RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getSystemState(),ret,false));
}
void ABCVm::loadFloat(call_context *th,asAtom& ret, asAtom& arg1)
{
	float addr=asAtomHandler::toNumber(arg1);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	number_t res=appDomain->readFromDomainMemory<float>(addr);
	ret = asAtomHandler::fromNumber(appDomain->getSystemState(),res,false);
}

void ABCVm::loadDouble(call_context *th)
{
	RUNTIME_STACK_POP_CREATE(th,arg1);
	double addr=asAtomHandler::toNumber(*arg1);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	number_t ret=appDomain->readFromDomainMemory<double>(addr);
	ASATOM_DECREF_POINTER(arg1);
	RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getSystemState(),ret,false));
}
void ABCVm::loadDouble(call_context *th,asAtom& ret, asAtom& arg1)
{
	float addr=asAtomHandler::toNumber(arg1);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	number_t res=appDomain->readFromDomainMemory<double>(addr);
	ret = asAtomHandler::fromNumber(appDomain->getSystemState(),res,false);
}

void ABCVm::storeFloat(call_context *th)
{
	RUNTIME_STACK_POP_CREATE(th,arg1);
	RUNTIME_STACK_POP_CREATE(th,arg2);
	number_t addr=asAtomHandler::toNumber(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	float val=(float)asAtomHandler::toNumber(*arg2);
	ASATOM_DECREF_POINTER(arg2);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	appDomain->writeToDomainMemory<float>(addr, val);
}
void ABCVm::storeFloat(call_context *th, asAtom& arg1, asAtom& arg2)
{
	number_t addr=asAtomHandler::toNumber(arg1);
	float val=(float)asAtomHandler::toNumber(arg2);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	appDomain->writeToDomainMemory<float>(addr, val);
}

void ABCVm::storeDouble(call_context *th)
{
	RUNTIME_STACK_POP_CREATE(th,arg1);
	RUNTIME_STACK_POP_CREATE(th,arg2);
	number_t addr=asAtomHandler::toNumber(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	double val=asAtomHandler::toNumber(*arg2);
	ASATOM_DECREF_POINTER(arg2);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	appDomain->writeToDomainMemory<double>(addr, val);
}
void ABCVm::storeDouble(call_context *th, asAtom& arg1, asAtom& arg2)
{
	number_t addr=asAtomHandler::toNumber(arg1);
	double val=asAtomHandler::toNumber(arg2);
	ApplicationDomain* appDomain = th->mi->context->root->applicationDomain.getPtr();
	appDomain->writeToDomainMemory<double>(addr, val);
}



//Pre: we already know that n is not zero and that we are going to use an RT multiname from getMultinameRTData
multiname* ABCContext::s_getMultiname_d(call_context* th, number_t rtd, int n)
{
	//We are allowed to access only the ABCContext, as the stack is not synced
	multiname* ret;

	multiname_info* m=&th->mi->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new (getVm(th->mi->context->root->getSystemState())->vmDataMemory) multiname(getVm(th->mi->context->root->getSystemState())->vmDataMemory);
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

	multiname_info* m=&th->mi->context->constant_pool.multinames[n];
	if(m->cached==NULL)
	{
		m->cached=new (getVm(th->mi->context->root->getSystemState())->vmDataMemory) multiname(getVm(th->mi->context->root->getSystemState())->vmDataMemory);
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
	int fromStack = 0;
	if(n!=0)
	{
		const multiname_info* m=&constant_pool.multinames[n];
		if (m->cached && m->cached->isStatic)
			return m->cached;
		fromStack = m->runtimeargs;
	}
	
	asAtom rt1=asAtomHandler::invalidAtom;
	ASObject* rt2 = NULL;
	if(fromStack > 0)
	{
		if(!context)
			return NULL;
		RUNTIME_STACK_POP(context,rt1);
	}
	if(fromStack > 1)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,rt2,this->root->getSystemState());
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
				LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
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
				ret->name_i=asAtomHandler::getInt(n);
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
			ret->setName(n,root->getSystemState());
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
			ret->setName(n,root->getSystemState());
			if (isrefcounted)
			{
				ASATOM_DECREF(n);
				n2->decRef();
			}
			break;
		}
		default:
			LOG(LOG_ERROR,_("Multiname to String not yet implemented for this kind ") << hex << m->kind);
			throw UnsupportedException("Multiname to String not implemented");
	}
	return ret;
}
ABCContext::ABCContext(_R<RootMovieClip> r, istream& in, ABCVm* vm):scriptsdeclared(false),root(r),constant_pool(vm->vmDataMemory),
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

	constantAtoms_integer.resize(constant_pool.integer.size());
	for (uint32_t i = 0; i < constant_pool.integer.size(); i++)
	{
		constantAtoms_integer[i] = asAtomHandler::fromInt(constant_pool.integer[i]);
		if (asAtomHandler::getObject(constantAtoms_integer[i]))
			asAtomHandler::getObject(constantAtoms_integer[i])->setConstant();
	}
	constantAtoms_uinteger.resize(constant_pool.uinteger.size());
	for (uint32_t i = 0; i < constant_pool.uinteger.size(); i++)
	{
		constantAtoms_uinteger[i] = asAtomHandler::fromUInt(constant_pool.uinteger[i]);
		if (asAtomHandler::getObject(constantAtoms_uinteger[i]))
			asAtomHandler::getObject(constantAtoms_uinteger[i])->setConstant();
	}
	constantAtoms_doubles.resize(constant_pool.doubles.size());
	for (uint32_t i = 0; i < constant_pool.doubles.size(); i++)
	{
		ASObject* res = abstract_d_constant(root->getSystemState(),constant_pool.doubles[i]);
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
		Namespace* res = Class<Namespace>::getInstanceS(root->getSystemState(),getString(constant_pool.namespaces[i].name),BUILTIN_STRINGS::EMPTY,(NS_KIND)(int)constant_pool.namespaces[i].kind);
		if (constant_pool.namespaces[i].kind != 0)
			res->nskind =(NS_KIND)(int)(constant_pool.namespaces[i].kind);
		res->setConstant();
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
			auto it = root->getSystemState()->customclassoverriddenmethods.find(superclassName);
			if (it == root->getSystemState()->customclassoverriddenmethods.end())
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
		root->getSystemState()->customclassoverriddenmethods.insert(make_pair(className,instances[i].overriddenmethods));

		LOG(LOG_TRACE,_("Class ") << *getMultiname(instances[i].name,nullptr));
		LOG(LOG_TRACE,_("Flags:"));
		if(instances[i].isSealed())
			LOG(LOG_TRACE,_("\tSealed"));
		if(instances[i].isFinal())
			LOG(LOG_TRACE,_("\tFinal"));
		if(instances[i].isInterface())
			LOG(LOG_TRACE,_("\tInterface"));
		if(instances[i].isProtectedNs())
			LOG(LOG_TRACE,_("\tProtectedNS ") << root->getSystemState()->getStringFromUniqueId(getString(constant_pool.namespaces[instances[i].protectedNs].name)));
		if(instances[i].supername)
			LOG(LOG_TRACE,_("Super ") << *getMultiname(instances[i].supername,NULL));
		if(instances[i].interface_count)
		{
			LOG(LOG_TRACE,_("Implements"));
			for(unsigned int j=0;j<instances[i].interfaces.size();j++)
			{
				LOG(LOG_TRACE,_("\t") << *getMultiname(instances[i].interfaces[j],NULL));
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
		if(methods[method_body[i].method].body!=NULL)
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
ABCVm::ABCVm(SystemState* s, MemoryAccount* m):m_sys(s),status(CREATED),isIdle(true),shuttingdown(false),
	events_queue(reporter_allocator<eventType>(m)),idleevents_queue(reporter_allocator<eventType>(m)),nextNamespaceBase(2),currentCallContext(NULL),
	vmDataMemory(m),cur_recursion(0)
{
	limits.max_recursion = 256;
	limits.script_timeout = 20;
	m_sys=s;
	stacktrace=new stacktrace_entry[256];
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
	Locker l(event_queue_mutex);
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
	delete[] stacktrace;
}

int ABCVm::getEventQueueSize()
{
	return events_queue.size();
}

void ABCVm::publicHandleEvent(EventDispatcher* dispatcher, _R<Event> event)
{
	if (dispatcher && dispatcher->is<RootMovieClip>() && dispatcher->as<RootMovieClip>()->isWaitingForParser() && event->type == "enterFrame")
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
	if(dispatcher->classdef->isSubClass(Class<DisplayObject>::getClass(dispatcher->getSystemState())))
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
				mevent->relatedObject->incRef();
				rcur = mevent->relatedObject.getPtr();
			}
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
			(*i)->incRef();
			event->currentTarget=_MR(*i);
			(*i)->handleEvent(event);
			event->currentTarget=NullRef;
		}
	}

	//Do target phase
	if(doTarget)
	{
		event->eventPhase = EventPhase::AT_TARGET;
		dispatcher->incRef();
		event->currentTarget=_MR(dispatcher);
		dispatcher->handleEvent(event);
	}

	//Do bubbling phase
	if(event->bubbles && !parents.empty())
	{
		event->eventPhase = EventPhase::BUBBLING_PHASE;
		auto i = parents.begin();
		for(;i!=parents.end();++i)
		{
			(*i)->incRef();
			event->currentTarget=_MR(*i);
			(*i)->handleEvent(event);
			event->currentTarget=NullRef;
		}
	}
	if (event->type == "mouseDown" && dispatcher->is<InteractiveObject>())
	{
		dispatcher->incRef();
		dispatcher->getSystemState()->stage->setFocusTarget(_MR(dispatcher->as<InteractiveObject>()));
	}
	else
		dispatcher->getSystemState()->stage->setFocusTarget(NullRef);
	
	dispatcher->getSystemState()->stage->AVM1HandleEvent(dispatcher,event.getPtr());
	
	/* This must even be called if stop*Propagation has been called */
	if(!event->defaultPrevented)
		dispatcher->defaultEventBehavior(event);
	dispatcher->afterExecution(event);
	
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
					asAtom result=asAtomHandler::invalidAtom;
					if (asAtomHandler::is<AVM1Function>(ev->f))
						asAtomHandler::as<AVM1Function>(ev->f)->call(&result,&ev->obj,ev->args,ev->numArgs);
					else
						asAtomHandler::callFunction(ev->f,result,ev->obj,ev->args,ev->numArgs,true);
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
			case EXTERNAL_CALL:
			{
				ExternalCallEvent* ev=static_cast<ExternalCallEvent*>(e.second.getPtr());
				try
				{
					asAtom* newArgs=NULL;
					if (ev->numArgs > 0)
					{
						newArgs=g_newa(asAtom, ev->numArgs);
						for (uint32_t i = 0; i < ev->numArgs; i++)
						{
							newArgs[i] = asAtomHandler::fromObject(ev->args[i]);
						}
					}
					asAtom res=asAtomHandler::invalidAtom;
					asAtomHandler::callFunction(ev->f,res,asAtomHandler::nullAtom,newArgs,ev->numArgs,true);
					*(ev->result)=asAtomHandler::toObject(res,m_sys);
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
					ev->clip->advanceFrame();
				else
					m_sys->stage->advanceFrame();
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
				Locker l(event_queue_mutex);
				while (!idleevents_queue.empty())
				{
					events_queue.push_back(idleevents_queue.front());
					idleevents_queue.pop_front();
				}
				isIdle = true;
#ifndef NDEBUG
//				if (getEventQueueSize() == 0)
//					ASObject::dumpObjectCounters(100);
#endif
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
bool ABCVm::addEvent(_NR<EventDispatcher> obj ,_R<Event> ev)
{
	/* We have to run waitable events directly,
	 * because otherwise waiting on them in the vm thread
	 * will block the vm thread from executing them.
	 */
	if(isVmThread() && ev->is<WaitableEvent>())
	{
		RELEASE_WRITE(ev->queued,true);
		handleEvent( make_pair(obj,ev) );
		return true;
	}


	Locker l(event_queue_mutex);

	//If the system should terminate new events are not accepted
	if(shuttingdown)
	{
		if (ev->is<WaitableEvent>())
			ev->as<WaitableEvent>()->signal();
		return false;
	}
	if (!obj.isNull())
		obj->onNewEvent(ev.getPtr());
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	sem_event_cond.signal();
	return true;
}
void ABCVm::addIdleEvent(_NR<EventDispatcher> obj ,_R<Event> ev)
{
	Locker l(event_queue_mutex);
	//If the system should terminate new events are not accepted
	if(shuttingdown)
		return;
	idleevents_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
}

Class_inherit* ABCVm::findClassInherit(const string& s, RootMovieClip* root)
{
	LOG(LOG_CALLS,_("Setting class name to ") << s);
	ASObject* target;
	ASObject* derived_class=root->applicationDomain->getVariableByString(s,target);
	if(derived_class==NULL)
	{
		//LOG(LOG_ERROR,_("Class ") << s << _(" not found in global for ")<<root->getOrigin());
		//throw RunTimeException("Class not found in global");
		return NULL;
	}

	assert_and_throw(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	assert_and_throw(static_cast<Class_base*>(derived_class)->class_index!=-1);
	Class_inherit* derived_class_tmp=static_cast<Class_inherit*>(derived_class);
	if(derived_class_tmp->isBinded())
	{
		//LOG(LOG_ERROR, "Class already binded to a tag. Not binding:"<<s<< " class:"<<derived_class_tmp->getQualifiedClassName());
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
	// ensure that traits are initialized for movies loaded from actionscript
	base->setIsInitialized(false);
	derived_class_tmp->bindToRoot();
	// the root movie clip may have it's own constructor, so we make sure it is called
	asAtom r = asAtomHandler::fromObject(base.getPtr());
	derived_class_tmp->handleConstruction(r,nullptr,0,true);
	base->setConstructorCallComplete();
}

bool ABCVm::buildClassAndBindTag(const string& s, DictionaryTag* t)
{
	Class_inherit* derived_class_tmp = findClassInherit(s, t->loadedFrom);
	if(!derived_class_tmp)
		return false;

	//It seems to be acceptable for the same base to be binded multiple times.
	//In such cases the first binding is bidirectional (instances created using PlaceObject
	//use the binded class and instances created using 'new' use the binded tag). Any other
	//bindings will be unidirectional (only instances created using new will use the binded tag)
	if(t->bindedTo==NULL)
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
		//Flush the invalidation queue
		if (!e.first.isNull() || 
				(e.second->getEventType() != EXTERNAL_CALL 
				 && e.second->getEventType() != INIT_FRAME) // don't flush between initFrame and executeFrameScript
				)
			m_sys->flushInvalidationQueue();
		if (!e.first.isNull())
			e.first->afterHandleEvent(e.second.getPtr());
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Error in VM ") << e.cause);
		m_sys->setError(e.cause);
		/* do not allow any more event to be enqueued */
		signalEventWaiters();
	}
	catch(ASObject*& e)
	{
		if(e->getClass())
			LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM ") << e->toString());
		else
			LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM (no type)"));
		if (e->is<ASError>())
		{
			LOG(LOG_ERROR,_("Unhandled ActionScript exception in VM ") << e->as<ASError>()->getStackTraceString());
			if (m_sys->ignoreUnhandledExceptions)
				return;
			m_sys->setError(e->as<ASError>()->getStackTraceString());
		}
		else
			m_sys->setError(_("Unhandled ActionScript exception"));
		/* do not allow any more event to be enqueued */
		shuttingdown = true;
		signalEventWaiters();
	}
}

method_info* ABCContext::get_method(unsigned int m)
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



void call_context::handleError(int errorcode)
{
	throwError<ASError>(errorcode);
}

bool ABCContext::isinstance(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("isinstance ") << *name);

	if(name->name_s_id == BUILTIN_STRINGS::ANY)
		return true;
	
	ASObject* target;
	asAtom ret=asAtomHandler::invalidAtom;
	root->applicationDomain->getVariableAndTargetByMultiname(ret,*name, target);
	if(asAtomHandler::isInvalid(ret)) //Could not retrieve type
	{
		LOG(LOG_ERROR,"isInstance: Cannot retrieve type:"<<*name);
		return false;
	}

	ASObject* type=asAtomHandler::toObject(ret,obj->getSystemState());
	bool real_ret=false;
	Class_base* objc=obj->classdef;
	Class_base* c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		real_ret=(c==Class<Integer>::getClass(obj->getSystemState()) || c==Class<Number>::getClass(obj->getSystemState()) || c==Class<UInteger>::getClass(obj->getSystemState()));
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

void ABCContext::declareScripts()
{
	if (scriptsdeclared)
		return;
	//Take script entries and declare their traits
	unsigned int i=0;

	for(;i<scripts.size();i++)
	{
		LOG(LOG_CALLS, _("Script N: ") << i );

		//Creating a new global for this script
		Global* global=Class<Global>::getInstanceS(root->getSystemState(),this, i);
#ifndef NDEBUG
		global->initialized=false;
#endif
		LOG(LOG_CALLS, _("Building script traits: ") << scripts[i].trait_count );


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
	LOG(LOG_CALLS, _("Last script (Entry Point)"));
	//Creating a new global for the last script
	Global* global=root->applicationDomain->getLastGlobalScope();
	root->getSystemState()->worker->state ="running";
	getVm(root->getSystemState())->addEvent(root->getSystemState()->worker,_MR(Class<Event>::getInstanceS(root->getSystemState(),"workerState")));

	//the script init of the last script is the main entry point
	if(!lazy)
	{
		int lastscript = scripts.size()-1;
		asAtom g = asAtomHandler::fromObject(global);
		runScriptInit(lastscript, g);
	}
	LOG(LOG_CALLS, _("End of Entry Point"));
}

void ABCContext::runScriptInit(unsigned int i, asAtom &g)
{
	LOG(LOG_CALLS, "Running script init for script " << i );

	assert(!hasRunScriptInit[i]);
	hasRunScriptInit[i] = true;

	method_info* m=get_method(scripts[i].init);
	SyntheticFunction* entry=Class<IFunction>::getSyntheticFunction(this->root->getSystemState(),m,m->numArgs());
	
	entry->addToScope(scope_entry(g,false));

	asAtom ret=asAtomHandler::invalidAtom;
	asAtom f =asAtomHandler::fromObject(entry);
	asAtomHandler::callFunction(f,ret,g,NULL,0,false);

	ASATOM_DECREF(ret);

	entry->decRef();
	LOG(LOG_CALLS, "Finished script init for script " << i );
}

int ABCVm::Run(void* d)
{
	ABCVm* th = (ABCVm*)d;
	//Spin wait until the VM is aknowledged by the SystemState
	setTLSSys(th->m_sys);
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
		th->event_queue_mutex.lock();
		while(th->events_queue.empty() && !th->shuttingdown)
			th->sem_event_cond.wait(th->event_queue_mutex);
		for (auto it = th->deletableObjects.begin(); it != th->deletableObjects.end(); it++)
			(*it)->decRef();
		th->deletableObjects.clear();
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
	message->setCurrentObjectEncoding(version == 3 ? ObjectEncoding::AMF3 : ObjectEncoding::AMF0);
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
		if (marker == 0x11 && message->getCurrentObjectEncoding() != ObjectEncoding::AMF3) // switch to AMF3
		{
			message->setCurrentObjectEncoding(ObjectEncoding::AMF3);
			message->readByte(marker);
		}
		asAtom v = asAtomHandler::fromObject(message.getPtr());
		asAtom obj=asAtomHandler::invalidAtom;
		ByteArray::readObject(obj,m_sys, v, NULL, 0);

		asAtom callback=asAtomHandler::invalidAtom;
		if(!client.isNull())
			client->getVariableByMultiname(callback,headerName);

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
			asAtomHandler::callFunction(callback,r,v, callbackArgs, 1,true);
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
		if (marker == 0x11 && message->getCurrentObjectEncoding() != ObjectEncoding::AMF3) // switch to AMF3
		{
			message->setCurrentObjectEncoding(ObjectEncoding::AMF3);
			message->readByte(marker);
		}
		asAtom v = asAtomHandler::fromObject(message.getPtr());
		asAtom ret=asAtomHandler::invalidAtom;
		ByteArray::readObject(ret,m_sys,v, NULL, 0);
	
		if(!responder.isNull())
		{
			multiname onResultName(NULL);
			onResultName.name_type=multiname::NAME_STRING;
			onResultName.name_s_id=m_sys->getUniqueStringId("onResult");
			onResultName.ns.emplace_back(m_sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
			asAtom callback=asAtomHandler::invalidAtom;
			responder->getVariableByMultiname(callback,onResultName);
			if(asAtomHandler::isFunction(callback))
			{
				ASATOM_INCREF(ret);
				asAtom callbackArgs[1] { ret };
				asAtom v = asAtomHandler::fromObject(responder.getPtr());
				asAtom r=asAtomHandler::invalidAtom;
				asAtomHandler::callFunction(callback,r,v, callbackArgs, 1,true);
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

void ABCVm::throwStackOverflow()
{
	throwError<ASError>(kStackOverflowError);
}

uint32_t ABCVm::getAndIncreaseNamespaceBase(uint32_t nsNum)
{
	return ATOMIC_ADD(nextNamespaceBase,nsNum)-nsNum;
}

tiny_string ABCVm::getDefaultXMLNamespace()
{
	return m_sys->getStringFromUniqueId(currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY);
}
uint32_t ABCVm::getDefaultXMLNamespaceID()
{
	return currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY;
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
			var = c->borrowedVariables.findObjVar(nameId,nsNameAndKind(c->getSystemState(),"",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->var))
			{
				assert_and_throw(asAtomHandler::isFunction(var->var));

				ASATOM_INCREF(var->var);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->var,NORMAL_METHOD,true);
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
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind(c->getSystemState(),"",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->getter))
			{
				ASATOM_INCREF(var->getter);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->getter,GETTER_METHOD,true);
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
			var=c->borrowedVariables.findObjVar(nameId,nsNameAndKind(c->getSystemState(),"",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
			if(var && asAtomHandler::isValid(var->setter))
			{
				ASATOM_INCREF(var->setter);
				c->setDeclaredMethodAtomByQName(nameId,mname.ns[0],var->setter,SETTER_METHOD,true);
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
			LOG(LOG_ERROR,_("Constant kind ") << hex << kind);
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
			LOG(LOG_ERROR,_("Constant kind ") << hex << kind);
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
	multiname* mname=getMultiname(t->name,NULL);
	//Should be a Qname
	assert_and_throw(mname->name_type==multiname::NAME_STRING);
#ifndef NDEBUG
	if(t->kind>>4)
		LOG(LOG_CALLS,_("Next slot has flags ") << (t->kind>>4));
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
			if(obj->hasPropertyByMultiname(*mname,false,false))
				return;

			//Check if this already defined in parent applicationdomains
			ASObject* target;
			asAtom oldDefinition=asAtomHandler::invalidAtom;
			root->applicationDomain->getVariableAndTargetByMultiname(oldDefinition,*mname, target);
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
				ci->isInterface = true;
				ci->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(obj->getSystemState(),Class_base::_toString),NORMAL_METHOD,false);
				LOG(LOG_CALLS,_("Building class traits"));
				for(unsigned int i=0;i<classes[t->classi].trait_count;i++)
					buildTrait(ci,additionalslots,&classes[t->classi].traits[i],false);
				//Add protected namespace if needed
				if((instances[t->classi].flags)&0x08)
				{
					ci->use_protected=true;
					int ns=instances[t->classi].protectedNs;
					const namespace_info& ns_info=constant_pool.namespaces[ns];
					ci->initializeProtectedNamespace(getString(ns_info.name),ns_info,root.getPtr());
				}
				LOG(LOG_CALLS,_("Adding immutable object traits to class"));
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

				if(instances[t->classi].supername)
				{
					// set superclass for classes that are not instantiated by newClass opcode (e.g. buttons)
					multiname mnsuper = *getMultiname(instances[t->classi].supername,NULL);
					ASObject* superclass=root->applicationDomain->getVariableByMultinameOpportunistic(mnsuper);
					if(superclass && superclass->is<Class_base>() && !superclass->is<Class_inherit>())
					{
						superclass->incRef();
						c->setSuper(_MR(superclass->as<Class_base>()));
					}
				}
				root->applicationDomain->classesBeingDefined.insert(make_pair(mname, c));
				ret=c;
				c->setIsInitialized();
			}
			// the variable on the Definition object is set to null now (it will be set to the real value after the class init function was executed in newclass opcode)
			// testing for class==null in actionscript code is used to determine if the class initializer function has been called
			variable* v = obj->is<Global>() ? obj->setVariableAtomByQName(mname->name_s_id,mname->ns[0], asAtomHandler::nullAtom,DECLARED_TRAIT)
											: obj->setVariableByQName(mname->name_s_id,mname->ns[0], ret,DECLARED_TRAIT);

			LOG(LOG_CALLS,_("Class slot ")<< t->slot_id << _(" type Class name ") << *mname << _(" id ") << t->classi);
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
				LOG(LOG_CALLS,"Getter trait: " << *mname << _(" #") << t->method);
			else if(kind == traits_info::Setter)
				LOG(LOG_CALLS,"Setter trait: " << *mname << _(" #") << t->method);
			else if(kind == traits_info::Method)
				LOG(LOG_CALLS,"Method trait: " << *mname << _(" #") << t->method);
			method_info* m=&methods[t->method];
			SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(obj->getSystemState(),m,m->numArgs());

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
			if(checkExisting && obj->hasPropertyByMultiname(*mname,false,false))
				return;

			multiname* tname=getMultiname(t->type_name,NULL);
			asAtom ret=asAtomHandler::invalidAtom;
			//If the index is valid we set the constant
			if(t->vindex)
				getConstant(ret,t->vkind,t->vindex);
			else if(tname->name_type == multiname::NAME_STRING && tname->name_s_id==BUILTIN_STRINGS::ANY
					&& tname->ns.size() == 1 && tname->hasEmptyNS)
				asAtomHandler::setUndefined(ret);
			else
				asAtomHandler::setNull(ret);

			LOG(LOG_CALLS,_("Const ") << *mname <<_(" type ")<< *tname<< " = " << asAtomHandler::toDebugString(ret));

			obj->initializeVariableByMultiname(*mname, ret, tname, this, CONSTANT_TRAIT,t->slot_id,isenumerable);
			break;
		}
		case traits_info::Slot:
		{
			//Check if this already defined in upper levels
			if(checkExisting && obj->hasPropertyByMultiname(*mname,false,false))
				return;

			multiname* tname=getMultiname(t->type_name,NULL);
			asAtom ret=asAtomHandler::invalidAtom;
			if(t->vindex)
			{
				getConstant(ret,t->vkind,t->vindex);
				LOG_CALL(_("Slot ") << t->slot_id << ' ' << *mname <<_(" type ")<<*tname<< " = " << asAtomHandler::toDebugString(ret) <<" "<<isBorrowed);
			}
			else
			{
				LOG_CALL(_("Slot ")<< t->slot_id<<  _(" vindex 0 ") << *mname <<_(" type ")<<*tname<<" "<<isBorrowed);
				ret = asAtomHandler::invalidAtom;
			}

			obj->initializeVariableByMultiname(*mname, ret, tname, this, isBorrowed ? INSTANCE_TRAIT : DECLARED_TRAIT,t->slot_id,isenumerable);
			if (t->slot_id == 0)
				additionalslots.push_back(mname);
			break;
		}
		default:
			LOG(LOG_ERROR,_("Trait not supported ") << *mname << _(" ") << t->kind);
			obj->setVariableByMultiname(*mname, asAtomHandler::undefinedAtom, ASObject::CONST_NOT_ALLOWED);
			break;
	}
}


void method_info::getOptional(asAtom& ret, unsigned int i)
{
	assert_and_throw(i<info.options.size());
	context->getConstant(ret,info.options[i].kind,info.options[i].val);
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
