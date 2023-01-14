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

#include "scripting/avm1/avm1key.h"
#include "scripting/avm1/avm1sound.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1media.h"
#include "scripting/avm1/avm1net.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/avm1/avm1ui.h"
#include "scripting/avm1/avm1xml.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesAVM1()
{
	if (m_sys->avm1global)
		return;
	Global* builtinavm1 = Class<Global>::getInstanceS(m_sys->worker,(ABCContext*)nullptr, 0,true);
	builtinavm1->setRefConstant();

	registerClassesToplevel(builtinavm1);

	Class<ASObject>::getRef(m_sys)->setDeclaredMethodByQName("addProperty","",Class<IFunction>::getFunction(m_sys,ASObject::addProperty),NORMAL_METHOD,true);
	Class<ASObject>::getRef(m_sys)->prototype->setVariableByQName("addProperty","",Class<IFunction>::getFunction(m_sys,ASObject::addProperty),DYNAMIC_TRAIT);
	Class<ASObject>::getRef(m_sys)->setDeclaredMethodByQName("registerClass","",Class<IFunction>::getFunction(m_sys,ASObject::registerClass),NORMAL_METHOD,false);
	Class<ASObject>::getRef(m_sys)->prototype->setVariableByQName("registerClass","",Class<IFunction>::getFunction(m_sys,ASObject::registerClass),DYNAMIC_TRAIT);

	builtinavm1->registerBuiltin("ASSetPropFlags","",_MR(Class<IFunction>::getFunction(m_sys,AVM1_ASSetPropFlags)));
	builtinavm1->registerBuiltin("setInterval","",_MR(Class<IFunction>::getFunction(m_sys,setInterval)));
	builtinavm1->registerBuiltin("clearInterval","",_MR(Class<IFunction>::getFunction(m_sys,clearInterval)));
	builtinavm1->registerBuiltin("setTimeout","",_MR(Class<IFunction>::getFunction(m_sys,setTimeout)));
	builtinavm1->registerBuiltin("updateAfterEvent","",_MR(Class<IFunction>::getFunction(m_sys,AVM1_updateAfterEvent)));

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
	builtinavm1->registerBuiltin("ContextMenuItem","",Class<AVM1ContextMenuItem>::getRef(m_sys));
	builtinavm1->registerBuiltin("TextField","",Class<AVM1TextField>::getRef(m_sys));
	builtinavm1->registerBuiltin("TextFormat","",Class<AVM1TextFormat>::getRef(m_sys));
	builtinavm1->registerBuiltin("XML","",Class<AVM1XMLDocument>::getRef(m_sys));
	builtinavm1->registerBuiltin("XMLNode","",Class<XMLNode>::getRef(m_sys));
	builtinavm1->registerBuiltin("XMLSocket","",Class<AVM1XMLSocket>::getRef(m_sys));

	builtinavm1->registerBuiltin("NetConnection","",Class<AVM1NetConnection>::getRef(m_sys));
	builtinavm1->registerBuiltin("NetStream","",Class<AVM1NetStream>::getRef(m_sys));
	builtinavm1->registerBuiltin("Video","",Class<AVM1Video>::getRef(m_sys));
	builtinavm1->registerBuiltin("AsBroadcaster","",Class<AVM1Broadcaster>::getRef(m_sys));
	builtinavm1->registerBuiltin("LocalConnection","",Class<AVM1LocalConnection>::getRef(m_sys));
	builtinavm1->registerBuiltin("LoadVars","",Class<AVM1LoadVars>::getRef(m_sys));

	if (m_sys->getSwfVersion() >= 6)
	{
		ASObject* systempackage = Class<ASObject>::getInstanceS(m_sys->worker);
		builtinavm1->setVariableByQName("System",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),systempackage,CONSTANT_TRAIT);
		
		systempackage->setVariableByQName("security","System",Class<Security>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		systempackage->setVariableByQName("capabilities","System",Class<Capabilities>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
	}
	if (m_sys->getSwfVersion() >= 8)
	{
		ASObject* flashpackage = Class<ASObject>::getInstanceS(m_sys->worker);
		builtinavm1->setVariableByQName("flash",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashpackage,CONSTANT_TRAIT);

		ASObject* flashdisplaypackage = Class<ASObject>::getInstanceS(m_sys->worker);
		flashpackage->setVariableByQName("display",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashdisplaypackage,CONSTANT_TRAIT);

		flashdisplaypackage->setVariableByQName("BitmapData","flash.display",Class<AVM1BitmapData>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);

		ASObject* flashfilterspackage = Class<ASObject>::getInstanceS(m_sys->worker);
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

		ASObject* flashgeompackage = Class<ASObject>::getInstanceS(m_sys->worker);
		flashpackage->setVariableByQName("geom",nsNameAndKind(m_sys,"",PACKAGE_NAMESPACE),flashgeompackage,CONSTANT_TRAIT);

		flashgeompackage->setVariableByQName("Matrix","flash.geom",Class<Matrix>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("ColorTransform","flash.geom",Class<ColorTransform>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Transform","flash.geom",Class<Transform>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Rectangle","flash.geom",Class<Rectangle>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
		flashgeompackage->setVariableByQName("Point","flash.geom",Class<Point>::getRef(m_sys).getPtr(),CONSTANT_TRAIT);
	}
	m_sys->avm1global=builtinavm1;
}
