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

#ifndef SCRIPTING_AVM1_TOPLEVEL_MICROPHONE_H
#define SCRIPTING_AVM1_TOPLEVEL_MICROPHONE_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::microphone` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1Microphone : public AVM1Object
{
public:
	AVM1Microphone(AVM1Activation& act);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	// Prototype methods/properties.
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setSilenceLevel);
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setCodec);
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setEncodeQuality);
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setFramesPerPacket);
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setRate;
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setGain);
	AVM1_FUNCTION_TYPE_DECL(AVM1Microphone, setUseEchoSuppression);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, ActivityLevel);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Codec);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, EncodeQuality);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, FramesPerPacket);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Gain);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Index);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Muted);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Name);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, Rate);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, SilenceLevel);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, SilenceTimeout);
	AVM1_GETTER_TYPE_DECL(AVM1Microphone, UseEchoSuppression);

	// Object methods/properties.
	AVM1_FUNCTION_DECL(get);
	AVM1_GETTER_DECL(Names);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_MICROPHONE_H */
