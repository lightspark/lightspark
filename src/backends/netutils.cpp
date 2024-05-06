/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2012  Timon Van Overveldt (timonvo@gmail.com)

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

#include "version.h"
#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "swf.h"
#include "backends/config.h"
#include "backends/netutils.h"
#include "backends/rtmputils.h"
#include "backends/streamcache.h"
#include "compat.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

using namespace lightspark;

/**
 * \brief Download manager constructor
 *
 * Can only be called from within a derived class
 */
DownloadManager::DownloadManager()
{
}

/**
 * \brief Download manager destructor
 */
DownloadManager::~DownloadManager()
{
}

/**
 * \brief Stops all the currently running downloaders
 */
void DownloadManager::stopAll()
{
	Locker l(mutex);

	std::list<Downloader*>::iterator it=downloaders.begin();
	for(;it!=downloaders.end();++it)
		(*it)->stop();
}

/**
 * \brief Destroyes all the pending downloads, must be called in the destructor of each derived class
 *
 * Traverses the list of active downloaders, calling \c stop() and \c destroy() on all of them.
 * If the downloader is already destroyed, destroy() won't do anything (no double delete).
 * Waits for the mutex before proceeding.
 * \see Downloader::stop()
 * \see Downloader::destroy()
 */
void DownloadManager::cleanUp()
{
	Locker l(mutex);

	while(!downloaders.empty())
	{
		std::list<Downloader*>::iterator it=downloaders.begin();
		//cleanUp should only happen after stopAll has been called
		assert((*it)->hasFinished());

		l.release();
		destroy(*it);
		l.acquire();
	}
}

/**
 * \brief Destroy a Downloader.
 *
 * Destroy a given \c Downloader.
 * \param downloader A pointer to the \c Downloader to be destroyed.
 * \see DownloadManager::download()
 */
void StandaloneDownloadManager::destroy(Downloader* downloader)
{
	//If the downloader was still in the active-downloader list, delete it
	if(removeDownloader(downloader))
	{
		downloader->waitForTermination();
		ThreadedDownloader* thd=dynamic_cast<ThreadedDownloader*>(downloader);
		if(thd)
			thd->waitFencing();
		delete downloader;
	}
}

/**
 * \brief Add a Downloader to the active downloads list
 *
 * Waits for the mutex at start and releases the mutex when finished.
 */
void DownloadManager::addDownloader(Downloader* downloader)
{
	Locker l(mutex);
	downloaders.push_back(downloader);
}

/**
 * \brief Remove a Downloader from the active downloads list
 *
 * Waits for the mutex at start and releases the mutex when finished.
 */
bool DownloadManager::removeDownloader(Downloader* downloader)
{
	Locker l(mutex);

	for(std::list<Downloader*>::iterator it=downloaders.begin(); it!=downloaders.end(); ++it)
	{
		if((*it) == downloader)
		{
			downloaders.erase(it);
			return true;
		}
	}
	return false;
}

/**
 * \brief Standalone download manager constructor.
 *
 * The standalone download manager produces \c ThreadedDownloader-type \c Downloaders.
 * It should only be used in the standalone version of LS.
 */
StandaloneDownloadManager::StandaloneDownloadManager()
{
	type = STANDALONE;
}

StandaloneDownloadManager::~StandaloneDownloadManager()
{
	cleanUp();
}

/**
 * \brief Create a Downloader for an URL.
 *
 * Returns a pointer to a newly created \c Downloader for the given URL.
 * \param[in] url The URL (as a \c URLInfo) the \c Downloader is requested for
 * \param[in] cached Whether or not to disk-cache the download (default=false)
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
Downloader* StandaloneDownloadManager::download(const URLInfo& url, _R<StreamCache> cache, ILoadable* owner)
{
	bool cached = dynamic_cast<FileStreamCache *>(cache.getPtr()) != NULL;
	LOG(LOG_INFO, "NET: STANDALONE: DownloadManager::download '" << url.getParsedURL()
			<< "'" << (cached ? " - cached" : ""));
	ThreadedDownloader* downloader;
	// empty URL means data is generated from calls to NetStream::appendBytes
	if(url.isEmpty())
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: Data generation mode");
		downloader=new LocalDownloader(url.getPath(), cache, owner,true);
	}
	else if(url.getProtocol() == "file")
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: local file");
		downloader=new LocalDownloader(url.getPath(), cache, owner);
	}
	else if(url.getProtocol().substr(0, 4) == "rtmp")
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: RTMP stream");
		downloader=new RTMPDownloader(url.getParsedURL(), cache, url.getStream(), owner);
	}
	else
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: remote file");
		downloader=new CurlDownloader(url.getParsedURL(), cache, owner);
	}
	downloader->enableFencingWaiting();
	addDownloader(downloader);
	getSys()->addDownloadJob(downloader);
	return downloader;
}

/**
 * \brief Create a Downloader for an URL and send data to the host
 *
 * Returns a pointer to a newly created \c Downloader for the given URL.
 * \param[in] url The URL (as a \c URLInfo) the \c Downloader is requested for
 * \param[in] data The binary data to send to the host
 * \param[in] headers Request headers in the full form, f.e. "Content-Type: ..."
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
Downloader* StandaloneDownloadManager::downloadWithData(const URLInfo& url, _R<StreamCache> cache, 
		const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers, ILoadable* owner)
{
	LOG(LOG_INFO, "NET: STANDALONE: DownloadManager::downloadWithData '" << url.getParsedURL());
	ThreadedDownloader* downloader;
	if(url.getProtocol() == "file")
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: local file - Ignoring data field");
		downloader=new LocalDownloader(url.getPath(), cache, owner);
	}
	else if(url.getProtocol() == "rtmpe")
		throw RunTimeException("RTMPE does not support additional data");
	else
	{
		LOG(LOG_INFO, "NET: STANDALONE: DownloadManager: remote file");
		downloader=new CurlDownloader(url.getParsedURL(), cache, data, headers, owner);
	}
	downloader->enableFencingWaiting();
	addDownloader(downloader);
	getSys()->addDownloadJob(downloader);
	return downloader;
}

/**
 * \brief Downloader constructor.
 *
 * Constructor for the Downloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cache StreamCache instance for caching this download.
 */
Downloader::Downloader(const tiny_string& _url, _R<StreamCache> _cache, ILoadable* o):
	url(_url),originalURL(url),                                   //PROPERTIES
	cache(_cache),                                                //CACHING
	owner(o),                                                     //PROGRESS
	redirected(false),requestStatus(0),                           //HTTP REDIR, STATUS & HEADERS
	length(0),                                                    //DOWNLOADED DATA
	emptyanswer(false)
{
}

/**
 * \brief Downloader constructor.
 *
 * Constructor for the Downloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
Downloader::Downloader(const tiny_string& _url, _R<StreamCache> _cache, const std::vector<uint8_t>& _data, const std::list<tiny_string>& h, ILoadable* o):
	url(_url),originalURL(url),                                      //PROPERTIES
	cache(_cache),                                                   //CACHING
	owner(o),                                                        //PROGRESS
	redirected(false),requestStatus(0),requestHeaders(h),data(_data),//HTTP REDIR, STATUS & HEADERS
	length(0),                                                       //DOWNLOADED DATA
	emptyanswer(false)
{
}

/**
 * \brief Downloader destructor.
 *
 * Destructor for the Downloader class. Can only be called from derived/friend classes (DownloadManager)
 * Calls \c waitForTermination() and waits for the mutex before proceeding.
 * \see Downloader::waitForTermination()
 */
Downloader::~Downloader()
{
}

/**
 * \brief Marks the downloader as failed
 *
 * Sets the \c failed and finished flag to \c true, sets the final length and 
 * signals \c dataAvailable if it is being waited for.
 * It also signals \c terminated to mark the end of the download.
 * A download should finish be either calling \c setFailed() or \c setFinished(), not both.
 * \post \c length == \c receivedLength
 */
void Downloader::setFailed()
{
	length = cache->markFinished(true);
}

/**
 * \brief Marks the downloader as finished
 *
 * Marks the downloader as finished, sets the final length and 
 * signals \c dataAvailable if it is being waited for.
 * It also signals \c terminated to mark the end of the download.
 * A download should finish be either calling \c setFailed() or \c setFinished(), not both.
 * \post \c length == \c receivedLength
 */
void Downloader::setFinished()
{
	length = cache->markFinished();
	LOG(LOG_INFO,"download finished:"<<url<<" "<<length);
}

/**
 * \brief Set the expected length of the download
 *
 * Sets the expected length of the download.
 * Can be called multiple times if the length isn't known up front (reallocating the buffer on the fly).
 */
void Downloader::setLength(uint32_t _length)
{
	//Set the length
	length=_length;

	cache->reserve(length);

	if (cache->getNotifyLoader())
		notifyOwnerAboutBytesTotal();
}

/**
 * \brief Appends data to the buffer
 *
 * Appends a given amount of received data to the buffer/cache.
 * This method will grow the expected length of the download on-the-fly as needed.
 * So when \c length == 0 this call will call \c setLength(added)
 * Waits for mutex at start and releases mutex when finished.
 * \post \c buffer/cache contains the added data
 * \post \c length = \c receivedLength + \c added
 * \see Downloader::setLength()
 */
void Downloader::append(uint8_t* buf, uint32_t added)
{
	if(added==0)
		return;

	cache->append((unsigned char *)buf, added);
	if (!cache->getNotifyLoader())
		return;
	if (cache->getReceivedLength() > length)
		setLength(cache->getReceivedLength());

	notifyOwnerAboutBytesLoaded();
}

/**
 * \brief Parse a string of multiple headers.
 *
 * Parses a string of multiple headers.
 * Header lines are expected to be seperated by \n
 * Calls \c parseHeader on every individual header.
 * \see Downloader::parseHeader()
 */
void Downloader::parseHeaders(const char* _headers, bool _setLength)
{
	if(_headers == nullptr)
		return;

	std::string headersStr(_headers);
	size_t cursor = 0;
	size_t newLinePos = headersStr.find("\n");
	while(newLinePos != std::string::npos)
	{
		if(headersStr[cursor] == '\n')
			cursor++;
		parseHeader(headersStr.substr(cursor, newLinePos-cursor), _setLength);
		cursor = newLinePos;
		newLinePos = headersStr.find("\n", cursor+1);
	}
}

/**
 * \brief Parse a string containing a single header.
 *
 * Parse a string containing a single header.
 * The header line is not expected to contain a newline character.
 * Waits for mutex at start and releases mutex when finished.
 */
void Downloader::parseHeader(std::string header, bool _setLength)
{
	if(header.substr(0, 9) == "HTTP/1.1 " || header.substr(0, 9) == "HTTP/1.0 " || header.substr(0, 7) == "HTTP/2 ") 
	{
		std::string status = header.substr(0, 7) == "HTTP/2 " ? header.substr(7, 3) : header.substr(9, 3);
		requestStatus = atoi(status.c_str());
		//HTTP error or server error or proxy error, let's fail
		//TODO: shouldn't we fetch the data anyway
		if(getRequestStatus()/100 == 4 || 
				getRequestStatus()/100 == 5 || 
				getRequestStatus()/100 == 6)
		{
			setFailed();
		}
		else if(getRequestStatus()/100 == 3) {;} //HTTP redirect
		else if(getRequestStatus()/100 == 2) //HTTP OK
		{
			if (getRequestStatus() == 204)
				emptyanswer = true;
		} 
	}
	else
	{
		std::string headerName;
		std::string headerValue;
		size_t colonPos;
		colonPos = header.find(":");
		if(colonPos != std::string::npos)
		{
			headerName = header.substr(0, colonPos);
			if(header[colonPos+1] == ' ')
				headerValue = header.substr(colonPos+2, header.length()-colonPos-1);
			else
				headerValue = header.substr(colonPos+1, header.length()-colonPos);

			std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
			//std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);
			headers.insert(std::make_pair(tiny_string(headerName), tiny_string(headerValue)));

			//Set the new real URL when we are being redirected
			if(getRequestStatus()/100 == 3 && headerName == "location")
			{
				LOG(LOG_INFO, "NET: redirect detected");
				setRedirected(URLInfo(url).goToURL(tiny_string(headerValue)).getParsedURL());
			}
			if(headerName == "content-length")
			{
				//Now read the length and allocate the byteArray
				//Only read the length when we're not redirecting
				if(getRequestStatus()/100 != 3)
				{
					int len = atoi(headerValue.c_str());
					if (len == 0)
						emptyanswer = true;
					setLength(len);
					return;
				}
			}
		}
	}
}

/**
 * \brief Forces the download to stop
 *
 * Sets the \c failed and finished flag to \c true, sets the final length and signals \c dataAvailable
 * \post \c failed == true & finished == true
 * \post \c length == receivedLength
 * \post \c dataAvailable is signalled
 * \post \c waitingForTermination == \c false
 * \post Signals \c terminated
 */
void Downloader::stop()
{
	cache->markFinished(true);
	length = cache->getReceivedLength();
}

void Downloader::notifyOwnerAboutBytesTotal() const
{
	if(owner)
		owner->setBytesTotal(length);
}

void Downloader::notifyOwnerAboutBytesLoaded() const
{
	if(owner)
		owner->setBytesLoaded(cache->getReceivedLength());
}

void ThreadedDownloader::enableFencingWaiting()
{
	RELEASE_WRITE(fenceState,true);
}

/**
 * \brief The jobFence for ThreadedDownloader.
 *
 * This is the very last thing \c ThreadPool does with the \c ThreadedDownloader.
 * \post The \c fenceState is set to false
 */
void ThreadedDownloader::jobFence()
{
	RELEASE_WRITE(fenceState,false);
}

void ThreadedDownloader::waitFencing()
{
	while(fenceState);
}

/**
 * \brief Constructor for the ThreadedDownloader class.
 *
 * Constructor for the ThreadedDownloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 */
ThreadedDownloader::ThreadedDownloader(const tiny_string& url, _R <StreamCache> cache, ILoadable* o):
	Downloader(url, cache, o),fenceState(false)
{
}

/**
 * \brief Constructor for the ThreadedDownloader class.
 *
 * Constructor for the ThreadedDownloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
ThreadedDownloader::ThreadedDownloader(const tiny_string& url, _R<StreamCache> cache,
				       const std::vector<uint8_t>& data,
				       const std::list<tiny_string>& headers, ILoadable* o):
	Downloader(url, cache, data, headers, o),fenceState(false)
{
}

/**
 * \brief Destructor for the ThreadedDownloader class.
 *
 * Waits for the \c fenced signal.
 * \post \c fenced signalled was handled
 *
ThreadedDownloader::~ThreadedDownloader()
{
	sem_wait(&fenced);
	//-- Fenced signalled
}*/

/**
 * \brief Constructor for the CurlDownloader class.
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 */
CurlDownloader::CurlDownloader(const tiny_string& _url, _R<StreamCache> _cache, ILoadable* o):
	ThreadedDownloader(_url, _cache, o)
{
}

/**
 * \brief Constructor for the CurlDownloader class.
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
CurlDownloader::CurlDownloader(const tiny_string& _url, _R<StreamCache> _cache,
			       const std::vector<uint8_t>& _data,
			       const std::list<tiny_string>& _headers, ILoadable* o):
	ThreadedDownloader(_url, _cache, _data, _headers, o)
{
}

/**
 * \brief Called by \c IThreadJob::stop to abort this thread.
 * Calls \c Downloader::stop.
 * \see Downloader::stop()
 */
void CurlDownloader::threadAbort()
{
	Downloader::stop();
}

/**
 * \brief Called by \c ThreadPool to start executing this thread
 */
void CurlDownloader::execute()
{
	if(url.empty())
	{
		setFailed();
		return;
	}
	LOG(LOG_INFO, "NET: CurlDownloader::execute: reading remote file: " << url.raw_buf());
#ifdef ENABLE_CURL
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.raw_buf());
		//Needed for thread-safety reasons.
		//This makes CURL not respect DNS resolving timeouts.
		//TODO: openssl needs locking callbacks. We should implement these.
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		//ALlow self-signed and incorrect certificates.
		//TODO: decide if we should allow them.
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
#if LIBCURL_VERSION_NUM>= 0x072000
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
#else
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
#endif
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//Its probably a good idea to limit redirections, 100 should be more than enough
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
		// TODO use same useragent as adobe
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Lightspark " VERSION);
		// Empty string means that CURL will decompress if the
		// server send a compressed file. (This has been
		// renamed to CURLOPT_ACCEPT_ENCODING in newer CURL,
		// we use the old name to support the old versions.)
		curl_easy_setopt(curl, CURLOPT_ENCODING, "");
		if (URLInfo(url).sameHost(getSys()->mainClip->getOrigin()) &&
		    !getSys()->getCookies().empty())
			curl_easy_setopt(curl, CURLOPT_COOKIE, getSys()->getCookies().c_str());

		struct curl_slist *headerList=NULL;
		bool hasContentType=false;
		if(!requestHeaders.empty())
		{
			std::list<tiny_string>::const_iterator it;
			for(it=requestHeaders.begin(); it!=requestHeaders.end(); ++it)
			{
				headerList=curl_slist_append(headerList, it->raw_buf());
				hasContentType |= it->lowercase().startsWith("content-type:");
			}
		}

		if(!data.empty())
		{
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			//data is const, it would not be invalidated
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &data.front());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
		}

		if(headerList)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		res = curl_easy_perform(curl);

		curl_slist_free_all(headerList);

		curl_easy_cleanup(curl);
		if(res!=0)
		{
			setFailed();
			return;
		}
	}
	else
	{
		setFailed();
		return;
	}
#else
	//ENABLE_CURL not defined
	LOG(LOG_ERROR,"NET: CURL not enabled in this build. Downloader will always fail.");
	setFailed();
	return;
#endif
	//Notify the downloader no more data should be expected
	setFinished();
}

/**
 * \brief Progress callback for CURL
 *
 * Gets called by CURL to report progress. Can be used to signal CURL to stop downloading.
 */
int CurlDownloader::progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(clientp);
	return th->threadAborting || th->cache->hasFailed();
}

/**
 * \brief Data callback for CURL
 *
 * Gets called by CURL when data is available to handle.
 * \see Downloader::append()
 */
size_t CurlDownloader::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	size_t added=size*nmemb;
	if(th->getRequestStatus()/100 == 2 || th->getRequestStatus()/100 == 3)
		th->append((uint8_t*)buffer,added);
	return added;
}

/**
 * \brief Header callback for CURL
 *
 * Gets called by CURL when a header needs to be handled.
 * \see Downloader::parseHeader()
 */
size_t CurlDownloader::write_header(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);

	std::string header((char*) buffer);
	//Strip newlines
	header = header.substr(0, header.find("\r\n"));
	header = header.substr(0, header.find("\n"));
	//We haven't set the length of the download uet, so set it from the headers
	th->parseHeader(header, true);

	return size*nmemb;
}

/**
 * \brief Constructor for the LocalDownloader class
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 */
LocalDownloader::LocalDownloader(const tiny_string& _url, _R<StreamCache> _cache, ILoadable* o, bool dataGeneration):
	ThreadedDownloader(_url, _cache, o),dataGenerationMode(dataGeneration)
{
}

/**
 * \brief Called by \c IThreadJob::stop to abort this thread.
 * Calls \c Downloader::stop.
 * \see Downloader::stop()
 */
void LocalDownloader::threadAbort()
{
	Downloader::stop();
}

/**
 * \brief Called by \c ThreadPool to start executing this thread
 * Waits for the mutex at start and releases the mutex when finished when the download is cached.
 * \see Downloader::append()
 * \see Downloader::openExistingCache()
 */
void LocalDownloader::execute()
{
	if(url.empty() && !dataGenerationMode)
	{
		setFailed();
		return;
	}
	else
	{
		LOG(LOG_INFO, "NET: LocalDownloader::execute: reading local file: " << url.raw_buf());
		//If the caching is selected, we override the normal behaviour and use the local file as the cache file
		//This prevents unneeded copying of the file's data

		FileStreamCache *fileCache = dynamic_cast<FileStreamCache *>(cache.getPtr());
		if (fileCache)
		{
			try
			{
				fileCache->useExistingFile(url);
				//Report that we've downloaded everything already
				length = fileCache->getReceivedLength();
				notifyOwnerAboutBytesLoaded();
				notifyOwnerAboutBytesTotal();
			}
			catch(exception& e)
			{
				LOG(LOG_ERROR, "Exception when using existing file: "<<url<<" "<<e.what());
				setFailed();
				return;
			}

		}
		//Otherwise we follow the normal procedure
		else {
			tiny_string s("file://");
			s += URLInfo::decode(url, URLInfo::ENCODE_ESCAPE);
			char* filepath =g_filename_from_uri(s.raw_buf(),nullptr,nullptr);
			std::ifstream file;
			file.open(filepath, std::ios::in|std::ios::binary);
			free(filepath);

			if(file.is_open())
			{
				file.seekg(0, std::ios::end);
				{
					setLength(file.tellg());
				}
				file.seekg(0, std::ios::beg);

				char buffer[bufSize];

				bool readFailed = 0;
				while(!file.eof())
				{
					if(file.fail() || cache->hasFailed())
					{
						readFailed = 1;
						break;
					}
					file.read(buffer, bufSize);
					append((uint8_t *) buffer, file.gcount());
				}
				if(readFailed)
				{
					LOG(LOG_ERROR, "NET: LocalDownloader::execute: reading from local file failed: " << url.raw_buf());
					setFailed();
					return;
				}
				file.close();
			}
			else
			{
				LOG(LOG_ERROR, "NET: LocalDownloader::execute: could not open local file: " << url.raw_buf());
				setFailed();
				return;
			}
		}
	}
	//Notify the downloader no more data should be expected
	if(!dataGenerationMode)
		setFinished();
}

DownloaderThreadBase::DownloaderThreadBase(_NR<URLRequest> request, IDownloaderThreadListener* _listener): listener(_listener), downloader(NULL)
{
	assert(listener);
	if(!request.isNull())
	{
		url=request->getRequestURL();
		requestHeaders=request->getHeaders();
		request->getPostData(postData);
	}
}

bool DownloaderThreadBase::createDownloader(_R<StreamCache> cache,
					    _NR<EventDispatcher> dispatcher,
					    ILoadable* owner,
					    bool checkPolicyFile)
{
	if(checkPolicyFile)
	{
		SecurityManager::EVALUATIONRESULT evaluationResult = \
			dispatcher->getSystemState()->securityManager->evaluatePoliciesURL(url, true);
		if(threadAborting)
			return false;
		if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			getVm(dispatcher->getSystemState())->addEvent(dispatcher,_MR(Class<SecurityErrorEvent>::getInstanceS(dispatcher->getInstanceWorker(),"SecurityError: "
												 "connection to domain not allowed by securityManager")));
			return false;
		}
	}
	if (url.getPathFile().endsWith("swz"))
	{
		// url points to an adobe signed library which we cannot handle, so we abort here
		LOG(LOG_NOT_IMPLEMENTED,"we don't handle adobe signed libraries");
		getVm(dispatcher->getSystemState())->addEvent(dispatcher,_MR(Class<IOErrorEvent>::getInstanceS(dispatcher->getInstanceWorker())));
		return false;
	}

	if(threadAborting)
		return false;

	//All the checks passed, create the downloader
	if(postData.empty())
	{
		//This is a GET request
		downloader=dispatcher->getSystemState()->downloadManager->download(url, cache, owner);
	}
	else
	{
		downloader=dispatcher->getSystemState()->downloadManager->downloadWithData(url, cache, postData, requestHeaders, owner);
	}

	return true;
}

void DownloaderThreadBase::jobFence()
{
	//Get a copy of the downloader, do hold the lock less time.
	//It's safe to set this->downloader to NULL, this is the last function that will
	//be called over this thread job
	Downloader* d=nullptr;
	{
		Locker l(downloaderLock);
		d=downloader;
		downloader=nullptr;
	}
	if(d)
		getSys()->downloadManager->destroy(d);

	listener->threadFinished(this);
}

void DownloaderThreadBase::threadAbort()
{
	//We have to stop the downloader
	Locker l(downloaderLock);
	if(downloader != nullptr)
		downloader->stop();
	threadAborting=true;
}
