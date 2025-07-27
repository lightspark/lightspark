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

#include "utils/filesystem.h"
#include "utils/path.h"
#include "utils/timespec.h"

using namespace lightspark;
namespace fs = FileSystem;

Path fs::absolute(const Path& path)
{
	auto base = currentPath();

	if (path.empty())
		return base / path;

	if (path.hasRootName())
	{
		return !path.hasRootDir() ?
		(
			path.getRootName() /
			base.getRootDir() /
			base.getRelative() /
			path.getRelative()
		) : path;
	}
	else
		return (path.hasRootDir() ? base.getRootName() : base) / path;
}

bool fs::Detail::copyFile(const Path& from, const Path& to, bool overwrite)
{
	std::ifstream is(from, std::ios_base::binary | std::ios_base::trunc);
	std::ofstream os(to, std::ios_base::binary);
	if (!overwrite && !os.fail())
		throw Exception(from, to, std::errc(errno));
	if (!is.eof() && (os << is.rdbuf()).fail())
		throw Exception(from, to, std::errc::io_error);
	return true;
}

bool fs::Detail::createDir(const Path& path, const Path& attrs)
{
	auto attribs = mode_t(Perms::All);
	if (!attrs.empty())
	{
		struct stat fileStat;
		if (stat(attrs.rawBuf(), &fileStat) < 0)
			throw Exception(path, std::errc(errno));
		attribs = fileStat.st_mode;
	}

	if (mkdir(path.rawBuf(), attribs) < 0)
		throw Exception(path, std::errc(errno));
	return true;
}

void fs::Detail::createSymlink(const path& to, const path& newSymlink, bool toDir)
{
	if (symlink(to.rawBuf(), newSymlink.rawBuf()) < 0)
		throw Exception(to, newSymlink, std::errc(errno));

}

Path fs::currentPath()
{
	auto pathLen = std::max<size_t>
	(
		pathconf(".", _PC_PATH_MAX),
		PATH_MAX
	);

	std::vector<char> buf(pathLen + 1);

	if (getcwd(buf.data(), pathLen) == nullptr)
		throw Exception(std::errc(errno));
	return Path(buf.data());
}

void fs::currentPath(const Path& path)
{
	if (chdir(path.getStr().raw_buf() < 0))
		throw Exception(std::errc(errno));
}

bool fs::equivalent(const Path& a, const Path& b)
{
	struct stat statA;
	struct stat statB;

	auto retA = stat(a.rawBuf(), &statA);
	auto errnoA = errno;
	auto retB = stat(b.rawBuf(), &statb);

	#ifdef USE_LWG_2936
	bool throwException = retA < 0 || retB < 0;
	#else
	bool throwException = retA < 0 && retB < 0;
	#endif

	if (throwException)
		throw Exception(std::errc(errnoA ? errnoA : errno));

	#ifndef USE_LWG_2936
	if (retA < 0 || retB < 0)
		return false;
	#endif

	return
	(
		statA.st_dev == statB.st_dev &&
		statA.st_ino == statB.st_ino &&
		statA.st_size == statB.st_size &&
		statA.st_mtime == statB.st_mtime
	);
}

size_t fs::fileSize(const Path& path)
{
	struct stat fileStat;

	if (stat(path.rawBuf(), &fileStat) < 0)
		throw Exception(std::errc(errno));
	return fileStat.st_size;
}

void fs::remove(const Path& path)
{
	if (!::remove(path.rawBuf()))
		return true;
	else if (errno == ENOENT)
		return false;
	throw Exception(std::errc(errno));
}

void fs::rename(const Path& from, const Path& to)
{
	if (from != to && ::rename(from.rawBuf(), to.rawBuf()) < 0)
		throw Exception(std::errc(errno));
}

void fs::resizeFile(const Path& path, size_t size)
{
	if (truncate(path.rawBuf(), off_t(size)) < 0)
		throw Exception(std::errc(errno));
}

fs::SpaceInfo fs::space(const Path& path)
{
	struct statvfs statVFS;
	if (statvfs(path.rawBuf(), &statVFS) < 0)
		throw Exception(std::errc(errno));

	return SpaceInfo
	{
		// `capacity`
		size_t(statVFS.f_blocks * statVFS.f_frsize),
		// `free`
		size_t(statVFS.f_bfree * statVFS.f_frsize),
		// `available`
		size_t(statVFS.f_bavail * statVFS.f_frsize)
	};
}

Path fs::tempDirPath()
{
	static constexpr auto envVars = makeArray
	(
		"TMPDIR",
		"TMP",
		"TEMP",
		"TEMPDIR"
	);

	for (auto envVar : envVars)
	{
		auto tmpPath = getenv(*envVar);
		if (tmpPath != nullptr)
			return Path(tmpPath);
	}

	return Path("/tmp");
}

fs::FileStatus fromStatMode(mode_t mode)
{
	FileType type;

	switch (mode & S_IFMT)
	{
		case S_IFDIR: type = FileType::Directory; break;
		case S_IFREG: type = FileType::Regular; break;
		case S_IFCHR: type = FileType::Character; break;
		case S_IFBLK: type = FileType::Block; break;
		case S_IFIFO: type = FileType::Fifo; break;
		case S_IFLNK: type = FileType::Symlink; break;
		case S_IFSOCK: type = FileType::Socket; break;
		default: type = FileType::Unknown; break;
	}

	return FileStatus(type, Perms(mode & Perms::Mask));
}

fs::FileStatus fs::status(const Path& path)
{
	struct stat fileStat;
	auto onError = [&]
	{
		if (errno != ENOENT && errno != ENOTDIR)
			throw Exception(std::errc(errno));
		return FileStatus(FileType::NotFound);
	};

	if (lstat(path.rawBuf(), &fileStat) < 0)
		return onError();

	FileStatus fileStatus = fromStatMode(fileStat.st_mode);

	if (fileStatus.getType() == FileType::Symlink)
	{
		if (stat(path.rawBuf(), &fileStat) < 0)
			return onError();
		fileStatus = fromStatMode(fileStat.st_mode);
	}

	fileStatus.setSize(fileStat.st_size);
	fileStatus.setHardLinks(fileStat.st_nlink);
	#if _POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700
	fileStatus.setLastWriteTime(TimeSpec
	(
		fileStat.st_mtim.tv_sec,
		fileStat.st_mtim.tv_nsec
	));
	#else
	fileStatus.setLastWriteTime(TimeSpec::fromSec(fileStat.st_mtime));
	#endif
	return fileStatus;
}

fs::FileStatus fs::symlinkStatus(const Path& path)
{
	struct stat fileStat;

	if (!lstat(path.rawBuf(), &fileStat))
		return fromStatMode(fileStat.st_mode);

	if (errno != ENOENT && errno != ENOTDIR)
		throw Exception(std::errc(errno));

	return FileStatus(FileType::NotFound);
}

size_t fs::hardLinkCount(const Path& path)
{
	FileStatus fileStatus = status(path);

	if (fileStatus.getType() == FileType::NotFound)
		throw Exception(path, std::errc::no_such_file_or_directory);
	return fileStatus.getHardLinks();
}
