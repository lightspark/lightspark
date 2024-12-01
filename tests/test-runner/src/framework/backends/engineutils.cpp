/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "framework/backends/engineutils.h"
#include "framework/backends/event_loop.h"
#include "framework/options.h"
#include "framework/runner.h"

void TestRunnerEngineData::notifyEventLoop()
{
	runner->eventLoop.setNotified(true);
}

bool TestRunnerEngineData::FileExists(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return fs::exists(path_t((std::string)filename));
}

bool TestRunnerEngineData::FileIsHidden(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	path_t path((std::string)filename);
	return fs::exists(path) && path.filename().generic_string()[0] == '.';
}

bool TestRunnerEngineData::FileIsDirectory(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return fs::is_directory(path_t((std::string)filename));
}

uint32_t TestRunnerEngineData::FileSize(SystemState* sys,const tiny_string& filename, bool isfullpath)
{
	path_t path((std::string)filename);
	return fs::is_regular_file(path) ? fs::file_size(path) : 0;
}

tiny_string TestRunnerEngineData::FileFullPath(SystemState* sys, const tiny_string& filename)
{
	return (basePath / filename).string();
}

tiny_string TestRunnerEngineData::FileBasename(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	return path_t((std::string)filename).parent_path().string();
}

tiny_string TestRunnerEngineData::FileRead(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	path_t path((std::string)filename);
	if (!fs::is_regular_file(path))
		return "";

	std::ifstream file(path, std::ios::in | std::ios::binary);
	return tiny_string(file, fs::file_size(path));
}

void TestRunnerEngineData::FileWrite(SystemState* sys, const tiny_string& filename, const tiny_string& data, bool isfullpath)
{
	path_t path((std::string)filename);
	if (!fs::is_regular_file(path))
		return;
	std::ofstream file(path, std::ios::out | std::ios::binary);
	file << data;
}


uint8_t TestRunnerEngineData::FileReadUnsignedByte(SystemState* sys, const tiny_string &filename, uint32_t startpos, bool isfullpath)
{
	path_t path((std::string)filename);
	if (!fs::is_regular_file(path))
		return 0;
	uint8_t ret;
	std::ifstream file(path, std::ios::in | std::ios::binary);
	file.seekg(startpos);
	file.read(reinterpret_cast<char*>(&ret), 1);
	return ret;
}

void TestRunnerEngineData::FileReadByteArray(SystemState* sys, const tiny_string &filename, ByteArray* res, uint32_t startpos, uint32_t length, bool isfullpath)
{
	path_t path((std::string)filename);
	if (!fs::is_regular_file(path))
		return;
	size_t len = std::min<decltype(fs::file_size(path))>(length, fs::file_size(path) - startpos);
	uint8_t* buf = new uint8_t[len];
	std::ifstream file(path, std::ios::in | std::ios::binary);
	file.seekg(startpos);
	file.read(reinterpret_cast<char*>(buf), len);
	res->writeBytes(buf, len);
	delete[] buf;
}

void TestRunnerEngineData::FileWriteByteArray(SystemState* sys, const tiny_string& filename, ByteArray* data, uint32_t startpos, uint32_t length, bool isfullpath)
{
	path_t path((std::string)filename);
	if (!fs::is_regular_file(path))
		return;
	size_t len =
	(
		length != UINT32_MAX &&
		startpos + length <= data->getLength()
	) ? length : data->getLength();
	uint8_t* buf = data->getBuffer(data->getLength(), false);
	std::ofstream file(path, std::ios::out | std::ios::binary);
	file.write(reinterpret_cast<char*>(buf+startpos), len);
}

bool TestRunnerEngineData::FileCreateDirectory(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	path_t path((std::string)filename);
	bool ret = fs::create_directory(path);
	fs::permissions
	(
		path,
		fs::perms::group_write | fs::perms::others_write,
		fs::perm_options::remove
	);
	return ret;
}

bool TestRunnerEngineData::FilGetDirectoryListing(SystemState* sys, const tiny_string &filename, bool isfullpath, std::vector<tiny_string>& filelist)
{
	path_t path((std::string)filename);
	if (!fs::is_directory(path))
		return false;

	std::transform
	(
		fs::directory_iterator {path},
		fs::directory_iterator {},
		std::back_inserter(filelist),
		[](const fs::directory_entry& entry)
		{
			return entry.path().string();
		}
	);
	return true;
}

bool TestRunnerEngineData::FilePathIsAbsolute(const tiny_string& filename)
{
	return path_t((std::string)filename).is_absolute();
}

void TestRunnerEngineData::setClipboardText(const std::string txt)
{
	runner->clipboard = txt;
}


double TestRunnerEngineData::getScreenDPI()
{
	return runner->options.screenDPI.valueOr(72.0);
}
