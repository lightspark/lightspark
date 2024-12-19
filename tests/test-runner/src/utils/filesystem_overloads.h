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

#ifndef UTILS_FILESYSTEM_OVERLOADS_H
#define UTILS_FILESYSTEM_OVERLOADS_H 1

#include <filesystem>
#include <string>

#include <lightspark/tiny_string.h>

#ifdef __APPLE__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace lightspark;
using path_t = fs::path;

static inline path_t operator/(const path_t& path, const tiny_string& str)
{
	return path_t(path).append((std::string)str);
}

static inline path_t operator/(const tiny_string& str, const path_t& path)
{
	return path_t((std::string)str) / path;
}

static inline path_t& operator/=(path_t& path, const tiny_string& str)
{
	return path.append((std::string)str);
}

static inline bool operator!=(const path_t& path, const tiny_string& str)
{
	return path != path_t((std::string)str);
}

static inline bool operator!=(const tiny_string& str, const path_t& path)
{
	return path_t((std::string)str) != path;
}

#endif /* UTILS_FILESYSTEM_OVERLOADS_H */
