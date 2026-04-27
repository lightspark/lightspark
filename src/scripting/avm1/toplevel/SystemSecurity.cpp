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

#include "backends/urlutils.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/SystemSecurity.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::system_security` crate.


using AVM1SystemSecurity;

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(allowDomain),
	AVM1_FUNCTION_PROTO(allowInsecureDomain),
	AVM1_FUNCTION_PROTO(loadPolicyFile),
	AVM1_GETTER_PROTO(chooseLocalSwfPath, ChooseLocalSwfPath),
	AVM1_FUNCTION_PROTO(escapeDomain),
	AVM1_GETTER_PROTO(sandboxType, SandboxType),
	AVM1_FUNCTION_PROTO(PolicyFileResolver)
};

AVM1SystemSecurity::AVM1SystemSecurity(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1SystemSecurity, allowDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.security.allowDomain()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1SystemSecurity, allowInsecureDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.security.allowInsecureDomain()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1SystemSecurity, loadPolicyFile)
{
	auto baseClip = act.getBaseClip()->as<MovieClip>();
	assert(("`baseClip` should always be a `MovieClip`", !baseClip.isNull()));

	tiny_string url;
	AVM1_ARG_UNPACK(url);

	auto policyURL = URLInfo(baseClip->getURL()).goToURL(url);
	LOG(LOG_INFO, "Loading policy file: " << policyURL);
	act.getSys()->securityManager->addPolicyFile(policyURL);
}

AVM1_GETTER_BODY(AVM1SystemSecurity, ChooseLocalSwfPath)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.security.chooseLocalSwfPath` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1SystemSecurity, escapeDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.security.escapeDomain()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_BODY(AVM1SystemSecurity, SandboxType)
{
	auto str = act.getSys()->securityManager->getSandboxName();
	if (str == nullptr)
		return "";
	return tiny_string(str);
}

AVM1_FUNCTION_BODY(AVM1SystemSecurity, PolicyFileResolver)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `System.security.PolicyFileResolver()` is a stub.");
	return AVM1Value::undefinedVal;
}
