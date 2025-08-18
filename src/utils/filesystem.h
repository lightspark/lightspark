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

#include <stack>

#include "compat.h"
#include "exceptions.h"
#include "smartrefs.h"
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
	) : Exception(cause, Path(), Path(), code) {}

	Exception
	(
		const std::string& cause,
		const Path& path1,
		const std::error_code& code
	) : Exception(cause, path1, Path(), code) {}

	Exception
	(
		const std::string& _cause,
		const Path& _path1,
		const Path& _path2,
		const std::error_code& code
	) : SystemException(_cause, code), path1(_path1), path2(_path2)
	{
		if (path1.empty())
			return;
		cause += std::string(": '") + std::string(path1.getStr()) + '\'';

		if (path2.empty())
			return;
		cause += std::string(", '") + std::string(path2.getStr()) + '\'';
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
	const char* what() const throw()
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

class FileStatus;

namespace Detail
{

FileStatus status(const Path& path, FileStatus* _symlinkStatus);

};

class FileStatus
{
private:
	FileType type;
	Perms perms;

	size_t size { size_t(-1) };
	size_t hardLinks { size_t(-1) };
	TimeSpec lastWriteTime;

	friend FileStatus Detail::status(const Path& path, FileStatus* _symlinkStatus);
	friend class DirEntry;
	friend class DirIter;

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

	size_t getSize() const { return size; }
	size_t getHardLinks() const { return hardLinks; }
	const TimeSpec& getLastWriteTime() const { return lastWriteTime; }

	bool statusKnown() const { return type != FileType::None; }
	bool exists() const { return statusKnown() && type != FileType::NotFound; }
	bool isBlockFile() const { return type == FileType::Block; }
	bool isCharacterFile() const { return type == FileType::Character; }
	bool isDir() const { return type == FileType::Directory; }
	bool isFifo() const { return type == FileType::Fifo; }
	bool isOther() const { return exists() && !isFile() && !isDir(); }
	bool isFile() const { return type == FileType::Regular; }
	bool isSocket() const { return type == FileType::Socket; }
	bool isSymlink() const { return type == FileType::Symlink; }

	bool operator==(const FileStatus& other) const noexcept
	{
		return type == other.type && perms == other.perms;
	}
};

class DirEntry
{
public:
	DirEntry() = default;
	DirEntry(const DirEntry& other) = default;
	DirEntry(DirEntry&& other) = default;
	DirEntry(const Path& _path) : path(_path) { refresh(); }

	~DirEntry() {}

	DirEntry& operator=(const DirEntry& other) = default;
	DirEntry& operator=(DirEntry&& other) = default;

	void assign(const Path& _path);
	void replaceFilename(const Path& _path);
	void refresh();

	const Path& getPath() const { return path; }
	operator const Path&() const { return path; }

	bool exists() const { return tryGetStatus().exists(); }
	bool isBlockFile() const { return tryGetStatus().isBlockFile(); }
	bool isCharFile() const { return tryGetStatus().isCharacterFile(); }
	bool isDir() const { return tryGetStatus().isDir(); }
	bool isFifo() const { return tryGetStatus().isFifo(); }
	bool isOther() const { return tryGetStatus().isOther(); }
	bool isFile() const { return tryGetStatus().isFile(); }
	bool isSocket() const { return tryGetStatus().isSocket(); }
	bool isSymlink() const { return tryGetStatus().isSymlink(); }

	size_t getFileSize() const { return status.getSize(); }
	const TimeSpec& getLastWriteTime() const { return status.getLastWriteTime(); }
	const FileStatus& getStatus() const { return status; }
	const FileStatus& getSymlinkStatus() const { return symlinkStatus; }
	size_t getHardLinkCount() const { return status.getHardLinks(); }

	bool operator==(const DirEntry& other) const { return path == other.path; }
	bool operator!=(const DirEntry& other) const { return path != other.path; }
	bool operator<(const DirEntry& other) const { return path < other.path; }
	bool operator<=(const DirEntry& other) const { return path <= other.path; }
	bool operator>(const DirEntry& other) const { return path > other.path; }
	bool operator>=(const DirEntry& other) const { return path >= other.path; }
private:
	friend class DirIter;
	FileStatus tryGetStatus() const;

	Path path;
	FileStatus status;
	FileStatus symlinkStatus;
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

	class Impl;

	class ImplBase : public RefCountable
	{
	public:
		Path basePath;
		DirOptions options;
		DirEntry dirEntry;
		std::error_code code;

		ImplBase
		(
			const Path& path,
			const DirOptions& opts
		) : basePath(path), options(opts) {}
		ImplBase(const ImplBase& other) = delete;
		~ImplBase() {}

		const Impl* asImpl() const
		{
			return reinterpret_cast<const Impl*>(this);
		}

		Impl* asImpl()
		{
			return reinterpret_cast<Impl*>(this);
		}
	};

	_R<ImplBase> impl;
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = DirEntry;
	using difference_type = ptrdiff_t;
	using pointer = const DirEntry*;
	using reference = const DirEntry&;

	DirIter(const Path& path = Path(), const DirOptions& opts = DirOptions::None);
	DirIter(const Path& path, std::error_code& code);
	DirIter(const Path& path, const DirOptions& opts, std::error_code& code);

	DirIter(const DirIter& other) = default;
	DirIter(DirIter&& other) = default;

	DirIter& operator=(const DirIter& other) = default;
	DirIter& operator=(DirIter&& other) = default;

	const DirEntry& operator*() const { return impl->dirEntry; }
	const DirEntry* operator->() const { return &impl->dirEntry; }

	DirIter& operator++();
	DirIter& inc(std::error_code& code);
	Proxy operator++(int)
	{
		Proxy tmp(**this);
		++*this;
		return tmp;
	}

	bool operator==(const DirIter& other) const { return *(*this) == *other; }
	bool operator!=(const DirIter& other) const { return *(*this) != *other; }
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
			if (!top()->getPath().empty())
				return top()->getPath();
			// Recreate the path that failed from the dir stack.
			Path path = origPath;
			for (const auto& dir : c)
				path /= dir->getPath();
			return path;
		}
	};

	DirStack dirStack;
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = DirEntry;
	using difference_type = ptrdiff_t;
	using pointer = const DirEntry*;
	using reference = const DirEntry&;

	RecursiveDirIter
	(
		const Path& path = Path(),
		const DirOptions& opts = DirOptions::None
	) : dirStack(opts, DirIter(path, opts)) {}

	RecursiveDirIter
	(
		const Path& path,
		std::error_code& code
	) : dirStack(DirOptions::None, DirIter(path, code)) {}

	RecursiveDirIter
	(
		const Path& path,
		const DirOptions& opts,
		std::error_code& code
	) : dirStack(opts, DirIter(path, opts, code)) {}

	RecursiveDirIter(const RecursiveDirIter& other) = default;
	RecursiveDirIter(RecursiveDirIter&& other) = default;

	RecursiveDirIter& operator=(const RecursiveDirIter& other) = default;
	RecursiveDirIter& operator=(RecursiveDirIter&& other) = default;

	DirOptions getOptions() const { return dirStack.options; }
	ssize_t depth() const { return dirStack.size() - 1; }
	bool isPending() const { return dirStack.pending; }

	const DirEntry& operator*() const { return dirStack.top().impl->dirEntry; }
	const DirEntry* operator->() const { return &dirStack.top().impl->dirEntry; }

	void pop();
	void disablePending() { dirStack.pending = false; }

	RecursiveDirIter& operator++();
	RecursiveDirIter& inc(std::error_code& code);
	DirIter::Proxy operator++(int)
	{
		DirIter::Proxy tmp(**this);
		++*this;
		return tmp;
	}

	bool operator==(const RecursiveDirIter& other) const { return *(*this) == *other; }
	bool operator!=(const RecursiveDirIter& other) const { return *(*this) != *other; }
};

Path absolute(const Path& path);
Path canonical(const Path& path);
void copy(const Path& from, const Path& to);
void copy(const Path& from, const Path& to, const CopyOptions& options);
bool copyFile(const Path& from, const Path& to);
bool copyFile(const Path& from, const Path& to, const CopyOptions& options);
void copySymlink(const Path& symlink, const Path& newSymlink);
bool createDirs(const Path& path);
bool createDir(const Path& path, const Path& attrs = Path());
void createDirSymlink(const Path& to, const Path& newSymlink);
void createSymlink(const Path& to, const Path& newSymlink);
Path currentPath();
void currentPath(const Path& path);
bool exists(const Path& path);
bool equivalent(const Path& a, const Path& b);
size_t fileSize(const Path& path);
bool isBlockFile(const Path& path);
bool isCharacterFile(const Path& path);
bool isDir(const Path& path);
bool isEmpty(const Path& path);
bool isFifo(const Path& path);
bool isOther(const Path& path);
bool isFile(const Path& path);
bool isSocket(const Path& path);
bool isSymlink(const Path& path);
TimeSpec getLastWriteTime(const Path& path);
void setLastWriteTime(const Path& path, const TimeSpec& newTime);
void setPerms(const Path& path, const Perms& perms, const PermOptions& opts = PermOptions::Replace);
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

namespace Detail
{

// Platform specific functions.
bool copyFile(const Path& from, const Path& to, bool overwrite);
bool createDir(const Path& path, const Path& attrs);
void createSymlink(const Path& to, const Path& newSymlink, bool toDir);
void setPerms(const Path& path, const Perms& perms, const PermOptions& opts, const FileStatus& fileStatus);
Path resolveSymlink(const Path& path);
FileStatus status(const Path& path, FileStatus* _symlinkStatus = nullptr);

// Non platform specific functions.
bool isNotFoundError(const std::error_code& code);

};

inline DirIter begin(DirIter it) { return it; }
inline DirIter end(const DirIter&) { return DirIter(); }
inline RecursiveDirIter begin(RecursiveDirIter it) { return it; }
inline RecursiveDirIter end(const RecursiveDirIter&) { return RecursiveDirIter(); }

};

namespace std
{
#define DirIter lightspark::FileSystem::DirIter
#define RecursiveDirIter lightspark::FileSystem::RecursiveDirIter

inline DirIter begin(DirIter it) { return it; }
inline DirIter end(const DirIter&) { return DirIter(); }
inline RecursiveDirIter begin(RecursiveDirIter it) { return it; }
inline RecursiveDirIter end(const RecursiveDirIter&) { return RecursiveDirIter(); }

#undef DirIter
#undef RecursiveDirIter
};
#endif /* UTILS_FILESYSTEM_H */
