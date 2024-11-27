/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_WIDE_CHAR_H
#define UTILS_WIDE_CHAR_H 1

#include <codecvt>
#include <cstdlib>
#include <locale>
#include <string>
#include <utility>

#include "traits.h"
#include "utils.h"

template<typename T = wchar_t, std::enable_if_t<is_wchar<T>::value, bool> = false>
struct WChar
{
	using StringType = std::basic_string<T>;
	T ch;
	
	template<class Archive>
	void save(Archive& archive) const
	{
		archive(fromWString((StringType)*this));
	}

	template<class Archive>
	void load(Archive& archive)
	{
		std::string str;
		archive(str);
		ch = toWString<T>(str)[0];
	}

	WChar& operator=(const T& c)
	{
		ch = c;
		return *this;
	}

	operator StringType() const { return StringType(1, ch); }
	operator std::string() const { return fromWString(StringType(1, ch)); }
	operator T() const { return ch; }
};

#endif /* UTILS_WIDE_CHAR_H */
