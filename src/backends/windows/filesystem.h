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

#ifndef UTILS_BACKENDS_WINDOWS_FILESYSTEM_H
#define UTILS_BACKENDS_WINDOWS_FILESYSTEM_H 1

#include <system_error>

#include <intsafe.h>

std::error_code makeSysError(DWORD error = 0);

namespace lightspark
{

class Path;

namespace FileSystem
{

class FileStatus;

struct StatusFromImpl
{
	template<typename T>
	static FileStatus fromINFO(const Path& path, const T& info, std::error_code& code);
	template<typename T>
	static FileStatus fromINFO(const Path& path, const T& info);
};

};

};
#endif /* UTILS_BACKENDS_WINDOWS_FILESYSTEM_H */
