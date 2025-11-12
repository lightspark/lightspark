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

#ifndef SCRIPTING_AVM1_OBJECT_DISPLAY_OBJECT_H
#define SCRIPTING_AVM1_OBJECT_DISPLAY_OBJECT_H 1

#include "gc/ptr.h"
#include "scripting/avm1/prop_map.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::object::StageObject`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class DisplayObject;
class GcContext
template<typename T>
class Optional;

// TODO: Move most of this into `DisplayObject`.
class AVM1DisplayObject : public AVM1Object
{
private:
	GcPtr<DisplayObject> dispObj;

	Optional<AVM1Value> resolvePathProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;
public:
	// Depths used/returned by ActionScript are offset by this amount from
	// depths used inside the SWF, or by the VM.
	// The depth of `DisplayObject`s placed on the timeline in Flash (the
	// authoring tool) starts at 0 in the SWF, but is negative when returned
	// by `MovieClip.getDepth()`.
	// Add this to convert from AVM1 depth, to SWF depth.
	static constexpr size_t AVM1depthOffset = 16384;

	// The mximum depth that AVM1 allows when swapping, or attching clips.
	static constexpr size_t AVM1maxDepth = 2130706428;

	// The mximum depth that AVM1 allows when removing clips.
	static constexpr size_t AVM1maxRemoveDepth = 2130706416;

	// Create a display object for a given display node.
	AVM1DisplayObject
	(
		AVM1Activation& activation,
		const GcPtr<DisplayObject>& _dispObj,
		const GcPtr<AVM1Object>& proto
	) : AVM1Object(activation, proto), dispObj(_dispObj) {}

	Optional<AVM1Value> getLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		bool isSlashPath
	) const override;

	void setLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value,
		const GcPtr<AVM1Object>& _this
	) override;

	bool hasProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override;

	GetKeysType getKeys
	(
		AVM1Activation& activation,
		bool includeHidden
	) const override;

	// Add common `DisplayObject` methods to the supplied prototype.
	static void defineProto
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	// Add common `InteractiveObject` methods to the supplied prototype.
	static void defineInteractiveProto
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto,
		bool isMovieClip = false
	);

	AVM1_FUNCTION_DECL(getDepth);
	AVM1_PROPERTY_TYPE_DECL(DisplayObject, Filters);
	AVM1_PROPERTY_TYPE_DECL(InteractiveObject, tabIndex);

	static void removeDisplayObject
	(
		AVM1Activation& activation,
		const GcPtr<DisplayObject>& _this
	);

	operator const DisplayObject*() const { return dispObj; }
	operator DisplayObject*() { return dispObj; }
	operator const GcPtr<DisplayObject>&() const { return dispObj; }
	operator GcPtr<DisplayObject>&() { return dispObj; }
};

#define AVM1_DISP_GETTER_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<DisplayObject>& _this \
	)
#define AVM1_DISP_GETTER_DECL(name) \
	AVM1Value name(AVM1_DISP_GETTER_ARGS)

#define AVM1_DISP_SETTER_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<DisplayObject>& _this, \
		const GcPtr<AVM1Value& value \
	)
#define AVM1_DISP_SETTER_DECL(name) \
	void name(AVM1_DISP_SETTER_ARGS)

using AVM1DisplayGetter = AVM1Value(*)(AVM1_DISP_GETTER_ARGS);
using AVM1DisplaySetter = void(*)(AVM1_DISP_SETTER_ARGS);

// Properties shared by `DisplayObject`s in AVM1, such as `_x`, and `_y`.
// These are only accessible for `MovieClip`s, `SimpleButton`s,
// `TextField`s, and `Video`s.
// These exist outside the global, or prototype handling. Instead, they're
// "special" properties stored in a separate map that `DisplayObject`s
// look at, in addition to the normal property lookup.
class AVM1DisplayProp
{
private:
	AVM1DisplayGetter getter;
	AVM1DisplaySetter setter;
public:
	AVM1_DISP_GETTER_DECL(get) const
	{
		return getter(act, _this);
	}

	AVM1_DISP_SETTER_DECL(set) const
	{
		if (setter == nullptr)
			return;
		setter(act, _this, value);
	}

	bool isReadOnly() const { return setter == nullptr; }
};

// The map from keys/indices to function pointers for special
// `DisplayObject` properties.
class AVM1DisplayPropMap
{
private:
	AVM1PropMap<AVM1DisplayProp> propMap;
public:
	// Creates the property map.
	AVM1DisplayPropMap();

	// Gets a property slot by name,
	// Used by `Action{G,S}et{Variable,Member}`.
	Optional<AVM1DisplayProp> getByName(const tiny_string& name) const
	{
		// NOTE: `DisplayObject` properties are *always* case insenstive,
		// regardless of the SWF version.
		return propMap.getProp(name, false);
	}

	// Gets a property slot by SWF 4 index.
	// The order is defined in the SWF spec.
	// Used by `Action{G,S}etProperty`.
	// SWF 19 spec pages. 85-86.
	Optional<AVM1DisplayProp> getByIndex(size_t idx) const
	{
		return propMap.getPropByIndex(idx);
	}
};

}
#endif /* SCRIPTING_AVM1_OBJECT_DISPLAY_OBJECT_H */
