/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024-2025  mr b0nk 500 (b0nk@b0nk.xyz)
    Copyright (C) 2024  Ludger Krämer <dbluelle@onlinehome.de>

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
#include <iostream>
#include <lightspark/scripting/flash/utils/ByteArray.h>
#include <lightspark/swf.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>

#include "framework/backends/engineutils.h"
#include "framework/backends/event_loop.h"
#include "framework/options.h"
#include "framework/runner.h"

namespace fs = FileSystem;

SDL_Window* TestRunnerEngineData::createWidget(uint32_t w, uint32_t h)
{
	return SDL_CreateWindow("Lightspark",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,SDL_WINDOW_BORDERLESS|SDL_WINDOW_HIDDEN| SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
}

bool TestRunnerEngineData::FileExists(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return Path(filename).exists();
}

bool TestRunnerEngineData::FileIsHidden(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	Path path(filename);
	return path.exists() && path.getFilename().getStr().startsWith('.');
}

bool TestRunnerEngineData::FileIsDirectory(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return Path(filename).isDir();
}

uint32_t TestRunnerEngineData::FileSize(SystemState* sys,const tiny_string& filename, bool isfullpath)
{
	Path path(filename);
	return path.isFile() ? fs::fileSize(path) : 0;
}

tiny_string TestRunnerEngineData::FileFullPath(SystemState* sys, const tiny_string& filename)
{
	return basePath / filename;
}

tiny_string TestRunnerEngineData::FileBasename(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return Path(filename).getParent();
}

tiny_string TestRunnerEngineData::FileRead(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	Path path(filename);
	if (!path.isFile())
		return "";

	std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
	return tiny_string(file, fs::fileSize(path));
}

void TestRunnerEngineData::FileWrite(SystemState* sys, const tiny_string& filename, const tiny_string& data, bool isfullpath)
{
	Path path(filename);
	if (!path.isFile())
		return;
	std::ofstream file(path.getStr(), std::ios::out | std::ios::binary);
	file << data;
}


uint8_t TestRunnerEngineData::FileReadUnsignedByte(SystemState* sys, const tiny_string &filename, uint32_t startpos, bool isfullpath)
{
	Path path(filename);
	if (!path.isFile())
		return 0;
	uint8_t ret;
	std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
	file.seekg(startpos);
	file.read(reinterpret_cast<char*>(&ret), 1);
	return ret;
}

void TestRunnerEngineData::FileReadByteArray(SystemState* sys, const tiny_string &filename, ByteArray* res, uint32_t startpos, uint32_t length, bool isfullpath)
{
	Path path(filename);
	if (!path.isFile())
		return;
	size_t len = std::min<size_t>(length, fs::fileSize(path) - startpos);
	uint8_t* buf = new uint8_t[len];
	std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
	file.seekg(startpos);
	file.read(reinterpret_cast<char*>(buf), len);
	res->writeBytes(buf, len);
	delete[] buf;
}

void TestRunnerEngineData::FileWriteByteArray(SystemState* sys, const tiny_string& filename, ByteArray* data, uint32_t startpos, uint32_t length, bool isfullpath)
{
	Path path(filename);
	if (!path.isFile())
		return;
	size_t len =
	(
		length != UINT32_MAX &&
		startpos + length <= data->getLength()
	) ? length : data->getLength();
	uint8_t* buf = data->getBuffer(data->getLength(), false);
	std::ofstream file(path.getStr(), std::ios::out | std::ios::binary);
	file.write(reinterpret_cast<char*>(buf+startpos), len);
}

bool TestRunnerEngineData::FileCreateDirectory(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return fs::createDir
	(
		filename,
		fs::Perms::All ^
		(
			fs::Perms::GroupWrite |
			fs::Perms::OthersWrite
		)
	);
}

bool TestRunnerEngineData::FilGetDirectoryListing(SystemState* sys, const tiny_string &filename, bool isfullpath, std::vector<tiny_string>& filelist)
{
	Path path(filename);
	if (!path.isDir())
		return false;

	std::transform
	(
		fs::DirIter(path),
		fs::DirIter(),
		std::back_inserter(filelist),
		[](const fs::DirEntry& entry)
		{
			return entry.getPath();
		}
	);
	return true;
}

bool TestRunnerEngineData::FilePathIsAbsolute(const tiny_string& filename)
{
	return Path(filename).isAbsolute();
}

void TestRunnerEngineData::setClipboardText(const std::string txt)
{
	runner->clipboard = txt;
}


double TestRunnerEngineData::getScreenDPI()
{
	return runner->options.screenDPI.valueOr(72.0);
}
