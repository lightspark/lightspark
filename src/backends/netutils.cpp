/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "scripting/abc.h"
#include "swf.h"
#include "backends/config.h"
#include "backends/netutils.h"
#include "backends/rtmputils.h"
#include "compat.h"
#include "backends/security.h"
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
	Mutex::Lock l(mutex);

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
	Mutex::Lock l(mutex);

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
	Mutex::Lock l(mutex);
	downloaders.push_back(downloader);
}

/**
 * \brief Remove a Downloader from the active downloads list
 *
 * Waits for the mutex at start and releases the mutex when finished.
 */
bool DownloadManager::removeDownloader(Downloader* downloader)
{
	Mutex::Lock l(mutex);

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
Downloader* StandaloneDownloadManager::download(const URLInfo& url, bool cached, ILoadable* owner)
{
	LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager::download '") << url.getParsedURL()
			<< "'" << (cached ? _(" - cached") : ""));
	ThreadedDownloader* downloader;
	if(url.getProtocol() == "file")
	{
		LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager: local file"));
		downloader=new LocalDownloader(url.getPath(), cached, owner);
	}
	else if(url.getProtocol().substr(0, 4) == "rtmp")
	{
		LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager: RTMP stream"));
		downloader=new RTMPDownloader(url.getParsedURL(), url.getStream(), owner);
	}
	else
	{
		LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager: remote file"));
		downloader=new CurlDownloader(url.getParsedURL(), cached, owner);
	}
	downloader->enableFencingWaiting();
	addDownloader(downloader);
	getSys()->addJob(downloader);
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
Downloader* StandaloneDownloadManager::downloadWithData(const URLInfo& url, const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers, ILoadable* owner)
{
	LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager::downloadWithData '") << url.getParsedURL());
	ThreadedDownloader* downloader;
	if(url.getProtocol() == "file")
	{
		LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager: local file - Ignoring data field"));
		downloader=new LocalDownloader(url.getPath(), false, owner);
	}
	else if(url.getProtocol() == "rtmpe")
		throw RunTimeException("RTMPE does not support additional data");
	else
	{
		LOG(LOG_INFO, _("NET: STANDALONE: DownloadManager: remote file"));
		downloader=new CurlDownloader(url.getParsedURL(), data, headers, owner);
	}
	downloader->enableFencingWaiting();
	addDownloader(downloader);
	getSys()->addJob(downloader);
	return downloader;
}

/**
 * \brief Downloader constructor.
 *
 * Constructor for the Downloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 */
Downloader::Downloader(const tiny_string& _url, bool _cached, ILoadable* o):
	cacheOpened(0),dataAvailable(0),terminated(0),hasTerminated(false),cacheHasOpened(false), //LOCKING
	waitingForCache(false),waitingForData(false),waitingForTermination(false), //STATUS
	forceStop(true),failed(false),finished(false),                //FLAGS
	url(_url),originalURL(url),                                   //PROPERTIES
	buffer(NULL),stableBuffer(NULL),                              //BUFFERING
	owner(o),                                                     //PROGRESS
	cachePos(0),cacheSize(0),keepCache(false),cached(_cached),    //CACHING
	redirected(false),requestStatus(0),                           //HTTP REDIR, STATUS & HEADERS
	length(0),receivedLength(0)                                   //DOWNLOADED DATA
{
	setg(NULL,NULL,NULL);
}

/**
 * \brief Downloader constructor.
 *
 * Constructor for the Downloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
Downloader::Downloader(const tiny_string& _url, const std::vector<uint8_t>& _data, const std::list<tiny_string>& h, ILoadable* o):
	cacheOpened(0),dataAvailable(0),terminated(0),hasTerminated(false),cacheHasOpened(false), //LOCKING
	waitingForCache(false),waitingForData(false),waitingForTermination(false), //STATUS
	forceStop(true),failed(false),finished(false),                   //FLAGS
	url(_url),originalURL(url),                                      //PROPERTIES
	buffer(NULL),stableBuffer(NULL),                                 //BUFFERING
	owner(o),                                                        //PROGRESS
	cachePos(0),cacheSize(0),keepCache(false),cached(false),         //CACHING
	redirected(false),requestStatus(0),requestHeaders(h),data(_data),//HTTP REDIR, STATUS & HEADERS
	length(0),receivedLength(0)                                      //DOWNLOADED DATA
{
	setg(NULL,NULL,NULL);
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
	waitForTermination();

	Mutex::Lock l(mutex);

	if(cached)
	{
		if(cache.is_open())
			cache.close();
		if(!keepCache && cacheFilename != "")
			unlink(cacheFilename.raw_buf());
	}
	if(buffer != NULL)
	{
		free(buffer);
	}
	if(stableBuffer != NULL && stableBuffer!=buffer)
	{
		free(stableBuffer);
	}
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API when there is no more data to read.
 * Waits for the mutex at start and releases the mutex when finished.
 * \throw RunTimeException Cache file could not be read
 */
Downloader::int_type Downloader::underflow()
{
	Mutex::Lock l(mutex);
	//Let's see if the other buffer contains new data
	syncBuffers();
	if(egptr()-gptr()>0)
	{
		//There is data already
		return *(uint8_t*)gptr();
	}
	const unsigned int startOffset=getOffset();
	const unsigned int startReceivedLength=receivedLength;
	assert(startOffset<=startReceivedLength);
	//If we have read all available data
	if(startReceivedLength==startOffset)
	{
		//The download has failed or has finished
		if(failed || finished)
			return EOF;
		//We haven't reached the end of the download, more bytes should follow
		else
		{
			waitForData_locked();
			syncBuffers();

			//Check if we haven't failed or finished (and there wasn't any new data)
			if(failed || (finished && startReceivedLength==receivedLength))
				return EOF;
		}
	}

	//We should have an initialized buffer here since there is some data
	assert_and_throw(buffer != NULL);
	//Temporary pointers to new streambuf read positions
	char* begin;
	char* cur;
	char* end;
	//Index in the buffer pointing to the data to be returned
	uint32_t index;

	if(cached)
	{
		waitForCache();

		size_t newCacheSize = receivedLength-(cachePos+cacheSize);
		if(newCacheSize > cacheMaxSize)
			newCacheSize = cacheMaxSize;

		//Move the start of our new window to the end of our last window
		cachePos = cachePos+cacheSize;
		cacheSize = newCacheSize;
		//Seek to the start of our new window
		cache.seekg(cachePos);
		//Read into our buffer window
		cache.read((char*)stableBuffer, cacheSize);
		if(cache.fail())
		{
			throw RunTimeException(_("Downloader::underflow: reading from cache file failed"));
		}

		begin=(char*)stableBuffer;
		cur=(char*)stableBuffer;
		end=(char*)stableBuffer+cacheSize;
		index=0;

	}
	else
	{
		begin=(char*)stableBuffer;
		cur=(char*)stableBuffer+startOffset;
		end=(char*)stableBuffer+receivedLength;
		index=startOffset;
	}

	//If we've failed, don't bother any more
	if(failed)
		return EOF;

	//Set our new iterators in the buffer (begin, cursor, end)
	setg(begin, cur, end);

	//Cast to unsigned, otherwise 0xff would become eof
	return (unsigned char)stableBuffer[index];
}

/**
  * Internal function to synchronize oldBuffer and buffer
  *
  * \pre Must be called from a function called by the streambuf API
  */
void Downloader::syncBuffers()
{
	if(stableBuffer!=buffer)
	{
		//The buffer have been changed
		free(stableBuffer);
		stableBuffer=buffer;
		//Remember the relative positions of the input pointers
		intptr_t curPos = (intptr_t) (gptr()-eback());
		intptr_t curLen = (intptr_t) (egptr()-eback());
		//Do some pointer arithmetic to point the input pointers to the right places in the new buffer
		setg((char*)stableBuffer,(char*)(stableBuffer+curPos),(char*)(stableBuffer+curLen));
	}
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API to seek to an absolute position
 * Mutex must be locked on entry.
 * \throw RunTimeException Cache file could not be read
 */
Downloader::pos_type Downloader::seekpos(pos_type pos, std::ios_base::openmode mode)
{
	assert_and_throw(mode==std::ios_base::in);
	assert_and_throw(buffer && stableBuffer);

	syncBuffers();

	// read from stream until we have enough data
	uint32_t tmplen = receivedLength;
	while (!hasTerminated && pos > receivedLength) 
	{
		waitForData_locked();
		syncBuffers();
		if (tmplen == receivedLength)
			break; // no new data read
		tmplen = receivedLength;
	}
	
	if(cached)
	{
		waitForCache();

		//The requested position is inside our current window
		if(pos >= cachePos && pos <= cachePos+cacheSize)
		{
			//Just move our cursor to the correct position in our window
			setg((char*)stableBuffer, (char*)stableBuffer+pos-cachePos, (char*)stableBuffer+cacheSize);
		}
		//The requested position is outside our current window
		else if(pos <= receivedLength)
		{
			cachePos = pos;
			cacheSize = receivedLength-pos;
			if(cacheSize > cacheMaxSize)
				cacheSize = cacheMaxSize;

			//Seek to the requested position
			cache.seekg(cachePos);
			//Read into our window
			cache.read((char*)stableBuffer, cacheSize);
			if(cache.fail())
				throw RunTimeException(_("Downloader::seekpos: reading from cache file failed"));

			//Our window starts at position pos
			setg((char*) stableBuffer, (char*) stableBuffer, ((char*) stableBuffer)+cacheSize);
		}
		//The requested position is bigger then our current amount of available data
		else if(pos > receivedLength)
			return -1;
	}
	else
	{
		//The requested position is valid
		if(pos <= receivedLength)
			setg((char*)stableBuffer,(char*)stableBuffer+pos,(char*)stableBuffer+receivedLength);
		//The requested position is bigger then our current amount of available data
		else
			return -1;
	}

	return pos;
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API to seek to a relative position
 * Waits for the mutex at start and releases the mutex when finished.
 */
Downloader::pos_type Downloader::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
	assert_and_throw(mode==std::ios_base::in);
	assert_and_throw(buffer != NULL);

	Mutex::Lock l(mutex);
	if (off != 0)
	{
		switch (dir)
		{
			case std::ios_base::beg:
				seekpos(off,mode);
				break;
			case std::ios_base::cur:
			{
				pos_type tmp = getOffset();
				seekpos(tmp+off,mode);
				break;
			}
			case std::ios_base::end:
				l.release();
				waitForTermination();
				l.acquire();
				if (finished)
					seekpos(length+off,mode);
				break;
			default:
				break;
		}
	}

	return getOffset();
}

/**
 * \brief Get the position of the read cursor in the (virtual) downloaded data
 *
 * Get the position of the read cursor in the (virtual) downloaded data.
 * If downloading to memory this method returns the position of the read cursor in the buffer.
 * If downloading to a cache file, this method returns the position of the read cursor in the buffer
 * + the position of the buffer window into the cache file.
 */
Downloader::pos_type Downloader::getOffset() const
{
	pos_type ret = gptr()-eback();
	if(cached)
		ret+=cachePos;
	return ret;
}

/**
 * \brief Marks the downloader as failed
 *
 * Sets the \c failed and finished flag to \c true, sets the final length and 
 * signals \c dataAvailable if it is being waited for.
 * It also signals \c terminated to mark the end of the download.
 * A download should finish be either calling \c setFailed() or \c setFinished(), not both.
 * \post \c failed == \c true & \c finished == \c true
 * \post \c length == \c receivedLength
 * \post Signals \c dataAvailable if it is being waited for (\c waitingForData == \c true). 
 * \post \c waitingForTermination == \c false
 * \post Signals \c terminated
 */
void Downloader::setFailed()
{
	failed=true;
	finished = true;
	//Set the final length
	length = receivedLength;

	//If we are waiting for data to become available, signal dataAvailable
	if(waitingForData)
	{
		waitingForData = false;
		dataAvailable.signal();
	}

	waitingForTermination = false;
	terminated.signal();
}

/**
 * \brief Marks the downloader as finished
 *
 * Marks the downloader as finished, sets the final length and 
 * signals \c dataAvailable if it is being waited for.
 * It also signals \c terminated to mark the end of the download.
 * A download should finish be either calling \c setFailed() or \c setFinished(), not both.
 * \post \c finished == \ctrue
 * \post \c length == \c receivedLength
 * \post Signals \c dataAvailable if it is being waited for (\c waitingForData == true).
 * \post \c waitingForTermination == \c false
 * \post Signals \c terminated
 */
void Downloader::setFinished()
{
	finished=true;
	//Set the final length
	length = receivedLength;

	//If we are waiting for data to become available, signal dataAvailable
	if(waitingForData)
	{
		waitingForData = false;
		dataAvailable.signal();
	}

	waitingForTermination = false;
	terminated.signal();
}

/**
 * \brief (Re)allocates the buffer
 *
 * (Re)allocates the buffer to a given size
 * Waits for mutex at start and releases mutex when finished.
 * \post \c buffer is (re)allocated
 * mutex must be locked on entry
 */
void Downloader::allocateBuffer(size_t size)
{
	//Create buffer
	if(buffer == NULL)
	{
		buffer = (uint8_t*) calloc(size, sizeof(uint8_t));
		stableBuffer = buffer;
		setg((char*)buffer,(char*)buffer,(char*)buffer);
	}
	//If the buffer already exists, reallocate
	else
	{
		assert(!cached);
		intptr_t curLen = receivedLength;
		//We have to extend the buffer, so create a new one
		if(stableBuffer!=buffer)
		{
			//We're already filling a different buffer from the one used to read
			//Extend it!
			buffer = (uint8_t*)realloc(buffer,size);
		}
		else
		{
			//Create a different buffer
			buffer = (uint8_t*) calloc(size, sizeof(uint8_t));
			//Copy the stableBuffer into this
			memcpy(buffer,stableBuffer,curLen);
		}
		//Synchronization of the buffers will be done at the first chance
	}
}

/**
 * \brief Creates & opens a temporary cache file
 *
 * Creates a temporary cache file in /tmp and calls \c openExistingCache() with that file.
 * Waits for mutex at start and releases mutex when finished.
 * \throw RunTimeException Temporary file could not be created
 * \throw RunTimeException Called when the downloader isn't cached or when the cache is already open
 * \see Downloader::openExistingCache()
 * mutex must be hold prior calling
 */
void Downloader::openCache()
{
	//Only act if the downloader is cached and the cache hasn't been opened yet
	if(cached && !cache.is_open())
	{
		//Create a temporary file(name)
		std::string cacheFilenameS = Config::getConfig()->getCacheDirectory() + "/" + Config::getConfig()->getCachePrefix() + "XXXXXX";
		char* cacheFilenameC = g_newa(char,cacheFilenameS.length()+1);
		strncpy(cacheFilenameC, cacheFilenameS.c_str(), cacheFilenameS.length());
		cacheFilenameC[cacheFilenameS.length()] = '\0';
		//char cacheFilenameC[30] = "/tmp/lightsparkdownloadXXXXXX";
		//strcpy(cacheFilenameC, "/tmp/lightsparkdownloadXXXXXX");
		int fd = g_mkstemp(cacheFilenameC);
		if(fd == -1)
			throw RunTimeException(_("Downloader::openCache: cannot create temporary file"));
		//We are using fstream to read/write to the cache, so we don't need this FD
		close(fd);

		//Let the openExistingCache function handle the rest
		openExistingCache(tiny_string(cacheFilenameC, true));
	}
	else
		throw RunTimeException(_("Downloader::openCache: downloader isn't cached or called twice"));
}

/**
 * \brief Opens an existing cache file
 *
 * Opens an existing cache file, allocates the buffer and signals \c cacheOpened.
 * Waits for mutex at start and releases mutex when finished.
 * \post \c cacheFilename is set
 * \post \c cache file is opened
 * \post \c buffer is initialized
 * \post \c cacheOpened is signalled
 * \throw RunTimeException File could not be opened
 * \throw RunTimeException Called when the downloader isn't cached or when the cache is already open
 * \see Downloader::allocateBuffer()
 * mutex must be hold on entering
 */
void Downloader::openExistingCache(tiny_string filename)
{
	//Only act if the downloader is cached and the cache hasn't been opened yet
	if(cached && !cache.is_open())
	{
		//Save the filename
		cacheFilename = filename;

		//Open the cache file
		cache.open(cacheFilename.raw_buf(), std::fstream::binary | std::fstream::in | std::fstream::out);
		if(!cache.is_open())
			throw RunTimeException(_("Downloader::openCache: cannot open temporary cache file"));

		allocateBuffer(cacheMaxSize);

		LOG(LOG_INFO, _("NET: Downloading to cache file: ") << cacheFilename);

		cacheOpened.signal();
	}
	else
		throw RunTimeException(_("Downloader::openCache: downloader isn't cached or called twice"));
}

/**
 * \brief Set the expected length of the download
 *
 * Sets the expected length of the download.
 * Can be called multiple times if the length isn't known up front (reallocating the buffer on the fly).
 * Waits for mutex at start and releases mutex when finished.
 * \post \c buffer is (re)allocated 
 * mutex must be hold prior calling
 */
void Downloader::setLength(uint32_t _length)
{
	//Set the length
	length=_length;

	//The first call to this function should open the cache
	if(cached)
	{
		if(!cache.is_open())
			openCache();
	}
	else
	{
		if(buffer == NULL)
			LOG(LOG_INFO, _("NET: Downloading to memory"));
		allocateBuffer(length);
	}
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

	Mutex::Lock l(mutex);

	//If the added data would overflow the buffer, grow it
	if((receivedLength+added)>length)
	{
		uint32_t newLength;
		assert(length>=receivedLength);
		//If reallocating the buffer ask for a minimum amount of space
		if((receivedLength+added)-length > bufferMinGrowth)
			newLength = receivedLength + added;
		else
			newLength = length + bufferMinGrowth;
		assert(newLength>=receivedLength+added);

		setLength(newLength);
	}

	if(cached)
	{
		//Seek to where we last wrote data
		cache.seekp(receivedLength);
		cache.write((char*) buf, added);
	}
	else
		memcpy(buffer+receivedLength, buf, added);

	receivedLength += added;

	if(waitingForData)
	{
		waitingForData = false;
		dataAvailable.signal();
	}

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
	if(_headers == NULL)
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
	Mutex::Lock l(mutex);

	if(header.substr(0, 9) == "HTTP/1.1 " || header.substr(0, 9) == "HTTP/1.0 ") 
	{
		std::string status = header.substr(9, 3);
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
		else if(getRequestStatus()/100 == 2) {;} //HTTP OK
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
				LOG(LOG_INFO, _("NET: redirect detected"));
				setRedirected(URLInfo(url).goToURL(tiny_string(headerValue)).getParsedURL());
			}
			if(headerName == "content-length")
			{
				//Now read the length and allocate the byteArray
				//Only read the length when we're not redirecting
				if(getRequestStatus()/100 != 3)
				{
					setLength(atoi(headerValue.c_str()));
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
	failed = true;
	finished = true;
	length = receivedLength;

	waitingForData = false;
	dataAvailable.signal();

	waitingForTermination = false;
	terminated.signal();
}

/**
 * \brief Wait for the cache file to be opened
 *
 * If \c !cacheHasOpened: wait for the \c cacheOpened signal and set \c cacheHasOpened to \c true
 * Waits for the mutex at start and releases the mutex when finished.
 * \post \c cacheOpened signals has been handled
 * \post \c cacheHasOpened = true
 * mutex must be locked on entry
 */
void Downloader::waitForCache()
{
	if(!cacheHasOpened)
	{
		waitingForCache = true;

		mutex.unlock();
		cacheOpened.wait();
		mutex.lock();

		cacheHasOpened = true;
	}
}

/**
 * \brief Wait for data to become available
 *
 * Wait for data to become available.
 * Waits for the mutex at start and releases the mutex when finished.
 * \post \c dataAvailable signal has been handled
 */
void Downloader::waitForData_locked()
{
	waitingForData = true;
	mutex.unlock();
	dataAvailable.wait();
	mutex.lock();
}

/**
 * \brief Wait for termination of the downloader
 *
 * If \c getSys()->isShuttingDown(), calls \c setFailed() and returns.
 * Otherwise if \c !hasTerminated: wait for the \c terminated signal and set \c hasTerminated to \c true
 * Waits for the mutex at start and releases the mutex when finished.
 * \post \c terminated signal has been handled
 * \post \c hasTerminated = true
 */
void Downloader::waitForTermination()
{
	Mutex::Lock l(mutex);
	if(getSys()->isShuttingDown())
	{
		setFailed();
		return;
	}

	if(!hasTerminated)
	{
		waitingForTermination = true;

		l.release();
		terminated.wait();
		l.acquire();

		hasTerminated = true;
	}
}

void Downloader::notifyOwnerAboutBytesTotal() const
{
	if(owner)
		owner->setBytesTotal(length);
}

void Downloader::notifyOwnerAboutBytesLoaded() const
{
	if(owner)
		owner->setBytesLoaded(receivedLength);
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
ThreadedDownloader::ThreadedDownloader(const tiny_string& url, bool cached, ILoadable* o):
	Downloader(url, cached, o),fenceState(false)
{
}

/**
 * \brief Constructor for the ThreadedDownloader class.
 *
 * Constructor for the ThreadedDownloader class. Can only be called from derived classes.
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
ThreadedDownloader::ThreadedDownloader(const tiny_string& url, const std::vector<uint8_t>& data,
				       const std::list<tiny_string>& headers, ILoadable* o):
	Downloader(url, data, headers, o),fenceState(false)
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
CurlDownloader::CurlDownloader(const tiny_string& _url, bool _cached, ILoadable* o):
	ThreadedDownloader(_url, _cached, o)
{
}

/**
 * \brief Constructor for the CurlDownloader class.
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] data Additional data to send to the host
 */
CurlDownloader::CurlDownloader(const tiny_string& _url, const std::vector<uint8_t>& _data,
			       const std::list<tiny_string>& _headers, ILoadable* o):
	ThreadedDownloader(_url, _data, _headers, o)
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
	LOG(LOG_INFO, _("NET: CurlDownloader::execute: reading remote file: ") << url.raw_buf());
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
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//Its probably a good idea to limit redirections, 100 should be more than enough
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
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

			//For POST it's mandatory to set the Content-Type
			assert(hasContentType);
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
	LOG(LOG_ERROR,_("NET: CURL not enabled in this build. Downloader will always fail."));
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
	return th->threadAborting || th->failed;
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
	if(th->getRequestStatus()/100 == 2)
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
LocalDownloader::LocalDownloader(const tiny_string& _url, bool _cached, ILoadable* o):ThreadedDownloader(_url, _cached, o)
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
	if(url.empty())
	{
		setFailed();
		return;
	}
	else
	{
		LOG(LOG_INFO, _("NET: LocalDownloader::execute: reading local file: ") << url.raw_buf());
		//If the caching is selected, we override the normal behaviour and use the local file as the cache file
		//This prevents unneeded copying of the file's data
		if(isCached())
		{
			Mutex::Lock l(mutex);
			//Make sure we don't delete the local file afterwards
			keepCache = true;

			openExistingCache(url);

			cache.seekg(0, std::ios::end);
			//Report that we've downloaded everything already
			length = cache.tellg();
			receivedLength = length;
			notifyOwnerAboutBytesLoaded();
			notifyOwnerAboutBytesTotal();
		}
		//Otherwise we follow the normal procedure
		else {
			std::ifstream file;
			file.open(url.raw_buf(), std::ios::in|std::ios::binary);

			if(file.is_open())
			{
				file.seekg(0, std::ios::end);
				{
					Mutex::Lock l(mutex);
					setLength(file.tellg());
				}
				file.seekg(0, std::ios::beg);

				char buffer[bufSize];

				bool readFailed = 0;
				while(!file.eof())
				{
					if(file.fail() || hasFailed())
					{
						readFailed = 1;
						break;
					}
					file.read(buffer, bufSize);
					append((uint8_t *) buffer, file.gcount());
				}
				if(readFailed)
				{
					LOG(LOG_ERROR, _("NET: LocalDownloader::execute: reading from local file failed: ") << url.raw_buf());
					setFailed();
					return;
				}
				file.close();
			}
			else
			{
				LOG(LOG_ERROR, _("NET: LocalDownloader::execute: could not open local file: ") << url.raw_buf());
				setFailed();
				return;
			}
		}
	}
	//Notify the downloader no more data should be expected
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

bool DownloaderThreadBase::createDownloader(bool cached,
					    _NR<EventDispatcher> dispatcher,
					    ILoadable* owner,
					    bool checkPolicyFile)
{
	if(checkPolicyFile)
	{
		SecurityManager::EVALUATIONRESULT evaluationResult = \
			getSys()->securityManager->evaluatePoliciesURL(url, true);
		if(threadAborting)
			return false;
		if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			getVm()->addEvent(dispatcher,_MR(Class<SecurityErrorEvent>::getInstanceS("SecurityError: "
												 "connection to domain not allowed by securityManager")));
			return false;
		}
	}

	if(threadAborting)
		return false;

	//All the checks passed, create the downloader
	if(postData.empty())
	{
		//This is a GET request
		downloader=getSys()->downloadManager->download(url, cached, owner);
	}
	else
	{
		downloader=getSys()->downloadManager->downloadWithData(url, postData, requestHeaders, owner);
	}

	return true;
}

void DownloaderThreadBase::jobFence()
{
	//Get a copy of the downloader, do hold the lock less time.
	//It's safe to set this->downloader to NULL, this is the last function that will
	//be called over this thread job
	Downloader* d=NULL;
	{
		SpinlockLocker l(downloaderLock);
		d=downloader;
		downloader=NULL;
	}
	if(d)
		getSys()->downloadManager->destroy(d);

	listener->threadFinished(this);
}

void DownloaderThreadBase::threadAbort()
{
	//We have to stop the downloader
	SpinlockLocker l(downloaderLock);
	if(downloader != NULL)
		downloader->stop();
	threadAborting=true;
}
