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

#ifndef FILESYSTEM_TESTS_H
#define FILESYSTEM_TESTS_H 1

#include <libtest++/test_runner.h>
#include <lightspark/utils/path.h>

#include "macros.h"

using namespace lightspark;
using namespace libtestpp;

namespace lightspark
{
	template<typename T>
	class Optional;
};

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

	TempDir(const TempOpt& opt = TempOpt::None);
	~TempDir();
	const Path& getPath() const { return path; }
};

void generateFile(const Path& path, ssize_t size = -1);

TEST_CASE_DECL(FileSystem, Exception);
TEST_CASE_DECL(FileSystem, Perms);
TEST_CASE_DECL(FileSystem, FileStatus);
TEST_CASE_DECL(FileSystem, DirEntry);
TEST_CASE_DECL(FileSystem, DirIter);
TEST_CASE_DECL(FileSystem, RecursiveDirIter);
TEST_CASE_DECL(FileSystem, absolute);
TEST_CASE_DECL(FileSystem, canonical);
TEST_CASE_DECL(FileSystem, copy);
TEST_CASE_DECL(FileSystem, copyFile);
TEST_CASE_DECL(FileSystem, copySymlink);
TEST_CASE_DECL(FileSystem, createDirs);
TEST_CASE_DECL(FileSystem, createDir);
TEST_CASE_DECL(FileSystem, createDirSymlink);
TEST_CASE_DECL(FileSystem, createHardLink);
TEST_CASE_DECL(FileSystem, createSymlink);
TEST_CASE_DECL(FileSystem, currentPath);
TEST_CASE_DECL(FileSystem, equivalent);
TEST_CASE_DECL(FileSystem, exists);
TEST_CASE_DECL(FileSystem, fileSize);
TEST_CASE_DECL(FileSystem, hardLinkCount);
TEST_CASE_DECL(FileSystem, isBlockFile);
TEST_CASE_DECL(FileSystem, isCharacterFile);
TEST_CASE_DECL(FileSystem, isDir);
TEST_CASE_DECL(FileSystem, isEmpty);
TEST_CASE_DECL(FileSystem, isFifo);
TEST_CASE_DECL(FileSystem, isOther);
TEST_CASE_DECL(FileSystem, isFile);
TEST_CASE_DECL(FileSystem, isSocket);
TEST_CASE_DECL(FileSystem, isSymlink);
TEST_CASE_DECL(FileSystem, lastWriteTime);
TEST_CASE_DECL(FileSystem, perms);
TEST_CASE_DECL(FileSystem, proximate);
TEST_CASE_DECL(FileSystem, readSymlink);
TEST_CASE_DECL(FileSystem, relative);
TEST_CASE_DECL(FileSystem, remove);
TEST_CASE_DECL(FileSystem, removeAll);
TEST_CASE_DECL(FileSystem, rename);
TEST_CASE_DECL(FileSystem, resizeFile);
TEST_CASE_DECL(FileSystem, space);
TEST_CASE_DECL(FileSystem, status);
TEST_CASE_DECL(FileSystem, statusKnown);
TEST_CASE_DECL(FileSystem, symlinkStatus);
TEST_CASE_DECL(FileSystem, tempDirPath);
TEST_CASE_DECL(FileSystem, weaklyCanonical);
TEST_CASE_DECL(FileSystem, stringView);

#endif /* FILESYSTEM_TESTS_H */
