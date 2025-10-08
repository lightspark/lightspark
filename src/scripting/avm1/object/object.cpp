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

#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object::{,Script}Object`.

AVM1Value call
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& oldValue,
	const AVM1Value& newValue,
	const GcPtr<AVM1Object>& _this
) const
{
	return callback->exec
	(
		activation,
		name,
		_this,
		0,
		{ name, oldValue, newValue, userData },
		AVM1ExecutionReason::Special,
		callback
	);
}

AVM1Object::AVM1Object
(
	const GcContext& ctx,
	const NullableGcPtr<AVM1Object>& proto
)
{
	if (proto.isNull())
		return;

	defineValue
	(
		ctx,
		"__proto__",
		proto,
		AVM1PropFlags::DontEnum | AVM1PropFlags::DontDelete
	);
}

AVM1Value AVM1Object::getData
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).transformOr(AVM1Value::undefinedVal, [](const AVM1Prop& prop)
	{
		return prop.getData();
	});
}

void AVM1Object::setData
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value
)
{
	// TODO: Call watchers.
	auto entry = properties.getEntry
	(
		name,
		activation.isCaseSensitive()
	);

	(void)entry->getEntry().andThen([&](AVM1Prop& prop)
	{
		prop.setData(value);
		return makeOptionalRef(prop);
	}).orElse([&]
	{
		entry->insert(AVM1Prop(value, AVM1PropFlags(0)));
	});
}

AVM1Object::GetOwnPropsType AVM1Object::getOwnProps() const
{
	GetOwnPropsType ret;

	for (const auto& pair : properties)
	{
		if (pair.second.isEnumerable())
			ret.emplace_back(pair.first, pair.second.getData());
	}
	return ret;
}

Optional<AVM1Value> AVM1Object::getLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	bool isSlashPath
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).filter([&](const AVM1Prop& prop)
	{
		return prop.allowSWFVersion(activation.getSwfVersion());
	).transform([](const AVM1Prop& prop)
	{
		return prop.getData();
	});
}

AVM1Value AVM1Object::lookupProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	bool isSlashPath
) const
{
	GcPtr<AVM1Object> _this(this);
	AVM1Value proto(this);

	if (is<AVM1SuperObject>())
	{
		auto super = as<AVM1SuperObject>();
		_this = super->getThis();
		proto = super->getProto(activation);
	}

	return searchPrototype
	(
		activation,
		proto,
		name,
		_this,
		isSlashPath
	).transformOr(AVM1Value::undefinedVal, [](const auto& pair)
	{
		return pair.first;
	});
}

AVM1Value AVM1Value::getStoredProp
(
	AVM1Activation& act,
	const tiny_string& name
) const
{
	using ExceptionType = AVM1Exception::Type;

	auto proto = this;
	for (uint8_t depth = 0; proto != nullptr; ++depth, proto = proto->getProto(act))
	{
		if (depth == 255)
			throw AVM1Exception(ExceptionType::MaxPrototypeRecursion);

		auto value = proto->getLocalProp(act, name, true);
		if (value.hasValue())
			return value.getValue();
	}
	return AVM1Value::undefinedVal;
}

void AVM1Object::setLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value,
	const GcPtr<AVM1Object>& _this
)
{
	auto entry = properties.getEntry
	(
		name,
		activation.isCaseSensitive()
	);

	(void)entry->getEntry().andThen([&](AVM1Prop& prop)
	{
		prop.setData(value);
		auto setter = prop.getSetter();
		if (setter.isNull())
			return makeOptionalRef(prop);
		auto exec = setter->as<AVM1Executable>();
		if (exec == nullptr)
			return makeOptionalRef(prop);
		(void)exec->exec
		(
			activation,
			"[Setter]",
			_this,
			1,
			{ value },
			AVM1ExecutionReason::Special,
			setter
		);
		return makeOptionalRef(prop);
	}).orElse([&]
	{
		entry->insert(AVM1Prop(value, AVM1PropFlags(0)));
	});
}

void AVM1Object::setProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value
)
{
	if (name.empty())
		return;

	GcPtr<AVM1Object> _this(this);
	AVM1Value proto(this);

	if (is<AVM1SuperObject>())
	{
		auto super = as<AVM1SuperObject>();
		_this = super->getThis();
		proto = super->getProto(activation);
	}

	auto _value = value;
	callWatcher(activation, name, _value, _this);

	if (hasOwnProp(activation, name))
	{
		setLocalProp(activation, name, _value, _this);
		return;
	}

	// Before actually inserting a new property, we need to walk the
	// prototype chain for virtual setters.
	AVM1Object* _proto;
	for (_proto = proto; _proto != nullptr; _proto = _proto->getProto(activation))
	{
		if (!_proto->hasOwnVirtual(activation, name))
			continue;
		auto setter = _proto->getSetter(activation, name);
		if (setter.isNull())
			return;

		auto exec = setter->as<AVM1Executable>();
		if (exec == nullptr)
			return;
		(void)exec->exec
		(
			activation,
			"[Setter]",
			_this,
			1,
			{ _value },
			AVM1ExecutionReason::Special,
			setter
		);
		return;
	}

	setLocalProp(activation, name, _value, _this);
}

AVM1Value AVM1Object::callMethod
(
	AVM1Activation& activation,
	const tiny_string& name,
	const std::vector<AVM1Value>& args,
	const AVM1ExecutionReason& reason
) const
{
	auto dispObj = as<AVM1DisplayObject>();
	if (dispObj != nullptr && dispObj->isAVM1Removed())
		return AVM1Value::undefinedVal;

	auto pair = searchPrototype
	(
		activation,
		this,
		name,
		this,
		false
	);

	if (!pair.hasValue() || !pair->first.is<AVM1Object>())
		return AVM1Value::undefinedVal;

	AVM1Object* method = pair->first;
	auto exec = method->as<AVM1Executable>();
	if (exec == nullptr)
		return method->call(activation, name, this, args);

	// NOTE: If the method was found on the object itself, then change
	// `depth` so that it's as if the method was found on the object's
	// prototype.
	auto depth = std::max<uint8_t>(pair->second, 1);
	return exec->exec
	(
		activation,
		name,
		depth,
		args,
		reason,
		method
	);
}

NullableGcPtr<AVM1Object> AVM1Object::getGetter
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).filter([&](const AVM1Prop& prop)
	{
		return prop.allowSWFVersion(activation.getSwfVersion());
	).transform([](const AVM1Prop& prop)
	{
		return prop.getGetter();
	});
}

NullableGcPtr<AVM1Object> AVM1Object::getSetter
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).filter([&](const AVM1Prop& prop)
	{
		return prop.allowSWFVersion(activation.getSwfVersion());
	).transform([](const AVM1Prop& prop)
	{
		return prop.getSetter();
	});
}

GcPtr<AVM1Object> AVM1Object::createBareObject
(
	AVM1Activation& activation,
	const GcPtr<AVM1Object>& _this
) const
{
	auto gcCtx = activation.getGcCtx();
	return new (gcCtx) AVM1Object(gcCtx, _this)
}


bool AVM1Object::deleteProp
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	auto entry = properties.getEntry
	(
		name,
		activation.isCaseSensitive()
	);

	return entry->getEntry().transformOr(false, [&](const AVM1Prop& prop)
	{
		if (!prop.canDelete())
			return false;
		(void)entry->removeEntry();
		return true;
	});
}

void AVM1Object::defineValue
(
	const tiny_string& name,
	const AVM1Value& value,
	const AVM1PropFlags& flags
)
{
	(void)properties.insertProp(name, AVM1Prop(value, flags), true);
}

void AVM1Object::setPropFlags
(
	const Optional<tiny_string>& name,
	const AVM1PropFlags& setFlags,
	const AVM1PropFlags& clearFlags
)
{
	auto setFlags = [&](AVM1Prop& prop)
	{
		auto newFlags = (prop.getFlags() & ~clearFlags) | setFlags;
		prop.setFlags(newFlags);
	};

	if (!name.hasValue())
	{
		// Change *all* flags.
		for (const auto& pair : properties)
			setFlags(pair.second);
		return;
	}

	(void)properties.getProp(name, false).andThen([&](AVM1Prop& prop)
	{
		setFlags(prop);
	});
}

void AVM1Object::addProp
(
	const tiny_string& name,
	const GcPtr<AVM1Object>& getter,
	const NullableGcPtr<AVM1Object>& setter,
	const AVM1PropFlags& flags
)
{
	auto entry = properties.getEntry(name, false);

	(void)entry->getEntry().andThen([&](AVM1Prop& prop)
	{
		prop.setVirtual(getter, setter);
		return makeOptionalRef(prop);
	}).orElse([&]
	{
		entry->insert(AVM1Prop(getter, setter, flags));
	});
}

void AVM1Object::addPropWithCase
(
	const tiny_string& name,
	const GcPtr<AVM1Object>& getter,
	const NullableGcPtr<AVM1Object>& setter,
	const AVM1PropFlags& flags
)
{
	auto entry = properties.getEntry
	(
		name,
		activation.isCaseSensitive()
	);

	(void)entry->getEntry().andThen([&](AVM1Prop& prop)
	{
		prop.setVirtual(getter, setter);
		return makeOptionalRef(prop);
	}).orElse([&]
	{
		entry->insert(AVM1Prop(getter, setter, flags));
	});
}

void AVM1Object::callWatcher
(
	AVM1Activation& activation,
	const tiny_string& name,
	AVM1Value& value,
	const GcPtr<AVM1Object>& _this,
) const
{
	auto watcher = watchers.getProp
	(
		name,
		activation.isCaseSensitive()
	);

	if (!watcher.hasValue())
		return;

	auto oldValue = getStoredProp(activation, name);

	try
	{
		value = watcher.call(activation, name, oldValue, value, _this);
	}
	catch (AVM1Value&)
	{
		value = AVM1Value::undefinedVal;
		std::rethrow_exception(std::current_exception());
	}
	catch (...)
	{
		value = AVM1Value::undefinedVal;
	}
}

void AVM1Object::watchProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const GcPtr<AVM1Object>& callback,
	const AVM1Value& userData
)
{
	watchers.insertProp
	(
		name,
		AVM1Watcher(callback, userData),
		activation.isCaseSensitive()
	);
}

bool AVM1Object::unwatchProp
(
	AVM1Activation& activation,
	const tiny_string& name
)
{
	return watchers.removeProp
	(
		name,
		activation.isCaseSensitive()
	).hasValue();
}

bool AVM1Object::hasProp
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	if (hasOwnProp(activation, name))
		return true;

	AVM1Object* proto = getProto(activation);
	return proto != nullptr && proto->hasProp(activation, name);
}

bool AVM1Object::hasOwnProp
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.containsKey(name, activation.isCaseSensitive());
}

bool AVM1Object::hasOwnVirtual
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).transformOr(false, [&](const AVM1Prop& prop)
	{
		return
		(
			prop.isVirtual() &&
			prop.allowSWFVersion(activation.getSwfVersion())
		);
	});
}

bool AVM1Object::isPropEnumerable
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	return properties.getProp
	(
		name,
		activation.isCaseSensitive()
	).transformOr(false, [&](const AVM1Prop& prop)
	{
		return prop.isEnumerable();
	});

}
