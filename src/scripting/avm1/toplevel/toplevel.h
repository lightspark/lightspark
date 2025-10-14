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

class ASBroadcasterFuncs;
class AVM1Activation;
class AVM1Object;
class AVM1Value;
class DisplayObject;
class GcContext;
template<typename T>
class Optional;

struct AVM1SystemPrototypes
{
	GcPtr<AVM1Object> button;
	GcPtr<AVM1Object> object;
	GcPtr<AVM1Object> objectCtor;
	GcPtr<AVM1Object> function;
	GcPtr<AVM1Object> movieClip;
	GcPtr<AVM1Object> sound;
	GcPtr<AVM1Object> textField;
	GcPtr<AVM1Object> textFormat;
	GcPtr<AVM1Object> array;
	GcPtr<AVM1Object> arrayCtor;
	GcPtr<AVM1Object> xmlNodeCtor;
	GcPtr<AVM1Object> xmlCtor;
	GcPtr<AVM1Object> string;
	GcPtr<AVM1Object> number;
	GcPtr<AVM1Object> boolean;
	GcPtr<AVM1Object> matrix;
	GcPtr<AVM1Object> matrixCtor;
	GcPtr<AVM1Object> point;
	GcPtr<AVM1Object> pointCtor;
	GcPtr<AVM1Object> rectangle;
	GcPtr<AVM1Object> rectangleCtor;
	GcPtr<AVM1Object> transformCtor;
	GcPtr<AVM1Object> sharedObjectCtor;
	GcPtr<AVM1Object> colorTransform;
	GcPtr<AVM1Object> colorTransformCtor;
	GcPtr<AVM1Object> contextMenu;
	GcPtr<AVM1Object> contextMenuCtor;
	GcPtr<AVM1Object> contextMenuItem;
	GcPtr<AVM1Object> contextMenuItemCtor;
	GcPtr<AVM1Object> dateCtor;
	GcPtr<AVM1Object> bitmapData;
	GcPtr<AVM1Object> video;
	GcPtr<AVM1Object> blurFilter;
	GcPtr<AVM1Object> bevelFilter;
	GcPtr<AVM1Object> glowFilter;
	GcPtr<AVM1Object> dropShadowFilter;
	GcPtr<AVM1Object> colorMatrixFilter;
	GcPtr<AVM1Object> displacementMapFilter;
	GcPtr<AVM1Object> convolutionFilter;
	GcPtr<AVM1Object> gradientBevelFilter;
	GcPtr<AVM1Object> gradientGlowFilter;
};

AVM1Value parseIntImpl
(
	AVM1Activation& act,
	const AVM1Value& str,
	Optional<const AVM1Value&> radix
);

using CreateGlobalsType = std::tuple
<
	GcPtr<AVM1SystemPrototypes>,
	GcPtr<AVM1Object>,
	GcPtr<ASBroadcasterFuncs>
>;

// Initialize the default global scope, and builtins for an AVM1 instance.
CreateGlobalsType createGlobals(GcContext& ctx);

// Depths used/returned by ActionScript are offset by this amount from
// depths used inside the SWF, or by the VM.
// The depth of `DisplayObject`s placed on the timeline in Flash (the
// authoring tool) starts at 0 in the SWF, but is negative when returned
// by `MovieClip.getDepth()`.
// Add this to convert from AVM1 depth, to SWF depth.
constexpr size_t AVM1depthOffset = 16384;

// The mximum depth that AVM1 allows when swapping, or attching clips.
constexpr size_t AVM1maxDepth = 2130706428;

// The mximum depth that AVM1 allows when removing clips.
constexpr size_t AVM1maxRemoveDepth = 2130706416;

void removeDisplayObject
(
	AVM1Activation& activation,
	const GcPtr<DisplayObject>& _this
);

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_TOPLEVEL_H */
