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

#ifndef SCRIPTING_AVM1_TOPLEVEL_TEXTFORMAT_H
#define SCRIPTING_AVM1_TOPLEVEL_TEXTFORMAT_H 1

#include "backends/html/text_format.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::text_format` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class GcContext;

class AVM1TextFormat : public AVM1Object, public TextFormat
{
public:
	AVM1TextFormat(AVM1Activation& act);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextFormat, getTextExtent);

	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Font);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Size);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Color);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, URL);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Target);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Bold);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Italic);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Underline);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Align);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, LeftMargin);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, RightMargin);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Indent);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Leading);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, BlockIndent);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, TabStops);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Bullet);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Display);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, Kerning);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextFormat, LetterSpacing);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_TEXTFORMAT_H */
