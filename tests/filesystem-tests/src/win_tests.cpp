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
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/enum.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>

#include <libtest++/test_runner.h>

#include "macros.h"
#include "filesystem_tests.h"
#include "path_tests.h"
#include "tests.h"
#include "win_tests.h"

using namespace lightspark;
using namespace libtestpp;

namespace fs = FileSystem;

using OutcomeType = Outcome::Type;

TEST_CASE_DECL(Win, lfn)
{
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	Path dir(R"(\\?\)");
	dir += fs::currentPath().getStr();

	char ch = 'A';
	for (; ch <= 'Z'; ++ch)
	{
		dir /= std::string(16, ch);
		CHECK_NOTHROW(s, fs::createDir(dir));
		CHECK_BOOL(s, dir.exists());
		generateFile(dir / "f0");
		REQUIRE_BOOL((dir / "f0").exists());
	}

	CHECK_GT(s, ch, 'Z');
	Path path = fs::currentPath() / Path::StringType(std::string(16, 'A'));
	fs::removeAll(path);
	CHECK_BOOL(s, !path.exists());
	CHECK_NOTHROW(s, fs::createDirs(dir));
	CHECK_BOOL(s, dir.exists());
	generateFile(dir / "f0");
	CHECK_BOOL(s, (dir / "f0").exists());

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Win, pathNamespace)
{
	std::stringstream s;

	{
		//std::error_code code;
		Path path(R"(\\localhost\c$\Windows)");
		/*
		(void)fs::symlinkStatus(path, code);
		CHECK_BOOL(s, !code.value());
		*/
		CHECK_NOTHROW(s, fs::symlinkStatus(path));
		/*
		auto path2 = fs::canonical(path, code);
		CHECK_BOOL(s, !code.value());
		*/
		Path path2;
		CHECK_NOTHROW(s, path2 = fs::canonical(path));
		CHECK_EQ(s, path2, path);
	}

	struct TestInfo
	{
		Path::StringType path;
		Path::StringType str;
		Path::StringType rootName;
		Path::StringType rootPath;
		Path::StringType iter;
	};

	#define TEST_INFO(path, str, rootName, rootPath, iter) \
		TestInfo { path, str, rootName, rootPath, iter }

	static std::array variants =
	{

		TEST_INFO
		(
			R"(C:\Windows\notepad.exe)",
			R"(C:\Windows\notepad.exe)",
			"C:",
			"C:\\",
			"C:,/,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\C:\Windows\notepad.exe)",
			R"(\\?\C:\Windows\notepad.exe)",
			"C:",
			"C:\\",
			"//?/,C:,/,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\??\C:\Windows\notepad.exe)",
			R"(\??\C:\Windows\notepad.exe)",
			"C:",
			"C:\\",
			"/?\?/,C:,/,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\.\C:\Windows\notepad.exe)",
			R"(\\.\C:\Windows\notepad.exe)",
			"\\\\.",
			"\\\\.\\",
			"//.,/,C:,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\HarddiskVolume1\Windows\notepad.exe)",
			R"(\\?\HarddiskVolume1\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,HarddiskVolume1,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\Harddisk0Partition1\Windows\notepad.exe)",
			R"(\\?\Harddisk0Partition1\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,Harddisk0Partition1,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\.\GLOBALROOT\Device\HarddiskVolume1\Windows\notepad.exe)",
			R"(\\.\GLOBALROOT\Device\HarddiskVolume1\Windows\notepad.exe)",
			"\\\\.",
			"\\\\.\\",
			"//.,/,GLOBALROOT,Device,HarddiskVolume1,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\GLOBALROOT\Device\Harddisk0\Partition1\Windows\notepad.exe)",
			R"(\\?\GLOBALROOT\Device\Harddisk0\Partition1\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,GLOBALROOT,Device,Harddisk0,Partition1,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\Volume{e8a4a89d-0000-0000-0000-100000000000}\Windows\notepad.exe)",
			R"(\\?\Volume{e8a4a89d-0000-0000-0000-100000000000}\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,Volume{e8a4a89d-0000-0000-0000-100000000000},Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\LOCALHOST\C$\Windows\notepad.exe)",
			R"(\\LOCALHOST\C$\Windows\notepad.exe)",
			"\\\\LOCALHOST",
			"\\\\LOCALHOST\\",
			"//LOCALHOST,/,C$,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\UNC\C$\Windows\notepad.exe)",
			R"(\\?\UNC\C$\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,UNC,C$,Windows,notepad.exe"
		),
		TEST_INFO
		(
			R"(\\?\GLOBALROOT\Device\Mup\C$\Windows\notepad.exe)",
			R"(\\?\GLOBALROOT\Device\Mup\C$\Windows\notepad.exe)",
			"\\\\?",
			"\\\\?\\",
			"//?,/,GLOBALROOT,Device,Mup,C$,Windows,notepad.exe"
		),
	};
	#undef TEST_INFO

	for (auto testInfo : variants)
	{
		/*
		std::cerr << std::endl;
		LOG(LOG_INFO, "Used path: " << testInfo.path);
		*/
		Path path(testInfo.path);
		CHECK_EQ(s, path.getStr(), testInfo.str);
		CHECK_BOOL(s, path.isAbsolute());
		CHECK_EQ(s, path.getRootName().getStr(), testInfo.rootName);
		CHECK_EQ(s, path.getRoot().getStr(), testInfo.rootPath);
		CHECK_EQ(s, iter(path), testInfo.iter);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Win, mappedFolder)
{

	// NOTE: This test expects a mounted volume located at `C:\fs-test`.
	// Otherwise, this test is ignored.

	if (!Path("C:\\fs-test").exists())
		return Outcome(OutcomeType::Ignored);

	std::stringstream s;

	CHECK_EQ
	(
		s,
		fs::canonical(R"(C:\fs-test\Test.txt)").getStr(),
		R"(C:\fs-test\Test.txt)"
	);

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Win, readOnlyRemove)
{
	using Perms = fs::Perms;
	using PermOpts = fs::PermOptions;
	std::stringstream s;

	TempDir t(TempDir::TempOpt::ChangePath);
	generateFile("foo", 512);
	auto allWrite = Perms::OwnerWrite | Perms::GroupWrite | Perms::OthersWrite;
	CHECK_NOTHROW(s, fs::setPerms("foo", allWrite, PermOpts::Remove));
	CHECK_NOTHROW(s, fs::remove("foo"));
	CHECK_BOOL(s, !fs::exists("foo"));

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}
