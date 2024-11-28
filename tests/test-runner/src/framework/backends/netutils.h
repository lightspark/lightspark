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

#ifndef FRAMEWORK_BACKENDS_NETUTILS_H
#define FRAMEWORK_BACKENDS_NETUTILS_H 1

#include <cstdint>
#include <list>
#include <vector>

#include <lightspark/backends/netutils.h>
#include <lightspark/backends/streamcache.h>
#include <lightspark/backends/urlutils.h>
#include <lightspark/tiny_string.h>

#include "utils/filesystem_overloads.h"

using namespace lightspark;

class TestRunnerDownloadManager : public DownloadManager
{
private:
	path_t basePath;
public:
	TestRunnerDownloadManager(const path_t& _basePath) : basePath(_basePath)
	{
		type = STANDALONE;
	}
	~TestRunnerDownloadManager() { cleanUp(); }

	Downloader* download
	(
		const URLInfo& url,
		_R<StreamCache> cache,
		ILoadable* owner
	) override;
	Downloader* downloadWithData
	(
		const URLInfo& url,
		_R<StreamCache> cache,
		const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers,
		ILoadable* owner
	) override;
	void destroy(Downloader* downloader) override;
};

class TestRunnerDownloader : public Downloader
{
private:
	bool isLocal;
public:
	TestRunnerDownloader
	(
		const tiny_string& url,
		_R<StreamCache> cache,
		ILoadable* owner,
		bool _isLocal
	) : Downloader(url, cache, owner), isLocal(_isLocal) {}
	TestRunnerDownloader
	(
		const tiny_string& url,
		_R<StreamCache> cache,
		const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers,
		ILoadable* owner,
		bool _isLocal
	) : Downloader(url, cache, data, headers, owner), isLocal(_isLocal) {}

	void run(const path_t& basePath, bool dataGeneration);
};

#endif /* FRAMEWORK_BACKENDS_NETUTILS_H */
