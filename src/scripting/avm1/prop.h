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

#ifndef SCRIPTING_AVM1_PROP_H
#define SCRIPTING_AVM1_PROP_H 1

#include <array>

#include "scripting/avm1/value.h"
#include "smartrefs.h"
#include "utils/enum.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::property::Property`.

namespace lightspark
{

class AVM1Object;

// The property flags of AVM1's runtime.
// NOTE: The values are significant, and should match the order used by
// `Object.ASSetPropFlags()`.
enum class AVM1PropFlags : uint16_t
{
	DontEnum = 1 << 0,
	DontDelete = 1 << 1,
	ReadOnly = 1 << 2,

	VersionMask = 0x1fff << 3,
	Version5 = 0b0000'0000'0000'0000,
	Version6 = 0b0000'0000'1000'0000,
	Version7 = 0b0000'0101'0000'0000,
	Version8 = 0b0001'0000'0000'0000,
	Version9 = 0b0010'0000'0000'0000,
	Version10 = 0b0100'0000'0000'0000,
};

// To check if a property is available in a specific SWF version, mask
// the property flags against the entry in this table. If the result is
// non zero, then the property should be hidden.
static constexpr std::array<uint16_t, 10> versionMasks =
{
	// NOTE: SWF 4, and eariler always hide properties.
	// This shouldn't be used in that case, since SWF 4 doesn't
	// really have much ActionScript support.
	0b0111'1111'1111'1000,
	0b0111'1111'1111'1000,
	0b0111'1111'1111'1000,
	0b0111'1111'1111'1000,
	0b0111'1111'1111'1000,
	// SWF 5, and later.
	0b0111'0100'1000'0000, // SWF 5.
	0b0111'0101'0000'0000, // SWF 6.
	0b0111'0000'0000'0000, // SWF 7.
	0b0110'0000'0000'0000, // SWF 8.
	0b0100'0000'0000'0000, // SWF 9.
};

class AVM1Prop
{
private:
	using ValueType = AVM1Value::Type;
	AVM1Value data;
	_NR<AVM1Object> getter;
	_NR<AVM1Object> setter;
	AVM1PropFlags flags;
public:
	// Constructs a stored property.
	AVM1Prop
	(
		const AVM1Value& _data,
		const AVM1PropFlags& _flags
	) : data(_data), flags(_flags) {}

	// Constructs a virtual property.
	AVM1Prop
	(
		const _R<AVM1Object>& _getter,
		const _NR<AVM1Object>& _setter,
		const AVM1PropFlags& _flags
	) :
	data(ValueType::Undefined),
	getter(_getter),
	setter(_setter),
	flags(_flags) {}

	const AVM1Value& getData() const { return data; }
	const _NR<AVM1Object>& getGetter() const { return getter; }
	const _NR<AVM1Object>& getSetter() const { return setter; }

	// Store data on this property, ignoring virtual setters.
	//
	// NOTE: Read only properties aren't affected.
	void setData(const AVM1Value& _data)
	{
		if (isReadOnly())
			return;
		data = _data;
		// NOTE: Overwriting a property also clears the SWF version
		// requirements.
		flags &= ~AVM1PropFlags::VersionMask;
	}

	// Make this property virtual, by attaching a getter/setter to it.
	void setVirtual
	(
		const _R<AVM1Object>& _getter,
		const _NR<AVM1Object>& _setter
	)
	{
		getter = _getter;
		setter = _setter;
	}

	// Get the property's flags.
	const AVM1PropFlags getFlags() const { return flags; }

	// Redefine the property's flags.
	void setFlags(const AVM1PropFlags& _flags) { flags = _flags; }

	bool isEnumerable() const { return !(flags & AVM1PropFlags::DontEnum); }
	bool isDeletable() const { return !(flags & AVM1PropFlags::DontDelete); }
	bool isReadOnly() const { return flags & AVM1PropFlags::ReadOnly; }
	bool isWritable() const { return !(flags & AVM1PropFlags::ReadOnly); }
	bool isVirtual() const { return !getter.isNull(); }

	// Checks if this property is accessable in the supplied SWF version.
	// If `false`, the property should return `undefined`.
	bool allowSWFVersion(uint8_t swfVersion) const
	{
		if (swfVersion >= versionMasks.size())
			return false;
		return flags & versionMasks[swfVersion];
	}
};

}
#endif /* SCRIPTING_AVM1_PROP_H */
