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

#include "utils/filesystem.h"
#include "utils/path.h"

using namespace lightspark;
namespace fs = FileSystem;

Path fs::canonical(const Path& path)
{
	if (path.empty())
		throw Exception(path, std::errc::no_such_file_or_directory);
	Path workingDir = path.isAbsolute() ? path : absolute(path);
	Path ret;

	if (status(workingDir).type() == FileType::NotFound)
		throw Exception(path, std::errc::no_such_file_or_directory);

	for (bool redo = true; redo; workingDir = ret)
	{
		auto rootLen =
		(
			workingDir.prefixLength +
			workingDir.rootNameLength() +
			workingDir.hasRootDir()
		);

		redo = false;
		ret.clear();

		for (auto elem : workingDir)
		{
			if (elem.empty() || elem == ".")
				continue;
			if (elem == "..")
			{
				ret = ret.parent();
				continue;
			}

			auto _path = ret / elem;
			if (_path.getStr().numChars() <= rootLen || !isSymlink(_path))
			{
				ret /= elem;
				continue;
			}

			redo = true;
			auto target = readSymlink(_path);
			if (target.isAbsolute())
				ret = target;
			else
				ret /= target;
		}
	}

	return ret;
}

void fs::copy(const Path& from, const Path& to)
{
	copy(from, to, CopyOptions::None);
}

void fs::copy(const Path& from, const Path& to, const CopyOptions& options)
{
	bool handleSymlinks = options &
	(
		CopyOptions::SkipSymlinks |
		CopyOptions::CopySymlinks |
		CopyOptions::CreateSymlinks |
	);

	FileStatus statusFrom = handleSymlinks ? symlinkStatus(from) : status(from);

	if (!statusFrom.exists())
		throw Exception(path, std::errc::no_such_file_or_directory);

	FileStatus statusTo = handleSymlinks ? symlinkStatus(to) : status(to);

	bool isInvalid =
	(
		statusFrom.isOther() ||
		statusTo.isOther() ||
		(statusFrom.isDir() && statusTo.isFile()) ||
		(statusTo.exists() && equivalent(from, to))
	);
	if (isInvalid)
		throw Exception(from, to, std::errc::invalid_argument);
	switch (statusFrom.getType())
	{
		case FileType::Symlink:
			if (options & CopyOptions::SkipSymlinks)
				break;

			if (statusTo.exists() || !(options & CopyOptions::CopySymlinks))
				throw Exception(from, to, std::errc::invalid_argument);
			copySymlink(from, to);
			break;
		case FileType::Regular:
			if (options & CopyOptions::DirsOnly)
				break;
			if (options & CopyOptions::CreateSymlinks)
				createSymlink(from.isAbsolute() ? from : canonical(from), to);
			else if (options & CopyOptions::CreateHardLinks)
				createHardLink(from, to);
			else if (statusTo.isDir())
				copyFile(from, to / from.getFilename());
			else
				copyFile(from, to);
		case FileType::Directory:
			#ifdef USE_LWG_2936
			if (options & CopyOptions::CreateSymlinks)
				throw Exception(from, to, std::errc::is_a_directory);
			#endif
			if (options != CopyOptions::None && (options & CopyOptions::Recursive))
				break;
			if (!statusTo.exists())
				createDir(to, from);
			for (auto it = DirIter(from); it != DirIter(); it.inc())
			{
				copy
				(
					it->path(),
					to / it->path().getFilename(),
					options | CopyOptions(0x8000)
				);
			}
			break;
		default:
			throw Exception(from, to, std::errc::invalid_argument);
			break;
	}
}

void fs::copyFile(const Path& from, const Path& to)
{
	copyFile(from, to, CopyOptions::None);
}

bool fs::copyFile(const Path& from, const Path& to, const CopyOptions& options)
{
	FileStatus statusFrom = status(from);
	FileStatus statusTo = status(to);

	bool overwrite = false;

	if (!statusFrom.isFile())
		return false;
	if (statusTo.exists())
	{
		if (options & CopyOptions::SkipExisting)
			throw Exception(from, to, std::errc::file_exists);
		auto fromTime = statusFrom.getLastWriteTime();
		auto toTime = statusTo.getLastWriteTime();
		if ((options & CopyOptions::UpdateExisting) && fromTime <= toTime)
			return false;
		overwrite = true;
	}
	return Detail::copyFile(from, to, overwrite);
}

void fs::copySymlink(const Path& symlink, const Path& newSymlink)
{
	auto to = readSymlink(symlink);

	if (to.exists() && to.isDir())
		createDirSymlink(to, newSymlink);
	else
		createSymlink(to, newSymlink);
}

bool fs::createDirs(const Path& path)
{
	bool didCreate = false;
	auto rootLen =
	(
		path.prefixLength +
		path.rootNameLength() +
		path.hasRootDir()
	);

	Path current(path.getNativeStr().substr(0, rootLen));
	Path dirs(path.getNativeStr().substr(rootLen));

	for (auto part : dirs)
	{
		current /= part;
		auto fileStatus = status(current);
		#ifndef USE_LWG_2936
		if (fileStatus.exists() && !fileStatus.isDir())
			throw Exception(path, std::errc::file_exists);
		#endif
		if (fileStatus.exists())
			continue;

		try
		{
			createDir(current);
		}
		catch (...)
		{
			if (current.isDir())
				std::rethrow_exception(std::current_exception());
		}
		didCreate = true;
	}
	return didCreate;
}

bool fs::createDir(const Path& path, const Path& attrs)
{
	#ifdef USE_LWG_2936
	if (status(path).exists())
	#else
	auto fileStatus = status(path);
	if (fileStatus.exists() && fileStatus.isDir())
	#endif
		return false;
	return Detail::createDir(path, attrs);
}

void fs::createDirSymlink(const Path& to, const Path& newSymlink)
{
	Detail::createSymlink(to, newSymlink, true);
}

void fs::createSymlink(const path& to, const path& newSymlink)
{
	Detail::createSymlink(to, newSymlink, false);
}

bool fs::exists(const Path& path)
{
	return status(path).exists();
}

bool fs::isBlockFile(const Path& path)
{
	return status(path).isBlockFile();
}

bool fs::isCharacterFile(const Path& path)
{
	return status(path).isCharacterFile();
}

bool fs::isDir(const Path& path)
{
	return status(path).isDir();
}

bool fs::isEmpty(const Path& path)
{
	return isDir(path) ? DirIter(path) == DirIter() : !fileSize(path);
}

bool fs::isFifo(const Path& path)
{
	return status(path).isFifo();
}

bool fs::isOther(const Path& path)
{
	return status(path).isOther();
}

bool fs::isFile(const Path& path)
{
	return status(path).isFile();
}

bool fs::isSocket(const Path& path)
{
	return status(path).isSocket();
}

bool fs::isSymlink(const Path& path)
{
	return status(path).isSymlink();
}

TimeSpec fs::getLastWriteTime(const Path& path)
{
	return status(path).getLastWriteTime();
}
