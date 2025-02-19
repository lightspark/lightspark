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

#ifndef CASE_STRING_H
#define CASE_STRING_H 1

#include "tiny_string.h"

namespace lightspark
{

struct CaseString
{
	tiny_string str;
	bool caseSensitive;

	CaseString
	(
		const tiny_string& _str,
		bool _caseSensitive
	) : str(_str), caseSensitive(_caseSensitive) {}
	CaseString(const tiny_string& _str) : CaseString(_str, true) {}

	bool operator==(const CaseString& other) const
	{
		return str.equalsWithCase(other.str, other.caseSensitive);
	}
};

}

namespace std
{

template<>
struct hash<lightspark::CaseString>
{
	size_t operator()(const lightspark::CaseString& str) const
	{
		return hash<lightspark::tiny_string>{}(str.str.lowercase());
	}
};

}
#endif /* CASE_STRING_H */
