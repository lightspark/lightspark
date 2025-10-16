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

#include <sstream>

#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/enum.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object::array_object::ArrayObject`.

AVM1Array::AVM1Array
(
	AVM1Activation& activation,
	const std::vector<AVM1Value>& elems
) : AVM1Array
(
	activation.getGcCtx(),
	activation.getCtx().getPrototypes().array->proto,
	elems
) {}

AVM1Array::AVM1Array
(
	const GcContext& ctx,
	const GcPtr<AVM1Object>& arrayProto,
	const std::vector<AVM1Value>& elems
) : AVM1Object(ctx, arrayProto)
{
	size_t length = 0;

	for (const auto& elem : elems)
	{
		defineValue
		(
			(std::stringstream() << length++).str(),
			elem,
			AVM1PropFlags(0)
		);
	}

	defineValue
	(
		"length",
		number_t(length),
		AVM1PropFlags::DontEnum | AVM1PropFlags::DontDelete
	);
}

static Optional<ssize_t> parseIndex(const tiny_string& str)
{
	return str.tryToNumber<int32_t>();
}

void AVM1Array::setLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value,
	const GcPtr<AVM1Object>& _this
)
{
	if (name == "length")
	{
		setLength(activation, value.toInt32(activation));
		AVM1Object::setLocalProp(activation, name, value, _this);
		return;
	}

	(void)parseIndex(name).andThen([&](ssize_t idx)
	{
		auto length = getLength(activation);
		if (idx >= length)
			setLength(activation, ++idx);
		return makeOptional(idx);
	});
	AVM1Object::setLocalProp(activation, name, value, _this);
}

void AVM1Array::setLength(AVM1Activation& activation, ssize_t length)
{
	auto oldLen = getData(activation, "length");
	if (!oldLen.is<number_t>())
	{
		AVM1Object::setLength(activation, length);
		return;
	}

	ssize_t _oldLen = oldLen.toInt32(activation);
	for (auto i = std::max<ssize_t>(length, 0); i < _oldLen; ++i)
		deleteElement(activation, i);

	AVM1Object::setLength(activation, length);
}

void AVM1Array::setElement
(
	AVM1Activation& activation,
	ssize_t idx,
	const AVM1Value& value
)
{
	auto length = getLength(activation);
	if (idx >= length)
		setLength(activation, ++idx);

	AVM1Object::setElement(activation, idx, value);
}


constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

static constexpr auto protoDecls =
{
	AVM1Decl("push", AVM1Array::push, protoFlags),
	AVM1Decl("unshift", AVM1Array::unshift, protoFlags),
	AVM1Decl("shift", AVM1Array::shift, protoFlags),
	AVM1Decl("pop", AVM1Array::pop, protoFlags),
	AVM1Decl("reverse", AVM1Array::reverse, protoFlags),
	AVM1Decl("join", AVM1Array::join, protoFlags),
	AVM1Decl("slice", AVM1Array::slice, protoFlags),
	AVM1Decl("splice", AVM1Array::splice, protoFlags),
	AVM1Decl("concat", AVM1Array::concat, protoFlags),
	AVM1Decl("toString", AVM1Array::toString, protoFlags),
	AVM1Decl("sort", AVM1Array::sort, protoFlags),
	AVM1Decl("sortOn", AVM1Array::sortOn, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1Decl("CASEINSENSITIVE", number_t(AVM1Array::SortOptions::CaseInensitive)),
	AVM1Decl("DECENDING", number_t(AVM1Array::SortOptions::Decending)),
	AVM1Decl("UNIQUESORT", number_t(AVM1Array::SortOptions::UniqueSort)),
	AVM1Decl("RETURNINDEXEDSORT", number_t(AVM1Array::SortOptions::ReturnIndexedSort)),
	AVM1Decl("NUMERIC", number_t(AVM1Array::SortOptions::Numeric)),
};

GcPtr<AVM1SystemClass> AVM1Array::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto& gcCtx = ctx.getGcCtx();
	auto proto = NEW_GC_PTR(gcCtx, AVM1Array(gcCtx, superProto));
	auto _class = ctx.makeNativeClassWithProto(ctor, array, proto);
	ctx.definePropsOn(proto, protoDecls);

	// NOTE, PLAYER-SPECIFIC: These were added in Flash Player 7, but
	// are always available, even on SWF 6, and earlier, when running
	// in Flash Player 7.
	//
	// TODO: Make these conditional, once support for mimicking different
	// player versions is added.
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Array, ctor)
{
	if (args.size() != 1)
		new (_this.getPtr()) AVM1Array(act, args);
	else if (args[0].is<number_t>())
	{
		new (_this.getPtr()) AVM1Array(act);
		_this->setLength(act, args[0].toNumber(act));
	}
	return _this;
}

AVM1_FUNCTION_BODY(AVM1Array, array)
{
	if (args.size() != 1)
		return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, args));
	else if (args[0].is<number_t>())
	{
		auto array = NEW_GC_PTR(act.getGcCtx(), AVM1Array(act));
		array->setLength(act, args[0].toNumber(act));
		return array;
	}
}

AVM1_FUNCTION_BODY(AVM1Array, push)
{
	auto oldLength = _this->getLength(act);

	for (size_t i = 0; i < args.size(); ++i)
		_this->setElement(act, oldLength + i, args[i]);

	auto newLength = oldLength + args.size();

	_this->setLength(act, newLength);
	return number_t(newLength);
}

AVM1_FUNCTION_BODY(AVM1Array, unshift)
{
	auto oldLength = _this->getLength(act);
	auto newLength = oldLength + args.size();

	for (size_t i = 0; i < oldLength; ++i)
	{
		auto from = oldLength - i - 1;
		auto to = newLength - i - 1;

		if (_this->hasElement(act, from))
			_this->setElement(act, to, _this->getElement(act, from));
		else
			_this->deleteElement(act, to);
	}

	for (size_t i = 0; i < args.size(); ++i)
		_this->setElement(act, i, args[i]);

	if (_this->is<AVM1Array>())
		_this->setLength(act, newLength);
	return number_t(newLength);
}

AVM1_FUNCTION_BODY(AVM1Array, shift)
{
	auto len = _this->getLength(act);

	if (len == 0)
		return AVM1Value::undefinedVal;

	auto first = _this->getElement(act, 0);

	for (size_t i = 1; i < len; ++i)
	{
		if (_this->hasElement(act, from))
			_this->setElement(act, i - 1, _this->getElement(act, i));
		else
			_this->deleteElement(act, i - 1);
	}

	_this->deleteElement(act, len - 1);

	if (_this->is<AVM1Array>())
		_this->setLength(act, len - 1);
	return first;
}

AVM1_FUNCTION_BODY(AVM1Array, pop)
{
	auto len = _this->getLength(act);

	if (len == 0)
		return AVM1Value::undefinedVal;

	auto last = _this->getElement(act, len - 1);

	_this->deleteElement(act, len - 1);

	if (_this->is<AVM1Array>())
		_this->setLength(act, len - 1);
	return last;
}

AVM1_FUNCTION_BODY(AVM1Array, reverse)
{
	auto len = _this->getLength(act);

	for (size_t lowerIdx = 0; lowerIdx < len / 2; ++lowerIdx)
	{
		bool hasLower = _this->hasElement(act, lowerIdx);
		auto lowerValue =
		(
			hasLower ?
			_this->getElement(act, lowerIdx) :
			AVM1Value::undefinedVal
		);

		size_t upperIdx = len - lowerIdx - 1;
		bool hasUpper = _this->hasElement(act, upperIdx);
		auto upperValue =
		(
			hasUpper ?
			_this->getElement(act, upperIdx) :
			AVM1Value::undefinedVal
		);

		if (hasLower)
			_this->setElement(act, upperIdx, upperValue);
		else
			_this->deleteElement(act, upperIdx);

		if (hasUpper)
			_this->setElement(act, lowerIdx, lowerValue);
		else
			_this->deleteElement(act, lowerIdx);
	}

	// NOTE: The AS1/2 docs say that `Array.reverse()` returns a `Void`.
	// This isn't true, as `Array.reverse()` actually returns `this`.
	return _this;
}

AVM1_FUNCTION_BODY(AVM1Array, join)
{
	auto len = _this->getLength(act);

	tiny_string delim;
	AVM1_ARG_UNPACK(delim, ",");

	if (len <= 0)
		return "";

	tiny_string ret;

	for (size_t i = 0; i < len; ++i)
	{
		ret += _this->getElement(act, i);
		if (i < len - 1)
			ret += delim;
	}

	return ret;
}

// Handles an index argument that may be either positive (starting from
// the beginning), or negative (starting from the end).
// The returned index will be both positive, and clamped to the range
// `0 - len`.
static ssize_t makeIdxAbsolute(ssize_t idx, ssize_t len)
{
	return idx < 0 ? std::max<ssize_t>(idx + len, 0) : std::min(idx, len);
}

AVM1_FUNCTION_BODY(AVM1Array, slice)
{
	auto len = _this->getLength(act);

	AVM1_ARG_UNPACK_NAMED(unpacker);
	auto start = makeIdxAbsolute(unpacker[0].toInt32(act), len);
	auto end =
	(
		!unpacker[1].is<UndefinedVal>() ?
		makeIdxAbsolute(unpacker[1].toInt32(act), len) :
		len
	);

	std::vector<AVM1Value> elems;
	for (size_t i = start; i < end; ++i)
		elems.push_back(_this->getElement(act, i));

	_this->setLength(act, len - delCount + items.size());
	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, elems));
}

AVM1_FUNCTION_BODY(AVM1Array, splice)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	ssize_t start;
	AVM1_ARG_CHECK(unpacker(start));


	auto len = _this->getLength(act);
	start = makeIdxAbsolute(start, len);
	ssize_t delCount;
	if (unpacker(delCount, len - start).isValid() && delCount < 0)
		return AVM1Value::undefinedVal;
	else if (unpacker.isValid())
		delCount = std::min(delCount, len - start);

	std::vector<AVM1Value> elems;
	for (size_t i = 0; i < delCount; ++i)
		elems.push_back(_this->getElement(act, i));

	std::vector<AVM1Value> items = args.size() > 2 ?
	{
		std::next(args.begin(), 2),
		args.end()
	} : {};

	auto spliceImpl = [&](size_t i)
	{
		auto idx = i - delCount + items.size();
		if (_this->hasElement(act, i))
			_this->setElement(act, idx, _this->getElement(act, i));
		else
			_this->deleteElement(act, idx);
	};

	if (items.size() > delCount)
	{
		for (size_t i = len; i >= start + delCount; --i)
			spliceImpl(i);
	}
	else
	{
		for (size_t i = start + delCount; i < len; ++i)
			spliceImpl(i);
	}

	for (size_t i = 0; i < items.size(); ++i)
		_this->setElement(act, i, items[i]);

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, elems));
}

AVM1_FUNCTION_BODY(AVM1Array, concat)
{
	std::vector<AVM1Value> elems;

	auto addObjElems = [&](const GcPtr<AVM1Object>& obj)
	{
		auto len = obj->getLength(act);

		for (size_t i = 0; i < len; ++i)
			elems.push_back(obj->getElement(act, i));
	};

	addObjElems(_this);

	for (size_t i = 0; i < args.size(); ++i)
	{
		if (!_args[i].is<AVM1Array>())
			elems.push_back(_args[i]);
		else
			addObjElems(_args[i]);
	}

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, elems));
}

AVM1_FUNCTION_BODY(AVM1Array, toString)
{
	return join(act, _this, {});
}

// Compare between 2 values, with sort options.
static number_t sortCompare
(
	AVM1Activation& act,
	const AVM1Value& a,
	const AVM1Value& b,
	const AVM1Array::SortOptions& options
)
{
	using SortOpts = AVM1Array::SortOptions;
	bool isNumeric = options & SortOpts::Numeric;
	bool isCaseInsensitive = options & SortOpts::CaseInsensitive;
	int ret;
	if (isNumeric && a.is<number_t>() && b.is<number_t>())
	{
		auto numA = a.toNumber(act);
		auto numB = b.toNumber(act);
		ret = numA > numB ? 1 : numA == numB ? 0 : -1;
	}
	else
	{
		auto strA = a.toString(act);
		auto strB = b.toString(act);
		ret = isCaseInsensitive ? a.strcasecmp(b) : a.compare(b);
	}

	return options & SortOpts::Decending ? -ret : ret;
}

using CompareFunc = number_t(*)
(
	AVM1Activation& act,
	const AVM1Value& a,
	const AVM1Value& b,
	const AVM1Array::SortOptions& options
);

// Create a compare function, based on a user provided, custom ActionScript
// function.
static CompareFunc sortCompareCustom(const GcPtr<AVM1Object>& compareFunc)
{
	return []
	(
		AVM1Activation& act,
		const AVM1Value& a,
		const AVM1Value& b,
		const AVM1Array::SortOptions& options
	)
	{
		auto ret = compareFunc.call
		(
			act,
			"[Compare]",
			AVM1Value::undefinedVal,
			{ a, b }
		).toNumber(act);
		return numA > numB ? 1 : numA == numB ? 0 : -1;
	};
}

using FieldsList = std::vector<std::pair<tiny_string, SortOptions>>;

// Create a compare function, based on field names, and options.
static CompareFunc sortOnCompare(const FieldsList& fields)
{
	return []
	(
		AVM1Activation& act,
		const AVM1Value& a,
		const AVM1Value& b,
		const AVM1Array::SortOptions& options
	)
	{
		if (!a.is<AVM1Object>() && !b.is<AVM1Object>())
		{
			// Fallback to the standard comparison.
			return sortCompare(act, a, b options);
		}

		const auto& undefVal = AVM1Value::undefinedVal;

		auto objA = a.toObject(act);
		auto objB = b.toObject(act);
		for (const auto& field : fields)
		{
			auto propA = objA->getLocalProp
			(
				act,
				field.first,
				false
			).valueOr(undefVal);

			auto propB = objB->getLocalProp
			(
				act,
				field.first,
				false
			).valueOr(undefVal);

			auto ret = sortCompare(act, propA, propB, field.second);
			if (ret != 0)
				return ret;
		}

		// Got through all fields; so it must be equal.
		return 0;
	};
}

using SortArray = std::vector<std::pair<size_t, AVM1Value>>;

// Sort elements, using the quicksort algorithm, while mimicking Flash
// Player's behaviour.
static void qsort
(
	AVM1Activation& act,
	SortArray& elems,
	CompareFunc compareFunc,
	const AVM1Array::SortOptions& options
)
{
	using SortOpts = AVM1Array::SortOptions;

	// Bail early, if there are fewer than 2 elements.
	if (elems.size() < 2)
		return;

	// Stack for storing inclusive subarray bounderies (start, and end).
	std::vector<std::pair<size_t, size_t>> stack({ {0, elems.size() - 1} });

	while (!stack.empty())
	{
		size_t low;
		size_t high;
		std::tie(low, high) = stack.back();
		stack.pop_back();

		if (low >= high)
			continue;

		// Flash Player always chooses the leftmost element as the pivot.
		auto pivot = elems[low].second;
		auto left = low + 1;
		auto right = high;

		for (;;)
		{
			// Find an element thats greater than the pivot, from the left.
			for (; left < right; ++left)
			{
				const auto& item = elems[left].second;
				if (compareFunc(act, pivot, item, options) < 0)
					break;
			}

			// Find an element thats less than the pivot, from the right.
			for (; right > low; --right)
			{
				const auto& item = elems[right].second;
				if (compareFunc(act, pivot, item, options) > 0)
					break;
			}

			// When `left`, and `right` cross, then no element greater
			// than the pivot comes before an element less than the pivot.
			if (left >= right)
				break;

			// Otherwise, swap `left`, and `right`, and keep going.
			auto tmp = elems[left];
			elems[left] = elems[right];
			elems[right] = tmp;
		}

		// Move the pivot element in between the partitions.
		auto tmp = elems[low];
		elems[low] = elems[right];
		elems[right] = tmp;

		// Pusn the subarrays onto the stack, for further sorting.
		stack.emplace_back(right + 1, high);
		if (right > 0)
			stack.emplace_back(low, right - 1);
	}
}

// Common code for `Array.sort{,On}()`.
static AVM1Value sortImpl
(
	AVM1Activation& act,
	const GcPtr<AVM1Object> _this,
	CompareFunc compareFunc,
	const AVM1Array::SortOptions& options,
	bool isSortOn
)
{
	using SortOpts = AVM1Array::SortOptions;
	auto len = _this->getLength(act);

	SortArray elems;
	for (size_t i = 0; i < len; ++i)
		elems.emplace_back(i, _this->getElement(act, i));

	// Mask off `DECENDING` for `Array.sort()`, since that should be
	// handled by the call to `std::reverse()` below.
	bool isDecending = options & SortOpts::Decending;
	if (!isSortOn)
		options &= ~SortOpts::Decending;

	qsort(act, elems, compareFunc, options);

	if (!isSortOn && isDecending)
		std::reverse(elems.begin(), elems.end());

	if (options & SortOpts::UniqueSort)
	{
		// Check for uniqueness. Return `0` if there are duplicates.
		auto _compareFunc = isSortOn ? compareFunc : sortCompare;

		for (auto it = elems.begin(); it != elems.end(); ++it)
		{
			const auto& a = it++->second;
			if (it == elems.end())
				break;
			const auto& b = it->second;

			if (_compareFunc(act, a, b, options) == 0)
				return number_t(0);
		}
	}

	if (options & SortOpts::ReturnIndexedArray)
	{
		// `Array.RETURNINDEXEDARRAY` returns an `Array` containing the
		// sorted indices, and doesn't modify the original array.
		std::vector<AVM1Value> ret;
		for (const auto& pair : elems)
			ret.push_back(number_t(pair.first));

		return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, ret));
	}

	// The standard sort modifies the original array, and returns it.
	// The AS1/2 docs incorrectly state that this returns nothing, but
	// actualy returns the orignal array, fully sorted.
	for (size_t i = 0; i < elems.size(); ++i)
		_this->setElement(act, i, elems[i].second);
	return _this;
}

AVM1_FUNCTION_BODY(AVM1Array, sort)
{
	// Overloads:
	// 1. `Array.sort(options: Number = 0)`: Sort with the supplied
	// options.
	// 2. `Array.sort(compareFunction: Object, options: Number = 0)`:
	// Sort using the supplied compare function, and options.
	AVM1_ARG_UNPACK_NAMED(unpacker);

	NullableGcPtr<AVM1Object> compareFunc;
	SortOptions options(0);

	if (!unpacker[0].is<AVM1Object>() && !unpacker[0].is<number_t>())
		return AVM1Value::undefinedVal;
	if (unpacker.peek().is<AVM1Object>())
		unpacker(compareFunc);
	if (unpacker.peek().is<number_t>())
		unpacker(options);

	auto nativeCompareFunc = compareFunc.asOpt
	(
	).transformOr(sortCompare, [&](const auto& compareFunc)
	{
		return sortCompareCustom(compareFunc);
	});
	return sortImpl(act, _this, nativeCompareFunc, options, false);
}

AVM1_FUNCTION_BODY(AVM1Array, sortOn)
{
	// Overloads:
	// 1. `Array.sortOn(fieldName: String, options: Number = 0)`: Sort by
	// a find, and with the supplied options.
	// 2. `Array.sort(fieldName: Array, options: Array)`: Sort by fields,
	// in order of precedence, and with the supplied options respectively.
	AVM1_ARG_UNPACK_NAMED(unpacker);

	if (args.empty())
		return AVM1Value::undefinedVal;

	return [&] -> Optional<FieldsList>
	{
		if (!unpacker[0].is<AVM1Object>())
		{
			// Single field.
			tiny_string str;
			SortOptions opts;
			unpacker(str)(opts);

			return FieldsList { std::make_pair(str, opts) };
		}
		else if (!unpacker[0].is<AVM1Array>())
		{
			// Non `Array` fields.
			// Fallback to standard sorting.
			return sortImpl(act, _this, sortCompare, SortOptions(0), true);
		}

		// Array of field names.
		auto fieldNames = unpacker[0].as<AVM1Array>(act);
		auto len = fieldNames->getLength(act);

		// Bail early, if there aren't any fields.
		if (len <= 0)
			return nullOpt;

		FieldsList fields;
		for (size_t i = 0; i < len; ++i)
		{
			fields.emplace_back
			(
				fieldNames->getElement(act, i),
				SortOptions(0)
			);
		}

		if (args.size() == 1)
			return fields;

		if (unpacker[1].isPrimitive())
		{
			// Single options argument.
			SortOptions opts = unpacker[1].toInt32(act);

			for (auto& field : fields)
				field.second = opts;
			return fields;
		}
		else if (!unpacker[1].is<AVM1Array>())
		{
			// Non `Array` options.
			return fields;
		}

		// Array of options.
		auto optArray = unpacker[1].as<AVM1Array>();

		if (optArray->getLength(act) != len)
		{
			// Mismatching lengths.
			return fields;
		}

		for (size_t i = 0; i < fields.size(); ++i)
			fields[i].second = optArray->getElement(act, i).toInt32(act);
		return fields;
	}().transformOr(AVM1Value::undefinedVal, [&](const auto& fields)
	{
		return sortImpl
		(
			act,
			_this,
			sortOnCompare(fields),
			fields[0].second,
			true
		);
	});
}
