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

#ifndef UTILS_FILESYSTEM_H
#define UTILS_FILESYSTEM_H 1

#include "compat.h"
#include "tiny_string.h"
#include "utils/path.h"
#include "utils/timespec.h"

namespace lightspark
{

template<typename T>
class Optional;

};

// Based on `ghc::filesystem` from https://github.com/gulrak/filesystem
namespace lightspark::FileSystem
{

class Exception : public SystemException
{
private:
	Path path1;
	Path path2;
public:
	Exception
	(
		const std::string& cause,
		const std::error_code& code
	) : SystemException(cause, code) {}

	Exception
	(
		const std::string& _cause,
		const Path& _path1,
		const std::error_code& code
	) : Exception(_cause, code), path1(_path1)
	{
		if (path1.empty())
			return;
		cause += ": '" + path1.getStr() + '\'';
	}

	Exception
	(
		const std::string& _cause,
		const Path& _path1,
		const Path& _path2,
		const std::error_code& code
	) : Exception(_cause, _path1, code), path2(_path2)
	{
		if (path2.empty())
			return;
		cause += ", '" + path2.getStr() + '\'';
	}

	Exception(const std::error_code& code) : Exception(code.message(), code) {}

	Exception
	(
		const Path& path1,
		const std::error_code& code
	) : Exception(code.message(), path1, code) {}

	Exception
	(
		const Path& path1,
		const Path& path2,
		const std::error_code& code
	) : Exception(code.message(), path1, path2, code) {}

	Exception(const std::errc& code) : Exception(std::make_error_code(code)) {}

	Exception
	(
		const Path& path1,
		const std::errc& code
	) : Exception(path1, std::make_error_code(code)) {}


	Exception
	(
		const Path& path1,
		const Path& path2,
		const std::errc& code
	) : Exception(path1, path2, std::make_error_code(code)) {}

	const Path& getPath1() const { return path1; }
	const Path& getPath2() const { return path2; }
	const char* what() const override
	{
		return !cause.empty() ? cause.c_str() : "Lightspark filesystem error";
	}
};

struct SpaceInfo
{
	size_t capacity;
	size_t free;
	size_t available;
};

enum class FileType
{
	None,
	NotFound,
	Regular,
	Directory,
	Symlink,
	Block,
	Character,
	Fifo,
	Socket,
	Unknown,
};

enum class Perms : uint16_t
{
	None = 0,

	OwnerRead = 0400,
	OwnerWrite = 0200,
	OwnerExec = 0100,
	OwnerAll = 0700,

	GroupRead = 040,
	GroupWrite = 020,
	GroupExec = 010,
	GroupAll = 070,

	OthersRead = 04,
	OthersWrite = 02,
	OthersExec = 01,
	OthersAll = 07,

	All = 0777,
	SetUid = 04000,
	SetGid = 02000,
	StickyBit = 01000,

	Mask = 07777,
	Unknown = 0xffff,
};

enum class PermOptions : uint8_t
{
	Add = 1 << 0,
	Remove = 1 << 1,
	NoFollow = 1 << 2,
	Replace = Add | Remove,
};

enum class CopyOptions : uint16_t
{
	None = 0,

	SkipExisting = 1 << 0,
	OverwriteExisting = 1 << 1,
	UpdateExisting = 1 << 2,

	Recursive = 1 << 3,

	CopySymlinks = 1 << 4,
	SkipSymlinks = 1 << 5,

	DirectoriesOnly = 1 << 6,

	CreateSymlinks = 1 << 7,
	CreateHardLinks = 1 << 8,
};

enum class DirOptions : uint8_t
{
	None = 0,
	FollowSymlinks = 1 << 0,
	SkipPermDenied = 1 << 1,
};

class FileStatus
{
private:
	FileType type;
	Perms perms;

	size_t size { size_t(-1) };
	size_t hardLinks { size_t(-1) };
	TimeSpec lastWriteTime;

	friend FileStatus status(const Path& path);
	void setSize(size_t _size) { size = _size; }
	void setHardLinks(size_t _hardLinks) { hardLinks = _hardLinks; }
	void setLastWriteTime(const TimeSpec& _lastWriteTime)
	{
		lastWriteTime = _lastWriteTime;
	}
public:
	FileStatus
	(
		const FileType& _type = FileType::None,
		const Perms& _perms = Perms::Unknown
	) : type(_type), perms(_perms) {}
	FileStatus(const FileStatus& other) = default;
	FileStatus(FileStatus&& other) = default;

	~FileStatus() {}

	FileStatus& operator=(const FileStatus& other) = default;
	FileStatus& operator=(FileStatus&& other) = default;

	void setType(const FileType& _type) { type = _type; }
	void setPerms(const Perms& _perms) { perms = _perms; }

	const FileType& getType() const { return type; }
	const Perms& getPerms() const { return perms; }

	const size_t getSize() const { return size; }
	const size_t getHardLinks() const { return hardLinks; }
	const TimeSpec& getLastWriteTime() const { return lastWriteTime; }

	bool operator==(const FileStatus& other) const noexcept
	{
		return type == other.type && perms = other.perms;
	}
}

class DirEntry
{
public:
	DirEntry() = default;
	DirEntry(const DirEntry& other) = default;
	DirEntry(DirEntry&& other) = default;
	DirEntry(const Path& _path) : _path(path) { refresh(); }

	~DirEntry() {}

	DirEntry& operator=(const DirEntry& other) = default;
	DirEntry& operator=(DirEntry&& other) = default;

	void assign(const Path& _path);
	void replaceFilename(const Path& _path);
	void refresh();

	const Path& getPath() const { return path; }
	operator const Path&() const { return path; }

	bool exists() const;
	bool isBlockFile() const;
	bool isCharFile() const;
	bool isDir() const;
	bool isFifo() const;
	bool isOther() const;
	bool isFile() const;
	bool isSocket() const;
	bool isSymlink() const;

	size_t getFileSize() const { return fileSize; }
	const TimeSpec& getLastWriteTime() const { return lastWriteTime; }
	const FileStatus& getStatus() const { return status; }
	const FileStatus& getSymlinkStatus() const { return symlinkStatus; }
	size_t getHardLinkCount() const { return hardLinkCount; }

	bool operator==(const DirEntry& other) const { return path == other.path; }
	bool operator!=(const DirEntry& other) const { return path != other.path; }
	bool operator<(const DirEntry& other) const { return path < other.path; }
	bool operator<=(const DirEntry& other) const { return path <= other.path; }
	bool operator>(const DirEntry& other) const { return path > other.path; }
	bool operator>=(const DirEntry& other) const { return path >= other.path; }
private:
	friend class DirIter;
	Path path;
	FileStatus status;
	FileStatus symlinkStatus;
	size_t fileSize { size_t(-1) };
	size_t hardLinkCount { size_t(-1) };
	TimeSpec lastWriteTime;
};

class DirIterImpl
{
private:
	Path basePath;
	DirOptions options;
	DirEntry dirEntry;
public:
	DirIterImpl
	(
		const Path& path,
		const DirOptions& opts
	) : basePath(path), options(opts) {}
	DirIterImpl(const IterImpl& other) = default;
	virtual ~DirIterImpl() {}

	virtual void operator++() = 0;
	virtual void copyToDirEntry() = 0;
};

class DirIter
{
private:
	friend class RecursiveDirIter;

	class Proxy
	{
	private:
		friend class DirIter;
		friend class RecursiveDirIter;
		Proxy(const DirEntry& _entry) : entry(_entry) {}

		DirEntry entry;
	public:
		const DirEntry& operator*() const& { return entry; }
		DirEntry operator*() && { return std::move(entry); }
	};

	_R<DirIterImpl> impl;
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = DirEntry;
	using difference_type = ptrdiff_t;
	using pointer = const DirEntry*;
	using reference = const DirEntry&;

	DirIter();
	DirIter(const Path& path, const DirOptions& opts = DirOptions::None);

	DirIter(const DirIter& other);
	DirIter(DirIter&& other);

	DirIter& operator=(const DirIter& other) = default;
	DirIter& operator=(DirIter&& other) = default;

	const DirEntry& operator*() const { return impl->dirEntry; }
	const DirEntry* operator->() const { return &impl->dirEntry; }

	DirIter& operator++();
	Proxy operator++(int)
	{
		Proxy tmp(**this);
		++*this;
		return tmp;
	}

	bool operator==(const DirIter& other) const { impl->dirEntry == other.impl->dirEntry; }
	bool operator!=(const DirIter& other) const { impl->dirEntry != other.impl->dirEntry; }
};

class RecursiveDirIter
{
private:

	// Based on `_Dir_stack` from `libstdc++`.
	struct DirStack : public std::stack<DirIter>
	{
		tiny_string origPath;
		DirOptions options;
		bool pending;

		DirStack
		(
			const DirOptions& opts,
			const DirIter& dir
		) : options(opts), pending(true)
		{
			push(std::move(dir));
		}

		void clear() { c.clear(); }

		Path currentPath() const
		{
			if (!top()->path().empty())
				return top()->impl->dirEntry.path();
			// Recreate the path that failed from the dir stack.
			Path path = origPath;
			for (const auto& dir : c)
				path /= dir->path();
			return path;
		}
	}

	DirStack dirStack;
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = DirEntry;
	using difference_type = ptrdiff_t;
	using pointer = const DirEntry*;
	using reference = const DirEntry&;

	RecursiveDirIter();
	RecursiveDirIter(const Path& path, const DirOptions& opts = DirOptions::None);

	RecursiveDirIter(const RecursiveDirIter& other);
	RecursiveDirIter(RecursiveDirIter&& other);

	RecursiveDirIter& operator=(const RecursiveDirIter& other);
	RecursiveDirIter& operator=(RecursiveDirIter&& other);

	DirOptions getOptions() const;
	ssize_t depth() const;
	bool isPending() const;

	const DirEntry& operator*() const { return impl->dirEntry; }
	const DirEntry* operator->() const { return &impl->dirEntry; }

	void pop();
	void disablePending();

	RecursiveDirIter& operator++();
	DirIter::Proxy operator++(int)
	{
		DirIter::Proxy tmp(**this);
		++*this;
		return tmp;
	}

	bool operator==(const RecursiveDirIter& other) const { impl->dirEntry == other.impl->dirEntry; }
	bool operator!=(const RecursiveDirIter& other) const { impl->dirEntry != other.impl->dirEntry; }
};

Path absolute(const Path& path);
Path canonical(const Path& path);
void copy(const Path& from, const Path& to);
void copy(const Path& from, const Path& to, const CopyOptions& options);
bool copyFile(const Path& from, const Path& to);
bool copyFile(const Path& from, const Path& to, const CopyOptions& options);
void copySymlink(const Path& symlink, const Path& newSymlink);
bool createDirs(const Path& path);
bool createDir(const Path& path);
bool createDir(const Path& path, const Path& attrs);
void createDirSymlink(const Path& to, const Path& newSymlink);
void createSymlink(const path& to, const path& new_symlink);
Path currentPath();
void currentPath(const Path& path);
bool exists(const Path& path);
bool equivalent(const Path& a, const Path& b);
size_t fileSize(const Path& path);
bool isBlockFile(const Path& path);
bool isCharacterFile(const Path& path);
bool isDirectory(const Path& path);
bool isEmpty(const Path& path);
bool isFifo(const Path& path);
bool isOther(const Path& path);
bool isFile(const Path& path);
bool isSocket(const Path& path);
bool isSymlink(const Path& path);
TimeSpec getLastWriteTime(const Path& path);
void setLastWriteTime(const Path& path, const TimeSpec& newTime);
void getPerms(const Path& path, const Perms& perms, const PermOptions& opts = PermOptions::Replace);
Path proximate(const Path& path, const Path& base = currentPath());
Path readSymlink(const Path& path);
Path relative(const Path& path, const Path& base = currentPath());
bool remove(const Path& path);
size_t removeAll(const Path& path);
void rename(const Path& from, const Path& to);
void resizeFile(const Path& path, size_t size);
SpaceInfo space(const Path& path);
FileStatus status(const Path& path);
FileStatus symlinkStatus(const Path& path);
Path tempDirPath();
Path weaklyCanonical(const Path& path);
void createHardLink(const Path& to, const Path& newHardLink);
size_t hardLinkCount(const Path& path);

};

namespace std
{
#define DirIter lightspark::FileSystem::DirIter
#define RecursiveDirIter lightspark::FileSystem::RecursiveDirIter

DirIter begin(DirIter it) { return it; }
DirIter end(const DirIter&) { return DirIter(); }
RecursiveDirIter begin(RecursiveDirIter it) { return it; }
RecursiveDirIter end(const RecursiveDirIter&) { return RecursiveDirIter(); }

#undef DirIter
#undef RecursiveDirIter
};
#endif /* UTILS_FILESYSTEM_H */
