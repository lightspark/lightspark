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

#ifndef SCRIPTING_AVM1_TOPLEVEL_MOVIECLIP_H
#define SCRIPTING_AVM1_TOPLEVEL_MOVIECLIP_H 1

#include "scripting/avm1/object/display_object.h"

// Based on Ruffle's `avm1::globals::movie_clip` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class DisplayObject;
class GcContext;
class MovieClip;
template<typename T>
class Optional;
struct RectF;

#define AVM1_MOVIECLIP_GETTER_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<MovieClip>& _this \
	)
#define AVM1_MOVIECLIP_GETTER_DECL(name) \
	static AVM1Value get##name(AVM1_MOVIECLIP_GETTER_ARGS)
#define AVM1_MOVIECLIP_GETTER_BODY(name) \
	AVM1Value AVM1MovieClip::get##name(AVM1_MOVIECLIP_GETTER_ARGS)

#define AVM1_MOVIECLIP_SETTER_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<MovieClip>& _this, \
		const AVM1Value& value \
	)
#define AVM1_MOVIECLIP_SETTER_DECL(name) \
	static void set##name(AVM1_MOVIECLIP_SETTER_ARGS)
#define AVM1_MOVIECLIP_SETTER_BODY(name) \
	void AVM1MovieClip::set##name(AVM1_MOVIECLIP_SETTER_ARGS)

#define AVM1_MOVIECLIP_GETTER_SETTER_DECL(name) \
	AVM1_MOVIECLIP_GETTER_DECL(name); \
	AVM1_MOVIECLIP_SETTER_DECL(name)

#define AVM1_MOVIECLIP_FUNC_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<MovieClip>& _this, \
		const std::vector<AVM1Value>& args \
	)
#define AVM1_MOVIECLIP_FUNC_DECL(name, ...) \
	static AVM1Value name(AVM1_MOVIECLIP_FUNC_ARGS, ##__VA_ARGS__)
#define AVM1_MOVIECLIP_FUNC_BODY(name) \
	AVM1Value AVM1MovieClip::name(AVM1_MOVIECLIP_FUNC_ARGS, ##__VA_ARGS__)

class AVM1MovieClip : public AVM1DisplayObject
{
public:
	AVM1MovieClip(AVM1Activation& act, const GcPtr<MovieClip>& clip);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	// TODO: Use twips instead of float.
	static AVM1Value makeRect(AVM1Activation& act, const RectF& rect);
	static Optional<RectF> objectToRect
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& obj
	);
	
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(ScrollRect);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(Scale9Grid);
	AVM1_MOVIECLIP_FUNC_DECL(hitTest);
	AVM1_MOVIECLIP_FUNC_DECL(attachBitmap);
	AVM1_MOVIECLIP_FUNC_DECL(attachAudio);
	AVM1_MOVIECLIP_FUNC_DECL(lineStyle);
	AVM1_MOVIECLIP_FUNC_DECL(lineGradientStyle);
	AVM1_MOVIECLIP_FUNC_DECL(beginFill);
	AVM1_MOVIECLIP_FUNC_DECL(beginBitmapFill);
	AVM1_MOVIECLIP_FUNC_DECL(beginGradientFill);
	AVM1_MOVIECLIP_FUNC_DECL(moveTo);
	AVM1_MOVIECLIP_FUNC_DECL(lineTo);
	AVM1_MOVIECLIP_FUNC_DECL(curveTo);
	AVM1_MOVIECLIP_FUNC_DECL(endFill);
	AVM1_MOVIECLIP_FUNC_DECL(clear);
	AVM1_MOVIECLIP_FUNC_DECL(attachMovie);
	AVM1_MOVIECLIP_FUNC_DECL(createEmptyMovieClip);
	AVM1_MOVIECLIP_FUNC_DECL(createTextField);
	AVM1_MOVIECLIP_FUNC_DECL(duplicateMovieClip);
	static NullableGcPtr<MovieClip> cloneSprite
	(
		const GcContext& ctx,
		const GcPtr<MovieClip>& clip,
		const tiny_string& target,
		uint16_t depth,
		const NullableGcPtr<AVM1Object>& initObj
	);

	AVM1_MOVIECLIP_FUNC_DECL(getBytesLoaded);
	AVM1_MOVIECLIP_FUNC_DECL(getBytesTotal);
	AVM1_MOVIECLIP_FUNC_DECL(getInstanceAtDepth);
	AVM1_MOVIECLIP_FUNC_DECL(getNextHighestDepth);
	AVM1_MOVIECLIP_FUNC_DECL(gotoAndPlay);
	AVM1_MOVIECLIP_FUNC_DECL(gotoAndStop);
	AVM1_MOVIECLIP_FUNC_DECL(gotoFrame, bool stop, uint16_t sceneOffset);
	AVM1_MOVIECLIP_FUNC_DECL(nextFrame);
	AVM1_MOVIECLIP_FUNC_DECL(play);
	AVM1_MOVIECLIP_FUNC_DECL(prevFrame);
	AVM1_FUNCTION_DECL(removeMovieClip);
	AVM1_MOVIECLIP_FUNC_DECL(setMask);
	AVM1_MOVIECLIP_FUNC_DECL(startDrag);
	static void startDragImpl
	(
		AVM1Activation& act,
		const GcPtr<DisplayObject>& clip,
		bool lockCenter,
		const Optional<RectF>& constraint
	);

	AVM1_MOVIECLIP_FUNC_DECL(stop);
	AVM1_MOVIECLIP_FUNC_DECL(stopDrag);
	AVM1_MOVIECLIP_FUNC_DECL(swapDepths);
	AVM1_MOVIECLIP_FUNC_DECL(localToGlobal);
	AVM1_MOVIECLIP_FUNC_DECL(getBounds);
	AVM1_MOVIECLIP_FUNC_DECL(getRect);
	AVM1_MOVIECLIP_FUNC_DECL(getSWFVersion);
	AVM1_MOVIECLIP_FUNC_DECL(getURL);
	AVM1_MOVIECLIP_FUNC_DECL(globalToLocal);
	AVM1_MOVIECLIP_FUNC_DECL(loadMovie);
	AVM1_MOVIECLIP_FUNC_DECL(loadVariables);
	AVM1_MOVIECLIP_FUNC_DECL(unloadMovie);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(Transform);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(LockRoot);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(BlendMode);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(CacheAsBitmap);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(OpaqueBackground);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(Filters);
	AVM1_MOVIECLIP_GETTER_SETTER_DECL(TabIndex);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_MOVIECLIP_H */
