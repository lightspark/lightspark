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

#include <initializer_list>
#include <sstream>

#include "gc/context.h"
#include "request.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/LoadVars.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::load_vars` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1LoadVars;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(load, protoFlags),
	AVM1_FUNCTION_PROTO(send, protoFlags),
	AVM1_FUNCTION_PROTO(sendAndLoad, protoFlags),
	AVM1_FUNCTION_PROTO(decode, protoFlags),
	AVM1_FUNCTION_PROTO(getBytesLoaded, protoFlags),
	AVM1_FUNCTION_PROTO(getBytesTotal, protoFlags),
	AVM1_FUNCTION_PROTO(toString, protoFlags),
	AVM1Decl("contentType", "application/x-www-form-urlencoded", protoFlags),
	AVM1_FUNCTION_PROTO(onLoad, protoFlags),
	AVM1_FUNCTION_PROTO(onData, protoFlags),
	AVM1_FUNCTION_PROTO(addRequestHeader, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1LoadVars::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, load)
{
	tiny_string url;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(url), false);

	loadImpl(act, _this, url);
	return true;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, send)
{
	// NOTE: `send()` navigates the browser to a URL, with the supplied
	// query parameter.
	tiny_string url;
	tiny_string window;
	RequestMethod method;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(url)(window, "")
	(
		method,
		RequestMethod::Post
	), false);

	sys->navigateToURL(url, window, std::make_pair
	(
		method,
		act.objectToFormVals(_this)
	));
	return true;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, sendAndLoad)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string url;
	GcPtr<AVM1Object> target;
	Optional<RequestMethod> method;

	unpacker(url);
	AVM1_ARG_CHECK_RET(unpacker(target)(method, {}), false);

	loadImpl(act, target, url, _this, method);
	return true;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, decode)
{
	// NOTE: Despite the AS1/2 docs stating that `decode()` was added in
	// Flash Player 7, it's not version gated.
	if (args.empty())
		return AVM1Value::undefinedVal;

	// Decode the query string into properties, on this `Object`.
	auto queryVars = args[0].toString(act).split('&');
	for (const auto& var : queryVars)
	{
		if (var.empty())
			continue;

		auto pos = var.find('=');
		auto key = var.substr(0, pos);
		auto value = pos == tiny_string::npos ? "" : var.substr
		(
			pos + 1,
			tiny_string::npos
		);

		_this->setProp
		(
			act,
			URLInfo::decode(key, URLInfo::ENCODE_ESCAPE),
			URLInfo::decode(value, URLInfo::ENCODE_ESCAPE)
		);
	}

	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, getBytesLoaded)
{
	// NOTE: This forwards to an undocumented property on the object.
	return _this->getProp(act, "_bytesLoaded");
}

AVM1_FUNCTION_BODY(AVM1LoadVars, getBytesTotal)
{
	// NOTE: This forwards to an undocumented property on the object.
	return _this->getProp(act, "_bytesTotal");
}

AVM1_FUNCTION_BODY(AVM1LoadVars, toString)
{
	auto formVals = act.objectToFormVals(_this);
	std::stringstream s;

	bool first = true;
	for (const auto& pair : formVals)
	{
		if (!first)
			s << '&';
		s << URLInfo::encode
		(
			pair.first,
			URLInfo::ENCODE_ESCAPE
		) << '=' << URLInfo::encode
		(
			pair.second,
			URLInfo::ENCODE_ESCAPE
		);
		first = false;
	}
	return s.str();
}

AVM1_FUNCTION_BODY(AVM1LoadVars, onLoad)
{
	// NOTE: For some reason, the default implementation of `onLoad()`
	// is a no-op.
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1LoadVars, onData)
{
	// NOTE: The default implementation of `onData()` forwards to
	// `decode()`, and `onLoad()`.
	AVM1_ARG_UNPACK_NAMED(unpacker);
	bool success = unpacker[0].filter([&](const auto& val)
	{
		return !val.isNullOrUndefined();
	}).transformOr(false, [&](const auto& val)
	{
		_this->callMethod
		(
			act,
			"decode",
			{ val },
			AVM1ExecutionReason::FunctionCall
		);

		_this->setProp(act, "loaded", true);
		return true;
	});

	_this->callMethod
	(
		act,
		"onLoad",
		{ success },
		AVM1ExecutionReason::FunctionCall
	);
	return AVM1Value::undefinedVal;
}

static NullableGcPtr<AVM1Object> getCustomHeaders
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& loaderObj,
	bool& isValid
)
{
	isValid = true;
	if (!loaderObj->hasProp(act, "_customHeaders"))
		return NullGc;

	auto val = loaderObj->getProp(act, "_customHeaders");
	isValid = !val.isNullOrUndefined();
	return isValid ? val.toObject(act) : NullGc;
}

static NullableGcPtr<AVM1Object> getCustomHeaders
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& loaderObj
)
{
	bool dummy;
	return getCustomHeaders(act, loaderObj, dummy);
}

// Based on Gnash's `loadableobject_addRequestHeader()`.
AVM1_FUNCTION_BODY(AVM1LoadVars, addRequestHeader)
{
	bool isValid;
	auto customHeaders = getCustomHeaders(act, _this, isValid);
	if (!isValid)
	{
		LOG
		(
			LOG_ERROR,
			"`LoadVars.addRequestHeaders()`: `_customHeaders` isn't an "
			"`Object`."
		);
		return AVM1Value::undefinedVal;
	}
	else if (customHeaders.isNull())
	{
		customHeaders = NEW_GC_PTR(act.getGcCtx(), AVM1Array(act));

		// NOTE: `_customHeaders` is always defined on the first call to
		// `addRequestHeaders()`.
		_this->setProp(act, "_customHeaders", customHeaders);
	}

	if (args.empty())
	{
		// Return after initializing `_customHeaders`.
		LOG
		(
			LOG_ERROR,
			"`LoadVars.addRequestHeaders()` requires at least an argument."
		);
		return AVM1Value::undefinedVal;
	}

	auto pushPair = [&](const AVM1Value& key, const AVM1Value& value)
	{
		// NOTE: `key`, and `value` **MUST** be `String`s, otherwise, we
		// bail.
		if (!key.is<tiny_string>() || !value.is<tiny_string>())
			return;

		(void)customHeaders->callMethod
		(
			act,
			"push",
			{ key, value },
			AVM1ExecutionReason::FunctionCall
		);
	};

	if (args.size() > 1)
	{
		// `header{,Value}`: Push both arguments to `_customHeaders`.
		pushPair(args[0], args[1]);
		return AVM1Value::undefinedVal;
	}

	// Just `header`.
	//
	// NOTE: `header` **MUST** be an `Array` (of some kind).
	// Keys, and values are pushed in valid pairs to `_customHeaders`.
	if (args[0].isNullOrUndefined())
	{
		LOG
		(
			LOG_ERROR,
			"`LoadVars.addRequestHeaders()`: `header` isn't an `Array`."
		);
		return AVM1Value::undefinedVal;
	}

	auto header = args[0].toObject(act);
	// Ensure `header`'s length is even, by masking off the bottom bit.
	size_t size = header->getLength(act) & ~1;

	for (size_t i = 0; i < size; i += 2)
	{
		auto key = header->getElement(act, i);
		auto value = header->getElement(act, i + 1);
		pushPair(key, value);
	}
	return AVM1Value::undefinedVal;
}

void AVM1LoadVars::loadImpl
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& loaderObj,
	const tiny_string& url,
	const NullableGcPtr<AVM1Object>& sendObj,
	const Optional<RequestMethod>& method
)
{
	auto defineHiddenProp = [&]
	(
		const tiny_string& name,
		const AVM1Value& value
	)
	{
		if (loaderObj->hasProp(act, name))
			loaderObj->setProp(act, name, value);
		else
			loaderObj->defineValue(name, value, protoFlags);
	};

	auto request = sendObj.asOpt().andThen([&](const auto& obj)
	{
		// Send `sendObj`'s properties.
		auto contentType = _this->getProp
		(
			act,
			"contentType"
		).toString(act);
		auto request = act.objectToRequest
		(
			obj,
			url,
			method.valueOr(RequestMethod::Post),
			// NOTE: For some reason, Flash Player ignores `contentType`,
			// if no `method` string was supplied, but will always do a
			// string conversion on `contentType`.
			makeOptional(contentType).filter(method.hasValue())
		);

		if (method.hasValue() && *method == RequestMethod::Get)
			return makeOptional(request);

		// Check if we have any custom headers.
		auto customHeaders = getCustomHeaders(act, _this);
		if (customHeaders.isNull())
			return makeOptional(request);

		// Add custom headers, that were added by `addRequestHeader()`.
		//
		// NOTE: `_customHeaders`' length **SHOULD** always be even, but
		// mask off the bottom bit anyways, just in case.
		size_t size = customHeaders->getLength(act) & ~1;
		for (size_t i = 0; i < size; i += 2)
		{
			auto key = customHeaders->getElement(act, i);
			auto value = customHeaders->getElement(act, i + 1);
			// NOTE: Both elements **MUST** be `String`s, otherwise, we
			// move on to the next pair.
			if (!key.is<tiny_string>() || !value.is<tiny_string>())
				continue;
			request.addHeader(key, value);
		}

		return makeOptional(request);
	// Not sending anything.
	}).valueOr(Request(url));

	act.getSys()->loaderManager->loadFormIntoAVM1Object
	(
		targetObj,
		request
	);

	// Create the hidden properties.
	defineHiddenProp("_bytesLoaded", number_t(0));
	defineHiddenProp("_bytesTotal", AVM1Value::undefinedVal);
	defineHiddenProp("loaded", false);
}
