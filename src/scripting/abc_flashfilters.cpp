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

#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/filters/BevelFilter.h"
#include "scripting/flash/filters/BlurFilter.h"
#include "scripting/flash/filters/ColorMatrixFilter.h"
#include "scripting/flash/filters/ConvolutionFilter.h"
#include "scripting/flash/filters/DisplacementMapFilter.h"
#include "scripting/flash/filters/DropShadowFilter.h"
#include "scripting/flash/filters/GlowFilter.h"
#include "scripting/flash/filters/GradientBevelFilter.h"
#include "scripting/flash/filters/GradientGlowFilter.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashFilters(Global* builtin)
{
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
}
