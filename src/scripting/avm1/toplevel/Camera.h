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

#ifndef SCRIPTING_AVM1_TOPLEVEL_CAMERA_H
#define SCRIPTING_AVM1_TOPLEVEL_CAMERA_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::camera` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1Camera : public AVM1Object
{
public:
	AVM1Camera(AVM1Activation& act);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	// Prototype methods/properties.
	AVM1_FUNCTION_TYPE_DECL(AVM1Camera, setLoopback);
	AVM1_FUNCTION_TYPE_DECL(AVM1Camera, setMode);
	AVM1_FUNCTION_TYPE_DECL(AVM1Camera, setMotionLevel);
	AVM1_FUNCTION_TYPE_DECL(AVM1Camera, setQuality);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, ActivityLevel);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Bandwidth);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, CurrentFps);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Fps);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Height);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Index);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, KeyFrameInterval);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Loopback);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, MotionLevel);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, MotionTimeout);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Muted);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Name);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Quality);
	AVM1_GETTER_TYPE_DECL(AVM1Camera, Width);

	// Object methods/properties.
	AVM1_FUNCTION_DECL(get);
	AVM1_GETTER_DECL(Names);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_CAMERA_H */
