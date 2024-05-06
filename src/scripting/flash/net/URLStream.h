/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_NET_URLSTREAM_H
#define SCRIPTING_FLASH_NET_URLSTREAM_H 1

#include "interfaces/backends/netutils.h"
#include "compat.h"
#include "asobject.h"
#include "scripting/flash/events/flashevents.h"
#include "backends/urlutils.h"
#include "backends/netutils.h"
#include "scripting/flash/utils/flashutils.h"

namespace lightspark
{

class URLStream;

class URLStreamThread : public DownloaderThreadBase, public ILoadable
{
private:
	_R<URLStream> loader;
	_R<ByteArray> data;
	std::streambuf *streambuffer;
	uint64_t timestamp_last_progress;
	uint32_t bytes_total;
	void execute() override;
public:
	URLStreamThread(_R<URLRequest> request, _R<URLStream> ldr, _R<ByteArray> bytes);
	void setBytesTotal(uint32_t b) override;
	void setBytesLoaded(uint32_t b) override;
};

class URLStream: public EventDispatcher, public IDataInput, public IDownloaderThreadListener
{
private:
	URLInfo url;
	_NR<ByteArray> data;
	URLStreamThread *job;
	Mutex spinlock;
	void finalize() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getEndian);
	ASFUNCTION_ATOM(_setEndian);
	ASFUNCTION_ATOM(_getObjectEncoding);
	ASFUNCTION_ATOM(_setObjectEncoding);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(bytesAvailable);
	ASFUNCTION_ATOM(readBoolean);
	ASFUNCTION_ATOM(readByte);
	ASFUNCTION_ATOM(readBytes);
	ASFUNCTION_ATOM(readDouble);
	ASFUNCTION_ATOM(readFloat);
	ASFUNCTION_ATOM(readInt);
	ASFUNCTION_ATOM(readMultiByte);
	ASFUNCTION_ATOM(readObject);
	ASFUNCTION_ATOM(readShort);
	ASFUNCTION_ATOM(readUnsignedByte);
	ASFUNCTION_ATOM(readUnsignedInt);
	ASFUNCTION_ATOM(readUnsignedShort);
	ASFUNCTION_ATOM(readUTF);
	ASFUNCTION_ATOM(readUTFBytes);
	ASPROPERTY_GETTER(bool,connected);
public:
	URLStream(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void threadFinished(IThreadJob *job) override;
};

}
#endif /* SCRIPTING_FLASH_NET_URLSTREAM_H */
