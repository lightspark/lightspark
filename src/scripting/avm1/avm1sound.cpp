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

#include "scripting/avm1/avm1sound.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"

using namespace std;
using namespace lightspark;

void AVM1Sound::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, avm1constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachSound","",Class<IFunction>::getFunction(c->getSystemState(),attachSound),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVolume","",Class<IFunction>::getFunction(c->getSystemState(),getVolume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVolume","",Class<IFunction>::getFunction(c->getSystemState(),setVolume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPan","",Class<IFunction>::getFunction(c->getSystemState(),getPan),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPan","",Class<IFunction>::getFunction(c->getSystemState(),setPan),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),getPosition),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("duration","",Class<IFunction>::getFunction(c->getSystemState(),_getter_length),GETTER_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,avm1constructor)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);

	ARG_UNPACK_ATOM(th->clip,NullRef);
}

ASFUNCTIONBODY_ATOM(AVM1Sound,attachSound)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (argslen == 0)
	{
		LOG(LOG_ERROR,"AVM1:Sound.attachSound called without argument");
		return;
	}
	uint32_t nameID = asAtomHandler::toStringId(args[0],th->getSystemState());
	DefineSoundTag *soundTag = dynamic_cast<DefineSoundTag *>(th->clip.isNull() || th->clip->getRoot().isNull() ? th->getSystemState()->mainClip->dictionaryLookupByName(nameID) : th->clip->getRoot()->dictionaryLookupByName(nameID));
	if (!soundTag)
	{
		LOG(LOG_ERROR,"AVM1:Sound.attachSound called for wrong tag:"<<asAtomHandler::toDebugString(args[0]));
		return;
	}
	
	soundTag->isAttached=true;
	th->soundData = _R<StreamCache>(soundTag->getSoundData().getPtr());
	th->soundData->incRef();
	th->format= AudioFormat(soundTag->getAudioCodec(), soundTag->getSampleRate(), soundTag->getChannels());
	th->container=false;
	th->length = soundTag->getDurationInMS();
	if (soundTag->soundchanel)
		th->soundChannel = soundTag->soundchanel;
	else
		th->soundChannel  = soundTag->soundchanel = _MR(Class<SoundChannel>::getInstanceS(sys,th->soundData, th->format,false));
	if(th->clip)
	{
		th->soundChannel->incRef();
		th->clip->setSound(th->soundChannel.getPtr(),false);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getVolume)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
		asAtomHandler::setNumber(ret,sys,th->soundChannel->soundTransform->volume*100);
	else
		asAtomHandler::setInt(ret,sys,0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setVolume)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t volume;
	ARG_UNPACK_ATOM(volume);
	if (th->soundChannel)
		th->soundChannel->soundTransform->volume = volume/100.0;
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
		asAtomHandler::setNumber(ret,sys,th->soundChannel->soundTransform->pan*100);
	else
		asAtomHandler::setInt(ret,sys,0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t pan;
	ARG_UNPACK_ATOM(pan);
	if (th->soundChannel)
		th->soundChannel->soundTransform->pan = pan/100.0;
}
ASFUNCTIONBODY_ATOM(AVM1Sound,stop)
{
	if (argslen == 1)
		LOG(LOG_NOT_IMPLEMENTED,"stopping sound with linkage id");
	else
		sys->audioManager->stopAllSounds();
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getPosition)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
	{
		asAtom o = asAtomHandler::fromObjectNoPrimitive(th->soundChannel.getPtr());
		SoundChannel::getPosition(ret,sys,o,args,argslen);
	}
	else
		asAtomHandler::setInt(ret,sys,0);
}

void AVM1Sound::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == this->soundChannel.getPtr())
	{
		if (e->type == "soundComplete")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=getSystemState()->getUniqueStringId("onSoundComplete");
			getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			}
		}
	}
}

