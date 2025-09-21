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
#include <libloaderapi.h>
#include <ntdef.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#include <memory>
#include <vector>

#include "backends/windows/filesystem.h"
#include "utils/array.h"
#include "utils/enum.h"
#include "utils/filesystem.h"
#include "utils/path.h"
#include "utils/timespec.h"

using namespace lightspark;
namespace fs = FileSystem;

std::error_code makeSysError(DWORD error)
{
	return std::error_code(error ? error : GetLastError(), std::system_category());
}

Path getFullPathName(const Path& path)
{
	auto platStr = path.getPlatformStr();
	auto size = GetFullPathNameW(platStr.c_str(), 0, nullptr, nullptr);
	if (!size)
		throw fs::Exception(path, makeSysError());

	std::vector<wchar_t> buf(size, L'\0');
	auto size2 = GetFullPathNameW(platStr.c_str(), size, buf.data(), nullptr);
	if (!size2 || size2 >= size)
		throw fs::Exception(path, makeSysError());

	return Path(std::wstring(buf.data(), size2));
}

Path fs::absolute(const Path& path)
{
	if (path.empty())
		return absolute(currentPath()) / "";

	auto ret = getFullPathName(path);

	if (path.getFilename() == ".")
		ret /= ".";
	return ret;
}

bool fs::Detail::copyFile
(
	const Path& from,
	const Path& to,
	const Perms& fromPerms,
	const Perms& toPerms,
	bool overwrite
)
{
	auto platFrom = from.getPlatformStr();
	auto platTo = to.getPlatformStr();
	if (!CopyFileW(platFrom.c_str(), platTo.c_str(), !overwrite))
		throw Exception(from, to, makeSysError());
	return true;
}

bool fs::Detail::createDir(const Path& path, const Path& attrs)
{
	auto platStr = path.getPlatformStr();
	auto platAttrs = attrs.getPlatformStr();
	if (!attrs.empty() && !CreateDirectoryExW(platAttrs.c_str(), platStr.c_str(), nullptr))
		throw Exception(path, makeSysError());
	else if (attrs.empty() && !CreateDirectoryW(platStr.c_str(), nullptr))
		throw Exception(path, makeSysError());
	return true;
}

bool fs::Detail::createDir(const Path& path, const Perms& perms)
{
	auto platStr = path.getPlatformStr();
	if (!CreateDirectoryW(platStr.c_str(), nullptr))
		throw Exception(path, makeSysError());

	fs::setPerms(path, perms);
	return true;
}

void fs::Detail::createSymlink(const Path& to, const Path& newSymlink, bool toDir)
{
	using CreateSymbolicLinkWFunc = BOOLEAN(WINAPI*)
	(
		LPCWSTR lpSymlinkFileName,
		LPCWSTR lpTargetFileName,
		DWORD dwFlags
	);
	static CreateSymbolicLinkWFunc CreateSymbolicLinkW = (CreateSymbolicLinkWFunc)GetProcAddress
	(
		GetModuleHandleW(L"kernel32.dll"),
		"CreateSymbolicLinkW"
	);
	if (CreateSymbolicLinkW == nullptr)
		throw Exception(to, newSymlink, std::errc::not_supported);

	auto platTo = to.getPlatformStr();
	auto platSym = newSymlink.getPlatformStr();
	std::error_code code;
	auto fileStatus = status(to, code);
	if ((fileStatus.isDir() && !toDir) || (fileStatus.isFile() && toDir))
		throw Exception(to, newSymlink, std::errc::not_supported);

	auto flags = toDir ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
	if (CreateSymbolicLinkW(platSym.c_str(), platTo.c_str(), flags))
		return;

	auto error = GetLastError();
	flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
	if (error != ERROR_PRIVILEGE_NOT_HELD || !CreateSymbolicLinkW(platSym.c_str(), platTo.c_str(), flags))
		throw Exception(to, newSymlink, makeSysError(error));
}

class UniqueHandle
{
private:
	HANDLE handle;
public:
	constexpr UniqueHandle() : handle(INVALID_HANDLE_VALUE) {}
	constexpr UniqueHandle(const HANDLE& _handle) : handle(_handle) {}
	constexpr UniqueHandle(UniqueHandle&& other) : handle(other.release()) {}
	~UniqueHandle() { reset(); }
	constexpr UniqueHandle& operator=(UniqueHandle&& other)
	{
		reset(other.release());
		return *this;
	}

	constexpr const HANDLE& getHandle() { return handle; }
	constexpr HANDLE release()
	{
		HANDLE tmp = handle;
		handle = INVALID_HANDLE_VALUE;
		return tmp;
	}

	constexpr void reset(const HANDLE& _handle = INVALID_HANDLE_VALUE)
	{
		HANDLE tmp = handle;
		handle = _handle;
		if (tmp != INVALID_HANDLE_VALUE)
			CloseHandle(tmp);
	}

	void swap(UniqueHandle& other) { std::swap(handle, other.handle); }
	constexpr bool isNull() const { return handle == INVALID_HANDLE_VALUE; }
};

std::unique_ptr<REPARSE_DATA_BUFFER> getReparseData
(
	const Path& path,
	std::error_code& code
)
{
	auto platStr = path.getPlatformStr();
	UniqueHandle file(CreateFileW
	(
		platStr.c_str(),
		0,
		(
			FILE_SHARE_READ |
			FILE_SHARE_WRITE |
			FILE_SHARE_DELETE
		),
		nullptr,
		OPEN_EXISTING,
		(
			FILE_FLAG_OPEN_REPARSE_POINT |
			FILE_FLAG_BACKUP_SEMANTICS
		),
		nullptr
	));

	if (file.isNull())
	{
		code = makeSysError();
		return nullptr;
	}

	auto reparseData = std::unique_ptr<REPARSE_DATA_BUFFER>
	(
		reinterpret_cast<REPARSE_DATA_BUFFER*>
		(
			new uint8_t[MAXIMUM_REPARSE_DATA_BUFFER_SIZE]()
		)
	);
	ULONG used;
	auto ret = DeviceIoControl
	(
		file.getHandle(),
		FSCTL_GET_REPARSE_POINT,
		nullptr,
		0,
		reparseData.get(),
		MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
		&used,
		nullptr
	);

	if (!ret)
	{
		code = makeSysError();
		return nullptr;
	}
	return reparseData;
}

std::unique_ptr<REPARSE_DATA_BUFFER> getReparseData(const Path& path)
{
	std::error_code code;
	auto ret = getReparseData(path, code);
	if (code.value())
		throw fs::Exception(path, code);	
	return ret;
}

Path fs::Detail::resolveSymlink(const Path& path)
{
	auto reparseData = getReparseData(path);
	if (!IsReparseTagMicrosoft(reparseData->ReparseTag))
		return Path();

	Path ret;
	switch (reparseData->ReparseTag)
	{
		case IO_REPARSE_TAG_SYMLINK:
		{
			auto& symlinkBuf = reparseData->SymbolicLinkReparseBuffer;
			auto printName = Path::fromPlatformStr(&symlinkBuf.PathBuffer
			[
				symlinkBuf.PrintNameOffset / sizeof(WCHAR)
			], symlinkBuf.PrintNameLength / sizeof(WCHAR));
			auto subName = Path::fromPlatformStr(&symlinkBuf.PathBuffer
			[
				symlinkBuf.SubstituteNameOffset / sizeof(WCHAR)
			], symlinkBuf.SubstituteNameLength / sizeof(WCHAR));

			ret =
			(
				subName.endsWith(printName) &&
				printName.startsWith("\\??\\")
			) ? printName : subName;

			if (symlinkBuf.Flags & SYMLINK_FLAG_RELATIVE)
				ret = path.getParent() / ret;
			break;
		}
		case IO_REPARSE_TAG_MOUNT_POINT:
		{
			ret = getFullPathName(path);
			break;
		}
	}
	return ret;
}

Path fs::currentPath()
{
	// NOTE: `GetCurrentDirectoryW` returns the required size of the
	// buffer, **including the null terminator**, if the supplied buffer
	// isn't large enough.
	auto pathSize = GetCurrentDirectoryW(0, nullptr);
	std::vector<wchar_t> buf(pathSize);
	if (!GetCurrentDirectoryW(pathSize, buf.data()))
		throw Exception(makeSysError());
	return Path(std::wstring(buf.data(), pathSize - 1), Path::Format::Native);
}

void fs::currentPath(const Path& path)
{
	auto platStr = path.getPlatformStr();
	if (!SetCurrentDirectoryW(platStr.c_str()))
		throw Exception(path, makeSysError());
}

bool fs::equivalent(const Path& a, const Path& b)
{
	auto platA = a.getPlatformStr();
	auto platB = a.getPlatformStr();
	auto shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	UniqueHandle fileA(CreateFileW
	(
		platA.c_str(),
		0,
		shareFlags,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	));
	UniqueHandle fileB(CreateFileW
	(
		platB.c_str(),
		0,
		shareFlags,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	));

	auto errorA = GetLastError();
	#ifdef USE_LWG_2937
	if (fileA.isNull() || fileB.isNull())
	#else
	if (fileA.isNull() && fileB.isNull())
	#endif
		throw Exception(a, b, makeSysError(errorA ? errorA : 0));

	BY_HANDLE_FILE_INFORMATION infoA;
	if (!GetFileInformationByHandle(fileA.getHandle(), &infoA))
		throw Exception(a, b, makeSysError());

	BY_HANDLE_FILE_INFORMATION infoB;
	if (!GetFileInformationByHandle(fileB.getHandle(), &infoB))
		throw Exception(a, b, makeSysError());

	return
	(
		infoA.ftLastWriteTime.dwLowDateTime == infoB.ftLastWriteTime.dwLowDateTime &&
		infoA.ftLastWriteTime.dwHighDateTime == infoB.ftLastWriteTime.dwHighDateTime &&
		infoA.nFileIndexHigh == infoB.nFileIndexHigh &&
		infoA.nFileIndexLow == infoB.nFileIndexLow &&
		infoA.nFileSizeHigh == infoB.nFileSizeHigh &&
		infoA.nFileSizeLow == infoB.nFileSizeLow &&
		infoA.dwVolumeSerialNumber == infoB.dwVolumeSerialNumber
	);
}

size_t fs::fileSize(const Path& path)
{
	auto platStr = path.getPlatformStr();
	WIN32_FILE_ATTRIBUTE_DATA attr;

	if (!GetFileAttributesExW(platStr.c_str(), GetFileExInfoStandard, &attr))
		throw Exception(path, makeSysError());
	
	return size_t
	(
		size_t(attr.nFileSizeHigh) << (sizeof(attr.nFileSizeHigh) * 8) |
		attr.nFileSizeLow
	);
}

// The number of seconds between 01-01-1601, and 01-01-1970.
constexpr uint64_t unixEpochOffset = 11644473600;
constexpr uint64_t ivalPerSec = TimeSpec::nsPerSec / 100;
constexpr uint64_t ivalToUnixEpoch = unixEpochOffset * ivalPerSec;

TimeSpec fromFILETIME(const FILETIME& fileTime)
{
	ULARGE_INTEGER bigInt;
	bigInt.LowPart = fileTime.dwLowDateTime;
	bigInt.HighPart = fileTime.dwHighDateTime;

	return TimeSpec::fromNs((bigInt.QuadPart - ivalToUnixEpoch) * 100);
}

FILETIME toFILETIME(const TimeSpec& time)
{
	ULARGE_INTEGER bigInt;
	bigInt.QuadPart = ULONGLONG((time.toNs() / 100) + ivalToUnixEpoch);

	FILETIME fileTime;
	fileTime.dwLowDateTime = bigInt.LowPart;
	fileTime.dwHighDateTime = bigInt.HighPart;
	return fileTime;
}

void fs::setLastWriteTime(const Path& path, const TimeSpec& newTime)
{
	auto platStr = path.getPlatformStr();
	auto shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	UniqueHandle file(CreateFileW
	(
		platStr.c_str(),
		0,
		shareFlags,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	));

	FILETIME fileTime = toFILETIME(newTime);
	if (!SetFileTime(file.getHandle(), nullptr, nullptr, &fileTime))
		throw Exception(path, makeSysError());
}

void fs::Detail::setPerms
(
	const Path& path,
	const Perms& perms,
	const PermOptions& opts,
	const FileStatus& fileStatus
)
{
	auto platStr = path.getPlatformStr();
	auto oldAttr = GetFileAttributesW(platStr.c_str());
	if (oldAttr == INVALID_FILE_ATTRIBUTES)
		throw Exception(path, makeSysError());

	auto newAttr = 
	(
		bool(perms & Perms::OwnerWrite) ?
		oldAttr & ~DWORD(FILE_ATTRIBUTE_READONLY) :
		oldAttr | FILE_ATTRIBUTE_READONLY
	);

	if (oldAttr != newAttr || !SetFileAttributesW(platStr.c_str(), newAttr))
		throw Exception(path, makeSysError());
}

bool fs::remove(const Path& path)
{
	auto platStr = path.getPlatformStr();
	auto attr = GetFileAttributesW(platStr.c_str());

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		auto error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
			return false;
		throw Exception(path, makeSysError(error));
	}

	bool isReadOnly = attr & FILE_ATTRIBUTE_READONLY;
	bool isDir = attr & FILE_ATTRIBUTE_DIRECTORY;
	auto newAttr = attr & ~DWORD(FILE_ATTRIBUTE_READONLY);
	if (isReadOnly && !SetFileAttributesW(platStr.c_str(), newAttr))
		throw Exception(path, makeSysError());
	else if (isDir && !RemoveDirectoryW(platStr.c_str()))
		throw Exception(path, makeSysError());
	else if (!isDir && !DeleteFileW(platStr.c_str()))
		throw Exception(path, makeSysError());

	return true;
}

void fs::rename(const Path& from, const Path& to)
{
	if (from == to)
		return;

	auto platFrom = from.getPlatformStr();
	auto platTo = to.getPlatformStr();
	if (!MoveFileExW(platFrom.c_str(), platTo.c_str(), MOVEFILE_REPLACE_EXISTING))
		throw Exception(from, to, makeSysError());
}

void fs::resizeFile(const Path& path, size_t size)
{
	auto platStr = path.getPlatformStr();

	LARGE_INTEGER bigInt;
	bigInt.QuadPart = LONGLONG(size);

	if (bigInt.QuadPart < 0)
		throw Exception(path, std::errc::file_too_large);

	UniqueHandle file(CreateFileW
	(
		platStr.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	));

	if (file.isNull())
		throw Exception(path, makeSysError());
	if (!SetFilePointerEx(file.getHandle(), bigInt, nullptr, FILE_BEGIN))
		throw Exception(path, makeSysError());
}

fs::SpaceInfo fs::space(const Path& path)
{
	auto platStr = path.getPlatformStr();

	ULARGE_INTEGER availableBytes = { { 0, 0 } };
	ULARGE_INTEGER totalBytes = { { 0, 0 } };
	ULARGE_INTEGER totalBytesFree = { { 0, 0 } };
	auto ret = GetDiskFreeSpaceExW
	(
		platStr.c_str(),
		&availableBytes,
		&totalBytes,
		&totalBytesFree
	);
	if (!ret)
		throw Exception(path, makeSysError());

	return SpaceInfo
	{
		// `capacity`
		size_t(totalBytes.QuadPart),
		// `free`
		size_t(totalBytesFree.QuadPart),
		// `available`
		size_t(availableBytes.QuadPart)
	};
}

Path fs::tempDirPath()
{
	std::vector<wchar_t> buf(512);
	auto ret = GetTempPathW(buf.size() - 1, buf.data());
	if (!ret || ret >= buf.size())
		throw Exception(makeSysError());
	return Path(std::wstring(buf.data()));
}

bool isNotFound(DWORD error)
{
	return
	(
		error == ERROR_FILE_NOT_FOUND ||
		error == ERROR_PATH_NOT_FOUND ||
		error == ERROR_INVALID_NAME
	);
}

template<typename T>
bool isSymlinkFromINFO(const Path& path, const T& info, std::error_code& code)
{
	if (!(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
		return false;

	auto reparseData = getReparseData(path, code);
	return
	(
		!code.value() &&
		reparseData != nullptr &&
		IsReparseTagMicrosoft(reparseData->ReparseTag) &&
		reparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK
	);
}

template<>
bool isSymlinkFromINFO
(
	const Path& path,
	const WIN32_FIND_DATAW& info,
	std::error_code& code
)
{
	// NOTE: `dwReserved0` is undefined, unless `dwFileAttributes` has
	// `FILE_ATTRIBUTE_RELEASE_POINT` set, according to Microsoft's
	// documentation. But in practice, `dwReserved0` isn't cleared, which
	// causes it to report the symlink status incorrectly.
	// Note that Microsoft's documentation doesn't say whether there's a
	// null value `dwReserved0`, so, check for the symlink directly,
	// rather than returning the tag, which requires returning a null
	// value for non reparse point files.
	return
	(
		(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
		info.dwReserved0 == IO_REPARSE_TAG_SYMLINK
	);
}

bool hasExecutableExtension(const Path& path)
{
	auto ext = path.getExtension().getStr().trimStartMatches('.');
	if (ext.empty() || ext.numChars() != 3)
		return false;

	return
	(
		ext.caselessEquals("exe") ||
		ext.caselessEquals("cmd") ||
		ext.caselessEquals("bat") ||
		ext.caselessEquals("com")
	);
}

template<typename T>
fs::FileStatus fs::StatusFromImpl::fromINFO
(
	const Path& path,
	const T& info,
	std::error_code& code
)
{
	FileType type = FileType::Unknown;

	if (isSymlinkFromINFO(path, info, code))
		type = FileType::Symlink;
	else if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		type = FileType::Directory;
	else
		type = FileType::Regular;

	Perms perms = Perms::OwnerRead | Perms::GroupRead | Perms::OthersRead;
	if (!(info.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		perms |= Perms::OwnerWrite | Perms::GroupWrite | Perms::OthersWrite;
	if (hasExecutableExtension(path))
		perms |= Perms::OwnerExec | Perms::GroupExec | Perms::OthersExec;
	FileStatus ret(type, perms);
	ret.setSize(size_t
	(
		size_t(info.nFileSizeHigh) << (sizeof(info.nFileSizeHigh) * 8) |
		info.nFileSizeLow
	));
	ret.setLastWriteTime(fromFILETIME(info.ftLastWriteTime));
	ret.setHardLinks(size_t(-1));
	return ret;
}

template<typename T>
fs::FileStatus fs::StatusFromImpl::fromINFO(const Path& path, const T& info)
{
	std::error_code code;
	auto ret = fromINFO(path, info, code);

	if (code.value())
		throw Exception(path, code);
	return ret;
}

template fs::FileStatus fs::StatusFromImpl::fromINFO<WIN32_FIND_DATAW>
(
	const Path& path,
	const WIN32_FIND_DATAW& info,
	std::error_code& code
);

template fs::FileStatus fs::StatusFromImpl::fromINFO<WIN32_FIND_DATAW>
(
	const Path& path,
	const WIN32_FIND_DATAW& info
);

fs::FileStatus fs::Detail::status
(
	const Path& path,
	std::error_code& code,
	FileStatus* _symlinkStatus,
	size_t depth
)
{
	auto onError = [&]
	{
		code = makeSysError();
		if (!isNotFound(code.value()))
			return FileStatus(FileType::None);
		return FileStatus(FileType::NotFound);
	};

	auto platStr = path.getPlatformStr();

	if (depth > 16)
	{
		code = std::make_error_code
		(
			std::errc::too_many_symbolic_link_levels
		);
		return FileStatus(FileType::Unknown);
	}

	WIN32_FILE_ATTRIBUTE_DATA attr;
	if (!GetFileAttributesExW(platStr.c_str(), GetFileExInfoStandard, &attr))
		return onError();

	if (!(attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
		return StatusFromImpl::fromINFO(path, attr, code);

	auto reparseData = getReparseData(path, code);

	if (code.value() || reparseData == nullptr)
		return onError();

	const auto& reparseTag = reparseData->ReparseTag;
	if (!IsReparseTagMicrosoft(reparseTag) || reparseTag != IO_REPARSE_TAG_SYMLINK)
		return onError();

	// TODO: Add error code version of `resolveSymlink()`.
	auto target = resolveSymlink(path);
	if (code.value() || target.empty())
		return FileStatus(FileType::Unknown);

	if (_symlinkStatus != nullptr)
		*_symlinkStatus = StatusFromImpl::fromINFO(path, attr, code);
	return status(target, code, nullptr, depth + 1);
}

fs::FileStatus fs::symlinkStatus(const Path& path)
{
	auto onError = [&]
	{
		auto error = GetLastError();
		if (!isNotFound(error))
			throw Exception(path, makeSysError());
		return FileStatus(FileType::NotFound);
	};

	auto platStr = path.getPlatformStr();

	WIN32_FILE_ATTRIBUTE_DATA attr;
	if (!GetFileAttributesExW(platStr.c_str(), GetFileExInfoStandard, &attr))
		return onError();

	auto fileStatus = StatusFromImpl::fromINFO(path, attr);

	return fileStatus;
}

void fs::createHardLink(const Path& to, const Path& newHardLink)
{
	using CreateHardLinkWFunc = BOOLEAN(WINAPI*)
	(
		LPCWSTR lpFileName,
		LPCWSTR lpExistingFileName,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes 
	);

	static auto CreateHardLinkW = (CreateHardLinkWFunc)GetProcAddress
	(
		GetModuleHandleW(L"kernel32.dll"),
		"CreateHardLinkW"
	);
	if (CreateHardLinkW == nullptr)
		throw Exception(to, newHardLink, std::errc::not_supported);

	auto platTo = to.getPlatformStr();
	auto platHardLink = newHardLink.getPlatformStr();
	if (!CreateHardLinkW(platTo.c_str(), platHardLink.c_str(), nullptr))
		throw Exception(to, newHardLink, makeSysError());
}

size_t fs::hardLinkCount(const Path& path)
{
	auto platStr = path.getPlatformStr();
	auto shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	UniqueHandle file(CreateFileW
	(
		platStr.c_str(),
		0,
		shareFlags,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	));


	BY_HANDLE_FILE_INFORMATION info;
	if (file.isNull() || !GetFileInformationByHandle(file.getHandle(), &info))
		throw Exception(path, makeSysError());

	return info.nNumberOfLinks;
}
