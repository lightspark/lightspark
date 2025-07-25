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

#include <unistd.h>

#include "utils/filesystem.h"
#include "utils/path.h"

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
