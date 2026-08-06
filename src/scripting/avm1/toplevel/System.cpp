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
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::system` crate.

using AVM1System;

static constexpr auto objDecls =
{
	AVM1_PROPERTY_PROTO(exactSettings, ExactSettings),
	AVM1_PROPERTY_PROTO(useCodepage, UseCodepage),
	AVM1_FUNCTION_PROTO(setClipboard),
	AVM1_FUNCTION_PROTO(showSettings)
};

AVM1System::AVM1System(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

AVM1_GETTER_BODY(AVM1System, ExactSettings)
{
	return act.getSys()->securityManager->getExactSettings();
}

AVM1_SETTER_BODY(AVM1System, ExactSettings)
{
	auto secMgr = act.getSys()->securityManager;
	return secMgr->setExactSettings(value.toBool(act.getSwfVersion()));
}

AVM1_GETTER_BODY(AVM1System, UseCodepage)
{
	return act.getSys()->getUseCodepage();
}

AVM1_SETTER_BODY(AVM1System, UseCodepage)
{
	return act.getSys()->setUseCodepage(value.toBool
	(
		act.getSwfVersion()
	));
}

AVM1_FUNCTION_BODY(AVM1System, setClipboard)
{
	tiny_string text;
	AVM1_ARG_UNPACK(text);

	act.getSys()->getEngineData()->setClipboardText(text);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1System, showSettings)
{
	SettingsPanel tabID;
	// TODO: Default to the last displayed panel.
	AVM1_ARG_UNPACK(tabID, SettingsPanel::Privacy);

	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.showSettings()` is a stub.");
	return AVM1Value::undefinedVal;
}
