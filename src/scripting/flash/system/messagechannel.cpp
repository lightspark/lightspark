/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/concurrent/Condition.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

void MessageChannel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, state);
	c->setDeclaredMethodByQName("messageAvailable","",Class<IFunction>::getFunction(c->getSystemState(),messageAvailable,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("addEventListener","",Class<IFunction>::getFunction(c->getSystemState(),_addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",Class<IFunction>::getFunction(c->getSystemState(),_removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("receive","",Class<IFunction>::getFunction(c->getSystemState(),receive,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

void MessageChannel::finalize()
{
	while (!messagequeue.empty())
		messagequeue.pop();
}
ASFUNCTIONBODY_GETTER(MessageChannel, state);

ASFUNCTIONBODY_ATOM(MessageChannel,messageAvailable)
{
	MessageChannel* th=asAtomHandler::as<MessageChannel>(obj);
	Locker l(th->messagequeuemutex);
	ret = asAtomHandler::fromBool(!th->messagequeue.empty());
}

ASFUNCTIONBODY_ATOM(MessageChannel,_addEventListener)
{
	MessageChannel* th=asAtomHandler::as<MessageChannel>(obj);
	th->worker = th->receiver->isPrimordial ? nullptr : th->receiver.getPtr();
	EventDispatcher::addEventListener(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(MessageChannel,_removeEventListener)
{
	EventDispatcher::removeEventListener(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(MessageChannel,_toString)
{
	EventDispatcher::_toString(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(MessageChannel,close)
{
	MessageChannel* th=asAtomHandler::as<MessageChannel>(obj);
	if (th->state == "open")
		th->state="closing";
}
ASFUNCTIONBODY_ATOM(MessageChannel,receive)
{
	MessageChannel* th=asAtomHandler::as<MessageChannel>(obj);
	bool blockUntilReceived;
	ARG_UNPACK_ATOM(blockUntilReceived,false);
	if (blockUntilReceived)
		LOG(LOG_NOT_IMPLEMENTED,"MessageChannel.send ignores parameter blockUntilReceived");
	Locker l(th->messagequeuemutex);
	if (th->messagequeue.empty())
	{
		ret = asAtomHandler::nullAtom;
		return;
	}
	_NR<ASObject> msg = th->messagequeue.front();
	th->messagequeue.pop();
	if (msg->is<ASWorker>()
			|| msg->is<MessageChannel>()
			|| (msg->is<ByteArray>() && msg->as<ByteArray>()->shareable)
			|| msg->is<ASMutex>()
			|| msg->is<ASCondition>()
			)
		ret = asAtomHandler::fromObjectNoPrimitive(msg.getPtr());
	else
	{
		ret = msg->as<ByteArray>()->readObject();
	}
}
ASFUNCTIONBODY_ATOM(MessageChannel,send)
{
	MessageChannel* th=asAtomHandler::as<MessageChannel>(obj);
	if (th->state!= "open")
		throw Class<IOError>::getInstanceS(sys,"MessageChannel closed");
	_NR<ASObject> msg;
	int queueLimit;
	ARG_UNPACK_ATOM(msg)(queueLimit,-1);
	if (msg.isNull())
		return;
	if (queueLimit != -1)
		LOG(LOG_NOT_IMPLEMENTED,"MessageChannel.send ignores parameter queueLimit");
	Locker l(th->messagequeuemutex);
	if (msg->is<ASWorker>()
			|| msg->is<MessageChannel>()
			|| (msg->is<ByteArray>() && msg->as<ByteArray>()->shareable)
			|| msg->is<ASMutex>()
			|| msg->is<ASCondition>()
			)
		th->messagequeue.push(msg);
	else
	{
		ByteArray* b = Class<ByteArray>::getInstanceSNoArgs(sys);
		b->writeObject(msg.getPtr());
		b->setPosition(0);
		th->messagequeue.push(_MR(b));
	}
	th->incRef();
	getVm(sys)->addEvent(_MR(th),_MR(Class<Event>::getInstanceS(sys,"channelMessage")));
}
