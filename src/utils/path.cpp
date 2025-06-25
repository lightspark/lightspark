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

#include "utils/filesystem.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

using StringType = Path::StringType;
using ValueType = Path::ValueType;
using Format = Path::Format;

Path& Path::operator/=(const Path& other)
{
	if (other.empty())
	{
		tiny_string str = StringType::fromChar(nativeSeparator) + ':';
		if (!path.empty() && path.findLast(str) == StringType::npos)
			path += nativeSeparator;
		return *this;
	}

	bool assignOnly =
	(
		other.isAbsolute() &&
		(path == getRootName().path && other.path != "/")
	) || (other.hasRootName() && other.getRootName() != getRootName());
	if (assignOnly)
		return assign(other);

	if (other.hasRootDir())
		assign(getRootName());
	else if ((!hasRootDir() && isAbsolute()) || hasFilename())
		path += nativeSeparator;

	bool first = true;
	for (const auto& it : other)
	{
		if (first && other.hasRootName())
		{
			first = false;
			continue;
		}

		first = false;
		path += it.getNativeStr();
	}

	checkLongPath();
	return *this;
}

const StringType& Path::getGenericStr() const
{
	auto ret = path;
	auto sepStr = StringType::fromChar(genericSeparator);
	for (size_t i = 0; i < ret.numChars(); ++i)
	{
		if (ret[i] == nativeSeparator)
			ret.replace(i, 0, sepStr);
	}
	return ret;
}

int Path::compare(const Path& other) const
{
	#ifdef USE_LWG_2936
	auto ret = compareImpl(other);
	if (ret)
		return ret;

	bool _hasRootDir = hasRootDir();
	if (_hasRootDir != other.hasRootDir())
		return _hasRootDir ? 1 : -1;

	auto it1 = path.begin() + rootNameLength() + _hasRootDir;
	auto it2 = other.path.begin() + other.rootNameLength() + _hasRootDir;
	for (; it1 != path.end() && it2 != other.path.end(); ++it1, ++it2);
	if (it1 == path.end())
		return -(it2 == other.path.end());
	if (it2 == other.path.end())
		return 1;
	if (*it1 == nativeSeparator)
		return -1;
	if (*it2 == nativeSeparator)
		return 1;
	return *it1 < *it2 ? -1 : 1;
	#else
	return compareImpl(other);
	#endif
}
