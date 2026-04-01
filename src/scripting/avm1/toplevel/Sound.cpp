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

#include <initializer_list>
#include <sstream>

#include "backends/audio.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/Sound.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::sound` crate.

AVM1Sound::AVM1Sound
(
	AVM1Activation& act,
	NullableGcPtr<MovieClip> _clip
) : AVM1Object
(
	act,
	act.getPrototypes().sound->proto
),
soundInstance(nullptr),
clip(_clip),
soundPos(0),
isStreaming(false)
{
}

template<bool requiresSWF6 = false>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

template<>
constexpr auto protoFlags<true> =
(
	protoFlags<> |
	AVM1PropFlags::Version6
);

using AVM1Sound;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Pan, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Transform, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Volume, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, stop, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, attachSound, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, start, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Duration, protoFlags<true>),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Position, protoFlags<true>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, loadSound, protoFlags)<true>,
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, getBytesLoaded, protoFlags<true>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, getBytesTotal, protoFlags<true>),
};

GcPtr<AVM1SystemClass> AVM1Sound::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Sound, ctor)
{
	// The 1st argument is the `MovieClip` that "owns" all sounds started
	// by this object. `Sound.{setTransform,stop}()`, etc. will affect
	// all sounds owned by this clip, in that case.
	NullableGcPtr<MovieClip> clip;
	AVM1_ARG_UNPACK(clip);

	if (!clip.isNull())
	{
		act.resolveTargetClip
		(
			act.getTargetOrRootClip(),
			clip,
			false
		);
	}

	return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1Sound
	(
		act,
		clip
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getPan)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setPan)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getTransform)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setTransform)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getVolume)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setVolume)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, stop)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, attachSound)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, start)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getDuration)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setDuration)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getPosition)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setPosition)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, loadSound)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getBytesLoaded)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getBytesTotal)
{
}
