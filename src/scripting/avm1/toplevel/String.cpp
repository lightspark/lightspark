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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/toplevel/Number.h"
#include "scripting/avm1/toplevel/String.h"
#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::string` crate.

AVM1String::AVM1String
(
	const GcContext& ctx,
	GcPtr<AVM1Object> proto,
	const tiny_string& _str
) : AVM1Object(ctx, proto), str(_str) {}

using AVM1String;

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1String, valueOf, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1String, toString, protoFlags)
	AVM1_FUNCTION_PROTO(toUpperCase, protoFlags),
	AVM1_FUNCTION_PROTO(toLowerCase, protoFlags),
	AVM1_FUNCTION_PROTO(charAt, protoFlags),
	AVM1_FUNCTION_PROTO(charCodeAt, protoFlags),
	AVM1_FUNCTION_PROTO(concat, protoFlags),
	AVM1_FUNCTION_PROTO(indexOf, protoFlags),
	AVM1_FUNCTION_PROTO(lastIndexOf, protoFlags),
	AVM1_FUNCTION_PROTO(slice, protoFlags),
	AVM1_FUNCTION_PROTO(substring, protoFlags),
	AVM1_FUNCTION_PROTO(split, protoFlags),
	AVM1_FUNCTION_PROTO(substr, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_BODY(fromCharCode, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1String::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, number, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1String, ctor)
{
	tiny_string str;
	AVM1_ARG_UNPACK(str, "");

	INPLACE_NEW_GC_PTR(act.getGcCtx(), _this, AVM1String
	(
		act.getGcCtx(),
		_this->getProto(act).toObject(act),
		str
	));

	constexpr auto propFlags =
	(
		AVM1PropFlags::DontEnum |
		AVM1PropFlags::DontDelete
	);

	// NOTE: The `length` property lives on the object itself, rather
	// than it's prototype.
	_this->defineValue("length", str.numChars(), propFlags);

	return _this;
}

AVM1_FUNCTION_BODY(AVM1String, string)
{
	tiny_string str;
	AVM1_ARG_UNPACK(str, "");
	return str;
}

AVM1_FUNCTION_TYPE_BODY(AVM1String, AVM1String, valueOf)
{
	// NOTE: The object must be a `String`. This is because
	// `String.prototype.valueOf.call()` returns `undefined` for non
	// `String` objects.
	return _this->str;
}

AVM1_FUNCTION_TYPE_BODY(AVM1String, AVM1String, toString)
{
	// NOTE: The object must be a `String`. This is because
	// `String.prototype.toString.call()` returns `undefined` for non
	// `String` objects.
	return _this->str;
}

AVM1_FUNCTION_BODY(AVM1String, toUpperCase)
{
	return AVM1Value(_this).toString(act).uppercase();
}

AVM1_FUNCTION_BODY(AVM1String, toLowerCase)
{
	return AVM1Value(_this).toString(act).lowercase();
}

AVM1_FUNCTION_BODY(AVM1String, charAt)
{
	auto str = AVM1Value(_this).toString(act);

	ssize_t i;
	AVM1_ARG_UNPACK(i);

	if (i < 0 || i >= str.numChars())
		return "";
	return tiny_string::fromChar(str[i]);
}

AVM1_FUNCTION_BODY(AVM1String, charCodeAt)
{
	auto str = AVM1Value(_this).toString(act);
	bool isSWF5 = act.getSwfVersion() == 5;

	ssize_t i;
	AVM1_ARG_UNPACK(i);

	if (i >= 0 && i < str.numChars())
		return str[i];
	return isSWF5 && i >= 0 ? 0 : AVM1Number::NaN;
}

AVM1_FUNCTION_BODY(AVM1String, concat)
{
	auto ret = AVM1Value(_this).toString(act);
	for (const auto& arg : args)
		ret += arg.toString(act);
	return ret;
}

AVM1_FUNCTION_BODY(AVM1String, indexOf)
{
	auto str = AVM1Value(_this).toString(act);

	tiny_string pattern;
	size_t startIdx;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(pattern)(startIdx, 0));

	auto pos = str.find(pattern, iclamp(startIdx, 0, str.numChars()));
	return pos == tiny_string::npos ? -1 : pos;
}

AVM1_FUNCTION_BODY(AVM1String, lastIndexOf)
{
	auto str = AVM1Value(_this).toString(act);

	tiny_string pattern;
	ssize_t startIdx;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(pattern).unpackIfDefined
	(
		startIdx,
		str.numChars()
	));

	// Bail out, if we got a negative starting index.
	if (startIdx < 0)
		return -1;

	auto pos = str.rfind(pattern, imin(startIdx, str.numChars()));
	return pos == tiny_string::npos ? -1 : pos;
}

// Normalizes an index argument used in `String` functions such as
// `substring()`.
// The returned index will be within the range `0 ... size`.
size_t strIndex(ssize_t i, size_t size)
{
	return i < 0 ? 0 : std::min<size_t>(i, size);
}

// Normalizes a wrapping index argument used in `String` functions such
// as `slice()`.
// Negative values will subtract from `size`.
// The returned index will be within the range `0 ... size`.
size_t wrappingIndex(ssize_t i, size_t size)
{
	return i < 0 ? size + i : std::min<size_t>(i, size);
}

AVM1_FUNCTION_BODY(AVM1String, slice)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	// Bail out, if there aren't any arguments.
	if (args.empty())
		return AVM1Value::undefinedVal;

	auto str = AVM1Value(_this).toString(act);

	size_t startIdx = wrappingIndex
	(
		unpacker.unpack<int32_t>(),
		str.numChars()
	);

	size_t endIdx = unpacker.tryUnpackIfDefined
	<
		int32_t
	>().transformOr(size_t(str.numChars()), [&](auto idx)
	{
		return wrappingIndex(idx, str.numChars());
	});

	return startIdx < endIdx ? str.substr(startIdx, endIdx) : "";
}

AVM1_FUNCTION_BODY(AVM1String, substring)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	if (args.empty())
		return AVM1Value::undefinedVal;

	auto str = AVM1Value(_this).toString(act);

	size_t startIdx = strIndex
	(
		unpacker.unpack<int32_t>(),
		str.numChars()
	);

	size_t endIdx = unpacker.tryUnpackIfDefined
	<
		int32_t
	>().transformOr(size_t(str.numChars()), [&](auto idx)
	{
		return strIndex(idx, str.numChars());
	});

	// NOTE: `String.substring()` automatically swaps the start, and
	// end, if they're flipped.
	if (endIdx < startIdx)
		std::swap(endIdx, startIdx);
	return str.substr(startIdx, endIdx);
}

AVM1_FUNCTION_BODY(AVM1String, split)
{
	auto str = AVM1Value(_this).toString(act);
	bool isSWF5 = act.getSwfVersion() == 5;

	Optional<ssize_t> _limit;
	Optional<tiny_string> delim;
	AVM1_ARG_UNPACK.ifDefined().unpackAt(1, _limit)(delim);

	size_t limit = _limit.transformOr(size_t(-1), [](auto limit)
	{
		return imax(limit, 0);
	});

	// NOTE: Despite what the AS1/2 docs state, in SWF 5, the default
	// delimiter is a comma (`,`), while an empty string behaves the same
	// as `undefined` in later versions.
	if (isSWF5 && !delim.hasValue())
		delim = ",";

	if (!delim.hasValue() || (isSWF5 && delim->empty()))
		return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, { str }));
	if (str.empty())
		return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, { "" }));

	std::vector<tiny_string> array;

	if (delim->empty())
	{
		// Got an empty delimiter, split every character.
		for (auto ch : str)
			array.push_back(tiny_string::fromChar(ch));
		return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, array));
	}

	for (size_t i = 0; i < str.numChars(); i += delim->numChars())
	{
		size_t pos = str.find(*delim, i);
		pos = pos == tiny_string::npos ? size : pos;
		array.push_back(str.substr(i, pos - i));
		i = pos;
	}

	auto span = makeSpan(array).getFirst(std::min
	(
		limit,
		array.size()
	));
	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, span));
}

AVM1_FUNCTION_BODY(AVM1String, substr)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	if (args.empty())
		return AVM1Value::undefinedVal;

	auto str = AVM1Value(_this).toString(act);

	size_t startIdx = wrappingIndex
	(
		unpacker.unpack<int32_t>(),
		str.numChars()
	);

	ssize_t size = unpacker.unpackIfDefined<int32_t>(str.numChars());
	auto endIdx = wrappingIndex(startIdx + size, str.numChars());

	return startIdx < endIdx ? str.substr(startIdx, endIdx) : "";
}

AVM1_FUNCTION_BODY(AVM1String, fromCharCode)
{
	tiny_string ret;
	for (const auto& arg : args)
	{
		auto ch = arg.toUInt16(act);
		if (!ch)
		{
			// Break on null terminators.
			break;
		}
		ret += ch;
	}

	return ret;
}
