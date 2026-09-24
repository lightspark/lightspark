/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_TOPLEVEL_H
#define SCRIPTING_AVM1_TOPLEVEL_TOPLEVEL_H 1

#include <tuple>

#include "gc/ptr.h"

// Based on Ruffle's `avm1::globals` crate.

namespace lightspark
{

class AsBroadcasterFuncs;
class AVM1Activation;
class AVM1Global;
class AVM1Object;
class AVM1SystemClass;
class AVM1Value;
class DisplayObject;
class GcContext;
template<typename T>
class Optional;

struct AVM1SystemPrototypes
{
	GcPtr<AVM1SystemClass> button;
	GcPtr<AVM1SystemClass> object;
	GcPtr<AVM1SystemClass> function;
	GcPtr<AVM1SystemClass> movieClip;
	GcPtr<AVM1SystemClass> sound;
	GcPtr<AVM1SystemClass> textField;
	GcPtr<AVM1SystemClass> textFormat;
	GcPtr<AVM1SystemClass> array;
	GcPtr<AVM1SystemClass> xmlNode;
	GcPtr<AVM1SystemClass> xml;
	GcPtr<AVM1SystemClass> string;
	GcPtr<AVM1SystemClass> number;
	GcPtr<AVM1SystemClass> boolean;
	GcPtr<AVM1SystemClass> matrix;
	GcPtr<AVM1SystemClass> point;
	GcPtr<AVM1SystemClass> rectangle;
	GcPtr<AVM1SystemClass> transform;
	GcPtr<AVM1SystemClass> sharedObject;
	GcPtr<AVM1SystemClass> colorTransform;
	GcPtr<AVM1SystemClass> contextMenu;
	GcPtr<AVM1SystemClass> contextMenuItem;
	GcPtr<AVM1SystemClass> date;
	GcPtr<AVM1SystemClass> bitmapData;
	GcPtr<AVM1SystemClass> video;
	GcPtr<AVM1SystemClass> blurFilter;
	GcPtr<AVM1SystemClass> bevelFilter;
	GcPtr<AVM1SystemClass> glowFilter;
	GcPtr<AVM1SystemClass> dropShadowFilter;
	GcPtr<AVM1SystemClass> colorMatrixFilter;
	GcPtr<AVM1SystemClass> displacementMapFilter;
	GcPtr<AVM1SystemClass> convolutionFilter;
	GcPtr<AVM1SystemClass> gradientBevelFilter;
	GcPtr<AVM1SystemClass> gradientGlowFilter;
};

AVM1Value parseIntImpl
(
	AVM1Activation& act,
	const tiny_string& str,
	const Optional<int32_t>& radix
);

using CreateGlobalsType = std::tuple
<
	AVM1SystemPrototypes,
	GcPtr<AVM1Global>,
	GcPtr<AsBroadcasterFuncs>
>;

// Initialize the default global scope, and builtins for an AVM1 instance.
CreateGlobalsType createGlobals(GcContext& ctx);

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_TOPLEVEL_H */
