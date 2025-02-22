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

#ifndef CASELESS_STRING_H
#define CASELESS_STRING_H 1

#include "tiny_string.h"

namespace lightspark
{

class CaselessString
{
public:
	CaselessString(const tiny_string& _str) : str(_str) {}

	bool operator==(const tiny_string& other) const
	{
		return str.caselessEquals(other);
	}

	operator const tiny_string&() const { return str; }
	const tiny_string& getStr() const { return str; }
private:
	tiny_string str;
};

FORCE_INLINE bool operator==(const tiny_string& str, const CaselessString& other)
{
	return str.caselessEquals(other);
}

struct CaselessHash
{
	using is_transparent = void;

	size_t operator()(const tiny_string& str) const
	{
		return std::hash<tiny_string>{}(str.lowercase());
	}

	size_t operator()(const CaselessString& str) const
	{
		return std::hash<tiny_string>{}(str.getStr().lowercase());
	}
};

}

namespace std
{

template<>
struct hash<lightspark::CaselessString>
{
	size_t operator()(const lightspark::CaselessString& str) const
	{
		return hash<lightspark::tiny_string>{}(str.getStr().lowercase());
	}
};

}
#endif /* CASELESS_STRING_H */
