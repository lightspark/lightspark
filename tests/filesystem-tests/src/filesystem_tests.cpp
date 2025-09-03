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

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <iomanip>
#include <random>
#include <set>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <lightspark/compat.h>
#include <lightspark/logger.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/enum.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>
#include <lightspark/utils/type_traits.h>

#include <libtest++/test_runner.h>

#include "macros.h"
#include "filesystem_tests.h"
#include "tests.h"
#include "utils/enum_name.h"

using namespace lightspark;
using namespace libtestpp;

namespace fs = FileSystem;
namespace chrono = std::chrono;

using OutcomeType = Outcome::Type;
using StringType = Path::StringType;
using SysClock = chrono::system_clock;
using FloatDuration = chrono::duration<float, std::ratio<1>>;
using NsecDuration = chrono::nanoseconds;

class TempDir
{
private:
	Path path;
	Path originalDir;
public:
	enum class TempOpt
	{
		None,
		ChangePath
	};

	TempDir(const TempOpt& opt = TempOpt::None)
	{
		using HighResClock = chrono::high_resolution_clock;
		static auto seed = HighResClock::now().time_since_epoch().count();
		static auto rng = std::bind
		(
			std::uniform_int_distribution<int>(0, 35),
			std::mt19937(uint64_t(seed) ^ uint64_t(ptrdiff_t(&opt)))
		);

		StringType filename;
		do
		{
			StringType filename = "test-";
			for (size_t i = 0; i < 8; ++i)
				filename += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rng()];
			path = fs::canonical(fs::tempDirPath()) / filename;
		} while (path.exists());

		fs::createDirs(path);

		if (opt == TempOpt::ChangePath)
		{
			originalDir = fs::currentPath();
			fs::currentPath(path);
		}
	}

	~TempDir()
	{
		if (!originalDir.empty())
			fs::currentPath(originalDir);
		fs::removeAll(path);
	}

	const Path& getPath() const { return path; }
};

static void generateFile(const Path& path, ssize_t size = -1)
{
	std::ofstream file(path.getStr());

	if (size < 0)
		file << "Hello world!" << std::endl;
	else
		file << std::string(size, '*');
}

#ifdef _WIN32
#if !defined(_WIN64) && defined(KEY_WOW64_64KEY)
static bool isWow64Proc()
{
	using IsWow64Process_t = BOOL(WINAPI*)(HANDLE, PBOOL);
	BOOL isWow64 = FALSE;
	auto IsWow64Process = (IsWow64Process_t)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if (IsWow64Process == nullptr || !IsWow64Process(GetCurrentProcess(), &isWow64))
			isWow64 = FALSE;
	return isWow64 == TRUE;
}
#endif
bool isSymlinkCreationSupported()
{
	auto printWarning = []
	{
		std::cerr << "WARNING: Symlink creation isn't supported." << std::endl;
		return false;
	};

	HKEY key;
	REGSAM flags = KEY_READ;
	#ifdef _WIN64
	flags |= KEY_WOW64_64KEY;
	#elif defined(KEY_WOW64_64KEY)
	flags |= isWow64Proc() ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
	#else
	return printWarning();
	#endif

	auto error = RegOpenKeyExW
	(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
		0,
		flags,
		&key
	);
	if (error != ERROR_SUCCESS)
		return printWarning();

	DWORD val = 0;
	DWORD size = sizeof(DWORD);
	error = RegQueryValueExW
	(
		key,
		L"AllowDevelopmentWithoutDevLicense",
		0,
		nullptr,
		reinterpret_cast<LPBYTE>(&val),
		&size
	);
	RegCloseKey(key);

	if (error != ERROR_SUCCESS || !val)
		return printWarning();
	return true;
}
#else
bool isSymlinkCreationSupported()
{
	return true;
}
#endif

// TODO: Add a system time version of `compat_now()`.
TimeSpec sysNow()
{
	auto now = chrono::duration_cast<NsecDuration>(SysClock::now().time_since_epoch());
	return TimeSpec::fromNs(now.count());
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static std::ostream& operator<<(std::ostream& s, const T& val)
{
	return s << enumName(val);
}

static std::ostream& operator<<(std::ostream& s, const TimeSpec& time)
{
	return s << time.getSecs() << "s, " << time.getNsecs() << "ns";
}

static std::ostream& operator<<(std::ostream& s, const fs::Perms& val)
{
	return s << '0' << std::oct << uint16_t(val) << std::dec;
}

static std::ostream& operator<<(std::ostream& s, const fs::FileStatus& status)
{
	return s << "FileStatus { type: " << status.getType() << ", perms: " << status.getPerms() << " }";
}

static std::ostream& operator<<(std::ostream& s, const fs::DirIter& it)
{
	return s << it->getPath();
}

static std::ostream& operator<<(std::ostream& s, const fs::RecursiveDirIter& it)
{
	return s << it->getPath();
}

TEST_CASE_DECL(FileSystem, Exception)
{
	using namespace std::string_literals;
	std::stringstream s;

	std::error_code code(1, std::system_category());
	fs::Exception e("None"s, std::error_code());
	e = fs::Exception("Some error"s, code);
	CHECK_EQ(s, e.getCode().value(), 1);
	CHECK_BOOL(s, !StringType(e.what()).empty());
	CHECK_BOOL(s, e.getPath1().empty());
	CHECK_BOOL(s, e.getPath2().empty());
	e = fs::Exception("Some error"s, Path("foo/bar"), code);
	CHECK_BOOL(s, !StringType(e.what()).empty());
	CHECK_EQ(s, e.getPath1(), "foo/bar");
	CHECK_BOOL(s, e.getPath2().empty());
	e = fs::Exception("Some error"s, Path("foo/bar"), Path("some/other"), code);
	CHECK_BOOL(s, !StringType(e.what()).empty());
	CHECK_EQ(s, e.getPath1(), "foo/bar");
	CHECK_EQ(s, e.getPath2(), "some/other");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

static constexpr fs::Perms constexprOwnerAll()
{
	using Perms = fs::Perms;
	return Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec;
}

TEST_CASE_DECL(FileSystem, Perms)
{
	using Perms = fs::Perms;
	std::stringstream s;

	static_assert
	(
		constexprOwnerAll() == Perms::OwnerAll,
		"constexpr didn't result in `Perms::OwnerAll`."
	);
	CHECK_EQ(s, (Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec), Perms::OwnerAll);
	CHECK_EQ(s, (Perms::GroupRead | Perms::GroupWrite | Perms::GroupExec), Perms::GroupAll);
	CHECK_EQ(s, (Perms::OthersRead | Perms::OthersWrite | Perms::OthersExec), Perms::OthersAll);
	CHECK_EQ(s, (Perms::OwnerAll | Perms::GroupAll | Perms::OthersAll), Perms::All);
	CHECK_EQ(s, (Perms::All | Perms::SetUid | Perms::SetGid | Perms::StickyBit), Perms::Mask);

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, FileStatus)
{
	using FileStatus = fs::FileStatus;
	using FileType = fs::FileType;
	using Perms = fs::Perms;

	std::stringstream s;

	{
		FileStatus status;
		CHECK_EQ(s, status.getType(), FileType::None);
		CHECK_EQ(s, status.getPerms(), Perms::Unknown);
	}

	{
		FileStatus status(FileType::Regular);
		CHECK_EQ(s, status.getType(), FileType::Regular);
		CHECK_EQ(s, status.getPerms(), Perms::Unknown);
	}

	{
		FileStatus status(FileType::Directory, Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec);
		CHECK_EQ(s, status.getType(), FileType::Directory);
		CHECK_EQ(s, status.getPerms(), Perms::OwnerAll);
		status.setType(FileType::Block);
		CHECK_EQ(s, status.getType(), FileType::Block);
		status.setType(FileType::Character);
		CHECK_EQ(s, status.getType(), FileType::Character);
		status.setType(FileType::Fifo);
		CHECK_EQ(s, status.getType(), FileType::Fifo);
		status.setType(FileType::Symlink);
		CHECK_EQ(s, status.getType(), FileType::Symlink);
		status.setType(FileType::Socket);
		CHECK_EQ(s, status.getType(), FileType::Socket);
		status.setPerms(status.getPerms() | Perms::GroupAll | Perms::OthersAll);
		CHECK_EQ(s, status.getPerms(), Perms::All);
	}

	{
		FileStatus status0(FileType::Regular);
		FileStatus status(std::move(status0));
		CHECK_EQ(s, status.getType(), FileType::Regular);
		CHECK_EQ(s, status.getPerms(), Perms::Unknown);
	}

	{
		FileStatus status1(FileType::Regular, Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec);
		FileStatus status2(FileType::Regular, Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec);
		FileStatus status3(FileType::Directory, Perms::OwnerRead | Perms::OwnerWrite | Perms::OwnerExec);
		FileStatus status4(FileType::Regular, Perms::OwnerRead | Perms::OwnerWrite);
		CHECK_EQ(s, status1, status2);
		CHECK_NOT_BIN(s, status1, status3, ==);
		CHECK_NOT_BIN(s, status1, status4, ==);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, DirEntry)
{
	using FileType = fs::FileType;

	std::stringstream s;

	TempDir t;
	auto de = fs::DirEntry(t.getPath());
	CHECK_EQ(s, de.getPath(), t.getPath());
	CHECK_EQ(s, (Path)de, t.getPath());
	CHECK_BOOL(s, de.exists());
	CHECK_BOOL(s, !de.isBlockFile());
	CHECK_BOOL(s, !de.isCharFile());
	CHECK_BOOL(s, de.isDir());
	CHECK_BOOL(s, !de.isFifo());
	CHECK_BOOL(s, !de.isOther());
	CHECK_BOOL(s, !de.isFile());
	CHECK_BOOL(s, !de.isSocket());
	CHECK_BOOL(s, !de.isSymlink());
	CHECK_EQ(s, de.getStatus().getType(), FileType::Directory);
	CHECK_NOTHROW(s, de.refresh());
	fs::DirEntry none;
	CHECK_THROWS_AS(s, none.refresh(), fs::Exception);
	CHECK_THROWS_AS(s, de.assign(""), fs::Exception);
	generateFile(t.getPath() / "foo", 1234);
	// TODO: Add a system time version of `compat_now()`.
	auto now = sysNow();
	CHECK_NOTHROW(s, de.assign(t.getPath() / "foo"));
	de = fs::DirEntry(t.getPath() / "foo");
	CHECK_EQ(s, de.getPath(), t.getPath() / "foo");
	CHECK_BOOL(s, de.exists());
	CHECK_BOOL(s, !de.isBlockFile());
	CHECK_BOOL(s, !de.isCharFile());
	CHECK_BOOL(s, !de.isDir());
	CHECK_BOOL(s, !de.isFifo());
	CHECK_BOOL(s, !de.isOther());
	CHECK_BOOL(s, de.isFile());
	CHECK_BOOL(s, !de.isSocket());
	CHECK_BOOL(s, !de.isSymlink());
	CHECK_EQ(s, de.getFileSize(), 1234);
	CHECK_LT(s, static_cast<TimeSpec>(de.getLastWriteTime()).absDiff(now).getSecs(), 3);
	CHECK_EQ(s, de.getHardLinkCount(), 1);
	CHECK_THROWS_AS(s, de.replaceFilename("bar"), fs::Exception);
	CHECK_NOTHROW(s, de.replaceFilename("foo"));
	auto de2none = fs::DirEntry();
	CHECK_THROWS_AS(s, de2none.getHardLinkCount(), fs::Exception);
	CHECK_THROWS_AS(s, de2none.getLastWriteTime(), fs::Exception);
	CHECK_THROWS_AS(s, de2none.getFileSize(), fs::Exception);
	CHECK_EQ(s, de2none.getStatus().getType(), FileType::NotFound);
	generateFile(t.getPath() / "a");
	generateFile(t.getPath() / "b");
	auto d1 = fs::DirEntry(t.getPath() / "a");
	auto d2 = fs::DirEntry(t.getPath() / "b");
	CHECK_LT(s, d1, d2);
	CHECK_NOT_BIN(s, d2, d1, <);
	CHECK_BIN(s, d1, d2, <=);
	CHECK_NOT_BIN(s, d2, d1, <=);
	CHECK_GT(s, d2, d1);
	CHECK_NOT_BIN(s, d1, d2, >);
	CHECK_BIN(s, d2, d1, >=);
	CHECK_NOT_BIN(s, d1, d2, >=);
	CHECK_BIN(s, d1, d2, !=);
	CHECK_NOT_BIN(s, d2, d2, !=);
	CHECK_EQ(s, d1, d1);
	CHECK_NOT_BIN(s, d1, d2, ==);
	if (isSymlinkCreationSupported())
	{
		fs::createSymlink(t.getPath() / "nonexistent", t.getPath() / "broken");
		for (auto d3 : fs::DirIter(t.getPath()))
		{
			CHECK_NOTHROW(s, d3.getSymlinkStatus());
			CHECK_NOTHROW(s, d3.getStatus());
			CHECK_NOTHROW(s, d3.refresh());
		}
		fs::DirEntry entry(t.getPath() / "broken");
		CHECK_NOTHROW(s, entry.refresh());
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, DirIter)
{
	std::stringstream s;

	{
		TempDir t;
		CHECK_EQ(s, fs::DirIter(t.getPath()), fs::DirIter());
		generateFile(t.getPath() / "test", 1234);
		REQUIRE_BIN(fs::DirIter(t.getPath()), fs::DirIter(), !=);
		auto it = fs::DirIter(t.getPath());
		fs::DirIter it2(it);
		fs::DirIter it3;
		fs::DirIter it4;
		it3 = it;
		CHECK_EQ(s, it->getPath().getFilename(), "test");
		CHECK_EQ(s, it2->getPath().getFilename(), "test");
		CHECK_EQ(s, it3->getPath().getFilename(), "test");
		it4 = std::move(it3);
		CHECK_EQ(s, it4->getPath().getFilename(), "test");
		CHECK_EQ(s, it->getPath(), t.getPath() / "test");
		CHECK_BOOL(s, !it->isSymlink());
		CHECK_BOOL(s, it->isFile());
		CHECK_BOOL(s, !it->isDir());
		CHECK_EQ(s, it->getFileSize(), 1234);
		CHECK_EQ(s, ++it, fs::DirIter());
		CHECK_THROWS_AS(s, fs::DirIter(t.getPath() / "non-existing"), fs::Exception);
		size_t count = 0;
		for (auto de : fs::DirIter(t.getPath()))
			++count;
		CHECK_EQ(s, count, 1);
	}

	if (isSymlinkCreationSupported())
	{
		TempDir t;
		Path td = t.getPath() / "testdir";
		CHECK_EQ(s, fs::DirIter(t.getPath()), fs::DirIter());
		generateFile(t.getPath() / "test", 1234);
		fs::createDir(td);
		REQUIRE_NOTHROW(fs::createSymlink(t.getPath() / "test", td / "testlink"));
		std::error_code ec;
		REQUIRE_BIN(fs::DirIter(td), fs::DirIter(), !=);
		auto it = fs::DirIter(td);
		CHECK_EQ(s, it->getPath().getFilename(), "testlink");
		CHECK_EQ(s, it->getPath(), td / "testlink");
		CHECK_BOOL(s, it->isSymlink());
		CHECK_BOOL(s, it->isFile());
		CHECK_BOOL(s, !it->isDir());
		CHECK_EQ(s, it->getFileSize(), 1234);
		CHECK_EQ(s, ++it, fs::DirIter());
	}
	{
		// Check if resources are free'd when iterator reaches `end()`.
		TempDir t(TempDir::TempOpt::ChangePath);
		auto path = Path("test/");
		fs::createDir(path);
		fs::DirIter it;
		for (it = fs::DirIter(path); it != fs::DirIter(); ++it);
		CHECK_EQ(s, fs::removeAll(path), 1);
		CHECK_NOTHROW(s, fs::createDir(path));
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, RecursiveDirIter)
{
	std::stringstream s;

	{
		auto iter = fs::RecursiveDirIter(".");
		iter.pop();
		CHECK_EQ(s, iter, fs::RecursiveDirIter());
	}
	{
		TempDir t;
		CHECK_EQ(s, fs::RecursiveDirIter(t.getPath()), fs::RecursiveDirIter());
		generateFile(t.getPath() / "test", 1234);
		REQUIRE_BIN(fs::RecursiveDirIter(t.getPath()), fs::RecursiveDirIter(), !=);
		auto iter = fs::RecursiveDirIter(t.getPath());
		CHECK_EQ(s, iter->getPath().getFilename(), "test");
		CHECK_EQ(s, iter->getPath(), t.getPath() / "test");
		CHECK_BOOL(s, !iter->isSymlink());
		CHECK_BOOL(s, iter->isFile());
		CHECK_BOOL(s, !iter->isDir());
		CHECK_EQ(s, iter->getFileSize(), 1234);
		CHECK_EQ(s, ++iter, fs::RecursiveDirIter());
	}

	{
		TempDir t;
		Path td = t.getPath() / "testdir";
		fs::createDirs(td);
		generateFile(td / "test", 1234);
		REQUIRE_BIN(fs::RecursiveDirIter(t.getPath()), fs::RecursiveDirIter(), !=);
		auto iter = fs::RecursiveDirIter(t.getPath());

		CHECK_EQ(s, iter->getPath().getFilename(), "testdir");
		CHECK_EQ(s, iter->getPath(), td);
		CHECK_BOOL(s, !iter->isSymlink());
		CHECK_BOOL(s, !iter->isFile());
		CHECK_BOOL(s, iter->isDir());

		CHECK_BIN(s, ++iter, fs::RecursiveDirIter(), !=);

		CHECK_EQ(s, iter->getPath().getFilename(), "test");
		CHECK_EQ(s, iter->getPath(), td / "test");
		CHECK_BOOL(s, !iter->isSymlink());
		CHECK_BOOL(s, iter->isFile());
		CHECK_BOOL(s, !iter->isDir());
		CHECK_EQ(s, iter->getFileSize(), 1234);

		CHECK_EQ(s, ++iter, fs::RecursiveDirIter());
	}
	{
		TempDir t;
		//std::error_code ec;
		CHECK_EQ(s, fs::RecursiveDirIter(t.getPath(), fs::DirOptions::None), fs::RecursiveDirIter());
		/*
		CHECK_EQ(s, fs::RecursiveDirIter(t.getPath(), fs::directory_options::none, ec), fs::RecursiveDirIter());
		CHECK_BOOL(s, !ec);
		CHECK_EQ(s, fs::RecursiveDirIter(t.getPath(), ec), fs::RecursiveDirIter());
		CHECK_BOOL(s, !ec);
		*/
		generateFile(t.getPath() / "test");
		fs::RecursiveDirIter rd1(t.getPath());
		CHECK_BIN(s, fs::RecursiveDirIter(rd1), fs::RecursiveDirIter(), !=);
		fs::RecursiveDirIter rd2(t.getPath());
		CHECK_BIN(s, fs::RecursiveDirIter(std::move(rd2)), fs::RecursiveDirIter(), !=);
		fs::RecursiveDirIter rd3(t.getPath(), fs::DirOptions::SkipPermDenied);
		CHECK_EQ(s, rd3.getOptions(), fs::DirOptions::SkipPermDenied);
		fs::RecursiveDirIter rd4;
		rd4 = std::move(rd3);
		CHECK_BIN(s, rd4, fs::RecursiveDirIter(), !=);
		CHECK_NOTHROW(s, ++rd4);
		CHECK_EQ(s, rd4, fs::RecursiveDirIter());
		fs::RecursiveDirIter rd5;
		rd5 = rd4;
	}
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		generateFile("a");
		fs::createDir("d1");
		fs::createDir("d1/d2");
		generateFile("d1/b");
		generateFile("d1/c");
		generateFile("d1/d2/d");
		generateFile("e");
		std::multimap<StringType, ssize_t> result;
		for (auto it = fs::RecursiveDirIter("."); it != fs::end(it); ++it)
			result.insert(std::make_pair(it->getPath().getGenericStr(), it.depth()));
		std::stringstream os;
		for (auto pair : result)
			os << '[' << pair.first << ',' << pair.second << "],";
		CHECK_EQ(s, os.str(), "[./a,0],[./d1,0],[./d1/b,1],[./d1/c,1],[./d1/d2,1],[./d1/d2/d,2],[./e,0],");
	}
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		generateFile("a");
		fs::createDir("d1");
		fs::createDir("d1/d2");
		generateFile("d1/b");
		generateFile("d1/c");
		generateFile("d1/d2/d");
		generateFile("e");
		std::multiset<StringType> result;
		for (auto entry : fs::RecursiveDirIter("."))
			result.insert(entry.getPath().getGenericStr());
		std::stringstream os;
		for (const auto& str : result)
			os << str << ',';
		CHECK_EQ(s, os.str(), "./a,./d1,./d1/b,./d1/c,./d1/d2,./d1/d2/d,./e,");
	}
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		generateFile("a");
		fs::createDir("d1");
		fs::createDir("d1/d2");
		generateFile("d1/d2/b");
		generateFile("e");
		auto iter = fs::RecursiveDirIter(".");
		std::multimap<StringType, ssize_t> result;
		for (auto it = fs::RecursiveDirIter("."); it != fs::end(it); ++it)
		{
			result.insert(std::make_pair(it->getPath().getGenericStr(), it.depth()));

			if (it->getPath(), "./d1/d2")
				it.disablePending();
		}
		std::stringstream os;
		for (auto pair : result)
			os << '[' << pair.first << ',' << pair.second << "],";
		CHECK_EQ(s, os.str(), "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
	}
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		generateFile("a");
		fs::createDir("d1");
		fs::createDir("d1/d2");
		generateFile("d1/d2/b");
		generateFile("e");
		auto it = fs::RecursiveDirIter(".");
		std::multimap<StringType, ssize_t> result;
		while (it != fs::end(it))
		{
			result.insert(std::make_pair(it->getPath().getGenericStr(), it.depth()));
			if (it->getPath(), "./d1/d2")
				it.pop();
			else
				++it;
		}
		std::stringstream os;
		for (auto pair : result)
			os << '[' << pair.first << ',' << pair.second << "],";
		CHECK_EQ(s, os.str(), "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
	}
	if (isSymlinkCreationSupported())
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		fs::createDir("d1");
		generateFile("d1/a");
		fs::createDir("d2");
		generateFile("d2/b");
		fs::createDirSymlink("../d1", "d2/ds1");
		fs::createDirSymlink("d3", "d2/ds2");
		std::multiset<StringType> result;
		REQUIRE_NOTHROW([&]
		{
			for (const auto& entry : fs::RecursiveDirIter("d2", fs::DirOptions::FollowSymlinks))
				result.insert(entry.getPath().getGenericStr());
		}());
		std::stringstream os;
		for (const auto& str : result)
			os << str << ',';
		CHECK_EQ(s, os.str(), "d2/b,d2/ds1,d2/ds1/a,d2/ds2,");
		os.str("");
		result.clear();
		REQUIRE_NOTHROW([&]
		{
			for (const auto& entry : fs::RecursiveDirIter("d2"))
				result.insert(entry.getPath().getGenericStr());
		}());
		for (const auto& str : result)
			os << str << ',';
		CHECK_EQ(s, os.str(), "d2/b,d2/ds1,d2/ds2,");
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, absolute)
{
	std::stringstream s;

	CHECK_EQ(s, fs::absolute(""), fs::currentPath() / "");
	CHECK_EQ(s, fs::absolute(fs::currentPath()), fs::currentPath());
	CHECK_EQ(s, fs::absolute("."), fs::currentPath() / ".");
	std::stringstream tmp;
	CHECK_EQ(tmp, fs::absolute(".."), fs::currentPath().getParent());
	if (!tmp.str().empty())
	{
		tmp.str("");
		CHECK_EQ(tmp, fs::absolute(".."), fs::currentPath() / "..");
	}
	s << tmp.str();
	CHECK_EQ(s, fs::absolute("foo"), fs::currentPath() / "foo");
	/*
	std::error_code ec;
	CHECK_EQ(s, fs::absolute("", ec), fs::currentPath() / "");
	CHECK_BOOL(s, !ec);
	CHECK_EQ(s, fs::absolute("foo", ec), fs::currentPath() / "foo");
	CHECK_BOOL(s, !ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, canonical)
{
	std::stringstream s;

	CHECK_THROWS_AS(s, fs::canonical(""), fs::Exception);
	/*
	{
		std::error_code ec;
		CHECK_EQ(s, s, fs::canonical("", ec), "");
		CHECK_BOOL(s, ec);
	}
	*/
	CHECK_EQ(s, fs::canonical(fs::currentPath()), fs::currentPath());

	CHECK_EQ(s, fs::canonical("."), fs::currentPath());
	CHECK_EQ(s, fs::canonical(".."), fs::currentPath().getParent());
	CHECK_EQ(s, fs::canonical("/"), fs::currentPath().getRoot());
	CHECK_THROWS_AS(s, fs::canonical("foo"), fs::Exception);
	/*
	{
		std::error_code ec;
		CHECK_NOTHROW(s, fs::canonical("foo", ec));
		CHECK_BOOL(s, ec);
	}
	*/
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		auto dir = t.getPath() / "d0";
		fs::createDirs(dir / "d1");
		generateFile(dir / "f0");
		Path rel(dir.getFilename());
		CHECK_EQ(s, fs::canonical(dir), dir);
		CHECK_EQ(s, fs::canonical(rel), dir);
		CHECK_EQ(s, fs::canonical(dir / "f0"), dir / "f0");
		CHECK_EQ(s, fs::canonical(rel / "f0"), dir / "f0");
		CHECK_EQ(s, fs::canonical(rel / "./f0"), dir / "f0");
		CHECK_EQ(s, fs::canonical(rel / "d1/../f0"), dir / "f0");
	}

	if (isSymlinkCreationSupported())
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		fs::createDir(t.getPath() / "dir1");
		generateFile(t.getPath() / "dir1/test1");
		fs::createDir(t.getPath() / "dir2");
		fs::createDirSymlink(t.getPath() / "dir1", t.getPath() / "dir2/dirSym");
		CHECK_EQ(s, fs::canonical(t.getPath() / "dir2/dirSym/test1"), t.getPath() / "dir1/test1");
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, copy)
{
	using CopyOpts = fs::CopyOptions;
	std::stringstream s;

	{
		TempDir t(TempDir::TempOpt::ChangePath);
		//std::error_code ec;
		fs::createDir("dir1");
		generateFile("dir1/file1");
		generateFile("dir1/file2");
		fs::createDir("dir1/dir2");
		generateFile("dir1/dir2/file3");
		CHECK_NOTHROW(s, fs::copy("dir1", "dir3"));
		CHECK_BOOL(s, fs::exists("dir3/file1"));
		CHECK_BOOL(s, fs::exists("dir3/file2"));
		CHECK_BOOL(s, !fs::exists("dir3/dir2"));
		CHECK_NOTHROW(s, fs::copy("dir1", "dir4", CopyOpts::Recursive));
		/*
		CHECK_NOTHROW(s, fs::copy("dir1", "dir4", CopyOpts::Recursive, ec));
		CHECK_BOOL(s, !ec);
		*/
		CHECK_BOOL(s, fs::exists("dir4/file1"));
		CHECK_BOOL(s, fs::exists("dir4/file2"));
		CHECK_BOOL(s, fs::exists("dir4/dir2/file3"));
		fs::createDir("dir5");
		generateFile("dir5/file1");
		CHECK_THROWS_AS(s, fs::copy("dir1/file1", "dir5/file1"), fs::Exception);
		CHECK_NOTHROW(s, fs::copy("dir1/file1", "dir5/file1", CopyOpts::SkipExisting));
	}

	if (isSymlinkCreationSupported())
	{
		TempDir t(TempDir::TempOpt::ChangePath);
		//std::error_code ec;
		fs::createDir("dir1");
		generateFile("dir1/file1");
		generateFile("dir1/file2");
		fs::createDir("dir1/dir2");
		generateFile("dir1/dir2/file3");
		#ifdef USE_LWG_2682
		REQUIRE_THROWS_AS(fs::copy("dir1", "dir3", CopyOpts::CreateSymlinks | CopyOpts::Recursive), fs::Exception);
		#else
		REQUIRE_NOTHROW(fs::copy("dir1", "dir3", CopyOpts::CreateSymlinks | CopyOpts::Recursive));
		/*
		REQUIRE_NOTHROW(fs::copy("dir1", "dir3", CopyOpts::CreateSymlinks | CopyOpts::Recursive, ec));
		CHECK_BOOL(s, !ec);
		*/
		CHECK_BOOL(s, fs::exists("dir3/file1"));
		CHECK_BOOL(s, fs::isSymlink("dir3/file1"));
		CHECK_BOOL(s, fs::exists("dir3/file2"));
		CHECK_BOOL(s, fs::isSymlink("dir3/file2"));
		CHECK_BOOL(s, fs::exists("dir3/dir2/file3"));
		CHECK_BOOL(s, fs::isSymlink("dir3/dir2/file3"));
		#endif
	}

	{
		TempDir t(TempDir::TempOpt::ChangePath);
		//std::error_code ec;
		fs::createDir("dir1");
		generateFile("dir1/file1");
		generateFile("dir1/file2");
		fs::createDir("dir1/dir2");
		generateFile("dir1/dir2/file3");
		auto f1hl = fs::hardLinkCount("dir1/file1");
		auto f2hl = fs::hardLinkCount("dir1/file2");
		auto f3hl = fs::hardLinkCount("dir1/dir2/file3");
		CHECK_NOTHROW(s, fs::copy("dir1", "dir3", CopyOpts::CreateHardLinks | CopyOpts::Recursive));
		/*
		CHECK_NOTHROW(s, fs::copy("dir1", "dir3", CopyOpts::CreateHardLinks | CopyOpts::Recursive, ec));
		REQUIRE_BOOL(!ec);
		*/
		CHECK_BOOL(s, fs::exists("dir3/file1"));
		CHECK_EQ(s, fs::hardLinkCount("dir1/file1"), f1hl + 1);
		CHECK_BOOL(s, fs::exists("dir3/file2"));
		CHECK_EQ(s, fs::hardLinkCount("dir1/file2"), f2hl + 1);
		CHECK_BOOL(s, fs::exists("dir3/dir2/file3"));
		CHECK_EQ(s, fs::hardLinkCount("dir1/dir2/file3"), f3hl + 1);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, copyFile)
{
	using Perms = fs::Perms;
	using PermOpts = fs::PermOptions;
	using CopyOpts = fs::CopyOptions;
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 100);
	CHECK_BOOL(s, !fs::exists("bar"));
	CHECK_BOOL(s, fs::copyFile("foo", "bar"));
	CHECK_BOOL(s, fs::exists("bar"));
	CHECK_EQ(s, fs::fileSize("foo"), fs::fileSize("bar"));
	/*
	CHECK_EQ(s, fs::copy_file("foo", "bar2", ec));
	CHECK_EQ(s, !ec);
	*/
	compat_usleep(100);
	generateFile("foo2", 200);
	CHECK_BOOL(s, fs::copyFile("foo2", "bar", CopyOpts::UpdateExisting));
	CHECK_EQ(s, fs::fileSize("bar"), 200);
	CHECK_BOOL(s, !fs::copyFile("foo", "bar", CopyOpts::UpdateExisting));
	CHECK_EQ(s, fs::fileSize("bar"), 200);
	CHECK_BOOL(s, fs::copyFile("foo", "bar", CopyOpts::OverwriteExisting));
	CHECK_EQ(s, fs::fileSize("bar"), 100);
	CHECK_THROWS_AS(s, fs::copyFile("foobar", "foobar2"), fs::Exception);
	/*
	CHECK_NOTHROW(fs::copy_file("foobar", "foobar2", ec));
	CHECK_EQ(s, ec);
	*/
	CHECK_BOOL(s, !fs::exists("foobar"));
	Path file1("temp1.txt");
	Path file2("temp2.txt");
	generateFile(file1, 200);
	generateFile(file2, 200);
	auto allWrite = Perms::OwnerWrite | Perms::GroupWrite | Perms::OthersWrite;
	CHECK_NOTHROW(s, fs::setPerms(file1, allWrite, PermOpts::Remove));
	CHECK_BOOL(s, !(fs::status(file1).getPerms() & Perms::OwnerWrite));
	CHECK_NOTHROW(s, fs::setPerms(file2, allWrite, PermOpts::Add));
	CHECK_BOOL(s, bool(fs::status(file2).getPerms() & Perms::OwnerWrite));
	fs::copyFile(file1, file2, CopyOpts::OverwriteExisting);
	CHECK_BOOL(s, !(fs::status(file2).getPerms() & Perms::OwnerWrite));
	CHECK_NOTHROW(s, fs::setPerms(file1, allWrite, PermOpts::Add));
	CHECK_NOTHROW(s, fs::setPerms(file2, allWrite, PermOpts::Add));

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, copySymlink)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo");
	fs::createDir("dir");
	if (isSymlinkCreationSupported())
	{
		fs::createSymlink("foo", "sfoo");
		fs::createDirSymlink("dir", "sdir");
		CHECK_NOTHROW(s, fs::copySymlink("sfoo", "sfooc"));
		CHECK_BOOL(s, fs::exists("sfooc"));
		/*
		CHECK_NOTHROW(s, fs::copySymlink("sfoo", "sfooc2", ec));
		CHECK_BOOL(s, fs::exists("sfooc2"));
		CHECK_BOOL(s, !ec);
		*/
		CHECK_NOTHROW(s, fs::copySymlink("sdir", "sdirc"));
		CHECK_BOOL(s, fs::exists("sdirc"));
		/*
		CHECK_NOTHROW(s, fs::copySymlink("sdir", "sdirc2", ec));
		CHECK_BOOL(s, fs::exists("sdirc2"));
		CHECK_BOOL(s, !ec);
		*/
	}
	CHECK_THROWS_AS(s, fs::copySymlink("bar", "barc"), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::copySymlink("bar", "barc", ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, createDirs)
{
	#if 0
	return Outcome(OutcomeType::Ignored);
	#else
	std::stringstream s;

	TempDir t;
	Path p = t.getPath() / "testdir";
	Path p2 = p / "nested";
	REQUIRE_BOOL(!p.exists());
	REQUIRE_BOOL(!p2.exists());
	CHECK_BOOL(s, fs::createDirs(p2));
	CHECK_BOOL(s, p.isDir());
	CHECK_BOOL(s, p2.isDir());
	CHECK_BOOL(s, !fs::createDirs(p2));
	#ifdef USE_LWG_2935
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects LWG #2935 result conformance.");
	*/
	p = t.getPath() / "testfile";
	generateFile(p);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	bool created = false;
	CHECK_NOTHROW(s, (created = fs::createDirs(p)));
	CHECK_BOOL(s, !created);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	/*
	std::error_code ec;
	CHECK_NOTHROW(s, (created = fs::createDirs(p, ec)));
	CHECK_BOOL(s, !created);
	CHECK_BOOL(s, !ec);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	CHECK_BOOL(s, !fs::createDirs(p, ec));
	*/
	#else
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects conformance with P1164R1. (implemented by GCC with issue #86910.)");
	*/
	p = t.getPath() / "testfile";
	generateFile(p);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	CHECK_THROWS_AS(s, fs::createDirs(p), fs::Exception);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	/*
	std::error_code ec;
	CHECK_NOTHROW(s, fs::createDirs(p, ec));
	CHECK_BOOL(s, ec);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	CHECK_BOOL(s, !fs::createDirs(p, ec));
	*/
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
	#endif
}

TEST_CASE_DECL(FileSystem, createDir)
{
	std::stringstream s;

	TempDir t;
	Path p = t.getPath() / "testdir";
	REQUIRE_BOOL(!p.exists());
	CHECK_BOOL(s, fs::createDir(p));
	CHECK_BOOL(s, p.isDir());
	CHECK_BOOL(s, !p.isFile());
	CHECK_BOOL(s, fs::createDir(p / "nested", p));
	CHECK_BOOL(s, (p / "nested").isDir());
	CHECK_BOOL(s, !(p / "nested").isFile());
	#ifdef USE_LWG_2935
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects LWG #2935 result conformance.");
	*/
	p = t.path() / "testfile";
	generateFile(p);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	bool created = false;
	CHECK_NOTHROW(s, (created = fs::createDir(p)));
	CHECK_BOOL(s, !created);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	/*
	std::error_code ec;
	CHECK_NOTHROW((created = fs::createDir(p, ec)));
	CHECK_BOOL(s, !created);
	CHECK_BOOL(s, !ec);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	CHECK_BOOL(s, !fs::createDirs(p, ec));
	*/
	#else
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects conformance with P1164R1. (implemented by GCC with issue #86910.)");
	*/
	p = t.getPath() / "testfile";
	generateFile(p);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	REQUIRE_THROWS_AS(fs::createDir(p), fs::Exception);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	/*
	std::error_code ec;
	REQUIRE_NOTHROW(fs::createDir(p, ec));
	CHECK_BOOL(s, ec);
	CHECK_BOOL(s, p.isFile());
	CHECK_BOOL(s, !p.isDir());
	CHECK(!fs::createDir(p, ec));
	*/
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, createDirSymlink)
{
	if (!isSymlinkCreationSupported())
		return Outcome(OutcomeType::Ignored);

	std::stringstream s;

	TempDir t;
	fs::createDir(t.getPath() / "dir1");
	generateFile(t.getPath() / "dir1/test1");
	fs::createDir(t.getPath() / "dir2");
	fs::createDirSymlink(t.getPath() / "dir1", t.getPath() / "dir2/dirSym");
	CHECK_BOOL(s, (t.getPath() / "dir2/dirSym").exists());
	CHECK_BOOL(s, fs::isSymlink(t.getPath() / "dir2/dirSym"));
	CHECK_BOOL(s, (t.getPath() / "dir2/dirSym/test1").exists());
	CHECK_BOOL(s, (t.getPath() / "dir2/dirSym/test1").isFile());
	CHECK_THROWS_AS(s, fs::createDirSymlink(t.getPath() / "dir1", t.getPath() / "dir2/dirSym"), fs::Exception);
	/*
	std::error_code ec;
	CHECK_NOTHROW(s, fs::createDirSymlink(t.getPath() / "dir1", t.getPath() / "dir2/dirSym", ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, createHardLink)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 1234);
	CHECK_NOTHROW(s, fs::createHardLink("foo", "bar"));
	CHECK_BOOL(s, fs::exists("bar"));
	CHECK_BOOL(s, !fs::isSymlink("bar"));
	/*
	CHECK_NOTHROW(s, fs::createHardLink("foo", "bar2", ec));
	CHECK_BOOL(s, fs::exists("bar2"));
	CHECK_BOOL(s, !fs::isSymlink("bar2"));
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::createHardLink("nofoo", "bar"), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::createHardLink("nofoo", "bar", ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, createSymlink)
{
	if (!isSymlinkCreationSupported())
		return Outcome(OutcomeType::Ignored);

	std::stringstream s;

	TempDir t;
	fs::createDir(t.getPath() / "dir1");
	generateFile(t.getPath() / "dir1/test1");
	fs::createDir(t.getPath() / "dir2");
	fs::createSymlink(t.getPath() / "dir1/test1", t.getPath() / "dir2/fileSym");
	CHECK_BOOL(s, (t.getPath() / "dir2/fileSym").exists());
	CHECK_BOOL(s, fs::isSymlink(t.getPath() / "dir2/fileSym"));
	//CHECK_BOOL(s, (t.getPath() / "dir2/fileSym").isSymlink());
	CHECK_BOOL(s, (t.getPath() / "dir2/fileSym").exists());
	CHECK_BOOL(s, (t.getPath() / "dir2/fileSym").isFile());
	CHECK_THROWS_AS(s, fs::createSymlink(t.getPath() / "dir1", t.getPath() / "dir2/fileSym"), fs::Exception);
	/*
	std::error_code ec;
	CHECK_NOTHROW(s, fs::createSymlink(t.getPath() / "dir1", t.getPath() / "dir2/fileSym", ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, currentPath)
{
	std::stringstream s;

	TempDir t;
	//std::error_code ec;
	Path p1 = fs::currentPath();
	CHECK_NOTHROW(s, fs::currentPath(t.getPath()));
	CHECK_BIN(s, p1, fs::currentPath(), !=);
	CHECK_NOTHROW(s, fs::currentPath(p1));
	/*
	CHECK_NOTHROW(s, fs::currentPath(p1, ec));
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::currentPath(t.getPath() / "foo"), fs::Exception);
	CHECK_EQ(s, p1, fs::currentPath());
	/*
	CHECK_NOTHROW(s, fs::currentPath(t.getPath() / "foo", ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, equivalent)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	generateFile("foo", 1234);
	CHECK_BOOL(s, fs::equivalent(t.getPath() / "foo", "foo"));
	if (isSymlinkCreationSupported())
	{
		//std::error_code ec(42, std::system_category());
		fs::createSymlink("foo", "foo2");
		CHECK_BOOL(s, fs::equivalent("foo", "foo2"));
		/*
		CHECK(fs::equivalent("foo", "foo2", ec));
		CHECK(!ec);
		*/
	}
	#ifdef USE_LWG_2937
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects LWG #2937 result conformance.");
	*/
	/*
	std::error_code ec;
	bool result = false;
	*/
	REQUIRE_THROWS_AS(fs::equivalent("foo", "foo3"), fs::Exception);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo", "foo3", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_THROWS_AS(s, fs::equivalent("foo3", "foo"), fs::Exception);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo3", "foo", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_THROWS_AS(s, fs::equivalent("foo3", "foo4"), fs::Exception);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo3", "foo4", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, ec);
	*/
	#else
	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This test expects conformance predating LWG #2937 result.");
	*/
	//std::error_code ec;
	bool result = false;
	REQUIRE_NOTHROW(result = fs::equivalent("foo", "foo3"));
	CHECK_BOOL(s, !result);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo", "foo3", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_NOTHROW(s, result = fs::equivalent("foo3", "foo"));
	CHECK_BOOL(s, !result);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo3", "foo", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_THROWS_AS(s, result = fs::equivalent("foo4", "foo3"), fs::Exception);
	CHECK_BOOL(s, !result);
	/*
	CHECK_NOTHROW(s, result = fs::equivalent("foo4", "foo3", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, ec);
	*/
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, exists)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	CHECK_BOOL(s, !fs::exists(""));
	CHECK_BOOL(s, !fs::exists("foo"));
	/*
	CHECK_BOOL(s, !fs::exists("foo", ec));
	CHECK_BOOL(s, !ec);
	ec = std::error_code(42, std::system_category());
	CHECK_BOOL(s, !fs::exists("foo", ec));
	*/
	CHECK_BOOL(s, !fs::exists(u8"foo"));
	/*
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_BOOL(s, t.getPath().exists());
	/*
	CHECK_BOOL(s, t.getPath().exists(ec));
	CHECK_BOOL(s, !ec);
	ec = std::error_code(42, std::system_category());
	CHECK_BOOL(s, t.getPath().exists(ec));
	CHECK_BOOL(s, !ec);
	*/
	#ifdef _WIN32
	if (GetFileAttributesW(L"C:\\fs-test") != INVALID_FILE_ATTRIBUTES)
		CHECK_BOOL(s, fs::exists("C:\\fs-test"));
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, fileSize)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 0);
	generateFile("bar", 1234);
	CHECK_EQ(s, fs::fileSize("foo"), 0);
	/*
	ec = std::error_code(42, std::system_category());
	CHECK_EQ(s, fs::fileSize("foo", ec), 0);
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_EQ(s, fs::fileSize("bar"), 1234);
	/*
	ec = std::error_code(42, std::system_category());
	CHECK_EQ(s, fs::fileSize("bar", ec), 1234);
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_THROWS_AS(s, fs::fileSize("foobar"), fs::Exception);
	/*
	CHECK_EQ(s, fs::fileSize("foobar", ec), size_t(-1));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

#ifndef _WIN32
static size_t getHardLinkCount(const Path& path)
{
	struct stat st = {};
	return !lstat(path.rawBuf(), &st) ? st.st_nlink : size_t(-1);
}
#endif

TEST_CASE_DECL(FileSystem, hardLinkCount)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	#ifdef _WIN32
	// NOTE: Windows doesn't treat `{.,..}` as hard links, meaning it
	// starts with 1, and sub-directories don't change the count.
	CHECK_EQ(s, fs::hardLinkCount(t.getPath()), 1);
	fs::createDir("dir");
	CHECK_EQ(s, fs::hardLinkCount(t.getPath()), 1);
	#else
	// NOTE: Most filesystems follow the traditional UNIX design, and
	// treat `{.,..}` as hard links, meaning an empty directory has 2
	// hard links (1 for the parent, and the other for `.`), and adding
	// sub-directories adds 1 to the count because of it's `..`.
	CHECK_EQ(s, fs::hardLinkCount(t.getPath()), getHardLinkCount(t.getPath()));
	fs::createDir("dir");
	CHECK_EQ(s, fs::hardLinkCount(t.getPath()), getHardLinkCount(t.getPath()));
	#endif
	generateFile("foo");
	CHECK_EQ(s, fs::hardLinkCount(t.getPath() / "foo"), 1);
	/*
	ec = std::error_code(42, std::system_category());
	CHECK_EQ(s, s, fs::hardLinkCount(t.getPath() / "foo", ec), 1);
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::hardLinkCount(t.getPath() / "bar"), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::hardLinkCount(t.getPath() / "bar", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

class FileTypeMix
{
private:
	TempDir t;
	bool hasFifo;
	bool hasSocket;
public:
	FileTypeMix() :
	t(TempDir::TempOpt::ChangePath),
	hasFifo(false),
	hasSocket(false)
	{
		generateFile("regular");
		fs::createDir("directory");
		if (isSymlinkCreationSupported())
		{
			fs::createSymlink("regular", "file_symlink");
			fs::createDirSymlink("directory", "dir_symlink");
		}
		#ifndef _WIN32
		std::stringstream s;
		CHECK_EQ(s, mkfifo("fifo", 0644), 0);
		if (!s.str().empty())
			throw AssertionException(s.str());
		hasFifo = true;
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, "socket", sizeof(addr.sun_path));
		int fd = socket(PF_UNIX, SOCK_STREAM, 0);
		bind(fd, (struct sockaddr*)&addr, sizeof addr);
		hasSocket = true;
		#endif
	}

	~FileTypeMix() {}

	bool getHasFifo() const { return hasFifo; }
	bool getHasSocket() const { return hasSocket; }

	Path getBlockPath() const
	{
		if (fs::exists("/dev/sda"))
			return "/dev/sda";
		if (fs::exists("/dev/disk0"))
			return "/dev/disk0";
		return Path();
	}

	Path getCharPath() const
	{
		// TODO: Uncomment this once we have a native Solaris version.
		/*
		#if defined(__sun) && defined(__SVR4)
		if (fs::exists("/dev/null"))
			return "/dev/null";
		else if (fs::exists("NUL"))
			return "NUL";
		#endif
		*/
		return Path();
	}

	Path getTempPath() const { return t.getPath(); }
};

TEST_CASE_DECL(FileSystem, isBlockFile)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isBlockFile("directory"));
	CHECK_BOOL(s, !fs::isBlockFile("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isBlockFile("dir_symlink"));
		CHECK_BOOL(s, !fs::isBlockFile("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isBlockFile("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isBlockFile("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || fs::isBlockFile(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isBlockFile(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isBlockFile("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isBlockFile("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isBlockFile());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Block).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isBlockFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isBlockFile());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isCharacterFile)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isCharacterFile("directory"));
	CHECK_BOOL(s, !fs::isCharacterFile("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isCharacterFile("dir_symlink"));
		CHECK_BOOL(s, !fs::isCharacterFile("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isCharacterFile("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isCharacterFile("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isCharacterFile(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || fs::isCharacterFile(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isCharacterFile("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isCharacterFile("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isCharacterFile());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Character).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isCharacterFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isCharacterFile());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isDir)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, fs::isDir("directory"));
	CHECK_BOOL(s, !fs::isDir("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, fs::isDir("dir_symlink"));
		CHECK_BOOL(s, !fs::isDir("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isDir("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isDir("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isDir(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isDir(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isDir("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isDir("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isDir());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Directory).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isDir());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isDir());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isEmpty)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	CHECK_BOOL(s, fs::isEmpty(t.getPath()));
	/*
	CHECK_BOOL(s, fs::isEmpty(t.getPath(), ec));
	CHECK_BOOL(s, !ec);
	*/
	generateFile("foo", 0);
	generateFile("bar", 1234);
	CHECK_BOOL(s, fs::isEmpty("foo"));
	/*
	CHECK_BOOL(s, fs::isEmpty("foo", ec));
	CHECK_BOOL(s, !ec);
	*/
	CHECK_BOOL(s, !fs::isEmpty("bar"));
	/*
	CHECK_BOOL(s, !fs::isEmpty("bar", ec));
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::isEmpty("foobar"), fs::Exception);
	/*
	bool result = false;
	CHECK_NOTHROW(s, result = fs::isEmpty("foobar", ec));
	CHECK_BOOL(s, !result);
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isFifo)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isFifo("directory"));
	CHECK_BOOL(s, !fs::isFifo("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isFifo("dir_symlink"));
		CHECK_BOOL(s, !fs::isFifo("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || fs::isFifo("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isFifo("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isFifo(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isFifo(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isFifo("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isFifo("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isFifo());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Fifo).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isFifo());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isFifo());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isOther)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isOther("directory"));
	CHECK_BOOL(s, !fs::isOther("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isOther("dir_symlink"));
		CHECK_BOOL(s, !fs::isOther("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || fs::isOther("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || fs::isOther("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || fs::isOther(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || fs::isOther(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isOther("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isOther("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isOther());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isOther());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isOther());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isOther());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isOther());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Block).isOther());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Character).isOther());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Fifo).isOther());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Socket).isOther());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Unknown).isOther());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isFile)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isFile("directory"));
	CHECK_BOOL(s, fs::isFile("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isFile("dir_symlink"));
		CHECK_BOOL(s, fs::isFile("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isFile("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isFile("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isFile(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isFile(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isFile("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isFile("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isFile());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Regular).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isFile());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isFile());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isSocket)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isSocket("directory"));
	CHECK_BOOL(s, !fs::isSocket("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, !fs::isSocket("dir_symlink"));
		CHECK_BOOL(s, !fs::isSocket("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isSocket("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || fs::isSocket("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isSocket(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isSocket(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isSocket("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isSocket("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Symlink).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isSocket());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Socket).isSocket());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isSocket());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, isSymlink)
{
	std::stringstream s;
	FileTypeMix mix;

	//std::error_code ec;
	CHECK_BOOL(s, !fs::isSymlink("directory"));
	CHECK_BOOL(s, !fs::isSymlink("regular"));
	if (isSymlinkCreationSupported())
	{
		CHECK_BOOL(s, fs::isSymlink("dir_symlink"));
		CHECK_BOOL(s, fs::isSymlink("file_symlink"));
	}
	CHECK_BOOL(s, (!mix.getHasFifo() || !fs::isSymlink("fifo")));
	CHECK_BOOL(s, (!mix.getHasSocket() || !fs::isSymlink("socket")));
	CHECK_BOOL(s, (mix.getBlockPath().empty() || !fs::isSymlink(mix.getBlockPath())));
	CHECK_BOOL(s, (mix.getCharPath().empty() || !fs::isSymlink(mix.getCharPath())));
	CHECK_NOTHROW(s, fs::isSymlink("notfound"));
	/*
	CHECK_NOTHROW(s, fs::isSymlink("notfound", ec));
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::None).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::NotFound).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Regular).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Directory).isSymlink());
	CHECK_BOOL(s, fs::FileStatus(fs::FileType::Symlink).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Block).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Character).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Fifo).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Socket).isSymlink());
	CHECK_BOOL(s, !fs::FileStatus(fs::FileType::Unknown).isSymlink());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

/*
static time_t toTimeT(const TimeSpec& time)
{
	return (time + sysNow()).getSecs();
}
*/

static TimeSpec fromTimeT(time_t time)
{
	return TimeSpec::fromSec(time) + sysNow();
}

static TimeSpec timeFromString(const StringType& str)
{
	tm time;
	memset(&time, 0, sizeof(tm));
	std::stringstream s(str);
	s >> std::get_time(&time, "%Y-%m-%dT%H:%M:%S");
	if (s.fail())
		throw std::exception();
	return fromTimeT(mktime(&time));
}

TEST_CASE_DECL(FileSystem, lastWriteTime)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	TimeSpec fileTime;
	generateFile("foo");
	// TODO: Add a system time version of `compat_now()`.
	auto now = sysNow();
	CHECK_LT(s, fs::getLastWriteTime(t.getPath()).absDiff(now).getSecs(), 3);
	CHECK_LT(s, fs::getLastWriteTime("foo").absDiff(now).getSecs(), 3);
	CHECK_THROWS_AS(s, fs::getLastWriteTime("bar"), fs::Exception);
	/*
	CHECK_NOTHROW(s, fileTime = fs::getLastWriteTime("bar", ec));
	CHECK_EQ(s, fileTime, TimeSpec());
	CHECK_BOOL(s, ec);
	ec.clear();
	*/
	if (isSymlinkCreationSupported())
	{
		compat_usleep(100);
		fs::createSymlink("foo", "foo2");
		fileTime = fs::getLastWriteTime("foo");
		// Checks that the symlink time is fetched.
		CHECK_EQ(s, fileTime, fs::getLastWriteTime("foo2"));
	}

	auto nt = timeFromString("2015-10-21T04:30:00");
	CHECK_NOTHROW(s, fs::setLastWriteTime(t.getPath() / "foo", nt));
	CHECK_LT(s, fs::getLastWriteTime("foo").absDiff(nt).getSecs(), 1);
	nt = timeFromString("2015-10-21T04:29:00");
	/*
	CHECK_NOTHROW(s, fs::setLastWriteTime("foo", nt, ec));
	CHECK_LT(s, fs::getLastWriteTime("foo").absDiff(nt).getSecs(), 1);
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::setLastWriteTime("bar", nt), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::setLastWriteTime("bar", nt, ec));
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, perms)
{
	using Perms = fs::Perms;
	using PermOpts = fs::PermOptions;
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 512);
	auto allWrite = Perms::OwnerWrite | Perms::GroupWrite | Perms::OthersWrite;
	CHECK_NOTHROW(s, fs::setPerms("foo", allWrite, PermOpts::Remove));
	CHECK_BOOL(s, !(fs::status("foo").getPerms() & Perms::OwnerWrite));
	#ifndef _WIN32
	if (geteuid())
	#endif
	{
		CHECK_THROWS_AS(s, fs::resizeFile("foo", 1024), fs::Exception);
		CHECK_EQ(s, fs::fileSize("foo"), 512);
	}
	CHECK_NOTHROW(s, fs::setPerms("foo", Perms::OwnerWrite, PermOpts::Add));
	CHECK_BOOL(s, bool(fs::status("foo").getPerms() & Perms::OwnerWrite));
	CHECK_NOTHROW(s, fs::resizeFile("foo", 2048));
	CHECK_EQ(s, fs::fileSize("foo"), 2048);
	CHECK_NOTHROW(s, fs::setPerms("foo", Perms::OwnerWrite, PermOpts::Add));
	CHECK_THROWS_AS(s, fs::setPerms("bar", Perms::OwnerWrite, PermOpts::Add), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::setPerms("bar", Perms::OwnerWrite, PermOpts::Add, ec));
	CHECK_BOOL(s, ec);
	*/
	CHECK_THROWS_AS(s, fs::setPerms("bar", Perms::OwnerWrite, PermOpts(0)), fs::Exception);

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, proximate)
{
	std::stringstream s;

	//std::error_code ec;
	CHECK_EQ(s, fs::proximate("/a/d", "/a/b/c"), "../../d");
	/*
	CHECK_EQ(s, fs::proximate("/a/d", "/a/b/c", ec), "../../d");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_EQ(s, fs::proximate("/a/b/c", "/a/d"), "../b/c");
	/*
	CHECK_EQ(s, fs::proximate("/a/b/c", "/a/d", ec), "../b/c");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_EQ(s, fs::proximate("a/b/c", "a"), "b/c");
	/*
	CHECK_EQ(s, fs::proximate("a/b/c", "a", ec), "b/c");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_EQ(s, fs::proximate("a/b/c", "a/b/c/x/y"), "../..");
	/*
	CHECK_EQ(s, fs::proximate("a/b/c", "a/b/c/x/y", ec), "../..");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_EQ(s, fs::proximate("a/b/c", "a/b/c"), ".");
	/*
	CHECK_EQ(s, fs::proximate("a/b/c", "a/b/c", ec), ".");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_EQ(s, fs::proximate("a/b", "c/d"), "../../a/b");
	/*
	CHECK_EQ(s, fs::proximate("a/b", "c/d", ec), "../../a/b");
	CHECK_BOOL(s, !ec);
	*/
	#ifndef _WIN32
	if (hasHostRootNameSupport())
	{
		CHECK_EQ(s, fs::proximate("//host1/a/d", "//host2/a/b/c"), "//host1/a/d");
		/*
		CHECK_EQ(s, fs::proximate("//host1/a/d", "//host2/a/b/c", ec), "//host1/a/d");
		CHECK_BOOL(s, !ec);
		*/
	}
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, readSymlink)
{
	if (!isSymlinkCreationSupported())
		return Outcome(OutcomeType::Ignored);

	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo");
	fs::createSymlink(t.getPath() / "foo", "bar");
	CHECK_EQ(s, fs::readSymlink("bar"), t.getPath() / "foo");
	/*
	CHECK_EQ(s, fs::readSymlink("bar", ec), t.getPath() / "foo");
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::readSymlink("foobar"), fs::Exception);
	/*
	CHECK_EQ(s, fs::readSymlink("foobar", ec), Path());
	CHECK_BOOL(s, ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, relative)
{
	std::stringstream s;

	CHECK_EQ(s, fs::relative("/a/d", "/a/b/c"), "../../d");
	CHECK_EQ(s, fs::relative("/a/b/c", "/a/d"), "../b/c");
	CHECK_EQ(s, fs::relative("a/b/c", "a"), "b/c");
	CHECK_EQ(s, fs::relative("a/b/c", "a/b/c/x/y"), "../..");
	CHECK_EQ(s, fs::relative("a/b/c", "a/b/c"), ".");
	CHECK_EQ(s, fs::relative("a/b", "c/d"), "../../a/b");
	/*
	std::error_code ec;
	CHECK_EQ(s, fs::relative(fs::currentPath() / "foo", ec), "foo");
	CHECK_BOOL(s, !ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, remove)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo");
	CHECK_BOOL(s, fs::remove("foo"));
	CHECK_BOOL(s, !fs::exists("foo"));
	CHECK_BOOL(s, !fs::remove("foo"));
	/*
	generateFile("foo");
	CHECK_BOOL(s, fs::remove("foo", ec));
	CHECK_BOOL(s, !fs::exists("foo"));
	*/
	if (isSymlinkCreationSupported())
	{
		generateFile("foo");
		fs::createSymlink("foo", "bar");
		CHECK_BOOL(s, fs::symlinkStatus("bar").exists());
		//CHECK_BOOL(s, fs::remove("bar", ec));
		CHECK_BOOL(s, fs::remove("bar"));
		CHECK_BOOL(s, fs::exists("foo"));
		CHECK_BOOL(s, !fs::symlinkStatus("bar").exists());
	}
	CHECK_BOOL(s, !fs::remove("bar"));
	/*
	CHECK_BOOL(s, !fs::remove("bar", ec));
	CHECK_BOOL(s, !ec);
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, removeAll)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo");
	CHECK_EQ(s, fs::removeAll("foo"), 1);
	/*
	CHECK_EQ(s, fs::removeAll("foo", ec), 1);
	CHECK_BOOL(s, !ec);
	ec.clear();
	*/
	CHECK_EQ(s, fs::DirIter(t.getPath()), fs::DirIter());
	fs::createDirs("dir1/dir1a");
	fs::createDirs("dir1/dir1b");
	generateFile("dir1/dir1a/f1");
	generateFile("dir1/dir1b/f2");
	CHECK_NOTHROW(s, fs::removeAll("dir1/non-existing"));
	CHECK_EQ(s, fs::removeAll("dir1/non-existing"), 0);
	/*
	CHECK_NOTHROW(s, fs::removeAll("dir1/non-existing", ec));
	CHECK_BOOL(s, !ec);
	CHECK_EQ(s, fs::removeAll("dir1/non-existing", ec), 0);
	*/
	if (isSymlinkCreationSupported())
	{
		fs::createDirSymlink("dir1", "dir1link");
		CHECK_EQ(s, fs::removeAll("dir1link"), 1);
	}
	CHECK_EQ(s, fs::removeAll("dir1"), 5);
	CHECK_EQ(s, fs::DirIter(t.getPath()), fs::DirIter());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, rename)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 123);
	fs::createDir("dir1");
	CHECK_NOTHROW(s, fs::rename("foo", "bar"));
	CHECK_BOOL(s, !fs::exists("foo"));
	CHECK_BOOL(s, fs::exists("bar"));
	CHECK_NOTHROW(s, fs::rename("dir1", "dir2"));
	CHECK_BOOL(s, fs::exists("dir2"));
	generateFile("foo2", 42);
	CHECK_NOTHROW(s, fs::rename("bar", "foo2"));
	CHECK_BOOL(s, fs::exists("foo2"));
	CHECK_EQ(s, fs::fileSize("foo2"), 123u);
	CHECK_BOOL(s, !fs::exists("bar"));
	CHECK_NOTHROW(s, fs::rename("foo2", "foo"));
	/*
	CHECK_NOTHROW(s, fs::rename("foo2", "foo", ec));
	CHECK_BOOL(s, !ec);
	*/
	CHECK_THROWS_AS(s, fs::rename("foobar", "barfoo"), fs::Exception);
	/*
	CHECK_NOTHROW(s, fs::rename("foobar", "barfoo", ec));
	CHECK_BOOL(s, ec);
	*/
	CHECK_BOOL(s, !fs::exists("barfoo"));

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, resizeFile)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	generateFile("foo", 1024);
	CHECK_EQ(s, fs::fileSize("foo"), 1024);
	CHECK_NOTHROW(s, fs::resizeFile("foo", 2048));
	CHECK_EQ(s, fs::fileSize("foo"), 2048);
	/*
	CHECK_NOTHROW(fs::resizeFile("foo", 1000, ec));
	CHECK_BOOL(s, !ec);
	CHECK_EQ(s, fs::fileSize("foo"), 1000);
	*/
	CHECK_THROWS_AS(s, fs::resizeFile("bar", 2048), fs::Exception);
	CHECK_BOOL(s, !fs::exists("bar"));
	/*
	CHECK_NOTHROW(fs::resizeFile("bar", 4096, ec));
	CHECK_BOOL(s, ec);
	CHECK_BOOL(s, !fs::exists("bar"));
	*/

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, space)
{
	std::stringstream s;

	{
		fs::SpaceInfo info { size_t(-1), size_t(-1), size_t(-1) };
		CHECK_NOTHROW(s, info = fs::space(fs::currentPath()));
		CHECK_GT(s, info.capacity, 1024 * 1024);
		CHECK_GT(s, info.capacity, info.free);
		CHECK_BIN(s, info.free, info.available, >=);
	}
	/*
	{
		std::error_code ec;
		fs::SpaceInfo info { size_t(-1), size_t(-1), size_t(-1) };
		CHECK_NOTHROW(s, info = fs::space(fs::currentPath(), ec));
		CHECK_GT(s, info.capacity, 1024 * 1024);
		CHECK_GT(s, info.capacity, info.free);
		CHECK_BIN(s, info.free, info.available, >=);
		CHECK_BOOL(s, !ec);
	}
	{
		std::error_code ec;
		fs::SpaceInfo info { 0, 0, 0 };
		CHECK_NOTHROW(s, info = fs::space("foobar42", ec));
		CHECK_EQ(s, info.capacity, infoze_t(-1));
		CHECK_EQ(s, info.free, infoze_t(-1));
		CHECK_EQ(s, info.available, infoze_t(-1));
		CHECK_BOOL(s, ec);
	}
	*/
	CHECK_THROWS_AS(s, fs::space("foobar42"), fs::Exception);

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, status)
{
	using FileType = fs::FileType;
	using Perms = fs::Perms;
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	fs::FileStatus fs;
	CHECK_NOTHROW(s, fs = fs::status("foo"));
	CHECK_EQ(s, fs.getType(), FileType::NotFound);
	CHECK_EQ(s, fs.getPerms(), Perms::Unknown);
	/*
	CHECK_NOTHROW(s, fs = fs::status("bar", ec));
	CHECK_EQ(s, fs.getType(), FileType::NotFound);
	CHECK_EQ(s, fs.getPerms(), Perms::Unknown);
	CHECK_EQ(s, ec);
	ec.clear();
	*/
	fs = fs::status(t.getPath());
	CHECK_EQ(s, fs.getType(), FileType::Directory);
	auto ownerRw = Perms::OwnerRead | Perms::OwnerWrite;
	CHECK_EQ(s, (fs.getPerms() & ownerRw), ownerRw);
	generateFile("foobar");
	fs = fs::status(t.getPath() / "foobar");
	CHECK_EQ(s, fs.getType(), FileType::Regular);
	CHECK_EQ(s, (fs.getPerms() & ownerRw), ownerRw);
	if (isSymlinkCreationSupported())
	{
		fs::createSymlink(t.getPath() / "foobar", t.getPath() / "barfoo");
		fs = fs::status(t.getPath() / "barfoo");
		CHECK_EQ(s, fs.getType(), FileType::Regular);
		CHECK_EQ(s, (fs.getPerms() & ownerRw), ownerRw);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

// TODO: Add `FileSystem::statusKnown()`.
TEST_CASE_DECL(FileSystem, statusKnown)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, symlinkStatus)
{
	using FileType = fs::FileType;
	using Perms = fs::Perms;
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	//std::error_code ec;
	fs::FileStatus fs;
	CHECK_NOTHROW(s, fs = fs::symlinkStatus("foo"));
	CHECK_EQ(s, fs.getType(), FileType::NotFound);
	CHECK_EQ(s, fs.getPerms(), Perms::Unknown);
	/*
	CHECK_NOTHROW(s, fs = fs::symlinkStatus("bar", ec));
	CHECK_EQ(s, fs.getType(), FileType::NotFound);
	CHECK_EQ(s, fs.getPerms(), Perms::Unknown);
	CHECK_EQ(s, ec);
	ec.clear();
	*/
	fs = fs::symlinkStatus(t.getPath());
	CHECK_EQ(s, fs.getType(), FileType::Directory);
	auto ownerRw = Perms::OwnerRead | Perms::OwnerWrite;
	CHECK_EQ(s, (fs.getPerms() & ownerRw), ownerRw);
	generateFile("foobar");
	fs = fs::symlinkStatus(t.getPath() / "foobar");
	CHECK_EQ(s, fs.getType(), FileType::Regular);
	CHECK_EQ(s, (fs.getPerms() & ownerRw), ownerRw);
	if (isSymlinkCreationSupported())
	{
		fs::createSymlink(t.getPath() / "foobar", t.getPath() / "barfoo");
		fs = fs::symlinkStatus(t.getPath() / "barfoo");
		CHECK_EQ(s, fs.getType(), FileType::Symlink);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, tempDirPath)
{
	std::stringstream s;

	//std::error_code ec;
	CHECK_NOTHROW(s, fs::tempDirPath().exists());
	//CHECK_NOTHROW(s, fs::tempDirPath(ec).exists());
	CHECK_BOOL(s, !fs::tempDirPath().empty());
	//CHECK_BOOL(s, !ec);

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(FileSystem, weaklyCanonical)
{
	std::stringstream s;

	/*
	std::cerr << std::endl;
	LOG(LOG_INFO, "This might fail on implementations that return `fs::currentPath()` for `fs::canonical(\"\")`");
	*/
	CHECK_EQ(s, fs::weaklyCanonical(""), ".");
	if (fs::weaklyCanonical("") == ".")
	{
		CHECK_EQ(s, fs::weaklyCanonical("foo/bar"), "foo/bar");
		CHECK_EQ(s, fs::weaklyCanonical("foo/./bar"), "foo/bar");
		CHECK_EQ(s, fs::weaklyCanonical("foo/../bar"), "bar");
	}
	else
	{
		s.str("");
		CHECK_EQ(s, fs::weaklyCanonical("foo/bar"), fs::currentPath() / "foo/bar");
		CHECK_EQ(s, fs::weaklyCanonical("foo/./bar"), fs::currentPath() / "foo/bar");
		CHECK_EQ(s, fs::weaklyCanonical("foo/../bar"), fs::currentPath() / "bar");
	}

	{
		TempDir t(TempDir::TempOpt::ChangePath);
		auto dir = t.getPath() / "d0";
		fs::createDirs(dir / "d1");
		generateFile(dir / "f0");
		Path rel(dir.getFilename());
		CHECK_EQ(s, fs::weaklyCanonical(dir), dir);
		CHECK_EQ(s, fs::weaklyCanonical(rel), dir);
		CHECK_EQ(s, fs::weaklyCanonical(dir / "f0"), dir / "f0");
		CHECK_EQ(s, fs::weaklyCanonical(dir / "f0/"), dir / "f0/");
		CHECK_EQ(s, fs::weaklyCanonical(dir / "f1"), dir / "f1");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "f0"), dir / "f0");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "f0/"), dir / "f0/");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "f1"), dir / "f1");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "./f0"), dir / "f0");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "./f1"), dir / "f1");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "d1/../f0"), dir / "f0");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "d1/../f1"), dir / "f1");
		CHECK_EQ(s, fs::weaklyCanonical(rel / "d1/../f1/../f2"), dir / "f2");
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

// TODO: Add `string_view` support to `FileSystem`, once
// `tiny_string_view` is added.
TEST_CASE_DECL(FileSystem, stringView)
{
	return Outcome(OutcomeType::Ignored);
}
