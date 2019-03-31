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
	c->setDeclaredMethodByQName("attachSound","",Class<IFunction>::getFunction(c->getSystemState(),attachSound),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVolume","",Class<IFunction>::getFunction(c->getSystemState(),getVolume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVolume","",Class<IFunction>::getFunction(c->getSystemState(),getVolume),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,avm1constructor)
{
	AVM1Sound* th=obj.as<AVM1Sound>();
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);

	ARG_UNPACK_ATOM(th->clip,NullRef);
}

ASFUNCTIONBODY_ATOM(AVM1Sound,attachSound)
{
	AVM1Sound* th=obj.as<AVM1Sound>();
	if (argslen == 0)
	{
		LOG(LOG_ERROR,"AVM1:Sound.attachSound called without argument");
		return;
	}
	DefineSoundTag *soundTag = dynamic_cast<DefineSoundTag *>(th->getSystemState()->mainClip->dictionaryLookupByName(args[0].toStringId(th->getSystemState())));
	if (!soundTag)
	{
		LOG(LOG_ERROR,"AVM1:Sound.attachSound called for wrong tag:"<<args[0].toDebugString());
		return;
	}
	
	th->soundData = _R<StreamCache>(soundTag->getSoundData().getPtr());
	th->soundData->incRef();
	th->format= AudioFormat(soundTag->getAudioCodec(), soundTag->getSampleRate(), soundTag->getChannels());
	th->container=false;
	if(th->clip)
	{
		th->soundChannel = _MR(Class<SoundChannel>::getInstanceS(sys,th->soundData, th->format,false));
		th->soundChannel->incRef();
		th->clip->setSound(th->soundChannel.getPtr());
	}
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getVolume)
{
	AVM1Sound* th=obj.as<AVM1Sound>();
	if (th->soundChannel)
		ret.setNumber(sys,th->soundChannel->soundTransform->volume*100);
	else
		ret.setInt(0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setVolume)
{
	AVM1Sound* th=obj.as<AVM1Sound>();
	number_t volume;
	ARG_UNPACK_ATOM(volume);
	if (th->soundChannel)
		th->soundChannel->soundTransform->volume = volume/100.0;
}

