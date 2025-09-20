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

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>

#include "backends/windows/dir_iter.h"
#include "backends/windows/filesystem.h"
#include "utils/enum.h"
#include "utils/filesystem.h"
#include "utils/path.h"
#include "utils/timespec.h"
#include "utils/type_traits.h"

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
) : fs::DirIter::ImplBase(path, opts), handle(INVALID_HANDLE_VALUE)
{
	if (path.empty())
		return;

	auto platStr = (path / "*").getPlatformStr();

	ZeroMemory(&findData, sizeof(WIN32_FIND_DATAW));
	handle = FindFirstFileW(platStr.c_str(), &findData);
	if (handle == INVALID_HANDLE_VALUE)
	{
		auto error = GetLastError();
		basePath = Path();
		if (error != ERROR_ACCESS_DENIED || !(opts & DirOpts::SkipPermDenied))
			code = makeSysError(error);
		return;
	}

	std::wstring fileName(findData.cFileName);
	if (fileName == L"." || fileName == L"..")
	{
		inc(code);
		return;
	}

	dirEntry.path = path / fileName;
	copyToDirEntry(code);
}

DirIterImpl::~Impl()
{
	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
		handle = INVALID_HANDLE_VALUE;
	}
}

void DirIterImpl::inc(std::error_code& code)
{
	if (handle == INVALID_HANDLE_VALUE)
	{
		code = this->code;
		return;
	}

	std::wstring fileName;
	do
	{
		if (!FindNextFileW(handle, &findData))
		{
			auto error = GetLastError();
			if (error != ERROR_NO_MORE_FILES)
				this->code = code = makeSysError(error);
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
			dirEntry.path.clear();
			break;
		}

		dirEntry.path = basePath;
		dirEntry.path.appendName(findData.cFileName);
		copyToDirEntry(code);
		fileName = std::wstring(findData.cFileName);
	} while (fileName == L"." || fileName == L"..");
}

void DirIterImpl::copyToDirEntry(std::error_code& code)
{
	if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		dirEntry.status = Detail::status(dirEntry.path, code, &dirEntry.symlinkStatus);
		return;
	}

	dirEntry.status = StatusFromImpl::fromINFO(dirEntry.path, findData, code);
	dirEntry.symlinkStatus = dirEntry.status;

	if (!code.value())
		return;

	if (dirEntry.status.statusKnown() && dirEntry.symlinkStatus.statusKnown())
	{
		code.clear();
		return;
	}

	dirEntry.status.setSize(-1);
	dirEntry.symlinkStatus.setSize(-1);
	dirEntry.status.setLastWriteTime(TimeSpec());
	dirEntry.symlinkStatus.setLastWriteTime(TimeSpec());
}

#undef DirIterImpl
