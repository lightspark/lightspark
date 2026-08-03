/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef BACKENDS_NET_CONNECTION_H
#define BACKENDS_NET_CONNECTION_H 1

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "backends/netutils.h"
#include "compat.h"
#include "forwards/threading.h"
#include "gc/ptr.h"
#include "interfaces/backends/netutils.h"
#include "interfaces/threading.h"
#include "scripting/avm_object.h"
#include "smartrefs.h"
#include "utils/optional.h"
#include "tiny_string.h"

namespace lightspark
{

class AMF0Value;
class ASNetConnection;
class ASObject;
class AVM1NetConnection;
class AVM1Object;
class Downloader;
class Responder;

class NetConnection : public IThreadJob
{
	friend class NetStream;
public:
	using ResponderObject = AVMObject<Responder>;
private:
	enum PROXY_TYPE { PT_NONE, PT_HTTP, PT_CONNECT_ONLY, PT_CONNECT, PT_BEST };
	//Indicates whether the application is connected to a server through a persistent RMTP connection/HTTP server with Flash Remoting
	bool _connected;
	tiny_string protocol;
	URLInfo uri;
	//Data for remoting support (NetConnection::call)
	// The message data to be sent asynchronously
	Optional<AMF0Value> message;

	Mutex downloaderLock;
	Downloader* downloader;
	ResponderObject responder;
	size_t messageCount;
	//The connection is to a flash media server
	OBJECT_ENCODING objectEncoding;
	PROXY_TYPE proxyType;

	_NR<ASObject> client;
	size_t maxPeerConnections;
	tiny_string nearID;

	//IThreadJob interface
	void execute() override;
	void threadAbort() override;
	void jobFence() override;
public:
	NetConnection();

	virtual bool hasObj() const { return false; }
	virtual _NR<ASNetConnection> getASObj() const { return NullRef; }
	virtual _NGC<AVM1NetConnection> getAVM1Obj() const { return NullGc; }

	void connect(const URLInfo& url);
	void call
	(
		const tiny_string& cmd,
		const AMF0Value& msg,
		const ResponderObject& _responder = ResponderObject()
	);

	bool getConnected() const { return _connected; }
	const tiny_string& getConnectedProxyType() const;
	static const OBJECT_ENCODING& getDefaultObjectEncoding();
	static void setDefaultObjectEncoding(const OBJECT_ENCODING& encoding);
	const OBJECT_ENCODING& getObjectEncoding() const
	{
		return objectEncoding;
	}

	void setObjectEncoding(const OBJECT_ENCODING& encoding)
	{
		objectEncoding = encoding;
	}

	const tiny_string& getProtocol() const { return protocol; }
	const PROXY_TYPE& getProxyType() const { return proxyType; }
	void setProxyType(const PROXY_TYPE& type) { proxyType = type; }
	const URLInfo& getURI() const { return uri; }
	void close();

	_NR<ASObject> getClient() const { return client; }
	void setClient(_NR<ASObject> _client) { client = _client; }
	const tiny_string& getNearID() const { return nearID; }
	size_t getMaxPeerConnections() const { return maxPeerConnections; }
	void setMaxPeerConnections(size_t val)
	{
		maxPeerConnections = val;
	}
};

}
#endif /* BACKENDS_NET_CONNECTION_H */
