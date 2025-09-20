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

#include <algorithm>
#include <iterator>

#include "utils/filesystem.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

using ConstStrIter = Path::ConstStrIter;

ConstStrIter Path::Iter::incImpl(const ConstStrIter& it) const
{
	return last;
}

ConstStrIter Path::Iter::decImpl(const ConstStrIter& it) const
{
	return std::find
	(
	 	std::reverse_iterator<ConstStrIter>(it),
		std::reverse_iterator<ConstStrIter>(first),
		nativeSeparator
	).base();
}

Path::PlatformStringType Path::toPlatformStr(const StringType& str)
{
	return str;
}

Path::StringType Path::fromPlatformStr(const PlatformStringType& str)
{
	return str;
}

size_t Path::rootNameLengthImpl() const
{
	// Nothing to do here.
	return 0;
}

int Path::compareImpl(const Path& other) const
{
	return path.compare(other.path);
}

void Path::postprocessPathImpl(const Format& format)
{
	// Nothing to do here, since the native, and generic path separator
	// are the same.
}

void Path::checkLongPath()
{
	// Nothing to do here.
}

bool Path::isAbsolute() const
{
	return hasRootDir();
}
