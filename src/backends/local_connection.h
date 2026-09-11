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

#ifndef BACKENDS_LOCAL_CONNECTION_H
#define BACKENDS_LOCAL_CONNECTION_H 1

#include <unordered_map>
#include <vector>

#include "interfaces/threading.h"
#include "threading.h"
#include "utils/optional.h"
#include "utils/span.h"

// Based off Ruffle's `local_connection` crate.

namespace lightspark
{

class AMFValue;
class SystemState;
class URLInfo;
class tiny_string;

class LocalConnectionObject : public AVMObject
<
	AVM1LocalConnection,
	ASLocalConnection
>
{
private:
	using Base = AVMObject<AVM1LocalConnection, ASLocalConnection>;
public:
	using Base::Base;

	~LocalConnectionObject() {}
	void sendStatus(const tiny_string& status);
	void runMethod
	(
		const tiny_string& methodName,
		const Span<AMFValue>& args
	);
};

class LocalConnectionMessage
{
public:
	struct Message
	{
		tiny_string name;
		tiny_string methodName;
		std::vector<AMFValue> args;
	};
private:
	using LCMessage = LocalConnectionMessage;
	using LCObj = LocalConnectionObject;

	LCObj obj;
	Optional<Message> msg;
public:
	LCMessage() = default;
	LCMessage(const LCObj& _obj) : obj(_obj) {}
	LCMessage
	(
		const LCObj& obj,
		const tiny_string& name,
		const tiny_string& methodName,
		const Span<AMFValue>& args
	) : _obj(obj), msg(name, methodName, args) {}

	const LCObj& getObj() const { return obj; }
	Optional<const Message&> getMsg() const { return msg.asRef(); }
};

class LocalConnectionManager
{
private:
	using LCObj = LocalConnectionObject;
	using LCMessage = LocalConnectionMessage;

	SystemState* sys;
	Thread thread;
	Cond msgCond;
	Mutex msgMutex;
	Mutex connectionMutex;
	std::unordered_map<tiny_string, LCObj> connections;
	std::vector<LCMessage> messages;

	Optional<const LCObj&> getConnection
	(
		const tiny_string& name
	) const;

	Optional<const LCObj&> getConnectionNoLock
	(
		const tiny_string& name
	) const;

	Optional<LCMessage> getMsg();
	int worker();
public:
	LocalConnectionManager(SystemState* _sys);
	~LocalConnectionManager()

	void stop();
	void wait();

	void connect
	(
		const URLInfo& url,
		const LCObj& obj,
		const tiny_string& name
	);

	void send
	(
		const URLInfo& url,
		const LCObj& obj,
		const tiny_string& name,
		const tiny_string& methodName,
		const Span<AMFValue>& args
	);

	void disconnect(const tiny_string& name);
	static tiny_string getSuperdomain(const URLInfo& url);
	static tiny_string getDomain(const URLInfo& url);
};

}
#endif /* BACKENDS_LOCAL_CONNECTION_H */
