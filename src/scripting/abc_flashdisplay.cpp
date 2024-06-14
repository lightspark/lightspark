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

#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/Bitmap.h"
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
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/MorphShape.h"
#include "scripting/flash/display/MovieClip.h"
#include "scripting/flash/display/NativeWindow.h"
#include "scripting/flash/display/Shape.h"
#include "scripting/flash/display/SimpleButton.h"
#include "scripting/flash/display/Stage3D.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"

using namespace lightspark;

void ABCVm::registerClassesFlashDisplay(Global* builtin)
{
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
	
	if(m_sys->flashMode==SystemState::AIR)
		builtin->registerBuiltin("NativeWindow","flash.display",Class<NativeWindow>::getRef(m_sys));
}
