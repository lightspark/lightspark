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
#include <codecvt>
#include <iterator>
#include <locale>

#include <windef.h>

#include "utils/filesystem.h"
#include "utils/optional.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

using ConstStrIter = Path::ConstStrIter;

Optional<ConstStrIter> Path::Iter::incImpl(const ConstStrIter& it, bool fromStart) const
{
	auto _it = it;
	if (fromStart && it != last && *it == ':')
		return ++_it;
	return nullOpt;
}

ConstStrIter Path::Iter::decImpl(const ConstStrIter& it) const
{
	static const StringType separators = "\\:";
	auto _it = std::find_first_of
	(
	 	std::reverse_iterator<ConstStrIter>(it),
		std::reverse_iterator<ConstStrIter>(first),
		separators.begin(),
		separators.end()
	).base();

	if (std::distance(_it, first) > 0 && *_it == ':')
		++_it;
	return _it;
}

Path::PlatformStringType Path::toPlatformStr(const StringType& str)
{
	return std::wstring_convert
	<
		std::codecvt_utf8_utf16<wchar_t>
	>{}.from_bytes(str);
}

Path::StringType Path::fromPlatformStr(const PlatformStringType& str)
{
	return std::wstring_convert
	<
		std::codecvt_utf8_utf16<wchar_t>
	>{}.to_bytes(str);
}

size_t Path::rootNameLengthImpl() const
{
	if (path.numChars() < prefixLength + 2)
		return 0;
	
	auto ch = path.lowercase()[prefixLength];
	return ch >= 'a' && ch <= 'z' && path[prefixLength + 1] == ':' ? 2 : 0;
}

int Path::compareImpl(const Path& other) const
{
	auto rootLength1 = rootNameLength();
	auto rootLength2 = other.rootNameLength();
	auto otherPath = other.path.substr(0, rootLength2);
	auto ret = path.substr
	(
		0,
		rootLength1
	).strcasecmp(otherPath);
	#ifdef USE_LWG_2936
	return ret;
	#else
	if (ret)
		return ret;
	return path.substr
	(
		rootLength1,
		StringType::npos
	).compare(other.path.substr(rootLength2, StringType::npos));
	#endif
}

#ifndef DISABLE_AUTO_PREFIXES
bool needsAutoPrefix(const Path& path)
{
	return
	(
		path.isAbsolute() &&
		path.getStr().numChars() >= MAX_PATH - 12 &&
		!path.getStr().startsWith("\\\\?\\")
	);
}
#endif

void Path::postprocessPathImpl(const Format& format)
{
	static const auto sepStr = StringType::fromChar(nativeSeparator);
	for (size_t i = 0; i < path.numChars(); ++i)
	{
		if (path[i] == genericSeparator)
			path.replace(i, 1, sepStr);
	}

	#ifndef DISABLE_AUTO_PREFIXES
	if (needsAutoPrefix(*this))
		path = StringType("\\\\?\\") + path;

	prefixLength = 0;
	if (path.numChars() < 6 || path[2] != '?')
		return;

	auto ch = path.lowercase()[4];
	if (ch < 'a' || ch > 'z' || path[5] != ':')
		return;

	if (path.startsWith("\\\\?\\") || path.startsWith("\\??\\"))
		prefixLength = 4;
	#endif
}

void Path::checkLongPath()
{
	#ifndef DISABLE_AUTO_PREFIXES
	if (needsAutoPrefix(*this))
		postprocessPath(Format::Native);
	#else
	// Nothing to do here.
	#endif
}

bool Path::isAbsolute() const
{
	return hasRootName() && hasRootDir();
}
