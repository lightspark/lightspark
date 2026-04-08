/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_VIDEO_H
#define SCRIPTING_AVM1_TOPLEVEL_VIDEO_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/display_object.h"

// Based on both Ruffle's `avm1::globals::video` crate, and Gnash's
// implementation of `Video`.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class GcContext;
class Video;

class AVM1Video : public AVM1DisplayObject
{
public:
	AVM1Video(AVM1Activation& act, GcPtr<Video> video);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_TYPE_DECL(AVM1Video, attachVideo);
	AVM1_FUNCTION_TYPE_DECL(AVM1Video, clear);
	AVM1_PROPERTY_TYPE_DECL(AVM1Video, Deblocking);
	AVM1_PROPERTY_TYPE_DECL(AVM1Video, Smoothing);
	AVM1_GETTER_TYPE_DECL(AVM1Video, Width);
	AVM1_GETTER_TYPE_DECL(AVM1Video, Height);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_VIDEO_H */
