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

#ifndef SCRIPTING_AVM1_TOPLEVEL_SYSTEM_H
#define SCRIPTING_AVM1_TOPLEVEL_SYSTEM_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::system` crate.

namespace lightspark
{

class AVM1DeclContext;

enum class SettingsPanel : uint8_t
{
	Privacy,
	LocalStorage,
	Microphone,
	Camera,
};

class AVM1System : public AVM1Object
{
public:
	AVM1System(AVM1DeclContext& ctx);

	AVM1_PROPERTY_DECL(ExactSettings);
	AVM1_PROPERTY_DECL(UseCodepage);
	AVM1_FUNCTION_DECL(setClipboard);
	AVM1_FUNCTION_DECL(showSettings);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_SYSTEM_H */
