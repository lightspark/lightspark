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
#include <cstdlib>
#include <vector>

#include <libtest++/test_runner.h>

#include "macros.h"

#include "filesystem_tests.h"
#include "path_tests.h"
#ifdef _WIN32
#include "win_tests.h"
#endif

using namespace lightspark;
using namespace libtestpp;

struct Test
{
	const char* category;
	const char* name;
	const char* desc;
	Optional<Outcome>(*runner)();
};

static constexpr std::array testCases =
{
	TEST_CASE(Path, nativeSeparator, "Tests if `nativeSeparator` matches the platform separator."),
	#ifndef _WIN32
	TEST_CASE(Path, hostRootNameSupport, "Tests if `Path(\"//host\").hasRootName() == true` on this platform."),
	#endif
	TEST_CASE(Path, ctorDtor, "Tests `Path`'s constructors, and destructors."),
	TEST_CASE(Path, assign, "Tests `Path`'s assignment functions."),
	TEST_CASE(Path, append, "Tests `Path`'s append functions."),
	TEST_CASE(Path, concat, "Tests `Path`'s concatenation functions."),
	TEST_CASE(Path, modifier, "Tests `Path`'s modifier functions."),
	TEST_CASE(Path, nativeObserve, "Tests `Path`'s native format observers."),
	TEST_CASE(Path, genericObserve, "Tests `Path`'s generic format observers."),
	TEST_CASE(Path, compare, "Tests `Path.compare()`."),
	TEST_CASE(Path, decompose, "Tests `Path`'s decomposition functions."),
	TEST_CASE(Path, query, "Tests `Path`'s query functions."),
	TEST_CASE(Path, gen, "Tests `Path` generation."),
	TEST_CASE(Path, iter, "Tests `Path`'s iterators."),
	// NOTE: `non-member` is in quotes because they're actually member
	// functions.
	TEST_CASE(Path, nonMember, "Tests `Path`'s \"non-member\" functions."),
	TEST_CASE(Path, iostream, "Tests `Path`'s `iostream` operators."),

	TEST_CASE(FileSystem, Exception, "Tests `FileSystem::Exception`."),
	TEST_CASE(FileSystem, Perms, "Tests `FileSystem::Perms`."),
	TEST_CASE(FileSystem, FileStatus, "Tests `FileSystem::FileStatus`."),
	TEST_CASE(FileSystem, DirEntry, "Tests `FileSystem::DirEntry`."),
	TEST_CASE(FileSystem, DirIter, "Tests `FileSystem::DirIter`."),
	TEST_CASE(FileSystem, RecursiveDirIter, "Tests `FileSystem::RecursiveDirIter`."),
	TEST_CASE(FileSystem, absolute, "Tests `FileSystem::absolute()`."),
	TEST_CASE(FileSystem, canonical, "Tests `FileSystem::canonical()`."),
	TEST_CASE(FileSystem, copy, "Tests `FileSystem::copy()`."),
	TEST_CASE(FileSystem, copyFile, "Tests `FileSystem::copyFile()`."),
	TEST_CASE(FileSystem, copySymlink, "Tests `FileSystem::copySymlink()`."),
	TEST_CASE(FileSystem, createDirs, "Tests `FileSystem::createDirs()`."),
	TEST_CASE(FileSystem, createDir, "Tests `FileSystem::createDir()`."),
	TEST_CASE(FileSystem, createDirSymlink, "Tests `FileSystem::createDirSymlink()`."),
	TEST_CASE(FileSystem, createHardLink, "Tests `FileSystem::createHardLink()`."),
	TEST_CASE(FileSystem, createSymlink, "Tests `FileSystem::createSymlink()`."),
	TEST_CASE(FileSystem, currentPath, "Tests `FileSystem::currentPath()`."),
	TEST_CASE(FileSystem, equivalent, "Tests `FileSystem::equivalent()`."),
	TEST_CASE(FileSystem, exists, "Tests `FileSystem::exists()`."),
	TEST_CASE(FileSystem, fileSize, "Tests `FileSystem::fileSize()`."),
	TEST_CASE(FileSystem, hardLinkCount, "Tests `FileSystem::hardLinkCount()`."),
	TEST_CASE(FileSystem, isBlockFile, "Tests `FileSystem::isBlockFile()`."),
	TEST_CASE(FileSystem, isCharacterFile, "Tests `FileSystem::isCharacterFile()`."),
	TEST_CASE(FileSystem, isDir, "Tests `FileSystem::isDir()`."),
	TEST_CASE(FileSystem, isEmpty, "Tests `FileSystem::isEmpty()`."),
	TEST_CASE(FileSystem, isFifo, "Tests `FileSystem::isFifo()`."),
	TEST_CASE(FileSystem, isOther, "Tests `FileSystem::isOther()`."),
	TEST_CASE(FileSystem, isFile, "Tests `FileSystem::isFile()`."),
	TEST_CASE(FileSystem, isSocket, "Tests `FileSystem::isSocket()`."),
	TEST_CASE(FileSystem, isSymlink, "Tests `FileSystem::isSymlink()`."),
	TEST_CASE(FileSystem, lastWriteTime, "Tests `FileSystem::[gs]etLastWriteTime()`."),
	TEST_CASE(FileSystem, perms, "Tests `FileSystem::[gs]etPerms()`."),
	TEST_CASE(FileSystem, proximate, "Tests `FileSystem::proximate()`."),
	TEST_CASE(FileSystem, readSymlink, "Tests `FileSystem::readSymlink()`."),
	TEST_CASE(FileSystem, relative, "Tests `FileSystem::relative()`."),
	TEST_CASE(FileSystem, remove, "Tests `FileSystem::remove()`."),
	TEST_CASE(FileSystem, removeAll, "Tests `FileSystem::removeAll()`."),
	TEST_CASE(FileSystem, rename, "Tests `FileSystem::rename()`."),
	TEST_CASE(FileSystem, resizeFile, "Tests `FileSystem::resizeFile()`."),
	TEST_CASE(FileSystem, space, "Tests `FileSystem::space()`."),
	TEST_CASE(FileSystem, status, "Tests `FileSystem::status()`."),
	TEST_CASE(FileSystem, statusKnown, "Tests `FileSystem::statusKnown()`."),
	TEST_CASE(FileSystem, symlinkStatus, "Tests `FileSystem::symlinkStatus()`."),
	TEST_CASE(FileSystem, tempDirPath, "Tests `FileSystem::tempDirPath()`."),
	TEST_CASE(FileSystem, weaklyCanonical, "Tests `FileSystem::weaklyCanonical()`."),
	TEST_CASE(FileSystem, stringView, "Tests `FileSystem`'s `string_view` support."),

	#ifdef _WIN32
	TEST_CASE(Win, lfn, "Windows: Tests long filename support."),
	TEST_CASE(Win, pathNamespace, "Windows: Tests path namespace handling."),
	TEST_CASE(Win, mappedFolder, "Windows: Tests mapped folder handling."),
	TEST_CASE(Win, readOnlyRemove, "Windows: Tests removal of read-only files."),
	#endif
};

std::vector<Trial> addTests(const Arguments& args)
{
	std::vector<Trial> tests(testCases.size());

	std::transform
	(
		testCases.begin(),
		testCases.end(),
		tests.begin(),
		[](const Test& test)
		{
			return Trial::test
			(
				test.name,
				test.runner
			).with_kind(test.category);
		}
	);
	return tests;
}
