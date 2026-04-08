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
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/XML.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::xml` crate.

AVM1XML::AVM1XML(AVM1Activation& act) : AVM1XMLNode
(
	act,
	act.getPrototypes().xml->proto
), status(XMLStatus::NoError)
{
}

void AVM1XML::parse
(
	AVM1Activation& act,
	const tiny_string& str,
	bool ignoreWhite
)
{
}

void AVM1XML::load
(
	AVM1Activation& act,
	GcPtr<AVM1Object> loaderObj,
	const tiny_string& url,
	NullableGcPtr<AVM1XMLNode> sendObj
)
{
}

constexpr auto protoFlags = AVM1PropFlags::ReadOnly;

using AVM1XML;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, createElement),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, createTextNode),
	AVM1_FUNCTION_PROTO(load),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, sendAndLoad),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, onData),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, getBytesLoaded),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XML, getBytesTotal),
	AVM1Decl("contentType", "application/x-www-form-urlencoded"),
	AVM1_GETTER_TYPE_PROTO(AVM1XML, docTypeDecl, DocTypeDecl, protoFlags),
	AVM1Decl("ignoreWhite", false),
	AVM1_GETTER_TYPE_PROTO(AVM1XML, status, Status),
	AVM1_GETTER_TYPE_PROTO(AVM1XML, xmlDecl, XMLDecl)
};

GcPtr<AVM1SystemClass> AVM1XML::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1XML, ctor)
{
	auto xml = NEW_GC_PTR(act.getGcCtx(), AVM1XML(act));
	_this.constCast() = xml;

	tiny_string text;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(text), _this);

	try
	{
		xml->parse(act, text, _this->getProp
		(
			act,
			"ignoreWhite"
		).toBool(act.getSwfVersion()));
	}
	catch (std::exception& e)
	{
		LOG(LOG_ERROR, "Error occured while parsing XML: " << e.what());
	}

	return _this;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, createElement)
{
	tiny_string name;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(name));

	return NEW_GC_PTR(act.getGcCtx(), AVM1XMLNode
	(
		act,
		XMLNodeType::Element,
		name
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, createTextNode)
{
	tiny_string text;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(name));

	return NEW_GC_PTR(act.getGcCtx(), AVM1XMLNode
	(
		act,
		XMLNodeType::Text,
		text
	));
}

AVM1_FUNCTION_BODY(AVM1XML, load)
{
	AVM1Value urlVal;
	AVM1_ARG_UNPACK(urlVal);
	
	if (urlVal.isNull() || !_this->is<AVM1XML>())
		return false;

	_this->as<AVM1XML>()->load
	(
		act,
		_this,
		urlVal.toString(act)
	);
	return true;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, sendAndLoad)
{
	AVM1Value urlVal;
	NullableGcPtr<AVM1Object> target;
	AVM1_ARG_UNPACK_MULTIPLE(urlVal, target);
	
	if (urlVal.isNull() || target.isNull())
		return AVM1Value::undefinedVal;

	_this->as<AVM1XML>()->load
	(
		act,
		target,
		urlVal.toString(act)
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, onData)
{
	AVM1Value src;
	AVM1_ARG_UNPACK(src);

	return src.visit(makeVisitor
	(
		[&](const UndefinedVal&)
		{
			_this->callMethod
			(
				act,
				"onLoad",
				{ false },
				AVM1ExecutionReason::Special
			);
			return AVM1Value::undefinedVal;
		},
		[&](const auto&)
		{
			_this->callMethod
			(
				act,
				"parseXML",
				{ src.toString(act) },
				AVM1ExecutionReason::Special
			);

			_this->setProp(act, "loaded", true);

			_this->callMethod
			(
				act,
				"onLoad",
				{ true },
				AVM1ExecutionReason::Special
			);
			return AVM1Value::undefinedVal;
		}
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, getBytesLoaded)
{
	// NOTE: This forwards to an undocumented property on the `Object`.
	return _this->getProp(act, "_bytesLoaded");
}

AVM1_FUNCTION_TYPE_BODY(AVM1XML, AVM1XML, getBytesTotal)
{
	// NOTE: This forwards to an undocumented property on the `Object`.
	return _this->getProp(act, "_bytesTotal");
}

AVM1_GETTER_TYPE_BODY(AVM1XML, AVM1XML, DocTypeDecl)
{
	if (!_this->getDocType().hasValue())
		return AVM1Value::undefinedVal;
	return *_this->getDocType();
}

AVM1_GETTER_TYPE_BODY(AVM1XML, AVM1XML, Status)
{
	return _this->getStatus();
}

AVM1_GETTER_TYPE_BODY(AVM1XML, AVM1XML, XMLDecl)
{
	if (!_this->getXMLDecl().hasValue())
		return AVM1Value::undefinedVal;
	return *_this->getXMLDecl();
}
