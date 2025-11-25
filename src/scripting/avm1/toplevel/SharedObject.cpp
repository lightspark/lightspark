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

#include "backends/urlutils.h"
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
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string name;
	AVM1Value localPathVal;
	// TODO: Flash Player seems to do some kind of escaping on `name`:
	// e.g. "foo\uD800" becomes "fooE#FB#FB#D.sol".
	unpacker(name)(localPathVal);

	if (name.findFirst("~%&\\;:\"',<>?# ") != tiny_string::npos)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): Invalid character in `name`."
		);
		return AVM1Value::nullVal;
	}

	URLInfo movieURL(act.getBaseClip()->getMovie()->getURL());
	if (!movieURL.isValid())
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): Failed to parse the movie URL."
		);
		return AVM1Value::nullVal;
	}

	movieURL.setQuery("");
	movieURL.setFragment("");

	bool secure;
	unpacker(secure, false);

	// The `secure` argument disallows using the `SharedObject` from non
	// HTTPS origins.
	if (secure && movieURL.getProtocol() != "https")
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): Tried to load a secure "
			"`SharedObject` from a non HTTPS origin."
		);
		return AVM1Value::nullVal;
	}

	// NOTE: `SharedObject`s are sandboxed per domain.
	// By default, they're keyed based on the SWF URL, but `localPath`
	// can modify this path.
	auto moviePath = movieURL.getPath();

	// Remove any leading/trailing `/`s.
	moviePath = moviePath.stripPrefix('/').stripSuffix('/');

	auto movieHost = movieURL.getProtocol() == "file" ? [&]
	{
		// Remove any root prefixes from the path.
		moviePath = Path(moviePath).getRelative().getGenericStr();
		return "localhost";
	}() : movieURL.getHostname();

	auto localPath = localPathVal.tryAs
	<
		tiny_string
	>().transformOr(moviePath, [&](const auto& _localPath)
	{
		// An empty `localPath` always fails.
		if (_localPath.empty())
			return "";

		// Remove any leading/trailing `/`s.
		auto localPath = _localPath.stripPrefix('/').stripSuffix('/');

		// Verify that `localPath` is a prefix of the SWF path.
		bool isValid = moviePath.startsWith(localPath) &&
		(
			localPath.empty() ||
			moviePath.numChars() == localPath.numChars() ||
			moviePath.stripPrefix(localPath).startsWith('/')
		);

		if (isValid)
			return localPath;

		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): `localPath` doesn't match the SWF "
			"path."
		);
		return "";
	});

	if (localPath.empty())
		return AVM1Value::nullVal;

	// Create the final path: `foo.com/dir/movie.swf/name`.
	// NOTE: In Flash Player, if `name` is a path containing `/`s, then
	// it's prefixed with a `#`.
	tiny_string prefix = path.contains('/') ? "#" : "";
	auto fullPath = Path(moviePath) / localPath / prefix + name;

	// Don't allow paths with `..` in them, to prevent an SWF from
	// traversing the file system.
	bool hasParentDir = std::any_of
	(
		fullPath.begin()
		fullPath.end(),
		[&](const auto& path) { return path == ".."; }
	);

	if (hasParentDir)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): Invalid path with `..` segments."
		);
		return AVM1Value::nullVal;
	}

	// NOTE: Flash Player usually fails to save `SharedObject`s that use
	// `.` files/directories, so, disallow them entirely.
	bool hasDotPath = std::any_of
	(
		fullPath.begin()
		fullPath.end(),
		[&](const auto& path) { return path.getStr().startsWith('.'); }
	);

	if (hasDotPath)
	{
		LOG
		(
			LOG_ERROR,
			"SharedObject.getLocal(): Invalid path with `.` prefixed "
			"segments."
		);
		return AVM1Value::nullVal;
	}

	auto sys = act.getSys();

	auto sharedObj = sys->getAVM1SharedObject(fullPath);
	// If there's an existing `SharedObject`, return it.
	if (!sharedObj.isNull())
		return sharedObj;

	// NOTE: `data` should only exist for `SharedObject`s created by
	// `get{Local,Remote}()`.
	auto ctor = act.getPrototypes()->sharedObject->ctor;
	sharedObj = ctor->construct
	(
		act,
		{}
	).toObject(act)->as<AVM1SharedObject>();

	sharedObj->name = fullPath;

	// Try to read the `SharedObject`'s data from disk.
	sharedObj->defineValue("data", *sys->tryReadSharedObject
	(
		fullPath
	).andThen([&](const auto& reader)
	{
		// Data exists: Deserialize the data into an `Object`.
		return makeOptional(deserializeLSO(act, reader));
	}).orElse([&]
	{
		// No data: Create a new `Object`.
		return NEW_GC_PTR(act.getGcCtx(), AVM1Object
		(
			act.getGcCtx(),
			act.getPrototypes()->object->proto
		));
	}), AVM1PropFlags::DontDelete);

	sys->addAVM1SharedObject(fullPath, sharedObj);
	return sharedObj;
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
