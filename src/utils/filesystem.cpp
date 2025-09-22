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

#include "utils/enum.h"
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

	if (status(workingDir).getType() == FileType::NotFound)
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
				ret = ret.getParent();
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
	bool handleSymlinks = bool(options &
	(
		CopyOptions::SkipSymlinks |
		CopyOptions::CopySymlinks |
		CopyOptions::CreateSymlinks
	));

	FileStatus statusFrom = handleSymlinks ? symlinkStatus(from) : status(from);

	if (!statusFrom.exists())
		throw Exception(from, to, std::errc::no_such_file_or_directory);

	handleSymlinks = bool(options &
	(
		CopyOptions::SkipSymlinks |
		CopyOptions::CreateSymlinks
	));
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
			if (bool(options & CopyOptions::SkipSymlinks))
				break;

			if (statusTo.exists() || !(options & CopyOptions::CopySymlinks))
				throw Exception(from, to, std::errc::invalid_argument);
			copySymlink(from, to);
			break;
		case FileType::Regular:
			if (bool(options & CopyOptions::DirectoriesOnly))
				break;
			if (bool(options & CopyOptions::CreateSymlinks))
				createSymlink(from.isAbsolute() ? from : canonical(from), to);
			else if (bool(options & CopyOptions::CreateHardLinks))
				createHardLink(from, to);
			else if (statusTo.isDir())
				copyFile(from, to / from.getFilename(), options);
			else
				copyFile(from, to, options);
			break;
		case FileType::Directory:
			#ifdef USE_LWG_2936
			if (bool(options & CopyOptions::CreateSymlinks))
				throw Exception(from, to, std::errc::is_a_directory);
			#endif
			if (options != CopyOptions::None && !(options & CopyOptions::Recursive))
				break;
			if (!statusTo.exists())
				createDir(to, from);
			for (auto dir : DirIter(from))
			{
				copy
				(
					dir.getPath(),
					to / dir.getPath().getFilename(),
					options | CopyOptions(0x8000)
				);
			}
			break;
		default:
			throw Exception(from, to, std::errc::invalid_argument);
			break;
	}
}

bool fs::copyFile(const Path& from, const Path& to)
{
	return copyFile(from, to, CopyOptions::None);
}

bool fs::copyFile(const Path& from, const Path& to, const CopyOptions& options)
{
	std::error_code fromCode;
	FileStatus statusFrom = status(from, fromCode);
	FileStatus statusTo = status(to);

	bool overwrite = false;

	if (!statusFrom.isFile() && fromCode.value())
		throw Exception(from, to, fromCode);
	else if (!statusFrom.isFile())
		return false;
	if (statusTo.exists())
	{
		if (bool(options & CopyOptions::SkipExisting))
			return false;
		if (!statusTo.isFile() || equivalent(from, to) || !(options & (CopyOptions::OverwriteExisting | CopyOptions::UpdateExisting)))
			throw Exception(from, to, std::errc::file_exists);
		auto fromTime = statusFrom.getLastWriteTime();
		auto toTime = statusTo.getLastWriteTime();
		if (bool(options & CopyOptions::UpdateExisting) && fromTime <= toTime)
			return false;
		overwrite = true;
	}
	return Detail::copyFile(from, to, statusFrom.getPerms(), statusTo.getPerms(), overwrite);
}

void fs::copySymlink(const Path& symlink, const Path& newSymlink)
{
	auto to = readSymlink(symlink);

	if (to.exists() && to.isDir())
		createDirSymlink(to, newSymlink);
	else
		createSymlink(to, newSymlink);
}

bool fs::createDirs(const Path& path, const Perms& perms)
{
	bool didCreate = false;
	auto rootLen =
	(
		path.prefixLength +
		path.rootNameLength() +
		path.hasRootDir()
	);

	Path current(path.getNativeStr().substr(0, rootLen));
	Path dirs(path.getNativeStr().substr(rootLen, Path::StringType::npos));

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
			createDir(current, perms);
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

bool fs::createDir(const Path& path, const Perms& perms)
{
	#ifdef USE_LWG_2936
	if (status(path).exists())
	#else
	auto fileStatus = status(path);
	if (fileStatus.exists() && fileStatus.isDir())
	#endif
		return false;
	return Detail::createDir(path, perms);
}

void fs::createDirSymlink(const Path& to, const Path& newSymlink)
{
	Detail::createSymlink(to, newSymlink, true);
}

void fs::createSymlink(const Path& to, const Path& newSymlink)
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
	return symlinkStatus(path).isSymlink();
}

TimeSpec fs::getLastWriteTime(const Path& path)
{
	std::error_code code;
	auto ret = status(path, code).getLastWriteTime();
	if (code.value())
		throw fs::Exception(path, code);
	return ret;
}

void fs::setPerms(const Path& path, const fs::Perms& perms, const fs::PermOptions& opts)
{
	using PermOpts = fs::PermOptions;
	if (!(opts & (PermOpts::Replace | PermOpts::Add | PermOpts::Remove)))
		throw fs::Exception(path, std::errc::invalid_argument);

	auto fileStatus = fs::symlinkStatus(path);
	auto _perms = fileStatus.getPerms();
	bool addPerms = bool(opts & PermOpts::Add);
	bool removePerms = bool(opts & PermOpts::Remove);

	if (addPerms && !removePerms)
		_perms |= perms;
	else if (removePerms && !addPerms)
		_perms &= ~perms;

	fs::Detail::setPerms(path, _perms, opts, fileStatus);
}

Path fs::proximate(const Path& path, const Path& base)
{
	return weaklyCanonical(path).lexicallyProximate(weaklyCanonical(base));
}

Path fs::readSymlink(const Path& path)
{
	if (!symlinkStatus(path).isSymlink())
		throw Exception(path, std::errc::invalid_argument);
	return Detail::resolveSymlink(path);
}

Path fs::relative(const Path& path, const Path& base)
{
	return weaklyCanonical(path).lexicallyRelative(weaklyCanonical(base));
}

size_t fs::removeAll(const Path& path)
{
	if (path == "/")
		throw Exception(path, std::errc::not_supported);

	auto fileStatus = symlinkStatus(path);

	if (!fileStatus.exists())
		return 0;
	if (!fileStatus.isDir())
		return remove(path);

	size_t count = 0;
	std::error_code code;
	for (auto it = DirIter(path, code); it != DirIter(); it.inc(code))
	{
		if (code.value() != 0 && !Detail::isNotFoundError(code))
			throw Exception(path, code);

		bool recurse = !it->isSymlink() && it->isDir();
		count += recurse ? removeAll(it->getPath()) : remove(it->getPath());
	}

	return count + remove(path);
}

fs::FileStatus fs::status(const Path& path)
{
	std::error_code code;
	auto _status = status(path, code);
	if (!_status.statusKnown())
		throw Exception(path, code);
	return _status;
}

fs::FileStatus fs::status(const Path& path, std::error_code& code)
{
	return Detail::status(path, code);
}

Path fs::weaklyCanonical(const Path& path)
{
	Path ret;
	bool scan = true;

	for (auto elem : path)
	{
		if (!scan || exists(ret / elem))
		{
			ret /= elem;
			continue;
		}

		scan = false;
		ret = (!ret.empty() ? canonical(ret) : ret) / elem;
	}

	if (scan && !ret.empty())
		ret = canonical(ret);

	return ret.lexicallyNormal();
}
