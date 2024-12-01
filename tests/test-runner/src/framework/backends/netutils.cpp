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

#include <lightspark/backends/streamcache.h>

#include "framework/backends/netutils.h"

Downloader* TestRunnerDownloadManager::download
(
	const URLInfo& url,
	_R<StreamCache> cache,
	ILoadable* owner
)
{
	TestRunnerDownloader* downloader = nullptr;

	if (url.isEmpty() || url.getProtocol() == "file")
		downloader = new TestRunnerDownloader(url.getPath(), cache, owner, true);
	else
		downloader = new TestRunnerDownloader(url.getParsedURL(), cache, owner, false);

	addDownloader(downloader);
	downloader->run(basePath, url.isEmpty());
	return downloader;
}

Downloader* TestRunnerDownloadManager::downloadWithData
(
	const URLInfo& url,
	_R<StreamCache> cache,
	const std::vector<uint8_t>& data,
	const std::list<tiny_string>& headers,
	ILoadable* owner
)
{
	TestRunnerDownloader* downloader = nullptr;

	if (url.getProtocol() == "file")
		downloader = new TestRunnerDownloader(url.getPath(), cache, data, headers, owner, true);
	else
		downloader = new TestRunnerDownloader(url.getParsedURL(), cache, data, headers, owner, false);

	addDownloader(downloader);
	downloader->run(basePath, false);
	return downloader;
}

void TestRunnerDownloadManager::destroy(Downloader* downloader)
{
	if (removeDownloader(downloader))
		delete downloader;
}

void TestRunnerDownloader::run(const path_t& basePath, bool dataGeneration)
{
	if (url.empty() && isLocal && !dataGeneration)
	{
		setFailed();
		return;
	}

	auto query = URLInfo(url).getQuery();
	if (query.startsWith("debug-"))
	{
		auto debugQuery = query.stripPrefix("debug-");

		if (debugQuery == "success")
		{
			requestStatus = 200;
			redirected = false;
			tiny_string body("Hello, World!");
			append((uint8_t*)body.raw_buf(), body.numBytes());
			return;
		}
		// TODO: Add support for more detailed errors.
		else if (debugQuery.startsWith("error"))
		{
			requestStatus = 0;
			setFailed();
			return;
		}
	}

	tiny_string path = url;

	if (!isLocal)
	{
		// Convert a URL such as `https://localhost/foo/bar` into a local
		// path `basePath/localhost/foo/bar`.
		URLInfo fsURL(url);
		path_t _path = basePath;
		try
		{
			if (!fsURL.getHostname().empty())
				_path /= fsURL.getHostname();

			if (!fsURL.getPath().empty())
				_path /= URLInfo::decode
				(
					fsURL.getPath().substr(1, UINT32_MAX),
					URLInfo::ENCODE_URI
				);
		}
		catch (std::exception& e)
		{
			std::cerr << "Failed to convert URL " << url << " to a local path." << std::endl;
			setFailed();
			return;
		}
		path = _path.generic_string();
	}

	//If the caching is selected, we override the normal behaviour and use the local file as the cache file
	//This prevents unneeded copying of the file's data
	auto fileCache = dynamic_cast<FileStreamCache*>(cache.getPtr());
	if (fileCache != nullptr)
	{
		try
		{
			fileCache->useExistingFile(path);
			//Report that we've downloaded everything already
			length = fileCache->getReceivedLength();
			notifyOwnerAboutBytesLoaded();
			notifyOwnerAboutBytesTotal();
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception when using existing file: " << url << ", reason: " << e.what() << std::endl;
			setFailed();
			return;
		}
	}
	//Otherwise we follow the normal procedure
	else
	{
		std::ifstream file
		(
			path_t((std::string)URLInfo::decode(path, URLInfo::ENCODE_ESCAPE)),
			std::ios::in | std::ios::binary
		);

		if (file.is_open())
		{
			file.seekg(0, std::ios::end);
			{
				setLength(file.tellg());
			}
			file.seekg(0, std::ios::beg);

			std::vector<char> buffer(length);
			file.read(buffer.data(), buffer.size());
			append((uint8_t*)buffer.data(), file.gcount());

			if (file.fail() || cache->hasFailed())
			{
				std::cerr << "Failed to read from file: " << url << std::endl;
				setFailed();
				return;
			}
		}
		else
		{
			std::cerr << "Couldn't open file: " << url << std::endl;
			setFailed();
			return;
		}
	}

	//Notify the downloader no more data should be expected
	if (!dataGeneration)
		setFinished();
}
