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

#ifndef BACKENDS_STREAMCACHE_H
#define BACKENDS_STREAMCACHE_H 1

#include <list>
#include <istream>
#include <fstream>
#include <cstdint>
#include "threading.h"
#include "tiny_string.h"
#include "smartrefs.h"
#include "compat.h"

struct SDL_RWops;

namespace lightspark
{

/*
 * A single-writer-multiple-reader buffer for downloaded streams.
 *
 * Writing is done by one thread by calling append(), reading is done
 * through streambuf interface (constructred by createReader()) in a
 * separate thread. There can be several readers in multiple threads.
 *
 * This is an abstract base class.
 */
class SystemState;
class DLL_PUBLIC StreamCache : public RefCountable {
protected:
	StreamCache(SystemState* _sys);

	// stateMutex must be held while receivedLength, failed or
	// terminated are accessed
	Mutex stateMutex;
	// Amount of data already received
	size_t receivedLength;
	// Has the stream been completely downloaded or failed?
	bool failed:1;
	bool terminated:1;
	bool notifyLoader:1;
	SystemState* sys;

	// Wait until more than currentOffset bytes has been received
	// or until terminated
	void waitForData(size_t currentOffset);

	// Derived class implements this to store received data
	virtual void handleAppend(const unsigned char* buffer, size_t length)=0;

public:
	virtual ~StreamCache() {}

	// Gets the length of downloaded data
	size_t getReceivedLength() const { return receivedLength; }

	void setTerminated(bool t) { terminated = t; }
	bool hasTerminated() const { return terminated; }
	bool hasFailed() const { return failed; }
	bool getNotifyLoader() const { return notifyLoader; }
	void setNotifyLoader(bool notify) { notifyLoader = notify; }

	// Wait until the writer calls markTerminated
	void waitForTermination();

	// Set the expected length of the stream.
	// The default implementation does nothing, but the derived
	// classes can allocate memory here.
	virtual void reserve(size_t expectedLength) {}

	// Write new data to the buffer (writer thread)
	void append(const unsigned char* buffer, size_t length);

	// Writer should call this when all of the stream has been
	// append()'ed
	// returns the received length
	size_t markFinished(bool failed=false);

	// Create a streambuf for reading from this buffer (reader
	// thread). Every call returns a new, independent streambuf.
	// The caller must delete the returned value.
	virtual std::streambuf *createReader()=0;
	
	virtual void openForWriting() = 0;
};

class MemoryChunk;

/*
 * MemoryStreamCache buffers the stream in memory.
 */
class DLL_PUBLIC MemoryStreamCache : public StreamCache {
private:
	class DLL_LOCAL Reader : public std::streambuf {
	private:
		_R<MemoryStreamCache> buffer;
		// The chunk that is currently being read
		unsigned int chunkIndex;
		// Offset at the start of current chunk
		size_t chunkStartOffset;

		// Handles streambuf out-of-data events
		virtual int underflow();
		// Seeks to absolute position
		virtual std::streampos seekoff(std::streamoff, std::ios_base::seekdir, std::ios_base::openmode);
		// Seeks to relative position
		virtual std::streampos seekpos(std::streampos, std::ios_base::openmode);
		// Helper to get the current offset
		std::streampos getOffset() const;
	public:
		Reader(_R<MemoryStreamCache> b);
	};

	// Stream is stored into a sequence of memory chunks. The
	// chunks can grow, but they are never moved, so both readers
	// and writer can safely use them. However, the mutex must be
	// held when the container is accessed.
	Mutex chunkListMutex;
	std::vector<MemoryChunk *> chunks;

	// The last chunk, the next write will happen here (writer thread)
	MemoryChunk *writeChunk;

	// Variables controlling the memory allocation
	size_t nextChunkSize;
	static const size_t minChunkSize = 4*4096;

	// Allocate a new chunk, append it to chunks, update writeChunk
	void allocateChunk(size_t minLength) DLL_LOCAL;

	void handleAppend(const unsigned char* buffer, size_t length) override DLL_LOCAL;

public:
	MemoryStreamCache(SystemState *_sys);
	virtual ~MemoryStreamCache();

	void reserve(size_t expectedLength) override;

	std::streambuf *createReader() override;
	
	void openForWriting() override;
};

/*
 * FileStreamCache saves the stream in a temporary file.
 */
class DLL_PUBLIC FileStreamCache : public StreamCache {
private:
	/*
	 * Extends filebuf to wait for writer thread to supply more
	 * data when the end of temporary file is reached.
	 */
	class DLL_LOCAL Reader : public std::filebuf {
	private:
		_R<FileStreamCache> buffer;
		int underflow() override;
		std::streamsize xsgetn(char* s, std::streamsize n) override;
	public:
		Reader(_R<FileStreamCache> buffer);
	};

	//Cache filename
	tiny_string cacheFilename;
	//Cache fstream
	std::fstream cache;
	//True if the cache file doesn't need to be deleted on destruction
	bool keepCache:1;

	void openCache() DLL_LOCAL;
	void openExistingCache(const tiny_string& filename, bool forWriting=true) DLL_LOCAL;

	// Block until the cache file is opened by the writer stream
	bool waitForCache() DLL_LOCAL;

	void handleAppend(const unsigned char* buffer, size_t length) override DLL_LOCAL;

public:
	FileStreamCache(SystemState* _sys);
	virtual ~FileStreamCache();

	std::streambuf *createReader() override;

	// Use an existing file as cache. Must be called before append().
	void useExistingFile(const tiny_string& filename);
	void openForWriting() override;
};

// simple wrapper to use SDL_RWops as input for istream
// to let SDL deal with unicode filenames on windows
class DLL_PUBLIC lsfilereader: public std::filebuf
{
private:
	SDL_RWops* filehandler;
protected:
	std::streamsize xsgetn(char* s, std::streamsize n) override;
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
	std::streampos seekpos(std::streampos pos, std::ios_base::openmode) override;
public:
	lsfilereader(const char* filepath);
	~lsfilereader();
};

}

#endif // BACKENDS_STREAMCACHE_H
