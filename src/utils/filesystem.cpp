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

	if (status(workingDir).type() == FileType::NotFound)
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
				ret = ret.parent();
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
