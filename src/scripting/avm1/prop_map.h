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

#ifndef SCRIPTING_AVM1_PROP_MAP_H
#define SCRIPTING_AVM1_PROP_MAP_H 1

#include <tsl/ordered_map.h>

#define USE_STRING_ID
#ifdef USE_STRING_ID
#include "swf.h"
#else
#include "caseless_string.h"
#endif
#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/swftypes.h"
#include "utils/impl.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::property_map::PropertyMap`.

namespace lightspark
{

template<typename T>
class Entry
{
protected:
	#ifdef USE_STRING_ID
	using MapType = tsl::ordered_map<uint32_t, T>;
	#else
	using MapType = tsl::ordered_map<tiny_string, T, CaselessHash, std::equal_to<>>;
	#endif
	MapType& map;
public:
	virtual Optional<MapType::value_type> removeEntry() { return {}; }
	virtual Optional<const T&> getEntry() const { return {}; }
	virtual Optional<T&> getEntry() { return {}; }
	virtual Optional<T> insertEntry(const T& value) = 0;
	virtual bool isOccupied() const { return false; }
	virtual bool isVacant() const { return false; }
};

template<typename T>
class OccupiedEntry : public Entry<T>
{
private:
	size_t index;
public:
	OccupiedEntry
	(
		MapType& _map,
		size_t idx
	) : Entry<T>(_map), index(idx) {}

	Optional<MapType::value_type> removeEntry() override
	{
		auto it = map.nth(index);
		assert(it != map.end());
		return *it;
	}

	Optional<const T&> getEntry() const override
	{
		auto it = map.nth(index);
		assert(it != map.end());
		return it->second;
	}

	Optional<T&> getEntry() override
	{
		auto it = map.nth(index);
		assert(it != map.end());
		return it->second;
	}

	Optional<T> insertEntry(const T& value) override
	{
		auto it = map.nth(index);
		assert(it != map.end());
		auto ret = it->second;
		it->second = value;
		return ret;
	}

	bool isOccupied() const override { return true; }
};

template<typename T>
class VacantEntry : public Entry<T>
{
private:
	MapType::key_type key;
public:
	VacantEntry
	(
		MapType& _map,
		const MapType::key_type& _key
	) : Entry<T>(_map), key(_key) {}

	Optional<T> insertEntry(const T& value) override
	{
		map.insert(std::make_pair(key, value));
		return {};
	}

	bool isVacant() const override { return true; }
};


// A map of property names to values used by AVM1.
// This allows for dynamically choosing case sensitivity at runtime,
// which is needed due to SWF 6, and earlier being case insensitive.
// This also maintains insertion order, which is needed for accurate
// enumeration order.
template<typename T>
class AVM1PropMap
{
private:
	#ifdef USE_STRING_ID
	using MapType = tsl::ordered_map<uint32_t, T>;
	#else
	using MapType = tsl::ordered_map<tiny_string, T, CaselessHash, std::equal_to<>>;
	#endif
	MapType propMap;
public:
	AVM1PropMap() = default;
	AVM1PropMap& AVM1PropMap(const AVM1PropMap& other) = default;

	#ifdef USE_STRING_ID
	bool hasKey(uint32_t key) const
	{
		auto it = propMap.find(key);
		return it != propMap.end();
	}

	Impl<Entry<T>> getEntry(uint32_t key)
	{
		auto it = propMap.find(key);
		if (it != propMap.end())
		{
			return OccupiedEntry<T>
			(
				propMap,
				std::distance(propMap.begin(), it)
			);
		}
		return VacantEntry<T>(propMap, key);
	}

	// Gets the value of the specified property.
	Optional<const T&> getProp(uint32_t key) const
	{
		auto it = propMap.find(key);
		if (it != propMap.end())
			return it->second;
		return {};
	}

	// Gets a mutable reference to the value of the specified property.
	Optional<T&> getProp(uint32_t key)
	{
		auto it = propMap.find(key);
		if (it != propMap.end())
			return it->second;
		return {};
	}

	bool hasKey
	(
		const tiny_string& key,
		bool caseSensitive
	) const
	{
		auto sys = getSys();
		return hasKey(sys->getUniqueStringId(key, caseSensitive));
	}

	Impl<Entry<T>> getEntry
	(
		const tiny_string& key,
		bool caseSensitive
	)
	{
		auto sys = getSys();
		return getEntry<T>(sys->getUniqueStringId(key, caseSensitive));
	}

	Optional<const T&> getProp
	(
		const tiny_string& key,
		bool caseSensitive
	) const
	{
		auto sys = getSys();
		return getProp(sys->getUniqueStringId(key, caseSensitive));
	}

	Optional<T&> getProp
	(
		const tiny_string& key,
		bool caseSensitive
	)
	{
		auto sys = getSys();
		return getProp(sys->getUniqueStringId(key, caseSensitive));
	}
	#else
	bool hasKey(const tiny_string& key, bool caseSensitive) const
	{
		auto it = !caseSensitive ? propMap.find
		(
			CaselessString(key)
		) : propMap.find(key);
		return it != propMap.end();
	}

	Impl<Entry<T>> getEntry(const tiny_string& key, bool caseSensitive)
	{
		auto it = !caseSensitive ? propMap.find
		(
			CaselessString(key)
		) : propMap.find(key);

		if (it != propMap.end())
		{
			return OccupiedEntry<T>
			(
				propMap,
				std::distance(propMap.begin(), it)
			);
		}
		return VacantEntry<T>(propMap, key);
	}

	// Gets the value of the specified property.
	Optional<const T&> getProp(const tiny_string& key, bool caseSensitive) const
	{
		auto it = !caseSensitive ? propMap.find
		(
			CaselessString(key)
		) : propMap.find(key);

		if (it != propMap.end())
			return it->second;
		return {};
	}

	// Gets a mutable reference to the value of the specified property.
	Optional<T&> getProp(const tiny_string& key, bool caseSensitive)
	{
		auto it = !caseSensitive ? propMap.find
		(
			CaselessString(key)
		) : propMap.find(key);

		if (it != propMap.end())
			return it->second;
		return {};
	}
	#endif

	// Gets a value by index, based on insertion order.
	Optional<const T&> getPropByIndex(size_t idx) const
	{
		auto it = propMap.nth(idx);
		if (it != propMap.end())
			return it->second;
		return {};
	}

	#ifdef USE_STRING_ID
	Optional<T> insertProp(uint32_t key, const T& value)
	{
		return getEntry(key)->insertEntry(value);
	}

	Optional<T> insertProp
	(
		const tiny_string& key,
		const T& value,
		bool caseSensitive
	)
	{
		auto sys = getSys();
		return insertProp(sys->getUniqueStringId(key, caseSensitive), value);
	}
	#else
	Optional<T> insertProp(const tiny_string& key, const T& value, bool caseSensitive)
	{
		return getEntry(key, caseSensitive)->insertEntry(value);
	}
	#endif

	// Returns an iterator to the beginning of the property map, in Flash
	// Player's insertion order (most recently added first).
	MapType::iterator begin() { return propMap.rbegin(); }
	MapType::const_iterator cbegin() { return propMap.rcbegin(); }

	// Returns an iterator to the end of the property map, in Flash
	// Player's insertion order (most recently added first).
	MapType::iterator end() { return propMap.rend(); }
	MapType::const_iterator end() { return propMap.rcend(); }

	#ifdef USE_STRING_ID
	Optional<T> removeProp(uint32_t key)
	{
		auto it = propMap.find(key);
		if (it != propMap.end())
		{
			auto ret = it->second;
			propMap.erase(it);
			return ret;
		}
		return {};
	}

	Optional<T> removeProp
	(
		const tiny_string& key,
		const T& value,
		bool caseSensitive
	)
	{
		auto sys = getSys();
		return removeProp(sys->getUniqueStringId(key, caseSensitive));
	}
	#else
	Optional<T> removeProp(const tiny_string& key, bool caseSensitive)
	{
		auto it = !caseSensitive ? propMap.find
		(
			CaselessString(key)
		) : propMap.find(key);

		if (it != propMap.end())
		{
			auto ret = it->second;
			propMap.erase(it);
			return ret;
		}
		return {};
	}
	#endif
};

}
#endif /* SCRIPTING_AVM1_PROP_MAP_H */
