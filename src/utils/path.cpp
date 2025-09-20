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
#include "utils/filesystem.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

using ConstStrIter = Path::ConstStrIter;
using StringType = Path::StringType;
using ValueType = Path::ValueType;
using Format = Path::Format;

ConstStrIter Path::Iter::inc(const ConstStrIter& pos) const
{
	if (pos == last)
		return pos;

	auto it = pos;
	bool fromStart = it == first || it == prefix;
	if (fromStart && it == first && std::distance(prefix, first) > 0)
		return prefix;
	else if (*it++ != nativeSeparator)
	{
		// Handle implementation specific stuff.
		auto ret = incImpl(it);
		return ret != last ? ret : std::find(it, last, nativeSeparator);
	}

	// The only way to be on a separator, is if it's either a network
	// name, or the root directory.
	if (it == last || *it != nativeSeparator)
		return it;

	auto next = std::next(it);
	if (!fromStart || (next != last && *next == nativeSeparator))
	{
		// Skip any extra separators.
		for (; it != last && *it == nativeSeparator; ++it);
		return it;
	}
	// Found a leading double separator, treat it, and everything upto
	// the next separator as a single unit.
	return std::find(++it, last, nativeSeparator);
}

ConstStrIter Path::Iter::dec(const ConstStrIter& pos) const
{
	if (pos == first)
		return pos;

	auto it = std::prev(pos);

	// If this is the root separator, or a trailing separator, then we're
	// done.
	if (it == root || (pos == last && *it == nativeSeparator))
		return it;

	// Handle implementation specific stuff.
	it = decImpl(it);

	// Check if this is a network name.
	auto _first = first;
	auto dist = std::distance(first, it);
	if (dist == 2 && *_first++ == nativeSeparator && *_first == nativeSeparator)
		return std::prev(it, 2);
	return it;
}

void Path::Iter::updateCurrent()
{
	bool clearCurrent = iter == last ||
	(
		iter != first &&
		*iter == nativeSeparator && iter != root &&
		std::next(iter) == last
	);

	if (clearCurrent)
		current.clear();
	else
		current.assign(iter, inc(iter));
}

Path::Iter& Path::Iter::operator++()
{
	auto incCond = [&](const ConstStrIter& it)
	{
		return
		(
			// Didn't reach the end.
			it != last &&
			// It isn't the root position.
			it != root &&
			// We're on a separator.
			*it == nativeSeparator &&
			// The separator isn't the last character.
			std::next(iter) != last
		);
	};

	for (iter = inc(iter); incCond(iter); ++iter);
	updateCurrent();
	return *this;
}

Path& Path::operator/=(const Path& other)
{
	if (other.empty())
	{
		if (!path.empty() && !path.endsWith(nativeSeparator) && !path.endsWith(':'))
			path += nativeSeparator;
		return *this;
	}

	bool assignOnly =
	(
		other.isAbsolute() &&
		(path != getRootName().path || other.path != "/")
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
		else if (!first && (path.empty() || !path.endsWith(nativeSeparator)))
			path += nativeSeparator;

		first = false;
		path += it.getNativeStr();
	}

	checkLongPath();
	return *this;
}

bool Path::hasRootNamePrefix() const
{
	const auto prefix =
	(
		StringType::fromChar(nativeSeparator) +
		StringType::fromChar(nativeSeparator)
	);

	return
	(
		path.numChars() > prefixLength + 2 &&
		path.find(prefix) == prefixLength &&
		path[prefixLength + 2] != nativeSeparator
	);
}

size_t Path::rootNameLength() const
{
	size_t ret = rootNameLengthImpl();
	if (ret > 0)
		return ret;

	if (!hasRootNamePrefix())
		return 0;

	size_t pos = path.find(StringType::fromChar(nativeSeparator), prefixLength + 3);
	return pos != StringType::npos ? pos : path.numChars();
}

void Path::postprocessPath(const Format& format)
{
	// TODO: Throw on invalid UTF-8 data.

	if (path.empty())
		return;

	postprocessPathImpl(format);

	StringSizeType offset = hasRootNamePrefix() ? 2 : 0;

	size_t newEnd = prefixLength + offset;
	StringType newPath =
	(
		newEnd > 0 ?
		// NOTE: We need to post decrement `newEnd` here because it must
		// lag behind `i` when removing duplicate separators, or it'll
		// remove the first unique separator.
		path.substr(0, newEnd--) :
		StringType::fromChar(path[newEnd])
	);

	for (size_t i = newEnd + 1; i < path.numChars(); ++i)
	{
		bool isSame =
		(
			path[newEnd] == path[i] &&
			path[newEnd] == nativeSeparator
		);
		if (!isSame)
		{
			newEnd = i;
			newPath += path[i];
		}
	}
	path = newPath;
}

Path& Path::removeFilename()
{
	if (hasFilename())
		path = path.stripSuffix(getFilename().path);
	return *this;
}

Path& Path::replaceFilename(const Path& path)
{
	return removeFilename().append(path);
}

Path& Path::replaceExtension(const Path& _path)
{
	if (hasExtension())
		path = path.stripSuffix(getExtension().path);

	if (!_path.empty() && !_path.path.startsWith('.'))
		path += '.';
	return concat(_path);
}

void Path::appendName(const PlatformValueType* name)
{
	if (path.empty())
	{
		*this /= Path(name);
		return;
	}

	if (!path.endsWith(StringType::fromChar(nativeSeparator)))
		path += nativeSeparator;
	path += Path(name);
	checkLongPath();
}

StringType Path::getGenericStr() const
{
	auto ret = path;
	auto sepStr = StringType::fromChar(genericSeparator);
	for (size_t i = 0; i < ret.numChars(); ++i)
	{
		if (ret[i] == nativeSeparator)
			ret.replace(i, 1, sepStr);
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

Path Path::getRootName() const
{
	return Path
	(
		path.substr(prefixLength, rootNameLength()),
		Format::Native
	);
}

Path Path::getRootDir() const
{
	if (!hasRootDir())
		return Path();
	return Path(tiny_string::fromChar(nativeSeparator), Format::Native);
}

Path Path::getRoot() const
{
	return Path
	(
		getRootName().getStr() + getRootDir().getStr(),
		Format::Native
	);
}

Path Path::getRelative() const
{
	auto rootLen = prefixLength + rootNameLength() + hasRootDir();
	return Path
	(
		path.substr(std::min<size_t>
		(
			rootLen,
			path.numChars()
		), StringType::npos),
		Format::Generic
	);
}

Path Path::getParent() const
{
	auto rootLen = prefixLength + rootNameLength() + hasRootDir();
	if (rootLen >= path.numChars())
		return *this;
	if (empty())
		return Path();
	
	auto it = end().dec(path.end());
	auto it2 = std::next(path.begin(), rootLen);
	if (std::distance(it, it2) > 0 && *it != nativeSeparator)
		--it;
	return Path(path.begin(), it, Format::Native);
}

Path Path::getFilename() const
{
	return hasRelativePath() ? Path(*--end()) : Path();
}

Path Path::getStem() const
{
	auto filename = getFilename().getNativeStr();
	if (filename == "." || filename == "..")
		return Path(filename, Format::Native);

	auto pos = filename.rfind(".");
	if (pos == StringType::npos || !pos)
		return Path(filename, Format::Native);

	return Path(filename.substr(0, pos), Format::Native);
}

Path Path::getExtension() const
{
	auto filename = getFilename();
	if (filename.empty())
		return Path();
	auto pos = filename.path.rfind(".");
	if (pos == StringType::npos || !pos || filename.path == "..")
		return Path();

	return Path
	(
		filename.path.substr(pos, StringType::npos),
		Format::Native
	);
}

bool Path::hasRootDir() const
{
	auto rootLen = prefixLength + rootNameLength();
	return path.numChars() > rootLen && path[rootLen] == nativeSeparator;
}
bool Path::hasRelativePath() const
{
	auto rootLen = prefixLength + rootNameLength() + hasRootDir();
	return rootLen < path.numChars();
}

bool Path::hasFilename() const
{
	return hasRelativePath() && !getFilename().empty();
}

Path Path::lexicallyNormal() const
{
	Path ret;
	bool lastParentDir = false;

	auto appendRet = [&](const StringType& str)
	{
		if (!str.empty() || !lastParentDir)
			ret /= str;
		lastParentDir = str == "..";
	};

	for (StringType str : *this)
	{
		if (str == ".")
		{
			ret /= "";
			continue;
		}

		if (str != ".." || ret.empty())
		{
			appendRet(str);
			continue;
		}
		if (ret == getRoot())
			continue;
		if (*(--ret.end()) != "..")
		{
			ret.path = ret.path.stripSuffix
			(
				tiny_string::fromChar(nativeSeparator)
			);
			ret.removeFilename();
			continue;
		}

		appendRet(str);
	}

	if (ret.empty())
		return Path(".");
	return ret;
}

Path Path::lexicallyRelative(const Path& base) const
{
	if (getRootName() != base.getRootName())
		return Path();
	if (isAbsolute() != base.isAbsolute())
		return Path();
	if (!hasRootDir() && base.hasRootDir())
		return Path();

	auto it = begin();
	auto it2 = base.begin();
	for (; it != end() && it2 != base.end() && *it == *it2; ++it, ++it2);

	if (it == end() && it2 == base.end())
		return Path(".");

	ssize_t count = 0;
	for (const auto& elem : InputIterRange<ConstIter>(it2, base.end()))
	{
		if (elem == "..")
			count--;
		else if (!elem.empty() && elem != ".")
			count++;
	}

	if (!count && (it == end() || it->empty()))
		return Path(".");
	if (count < 0)
		return Path();

	Path ret;
	for (size_t i = 0; i < size_t(count); ++i)
		ret /= "..";
	for (const auto& elem : InputIterRange<ConstIter>(it, end()))
		ret /= elem;
	return ret;
}

Path Path::lexicallyProximate(const Path& base) const
{
	Path ret = lexicallyRelative(base);
	return ret.empty() ? *this : ret;
}

Path::Iter Path::begin() const
{
	return Iter(*this, path.begin());
}

Path::Iter Path::end() const
{
	return Iter(*this, path.end());
}
