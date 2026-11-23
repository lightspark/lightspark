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

#ifndef SCRIPTING_AVM1_TOPLEVEL_TEXTFIELD_H
#define SCRIPTING_AVM1_TOPLEVEL_TEXTFIELD_H 1

#include "scripting/avm1/function.h"
#include "scripting/avm1/object/display_object.h"

// Based on Ruffle's `avm1::globals::text_field` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class GcContext;
class TextField;

class AVM1TextField : public AVM1DisplayObject
{
public:
	AVM1TextField(AVM1Activation& act, const GcPtr<TextField>& textField);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, replaceSel);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, getTextFormat);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, setTextFormat);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, removeTextField);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, getNewTextFormat);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, setNewTextFormat);
	AVM1_FUNCTION_TYPE_DECL(AVM1TextField, replaceText);

	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, GridFitType);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, AntiAliasType);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Thickness);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Sharpness);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Scroll);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, MaxScroll);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, BorderColor);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, BackgroundColor);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, TextColor);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, AutoSize);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Text);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Type);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, HtmlText);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Variable);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, HScroll);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, MaxHScroll);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, MaxChars);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, EmbedFonts);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Html);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Border);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Background);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, WordWrap);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Password);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Multiline);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Selectable);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, Length);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, BottomScroll);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, TextWidth);
	AVM1_GETTER_TYPE_DECL(AVM1TextField, TextHeight);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, Restrict);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, CondenseWhite);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, MouseWheelEnabled);
	AVM1_PROPERTY_TYPE_DECL(AVM1TextField, StyleSheet);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_TEXTFIELD_H */
