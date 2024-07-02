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
#include "scripting/avm1/avm1sound.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"
#include "backends/audio.h"

using namespace std;
using namespace lightspark;

void AVM1Sound::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, avm1constructor, CLASS_SEALED);
	c->isSealed=false;
	c->setDeclaredMethodByQName("start","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachSound","",c->getSystemState()->getBuiltinFunction(attachSound),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVolume","",c->getSystemState()->getBuiltinFunction(getVolume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVolume","",c->getSystemState()->getBuiltinFunction(setVolume),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPan","",c->getSystemState()->getBuiltinFunction(getPan),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPan","",c->getSystemState()->getBuiltinFunction(setPan),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(getPosition),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(AVM1_IgnoreSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("duration","",c->getSystemState()->getBuiltinFunction(_getter_length),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("duration","",c->getSystemState()->getBuiltinFunction(AVM1_IgnoreSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loadSound","",c->getSystemState()->getBuiltinFunction(loadSound),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,avm1constructor)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	EventDispatcher::_constructor(ret,wrk,obj, NULL, 0);

	_NR<MovieClip> target;
	ARG_CHECK(ARG_UNPACK(target,NullRef));
	if (target)
	{
		th->clip = target.getPtr();
		th->clip->addOwnedObject(th);
	}
}

ASFUNCTIONBODY_ATOM(AVM1Sound,attachSound)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (argslen == 0)
	{
		LOG(LOG_ERROR,"AVM1:Sound.attachSound called without argument");
		return;
	}
	uint32_t nameID = asAtomHandler::toStringId(args[0],wrk);
	DefineSoundTag *soundTag = dynamic_cast<DefineSoundTag *>(!th->clip || th->clip->getRoot().isNull() ? th->getSystemState()->mainClip->dictionaryLookupByName(nameID) : th->clip->getRoot()->dictionaryLookupByName(nameID));
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
	th->setSoundChannel(soundTag->createSoundChannel(nullptr));
	if(th->clip)
	{
		th->soundChannel->incRef();
		th->clip->setSound(th->soundChannel,false);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getVolume)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		asAtomHandler::setNumber(ret,wrk,th->soundChannel->soundTransform->volume*100);
	}
	else
		asAtomHandler::setInt(ret,wrk,0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setVolume)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t volume;
	ARG_CHECK(ARG_UNPACK(volume));
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		th->soundChannel->soundTransform->volume = volume/100.0;
	}
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		asAtomHandler::setNumber(ret,wrk,th->soundChannel->soundTransform->pan*100);
	}
	else
		asAtomHandler::setInt(ret,wrk,0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t pan;
	ARG_CHECK(ARG_UNPACK(pan));
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		th->soundChannel->soundTransform->pan = pan/100.0;
	}
}
ASFUNCTIONBODY_ATOM(AVM1Sound,stop)
{
	if (argslen == 1)
		LOG(LOG_NOT_IMPLEMENTED,"stopping sound with linkage id");
	else
		wrk->getSystemState()->audioManager->stopAllSounds();
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getPosition)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
	{
		asAtom o = asAtomHandler::fromObjectNoPrimitive(th->soundChannel);
		SoundChannel::getPosition(ret,wrk,o,args,argslen);
	}
	else
		asAtomHandler::setInt(ret,wrk,0);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,loadSound)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);

	th->loading=false;
	tiny_string url;
	ARG_CHECK(ARG_UNPACK(url)(th->isStreaming));
	tiny_string realurl;
	if (url.find("://") == tiny_string::npos)
	{
		// relative url, so we add the main url
		realurl = wrk->getSystemState()->mainClip->getOrigin().getProtocol()+"://";
		realurl += wrk->getSystemState()->mainClip->getOrigin().getHostname()+":";
		realurl += Integer::toString(wrk->getSystemState()->mainClip->getOrigin().getPort())+"/";
		realurl += url;
	}
	else
		realurl = url;
	th->url = URLInfo(realurl);
	_R<StreamCache> c(_MR(new MemoryStreamCache(th->getSystemState())));
	th->soundData = c;

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(wrk)));
		return;
	}

	th->incRef();
	th->downloader=th->getSystemState()->downloadManager->download(th->url, th->soundData, th);
	if(th->downloader->hasFailed())
	{
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(wrk)));
	}
}

void AVM1Sound::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	ASWorker* wrk = getInstanceWorker();
	if (e->type == "soundComplete")
	{
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id=getSystemState()->getUniqueStringId("onSoundComplete");
		getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtom ret=asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObject(this);
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			asAtomHandler::as<AVM1Function>(func)->decRef();
		}
	}
	if (e->type == "progress" && !this->loading)
	{
		this->loading=true;
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
		getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtom ret=asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObject(this);
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			asAtomHandler::as<AVM1Function>(func)->decRef();
		}
		if (isStreaming)
		{
			asAtom ret = asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObject(this);
			this->play(ret,wrk,obj,nullptr,0);
		}
	}
}

