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

#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/object/display_object.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object::StageObject`.

const static AVM1DisplayPropMap dispPropMap;

Optional<AVM1Value> AVM1DisplayObject::getLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	bool isSlashPath
) const
{
	bool caseSensitive = activation.isCaseSensitive();

	// Property search order for `DisplayObject`s.
	// 1. Expandos (user defined properties on the underlying object).
	auto ret = AVM1Object::getLocalProp(activation, name, isSlashPath);

	if (ret.hasValue())
		return ret;

	bool isInternalProp = s.startsWith("_") && !s.startsWith("__");
	// 2. Path properties. i.e. `_root`, `_parent`, `_level<depth>`
	// (honours case sensitivity).
	if (isInternalProp)
		ret = resolvePathProp(activation, name);

	if (ret.hasValue())
		return ret;

	// 3. Child `DisplayObject`s with the supplied instance name.
	auto child = dispObj->tryAs
	<
		DisplayObjectContainer
	>().transformOr(nullptr, [&](const auto& cont)
	{
		return cont.getChildByName(name, caseSensitive);
	});

	if (child != nullptr)
	{
		// NOTE: If the object can't be represented as an AVM1 object,
		// such as `Shape`, then any attempt to access it'll return the
		// parent instead.
		if (!isSlashPath && child->is<Shape>())
			return child->getParent().asAVM1Object();
		return child->asAVM1Object();
	}

	// 4. Internal properties, such as `_x`, and `_y` (always case
	// insensitive).
	if (!isInternalProp)
		return {};

	return dispPropMap.getByName(name).transform([&](const auto& prop)
	{
		return prop.get(activation, dispObj);
	});
}

void AVM1DisplayObject::setLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value,
	const GcPtr<AVM1Object>& _this
)
{
	// If a `TextField` was bound to this property, update it's text.
	dispObj->notifyPropChange(activation, name, value);

	if (hasOwnProp(activation, name))
	{
		AVM1Object::setLocalProp(activation, name, value, _this);
		return;
	}

	(void)dispPropMap.getByName(name).andThen([&](const auto& prop)
	{
		// NOTE: Internal properties (such as `_x`, and `_y`) take
		// priority over properties in prototypes.
		prop.set(activation, name, value);
	}).orElse([&]
	{
		AVM1Object::setLocalProp(activation, name, value, _this);
	});
}

// NOTE: `Object.hasOwnProperty()` doesn't return true for child
// `DisplayObject`s.
bool AVM1DisplayObject::hasProp
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	bool caseSensitive = activation.isCaseSensitive();

	if (!dispObj->isAVM1Removed() && AVM1Object::hasProp(activation, name))
		return true;

	bool isInternalProp = s.startsWith("_");
	if (isInternalProp && dispPropMap.getByName(name).hasValue())
		return true;

	bool ret = dispObj->tryAs
	<
		DisplayObjectContainer
	>.filter
	(
		!dispObj->isAVM1Removed()
	).transformOr(false, [&](const auto& container)
	{
		return container.hasChildByName(name, caseSensitive);
	});

	return ret ||
	(
		isInternalProp &&
		resolvePathProp(activation, name).hasValue()
	);
}

AVM1Object::GetKeysType AVM1DisplayObject::getKeys
(
	AVM1Activation& activation,
	bool includeHidden
) const
{
	// NOTE: Keys from the underlying object are first in the list,
	// followed by child `DisplayObject`s, sorted by depth in decending
	// order.
	auto ret = AVM1Object::getKeys(activation, includeHidden);

	auto container = dispObj->as<DisplayObjectContainer>();
	if (container == nullptr)
		return ret;

	// NOTE: `SimpleButton`, and `MovieClip` children are included in the
	// list.
	const auto& dispList = container->getDisplayList();

	for (auto it = dispList.crbegin(); it != dispList.crend(); ++it)
	{
		if (!*it->is<InteractiveObject>())
			continue;
		#ifdef USE_STRING_ID
		ret.push_back(*it->getNameId());
		#else
		ret.push_back(*it->getName());
		#endif
	}

	return ret;
}

static int parseLevelId(const tiny_string& digits)
{
	bool isNegative = digits.startsWith('-');

	int levelID = 0;
	for (size_t i = isNegative; i < digits.numChars() && isdigit(digits[i]); ++i)
		levelID = (levelID * 10) + (digits[i] - '0');

	return isNegative ? -levelID : levelID;
}

Optional<AVM1Value> AVM1DisplayObject::resolvePathProp
(
	AVM1Activation& activation,
	const tiny_string& name
) const
{
	const auto& undefVal = AVM1Value::undefinedVal;
	bool caseSensitive = activation.isCaseSensitive();

	if (name.equalsWithCase("_root", caseSensitive))
		return activation.getRootObject();
	else if (name.equalsWithCase("_parent", caseSensitive))
	{
		auto parent = dispObj->getParent();
		if (parent != nullptr)
			return parent->asAVM1Object();
		return undefVal;
	}
	else if (name.equalsWithCase("_global", caseSensitive))
		return activation.getCtx().getGlobalObject();


	// Resolve level names (`_level<depth>`).
	auto prefix = name.substr(0, 6);
	bool isLevelName =
	(
		prefix.equalsWithCase("_level", caseSensitive) ||
		// NOTE: `_flash` is an alias for `_level`, which is a relic of
		// the earliest versions of Flash Player.
		prefix.equalsWithCase("_flash", caseSensitive)
	);

	if (!isLevelName)
		return {};

	return activation.getLevel(parseLevelId(name.substr
	(
		6,
		tiny_string::npos
	))).asOpt().transformOr(undefVal, [&](const DisplayObject& obj)
	{
		return obj.asAVM1Object();
	});
}

AVM1_DISP_GETTER_DECL(getX)
{
	return _this->getX().toPixels();
}

AVM1_DISP_SETTER_DECL(setX)
{
	propToNum(act, value).andThen([&](const number_t& x)
	{
		_this->setX(Twips::fromPixels(x));
		return makeOptional(x);
	});
}

AVM1_DISP_GETTER_DECL(getY)
{
	return _this->getY().toPixels();
}

AVM1_DISP_SETTER_DECL(setY)
{
	propToNum(act, value).andThen([&](const number_t& y)
	{
		_this->setY(Twips::fromPixels(y));
		return makeOptional(y);
	});
}

AVM1_DISP_GETTER_DECL(getXScale)
{
	return _this->getScaleX() * 100.0;
}

AVM1_DISP_SETTER_DECL(setXScale)
{
	propToNum(act, value).andThen([&](const number_t& scaleX)
	{
		_this->setScaleX(scaleX / 100.0);
		return makeOptional(scaleX);
	});
}

AVM1_DISP_GETTER_DECL(getYScale)
{
	return _this->getScaleY() * 100.0;
}

AVM1_DISP_SETTER_DECL(setYScale)
{
	propToNum(act, value).andThen([&](const number_t& scaleY)
	{
		_this->setScaleY(scaleY / 100.0);
		return makeOptional(scaleY);
	});
}

AVM1_DISP_GETTER_DECL(getCurrentFrame)
{
	const auto& undefVal = AVM1Value::undefinedVal;

	return _this->tryAs<MovieClip>().transformOr(undefVal, [&](const auto& clip)
	{
		return number_t(clip.getCurrentFrame());
	});
}

AVM1_DISP_GETTER_DECL(getTotalFrames)
{
	const auto& undefVal = AVM1Value::undefinedVal;

	return _this->tryAs<MovieClip>().transformOr(undefVal, [&](const auto& clip)
	{
		return number_t(clip.getTotalFrames());
	});
}

AVM1_DISP_GETTER_DECL(getAlpha)
{
	return _this->getAlpha() * 100.0;
}

AVM1_DISP_SETTER_DECL(setAlpha)
{
	propToNum(act, value).andThen([&](const number_t& alpha)
	{
		_this->setAlpha(alpha / 100.0);
		return makeOptional(alpha);
	});
}

AVM1_DISP_GETTER_DECL(getVisible)
{
	return _this->getVisible();
}

AVM1_DISP_SETTER_DECL(setVisible)
{
	// NOTE: Because this property dates back to SWF 4, it actually gets
	// converted to an integer.
	// `_visible = "false";` gets converted to `NaN`, and has no effect.
	propToNum(act, value).andThen([&](const number_t& visible)
	{
		_this->setVisible(visible != 0.0);
		return makeOptional(visible);
	});
}

AVM1_DISP_GETTER_DECL(getWidth)
{
	return _this->getWidth();
}

AVM1_DISP_SETTER_DECL(setWidth)
{
	propToNum(act, value).andThen([&](const number_t& width)
	{
		_this->setWidth(width);
		return makeOptional(width);
	});
}

AVM1_DISP_GETTER_DECL(getHeight)
{
	return _this->getHeight();
}

AVM1_DISP_SETTER_DECL(setHeight)
{
	propToNum(act, value).andThen([&](const number_t& height)
	{
		_this->setHeight(height);
		return makeOptional(height);
	});
}

AVM1_DISP_GETTER_DECL(getRotation)
{
	return _this->getRotation();
}

AVM1_DISP_SETTER_DECL(setRotation)
{
	propToNum(act, value).andThen([&](number_t degrees)
	{
		// Normalize into the range -180-180.
		degrees = std::fmod(degrees, 360);
		if (degrees < -180 || degrees > 180)
			degrees += degrees < -180 ? 360 : -360;
		_this->setRotation(degrees);
		return makeOptional(degrees);
	});
}

AVM1_DISP_GETTER_DECL(getTarget)
{
	return _this->getSlashPath();
}

AVM1_DISP_GETTER_DECL(getFramesLoaded)
{
	const auto& undefVal = AVM1Value::undefinedVal;

	return _this->tryAs<MovieClip>().transformOr(undefVal, [&](const auto& clip)
	{
		return number_t(std::min
		(
			clip.getFramesLoaded(),
			clip.getTotalFrames()
		));
	});
}

AVM1_DISP_GETTER_DECL(getName)
{
	#ifdef USE_STRING_ID
	return _this->getNameId();
	#else
	return _this->getName();
	#endif
}

AVM1_DISP_SETTER_DECL(setName)
{
	#ifdef USE_STRING_ID
	auto name = value.toStringId(act);
	#else
	auto name = value.toString(act);
	#endif
	_this->setName(name);
}

AVM1_DISP_GETTER_DECL(getDropTarget)
{
	const auto& undefVal = AVM1Value::undefinedVal;

	return _this->tryAs<MovieClip>().andThen([&](const auto& clip)
	{
		return clip.getDropTarget();
	}).transform([&](const auto& target)
	{
		return AVM1Value(target.getSlashPath());
	}).orElse([&]
	{
		if (act.getSwfVersion() < 6)
			return undefVal;
		return "";
	});
}

AVM1_DISP_GETTER_DECL(getURL)
{
	return _this->tryAs<MovieClip>().transformOr("", [&](const auto& clip)
	{
		return clip.getURL();
	});
}

AVM1_DISP_GETTER_DECL(getHighQuality)
{
	switch (act.getStage()->getQuality())
	{
		case Quality::Best: return number_t(2); break;
		case Quality::High: return number_t(1); break;
		default: return number_t(0); break;
	}
}

AVM1_DISP_SETTER_DECL(setHighQuality)
{
	auto num = value.toNumber(act);
	if (std::isnan(num))
		return;
	// NOTE 0 = `Low`, 1 = `High`, 2 = `Best`, but with some odd rules
	// for non integer values.
	auto quality =
	(
		num > 1.5 ? Quality::Best :
		num != 0.0 ? Quality::High :
		Quality::Low
	);

	act.getStage()->setQuality(act.getCtx(), quality);
}

AVM1_DISP_GETTER_DECL(getFocusRect)
{
	if (refersToStageFocusRect(act, _this))
	{
		auto stageFocusRect = act.getStage()->getStageFocusRect();
		if (act.getSwfVersion() <= 5)
			return number_t(stageFocusRect);
		return stageFocusRect;
	}

	const auto& undefVal = AVM1Value::undefinedVal;

	return _this->tryAs<InteractiveObject>().andThen([&](const auto& obj)
	{
		return obj.getFocusRect();
	}).transformOr(undefVal, [&](bool focusRect) { return focusRect });
}

AVM1_DISP_SETTER_DECL(setFocusRect)
{
	if (refersToStageFocusRect(act, _this))
	{
		// `null`, and `undefined` are ignored.
		if (value.is<NullVal>() || value.is<UndefinedVal>())
			return;

		act.getStage()->setStageFocusRect
		(
			1value.is<AVM1Object>() &&
			value.toNumber(act) != 0
		);
	}

	(void)_this->tryAs<InteractiveObject>().andThen([&](const auto& obj)
	{
		Optional<bool> focusRect;
		if (!value.is<NullVal>() && !value.is<UndefinedVal>())
			focusRect = value.toBool(act.getSwfVersion());
		obj.setFocusRect(focusRect);
		return makeOptionalRef(obj);
	});
}

AVM1_DISP_GETTER_DECL(getSoundBufTime)
{
	return number_t(act.getSys()->static_SoundMixer_bufferTime);
}

AVM1_DISP_SETTER_DECL(setSoundBufTime)
{
	LOG(LOG_NOT_IMPLEMENTED, "_soundbuftime is currently ignored.");
	auto num = value.toNumber(act);
	if (std::isnan(num))
		return;

	act.getSys()->static_SoundMixer_bufferTime = num;
}

AVM1_DISP_GETTER_DECL(getQuality)
{
	switch (act.getStage()->getQuality())
	{
		case Quality::Low: return "LOW"; break;
		case Quality::High: return "HIGH"; break;
		case Quality::Best: return "BEST"; break;
		case Quality::High8x8:
		case Quality::High8x8Linear: return "8X8"; break;
		case Quality::High16x16:
		case Quality::High16x16Linear: return "16X16"; break;
		default: return AVM1Value::undefinedVal; break;
	}
}

AVM1_DISP_SETTER_DECL(setQuality)
{
	Quality quality;
	auto str = value.toString(act);
        if (str.caselessEquals("low"))
		quality = Quality::Low;
        else if (str.caselessEquals("medium"))
		quality = Quality::Medium;
        else if (str.caselessEquals("high"))
		quality = Quality::High;
        else if (str.caselessEquals("best"))
		quality = Quality::Best;
        else if (str.caselessEquals("8x8"))
		quality = Quality::High8x8;
        else if (str.caselessEquals("8x8linear"))
		quality = Quality::High8x8Linear;
        else if (str.caselessEquals("16x16"))
		quality = Quality::High16x16;
        else if (str.caselessEquals("16x16linear"))
		quality = Quality::High16x16Linear;
	else
		return;

	act.getStage()->setQuality(act.getCtx(), quality);
}

AVM1_DISP_GETTER_DECL(getXMouse)
{
	return _this->getLocalMousePos().x.toPixels();
}

AVM1_DISP_SETTER_DECL(getYMouse)
{
	return _this->getLocalMousePos().y.toPixels();
}

AVM1DisplayPropMap::AVM1DisplayPropMap()
{
	// NOTE: The order is important.
	// It should match the SWF spec for `Action{G,S}etProperty`.
	#define GETTER_SETTER(name) { get##name, set##name }
	#define GETTER(name) { get##name, nullptr }
	propMap.insertProp("_x", GETTER_SETTER(X), false);
	propMap.insertProp("_y", GETTER_SETTER(Y), false);
	propMap.insertProp("_xscale", GETTER_SETTER(XScale), false);
	propMap.insertProp("_yscale", GETTER_SETTER(YScale), false);
	propMap.insertProp("_currentframe", GETTER(CurrentFrame), false);
	propMap.insertProp("_totalframes", GETTER(TotalFrames), false);
	propMap.insertProp("_alpha", GETTER_SETTER(Alpha), false);
	propMap.insertProp("_visible", GETTER_SETTER(Visible), false);
	propMap.insertProp("_width", GETTER_SETTER(Width), false);
	propMap.insertProp("_height", GETTER_SETTER(Height), false);
	propMap.insertProp("_rotation", GETTER_SETTER(Rotation), false);
	propMap.insertProp("_target", GETTER(Target), false);
	propMap.insertProp("_framesloaded", GETTER(FramesLoaded), false);
	propMap.insertProp("_name", GETTER_SETTER(Name), false);
	propMap.insertProp("_droptarget", GETTER(DropTarget), false);
	propMap.insertProp("_url", GETTER(URL), false);
	propMap.insertProp("_highquality", GETTER_SETTER(HighQuality), false);
	propMap.insertProp("_focusrect", GETTER_SETTER(FocusRect), false);
	propMap.insertProp("_soundbuftime", GETTER_SETTER(SoundBufTime), false);
	propMap.insertProp("_quality", GETTER_SETTER(Quality), false);
	propMap.insertProp("_xmouse", GETTER(XMouse), false);
	propMap.insertProp("_ymouse", GETTER(YMouse), false);
	#undef GETTER_SETTER(name)
	#undef GETTER(name)
}
