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
#include <vector>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>

#include <libtest++/test_runner.h>

#include "macros.h"
#include "filesystem_tests.h"

using namespace lightspark;
using namespace libtestpp;

using OutcomeType = Outcome::Type;

TEST_CASE_DECL(FileSystem, Exception)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, Perms)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, FileStatus)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, DirEntry)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, DirIter)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, RecursiveDirIter)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, absolute)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, canonical)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, copy)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, copyFile)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, copySymlink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, createDirs)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, createDir)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, createDirSymlink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, createHardLink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, createSymlink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, currentPath)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, equivalent)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, exists)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, fileSize)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, hardLinkCount)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isBlockFile)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isCharacterFile)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isDir)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isEmpty)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isFifo)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isOther)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isFile)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isSocket)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, isSymlink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, lastWriteTime)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, perms)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, proximate)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, readSymlink)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, relative)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, remove)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, removeAll)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, rename)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, resizeFile)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, space)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, status)
{
	return Outcome(OutcomeType::Ignored);
}

// TODO: Add `FileSystem::statusKnown()`.
TEST_CASE_DECL(FileSystem, statusKnown)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, symlinkStatus)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, tempDirPath)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(FileSystem, weaklyCanonical)
{
	return Outcome(OutcomeType::Ignored);
}

// TODO: Add `string_view` support to `FileSystem`, once
// `tiny_string_view` is added.
TEST_CASE_DECL(FileSystem, stringView)
{
	return Outcome(OutcomeType::Ignored);
}
