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

#ifndef BACKENDS_POSIX_DIR_ITER_H
#define BACKENDS_POSIX_DIR_ITER_H 1

#include "compat.h"
#include "tiny_string.h"
#include "utils/filesystem.h"
#include "utils/path.h"
#include "utils/timespec.h"

// Based on `ghc::filesystem` from https://github.com/gulrak/filesystem
namespace lightspark::FileSystem
{

class DirIter::Impl : public DirIter::ImplBase
{
private:
	DIR* dir;
	dirent* entry;
public:
	Impl(const Path& path, const DirOptions& opts);
	Impl(const Impl& other) = delete;
	~Impl();

	void inc(std::error_code& code);
	void copyToDirEntry();
};

};
#endif /* BACKENDS_POSIX_DIR_ITER_H */
