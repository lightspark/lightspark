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

#ifndef SCRIPTING_AVM1_TOPLEVEL_BUTTON_H
#define SCRIPTING_AVM1_TOPLEVEL_BUTTON_H 1

#include "scripting/avm1/object/display_object.h"

// Based on Ruffle's `avm1::globals::button` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class GcContext;
class Button;

class AVM1Button : public AVM1DisplayObject
{
public:
	AVM1Button(AVM1Activation& act, const GcPtr<Button>& button);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_PROPERTY_TYPE_DECL(Button, Scale9Grid);
	AVM1_PROPERTY_TYPE_DECL(Button, BlendMode);
	AVM1_PROPERTY_TYPE_DECL(Button, CacheAsBitmap);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_BUTTON_H */
