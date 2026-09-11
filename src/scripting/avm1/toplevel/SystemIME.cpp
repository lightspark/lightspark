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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/SystemIME.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::system_ime` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

using AVM1SystemIME;

static constexpr auto objDecls =
{
	AVM1Decl("UNKNOWN", "UNKNOWN", protoFlags),
	AVM1Decl("KOREAN", "KOREAN", protoFlags),
	AVM1Decl("JAPANESE_KATAKANA_HALF", "JAPANESE_KATAKANA_HALF", protoFlags),
	AVM1Decl("JAPANESE_KATAKANA_FULL", "JAPANESE_KATAKANA_FULL", protoFlags),
	AVM1Decl("JAPANESE_HIRAGANA", "JAPANESE_HIRAGANA", protoFlags),
	AVM1Decl("CHINESE", "CHINESE", protoFlags),
	AVM1Decl("ALPHANUMERIC_HALF", "ALPHANUMERIC_HALF", protoFlags),
	AVM1Decl("ALPHANUMERIC_FULL", "ALPHANUMERIC_FULL", protoFlags),
	AVM1_FUNCTION_PROTO(getEnabled, protoFlags),
	AVM1_FUNCTION_PROTO(setEnabled, protoFlags),
	AVM1_FUNCTION_PROTO(getConversionMode, protoFlags),
	AVM1_FUNCTION_PROTO(setConversionMode, protoFlags),
	AVM1_FUNCTION_PROTO(setCompositionString, protoFlags),
	AVM1_FUNCTION_PROTO(doConversion, protoFlags)
};

AVM1SystemIME::AVM1SystemIME(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1SystemIME, getEnabled)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.getEnabled()` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1SystemIME, setEnabled)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.setEnabled()` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1SystemIME, getConversionMode)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.getConversionMode()` is a stub.");
	return "KOREAN";
}

AVM1_FUNCTION_BODY(AVM1SystemIME, setConversionMode)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.setConversionMode()` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1SystemIME, setCompositionString)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.setCompositionString()` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1SystemIME, doConversion)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.IME.doConversion()` is a stub.");
	return true;
}
