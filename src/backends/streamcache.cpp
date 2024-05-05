/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Antti Ajanki (antti.ajanki@iki.fi)

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

#include <string.h>
#include <unistd.h>
#include <glib.h>
#include "backends/streamcache.h"
#include "backends/config.h"
#include "exceptions.h"
#include "logger.h"
#include "netutils.h"
#include "swf.h"
#include <SDL.h>

using namespace std;
using namespace lightspark;

StreamCache::StreamCache(SystemState* _sys)
  : receivedLength(0), failed(false), terminated(false),notifyLoader(true),sys(_sys)
{
}

size_t StreamCache::markFinished(bool _failed)
{
	Locker locker(stateMutex);
	if (terminated)
		return receivedLength;

	failed = _failed;
	terminated = true;
	sys->sendMainSignal();
	return receivedLength;
}

void StreamCache::waitForData(size_t currentOffset)
{
	stateMutex.lock();
	while (receivedLength <= currentOffset && !terminated)
	{
		stateMutex.unlock();
		sys->waitMainSignal();
		stateMutex.lock();
	}
	stateMutex.unlock();
}

void StreamCache::waitForTermination()
{
	stateMutex.lock();
	while (!terminated)
	{
		stateMutex.unlock();
		sys->waitMainSignal();
		stateMutex.lock();
	}
	stateMutex.unlock();
}

void StreamCache::append(const unsigned char* buffer, size_t length)
{
	if (!buffer || length == 0 || terminated)
		return;

	handleAppend(buffer, length);

	{
		stateMutex.lock();
		receivedLength += length;
		stateMutex.unlock();
		sys->sendMainSignal();
	}
}

class lightspark::MemoryChunk {
public:
	MemoryChunk(size_t len);
	~MemoryChunk();
	unsigned char * const buffer;
	const size_t capacity;
	ACQUIRE_RELEASE_VARIABLE(size_t, used);
};

MemoryChunk::MemoryChunk(size_t len) :
	buffer(new unsigned char[len]), capacity(len), used(0)
{
}

MemoryChunk::~MemoryChunk()
{
	delete[] buffer;
}

MemoryStreamCache::MemoryStreamCache(SystemState* _sys):StreamCache(_sys),
	writeChunk(nullptr), nextChunkSize(0)
{
}

MemoryStreamCache::~MemoryStreamCache()
{
	for (auto it=chunks.begin(); it!=chunks.end(); ++it)
		delete *it;
}

// Rounds val up to the next multiple of pow(2, s).
static size_t nextMultipleOf2Pow(size_t val, size_t s)
{
	return ((size_t)((double)(val-1) / (1 << s)) + 1) << s;
}

void MemoryStreamCache::allocateChunk(size_t minLength)
{
	size_t len = imax(imax(minLength, minChunkSize), nextChunkSize);
	len = nextMultipleOf2Pow(len, 12);
	assert(len >= minLength);
	nextChunkSize = len;

	writeChunk = new MemoryChunk(len);
	chunks.push_back(writeChunk);
}

void MemoryStreamCache::handleAppend(const unsigned char* data, size_t length)
{
	Locker locker(chunkListMutex);
	assert(length > 0);

	if (!writeChunk || (ACQUIRE_READ(writeChunk->used) >= writeChunk->capacity))
		allocateChunk(length);

	assert(writeChunk);

	size_t used = ACQUIRE_READ(writeChunk->used);
	if (writeChunk->capacity >= used + length)
	{
		// Data fits in to the current chunk
		memcpy(writeChunk->buffer + used, data, length);
		RELEASE_WRITE(writeChunk->used, used + length);
	}
	else
	{
		// Write as much as possible to the current buffer
		size_t unused = writeChunk->capacity - used;
		memcpy(writeChunk->buffer + used, data, unused);
		RELEASE_WRITE(writeChunk->used, writeChunk->capacity);

		// allocate a new chunk by a recursive call
		handleAppend(data + unused, length - unused);
	}
}

void MemoryStreamCache::reserve(size_t expectedLength)
{
	if (expectedLength <= receivedLength)
		return;

	// Set the next chunk to be large enough to hold the remaining
	// of the stream. The memory will be actually allocated in
	// append().
	size_t unused = 0;
	if (writeChunk)
		unused = writeChunk->capacity - ACQUIRE_READ(writeChunk->used);

	size_t allocated = receivedLength + unused;
	if (expectedLength > allocated)
		nextChunkSize = expectedLength - allocated;
}

std::streambuf *MemoryStreamCache::createReader()
{
	incRef();
	return new MemoryStreamCache::Reader(_MR(this));
}

void MemoryStreamCache::openForWriting()
{
	LOG(LOG_ERROR,"openForWriting not implemented in MemoryStreamCache");
}

MemoryStreamCache::Reader::Reader(_R<MemoryStreamCache> b) :
	buffer(b), chunkIndex(0), chunkStartOffset(0)
{
	setg(nullptr, nullptr, nullptr);
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API when there is no more data to read.
 */
int MemoryStreamCache::Reader::underflow()
{
	Locker locker(buffer->chunkListMutex);

	// Wait until there is some data to be read or until terminated
	bool hasMoreChunks = chunkIndex+1 < buffer->chunks.size();
	bool lastChunkHasBytes = (chunkIndex+1 == buffer->chunks.size()) && 
		((size_t)(gptr() - eback()) < ACQUIRE_READ(buffer->chunks[chunkIndex]->used));
	if (!buffer->hasTerminated() && !hasMoreChunks && !lastChunkHasBytes)
	{
		locker.release();
		buffer->waitForData(getOffset());
		locker.acquire();
	}

	if (chunkIndex >= buffer->chunks.size())
	{
		// This can only happen if the stream is terminated
		// before any data is written.
		assert(chunkIndex == 0);
		assert(buffer->hasTerminated());
		return EOF;
	}

	MemoryChunk *chunk = buffer->chunks[chunkIndex];
	size_t used = ACQUIRE_READ(chunk->used);
	unsigned char *cursor;
	unsigned char *end = chunk->buffer + used;

	if (gptr() == nullptr)
	{
		// On the first call gptr() is NULL (as set in the
		// constructor). Nothing has been read yet.
		cursor = chunk->buffer;
	}
	else if ((unsigned char*)gptr() < end)
	{
		// Data left in this chunk
		cursor = (unsigned char*)gptr();
	}
	else if (chunkIndex == buffer->chunks.size()-1)
	{
		// This is the last received chunk and there is no
		// data to be read => we're finished
		assert(buffer->hasTerminated());
		return EOF;
	}
	else
	{
		// Move to the next chunk
		chunkStartOffset = chunkStartOffset + used;
		chunkIndex++;

		// chunkIndex is still valid, because the case
		// size()-1 was handled above
		assert_and_throw(chunkIndex < buffer->chunks.size());

		chunk = buffer->chunks[chunkIndex];
		cursor = chunk->buffer;
		end = chunk->buffer + ACQUIRE_READ(chunk->used);
	}

	setg((char *)chunk->buffer, (char *)cursor, (char *)end);

	assert(cursor != end); // there is at least one byte to return
	return (int)cursor[0];
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API to seek to a relative position
 */
streampos MemoryStreamCache::Reader::seekoff(streamoff off, std::ios_base::seekdir dir,
					     std::ios_base::openmode mode)
{
	if (mode != std::ios_base::in)
		return -1;

	switch (dir)
	{
		case std::ios_base::beg:
			seekpos(off, mode);
			break;
		case std::ios_base::cur:
			// TODO: optimize by checking if the new
			// offset is in the current chunk
			seekpos(getOffset() + off, mode);
			break;
		case std::ios_base::end:
			buffer->waitForTermination();
			if (buffer->hasFailed())
				return -1;

			seekpos((streampos)buffer->getReceivedLength() + off, mode);
			break;
		default:
			break;
	}

	return getOffset();
}

/**
 * \brief Called by the streambuf API
 *
 * Called by the streambuf API to seek to an absolute position
 */
streampos MemoryStreamCache::Reader::seekpos(streampos pos, std::ios_base::openmode mode)
{
	if (mode != std::ios_base::in || pos < 0)
		return -1;

	if (pos >= (streampos)buffer->getReceivedLength())
		buffer->waitForTermination();

	Locker locker(buffer->chunkListMutex);
	streampos offset = 0;
	for (auto it=buffer->chunks.begin(); it!=buffer->chunks.end(); ++it)
	{
		streampos used = (streampos)ACQUIRE_READ((*it)->used);
		if (pos >= offset + used)
		{
			offset += used;
		}
		else
		{
			setg((char *)(*it)->buffer,
			     (char *)((*it)->buffer + (pos - offset)),
			     (char *)((*it)->buffer + used));
			return pos;
		}
	}

	return -1;
}

/**
 * Get the position of the read cursor in the (virtual) downloaded data.
 */
streampos MemoryStreamCache::Reader::getOffset() const
{
	return chunkStartOffset + (size_t)(gptr() - eback());
}

FileStreamCache::FileStreamCache(SystemState* _sys):StreamCache(_sys),
  keepCache(false)
{
}

FileStreamCache::~FileStreamCache()
{
	if (cache.is_open())
		cache.close();
	if (!keepCache && !cacheFilename.empty())
		unlink(cacheFilename.raw_buf());
}

void FileStreamCache::handleAppend(const unsigned char* buffer, size_t length)
{
	if (!cache.is_open())
		openCache();

	cache.write((const char*)buffer, length);
	cache.sync();
}

/**
 * \brief Creates & opens a temporary cache file
 *
 * Creates a temporary cache file in /tmp and calls \c openExistingCache() with that file.
 * Waits for mutex at start and releases mutex when finished.
 * \throw RunTimeException Temporary file could not be created
 * \throw RunTimeException Called when  the cache is already open
 * \see Downloader::openExistingCache()
 */
void FileStreamCache::openCache()
{
	if (cache.is_open())
	{
		markFinished(true);
		throw RunTimeException("FileStreamCache::openCache called twice");
	}

	//Create a temporary file(name)
	std::string cacheFilenameS = Config::getConfig()->getCacheDirectory() + G_DIR_SEPARATOR_S + Config::getConfig()->getCachePrefix() + "XXXXXX";
	char* cacheFilenameC = g_newa(char,cacheFilenameS.length()+1);
	strncpy(cacheFilenameC, cacheFilenameS.c_str(), cacheFilenameS.length());
	cacheFilenameC[cacheFilenameS.length()] = '\0';
	int fd = g_mkstemp(cacheFilenameC);
	if(fd == -1)
	{
		markFinished(true);
		throw RunTimeException("FileStreamCache::openCache: cannot create temporary file");
	}

	//We are using fstream to read/write to the cache, so we don't need this FD
	close(fd);

	//Let the openExistingCache function handle the rest
	openExistingCache(tiny_string(cacheFilenameC, true));
}

/**
 * \brief Opens an existing cache file
 *
 * Opens an existing cache file, allocates the buffer and signals \c cacheOpened.
 * \post \c cacheFilename is set
 * \post \c cache file is opened
 * \throw RunTimeException File could not be opened
 * \throw RunTimeException Called when the cache is already open
 */
void FileStreamCache::openExistingCache(const tiny_string& filename, bool forWriting)
{
	if (cache.is_open())
	{
		markFinished(true);
		throw RunTimeException("FileStreamCache::openCache called twice");
	}

	cacheFilename = filename;

	//Open the cache file
	ios_base::openmode mode;
	if (forWriting)
		mode = std::fstream::binary | std::fstream::out;
	else
		mode = std::fstream::binary | std::fstream::in;
	cache.open(cacheFilename.raw_buf(), mode);
	if (!cache.is_open())
	{
		markFinished(true);
		throw RunTimeException("FileStreamCache::openCache: cannot open temporary cache file");
	}

	LOG(LOG_INFO, "NET: Downloading to cache file: " << cacheFilename);
}

void FileStreamCache::useExistingFile(const tiny_string& filename)
{
	//Make sure we don't delete the local file afterwards
	keepCache = true;

	cacheFilename = filename;
	openExistingCache(filename, false);

	cache.seekg(0, std::ios::end);
	receivedLength = cache.tellg();

	// We already have the whole file
	markFinished();
}

void FileStreamCache::openForWriting()
{
	if (cache.is_open())
		return;
	openCache();
}

bool FileStreamCache::waitForCache()
{
	if (cache.is_open())
		return true;

	// Cache file will be opened when the first byte is received
	waitForData(0);

	return cache.is_open();
}

std::streambuf *FileStreamCache::createReader()
{
	if (!waitForCache())
	{
		LOG(LOG_ERROR,"could not open cache file");
		return NULL;
	}

	incRef();
	FileStreamCache::Reader *fbuf = new FileStreamCache::Reader(_MR(this));
	fbuf->open(cacheFilename.raw_buf(), std::fstream::binary | std::fstream::in);
	if (!fbuf->is_open())
	{
		delete fbuf;
		return NULL;
	}
	return fbuf;
}

FileStreamCache::Reader::Reader(_R<FileStreamCache> b) : buffer(b)
{
}

int FileStreamCache::Reader::underflow()
{
	if (!buffer->hasTerminated())
		buffer->waitForData(seekoff(0, ios_base::cur, ios_base::in));

	return filebuf::underflow();
}

streamsize FileStreamCache::Reader::xsgetn(char* s, streamsize n)
{
	streamsize read=filebuf::xsgetn(s, n);

	// If not enough data was available, wait for writer
	while (read < n)
	{
		buffer->waitForData(seekoff(0, ios_base::cur, ios_base::in));

		streamsize b = filebuf::xsgetn(s+read, n-read);

		// No more data after waiting, this must be EOF
		if (b == 0)
			return read;

		read += b;
	}

	return read;
}

streamsize lsfilereader::xsgetn(char *s, streamsize n)
{
	if (filehandler)
		return SDL_RWread(filehandler,s,1,n);
	else
	{
		LOG(LOG_ERROR,"reading from lsfilereader without file");
		return 0;
	}
}

streampos lsfilereader::seekoff(streamoff off, ios_base::seekdir way, ios_base::openmode which)
{
	if (!filehandler)
	{
		LOG(LOG_ERROR,"lsfilereader without file");
		return streampos(streamoff(-1));
	}
	switch (way)
	{
		case ios_base::beg:
			return SDL_RWseek(filehandler,off,RW_SEEK_SET);
		case ios_base::cur:
			return SDL_RWseek(filehandler,off,RW_SEEK_CUR);
		case ios_base::end:
			return SDL_RWseek(filehandler,off,RW_SEEK_END);
		default:
			LOG(LOG_ERROR,"unhandled value in lsfilereader.seekoff:"<<way);
			return streampos(streamoff(-1));
	}
}

streampos lsfilereader::seekpos(streampos pos, ios_base::openmode)
{
	if (!filehandler)
	{
		LOG(LOG_ERROR,"lsfilereader without file");
		return streampos(streamoff(-1));
	}
	return SDL_RWseek(filehandler,pos,RW_SEEK_SET);
}

lsfilereader::lsfilereader(const char *filepath)
{
	filehandler = SDL_RWFromFile(filepath, "rb");
}

lsfilereader::~lsfilereader()
{
	if (filehandler)
		SDL_RWclose(filehandler);
	filehandler=nullptr;
}
