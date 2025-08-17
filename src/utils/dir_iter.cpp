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

#include <system_error>

#include "backends/dir_iter.h"
#include "utils/enum.h"
#include "utils/filesystem.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

void fs::DirEntry::assign(const Path& _path)
{
	path.assign(_path);
	refresh();
}

void fs::DirEntry::replaceFilename(const Path& _path)
{
	path.replaceFilename(_path);
	refresh();
}

void fs::DirEntry::refresh()
{
	try
	{
		status = fs::Detail::status(path, &symlinkStatus);
	}
	catch (...)
	{
		if (!status.statusKnown() || !symlinkStatus.isSymlink())
			std::rethrow_exception(std::current_exception());
	}
}

fs::FileStatus fs::DirEntry::tryGetStatus() const
{
	return status.statusKnown() ? status : fs::status(Path());
}

using DirOpts = fs::DirOptions;

fs::DirIter::DirIter
(
	const Path& path,
	const DirOpts& opts
) : impl(_MR(new Impl(path, opts)))
{
	static_assert
	(
		std::is_base_of<ImplBase, Impl>::value,
		"DirIter::Impl isn't derived from DirIter::ImplBase."
	);

	if (impl->code.value())
		throw Exception(path, impl->code);
	impl->code.clear();
}

fs::DirIter::DirIter
(
	const Path& path,
	std::error_code& code
) : impl(_MR(new Impl(path, DirOpts::None)))
{
	if (impl->code.value())
		code = impl->code;
}

fs::DirIter::DirIter
(
	const Path& path,
	const DirOpts& opts,
	std::error_code& code
) : impl(_MR(new Impl(path, opts)))
{
	if (impl->code.value())
		code = impl->code;
}

fs::DirIter& fs::DirIter::operator++()
{
	std::error_code code;
	impl->asImpl()->inc(code);
	if (code.value())
		throw Exception((*this)->getPath(), code);
	return *this;
}

fs::DirIter& fs::DirIter::inc(std::error_code& code)
{
	impl->asImpl()->inc(code);
	return *this;
}

fs::RecursiveDirIter& fs::RecursiveDirIter::operator++()
{
	std::error_code code;
	inc(code);
	if (code.value())
	{
		auto path =
		(
			dirStack.empty() ?
			Path() :
			dirStack.top()->getPath()
		);
		throw Exception((*this)->getPath(), code);
	}
	return *this;
}

fs::RecursiveDirIter& fs::RecursiveDirIter::inc(std::error_code& code)
{
	bool isSymlink;
	bool isDir;

	try
	{
		isSymlink = (*this)->isSymlink();
		isDir = (*this)->isDir();
	}
	catch (Exception& e)
	{
		if (!isSymlink || !Detail::isNotFoundError(e.getCode()))
			code = e.getCode();
	}

	if (code.value())
		return *this;

	if (isPending() && isDir && (!isSymlink || bool(getOptions() & DirOpts::FollowSymlinks)))
		dirStack.push(DirIter((*this)->getPath(), dirStack.options, code));
	else
		dirStack.top().inc(code);

	if (!code.value())
	{
		while (depth() > 0 && dirStack.top() == DirIter())
		{
			dirStack.pop();
			dirStack.top().inc(code);
		}
	}
	else if (!dirStack.empty())
		dirStack.pop();
	dirStack.pending = true;

	return *this;
}

void fs::RecursiveDirIter::pop()
{
	if (!depth())
	{
		*this = RecursiveDirIter();
		return;
	}

	do
	{
		dirStack.pop();
		++dirStack.top();
	} while (depth() > 0 && dirStack.top() == DirIter());
}
