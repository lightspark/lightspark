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

#include "gc/context.h"
#include "parsing/amf0_generator.h"
#include "parsing/lso.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/SharedObject.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::shared_object` crate.

AVM1SharedObject::AVM1SharedObject(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().sharedObject->proto
) {}

void AVM1SharedObject::serializeValue
(
	AVM1Activation& act,
	const tiny_string& name,
	const AVM1Value& value,
	Amf0Serializer& serializer
)
{
	auto objVisitor = makeVisitor
	(
		[&](const GcPtr<AVM1FunctionObject>&) {},
		[&](const GcPtr<DisplayObject>&)
		{
			serializer.writeUndefined(name);
		},
		[&](const GcPtr<AVM1Array>& array)
		{
			auto ref = serializer.tryGetRef(array.getPtr());
			if (ref.hasValue())
			{
				serializer.writeRef(name, *ref);
				return;
			}

			Amf0ArraySerializer writer
			(
				serializer,
				array.getPtr()
			);

			serialize(act, array, writer);
			writer.commit(name, array->getLength(act));
		},
		[&](const GcPtr<AVM1XMLNode>& node)
		{
			serializer.writeXML(name, node->toString(act));
		},
		[&](const GcPtr<AVM1Date>& date)
		{
			serializer.writeDate(name, date->getTime());
		},
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto ref = serializer.tryGetRef(obj.getPtr());
			if (ref.hasValue())
			{
				serializer.writeRef(name, *ref);
				return;
			}

			Amf0ObjectSerializer writer
			(
				serializer,
				obj.getPtr()
			);

			serialize(act, obj, writer);
			writer.commit(name);
		}
	);

	value.visit(makeVisitor
	(
		[&](number_t num) { serializer.writeNumber(name, num); },
		[&](bool _bool) { serializer.writeBool(name, _bool); },
		#ifdef USE_STRING_ID
		[&](uint32_t _str)
		{
			const auto& str = act.getSys()->getStringFromUniqueId(_str);
		#else
		[&](const tiny_string& str)
		{
		#endif
			serializer.writeStr(name, str);
		},
		[&](const GcPtr<AVM1MovieClipRef>&)
		{
			serializer.writeUndefined(name);
		},
		[&](const UndefinedVal&) { serializer.writeUndefined(name); },
		[&](const NullVal&) { serializer.writeNull(name); },
		[&](const GcPtr<AVM1Object>& obj) { obj->visit(objVisitor); }
	));
}

void AVM1SharedObject::serialize
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj,
	Amf0Serializer& serializer
)
{
	auto keys = obj->getKeys(act, false);

	// NOTE: Flash Player writes the serialized `Object` properties in
	// reverse order.
	for (auto it = keys.rbegin(); it != keys.rend(); ++it)
	{
		auto prop = obj->tryGetProp(act, *it);
		if (!prop.hasValue())
			continue;
		serializeValue(act, *it, *prop, serializer);
	}
}

LocalSharedObject AVM1SharedObject::makeLSO
(
	AVM1Activation& act,
	const tiny_string& name,
	const GcPtr<AVM1Object>& obj
)
{
	Amf0Serializer serializer;
	serialize(act, obj, serializer);

	auto _name = Path(name).getFilename().getGenericStr();
	return serializer.makeLSO(!_name.empty() ? _name : "<unknown>");
}

bool AVM1SharedObject::flushData(AVM1Activation& act) const
{
	try
	{
		std::vector<uint8_t> data;

		LSOWriter(makeLSO
		(
			act,
			name,
			getProp(act, "data").toObject(act)
		), data, false);

		auto engineData = act.getSys()->getEngineData();

		// NOTE: Flash Player doesn't write empty LSOs to disk.
		return data.empty() || engineData->flushSharedObject
		(
			name,
			data
		);
	}
	catch (AmfException& e)
	{
		LOG
		(
			LOG_ERROR,
			"AVM1SharedObject::flushData(): "
			"Exception occured while trying to serialize the "
			"`SharedObject`. Reason: " << e.what()
		);
	}
	catch (LSOException& e)
	{
		LOG
		(
			LOG_ERROR,
			"AVM1SharedObject::flushData(): LSO exception occured. "
			"Reason: " << e.what()
		);
	}
	catch (std::exception& e)
	{
		LOG
		(
			LOG_ERROR,
			"AVM1SharedObject::flushData(): `std::exception` occured. "
			"Reason: " << e.what()
		);
	}
	return false;
}

template<bool dontDelete = true>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

template<>
constexpr auto protoFlags<false> = AVM1PropFlags::DontEnum;

using AVM1SharedObject;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(clear, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, close, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, connect, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, flush, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, getSize, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, send, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, setFps, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, onStatus, protoFlags<>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, onSync, protoFlags<>)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, deleteAll, protoFlags<false>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1SharedObject, getDiskUsage, protoFlags<false>),
	AVM1_FUNCTION_PROTO(getLocal),
	AVM1_FUNCTION_PROTO(getRemote)
};

GcPtr<AVM1SystemClass> AVM1SharedObject::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->proto, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1SharedObject, ctor)
{
	new (_this.getPtr()) AVM1SharedObject(act);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1SharedObject, clear)
{
	auto sys = act.getSys();
	auto data = _this->getProp(act, "data").toObject(act);

	for (const auto& key : data->getKeys(act, false))
		data->deleteProp(act, key);

	auto sharedObj = _this->as<AVM1SharedObject>();
	if (!sharedObj.isNull())
		sys->getEngineData()->removeSharedObject(sharedObj->name);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, close)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.close()` is partially implemented."
	);

	_this->flushData(act);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, connect)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.connect()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, flush)
{
	int32_t minDiskSpace;
	if (AVM1_ARG_UNPACK(minDiskSpace, 0).isValid())
	{
		LOG
		(
			LOG_NOT_IMPLEMENTED,
			"AVM1: `SharedObject.flush()`: `minDiskSpace` is currently "
			"ignored."
		);
	}

	return _this->flushData(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, getSize)
{
	try
	{
		std::vector<uint8_t> data;

		LSOWriter(makeLSO
		(
			act,
			name,
			getProp(act, "data").toObject(act)
		), data, false);
		return number_t(data.size());
	}
	catch (AmfException& e)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getSize(): Exception occured while trying to "
			" serialize the `SharedObject`. Reason: " << e.what()
		);
	}
	catch (LSOException& e)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getSize(): LSO exception occured. "
			"Reason: " << e.what()
		);
	}
	catch (std::exception& e)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getSize(): `std::exception` occured. "
			"Reason: " << e.what()
		);
	}
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, send)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.send()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, setFps)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.setFps()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, onStatus)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.onStatus()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, onSync)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.onSync()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, deleteAll)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.deleteAll()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1SharedObject, AVM1SharedObject, getDiskUsage)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.getDiskUsage()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1SharedObject, getLocal)
{
}

AVM1_FUNCTION_BODY(AVM1SharedObject, getRemote)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `SharedObject.getRemote()` is a stub."
	);
	return AVM1Value::undefinedVal;
}
