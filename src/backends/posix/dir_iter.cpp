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

#include <fstream>
#include <unistd.h>

#include "backends/posix/dir_iter.h"
#include "utils/enum.h"
#include "utils/filesystem.h"
#include "utils/path.h"
#include "utils/timespec.h"

using namespace lightspark;
namespace fs = FileSystem;

using DirOpts = fs::DirOptions;
// NOTE: This can't be done with `using` because `DirIter::Impl` is
// private.
#define DirIterImpl fs::DirIter::Impl

DirIterImpl::Impl
(
	const Path& path,
	const DirOpts& opts
) : fs::DirIter::ImplBase(path, opts), dir(nullptr), entry(nullptr)
{
	if (path.empty())
		return;

	do
	{
		dir = opendir(path.rawBuf());
	} while (errno == EINTR && dir == nullptr);

	if (dir != nullptr)
	{
		inc(code);
		return;
	}

	auto error = errno;
	basePath = Path();

	if ((error != EACCES && error != EPERM) || !(opts & DirOpts::SkipPermDenied))
		code = std::make_error_code(std::errc(errno));
}

DirIterImpl::~Impl()
{
	if (dir != nullptr)
		closedir(dir);
}

void DirIterImpl::inc(std::error_code& code)
{
	if (dir == nullptr)
		return;

	bool skip;
	do
	{
		skip = false;
		errno = 0;
		do
		{
			entry = readdir(dir);
		} while (errno == EINTR && entry == nullptr);
		if (entry == nullptr)
		{
			closedir(dir);
			dir = nullptr;
			dirEntry.path.clear();
			if (errno && errno != EINTR)
				code = std::make_error_code(std::errc(errno));
			break;
		}
		dirEntry.path = basePath;
		dirEntry.path.appendName(entry->d_name);
		copyToDirEntry();
		auto error = code.value();
		const auto& opts = options;
		if (error && (error == EACCES || error == EPERM) && bool(opts & DirOpts::SkipPermDenied))
		{
			code.clear();
			skip = true;
		}
	} while (skip || !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."));
}

void DirIterImpl::copyToDirEntry()
{
	dirEntry.symlinkStatus.setPerms(Perms::Unknown);
	dirEntry.symlinkStatus.setType(fileTypeFromDirEnt(*entry));
	if (dirEntry.symlinkStatus.isSymlink())
		dirEntry.status = dirEntry.symlinkStatus;
	else
	{
		dirEntry.symlinkStatus.setPerms(Perms::Unknown);
		dirEntry.symlinkStatus.setType(FileType::None);
	}

	dirEntry.status.setSize(-1);
	dirEntry.status.setHardLinks(-1);
	dirEntry.status.setLastWriteTime(TimeSpec());
}

#undef DirIterImpl
