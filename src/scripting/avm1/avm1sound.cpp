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
#include "scripting/toplevel/UInteger.h"
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
	c->prototype->setVariableByQName("start","",c->getSystemState()->getBuiltinFunction(play),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("attachSound","",c->getSystemState()->getBuiltinFunction(attachSound),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getVolume","",c->getSystemState()->getBuiltinFunction(getVolume),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setVolume","",c->getSystemState()->getBuiltinFunction(setVolume),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getPan","",c->getSystemState()->getBuiltinFunction(getPan),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setPan","",c->getSystemState()->getBuiltinFunction(setPan),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getTransform","",c->getSystemState()->getBuiltinFunction(getTransform),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setTransform","",c->getSystemState()->getBuiltinFunction(setTransform),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getDuration","",c->getSystemState()->getBuiltinFunction(AVM1_duration,0,Class<Number>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("loadSound","",c->getSystemState()->getBuiltinFunction(loadSound),DYNAMIC_TRAIT);
	c->prototype->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(getPosition,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(AVM1_IgnoreSetter),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("duration","",c->getSystemState()->getBuiltinFunction(AVM1_duration,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("duration","",c->getSystemState()->getBuiltinFunction(AVM1_IgnoreSetter),SETTER_METHOD,false,false);

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
		th->soundChannel = th->clip->getSoundChannel();
		if (th->soundChannel)
		{
			th->soundChannel->incRef();
			th->soundChannel->addStoredMember();
		}
		else
		{
			th->soundChannel = Class<SoundChannel>::getInstanceSNoArgs(wrk);
			th->soundChannel->incRef();
			th->soundChannel->addStoredMember();
			th->clip->setSound(th->soundChannel,false);
		}
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
	DefineSoundTag *soundTag = dynamic_cast<DefineSoundTag *>(!th->clip ? th->getSystemState()->mainClip->applicationDomain->dictionaryLookupByName(nameID) : th->clip->loadedFrom->dictionaryLookupByName(nameID));
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
		asAtomHandler::setNumber(ret,wrk,th->soundChannel->soundTransform->volume);
	}
	else
		asAtomHandler::setInt(ret,wrk,wrk->getSystemState()->static_SoundMixer_soundTransform->volume);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setVolume)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t volume;
	ARG_CHECK(ARG_UNPACK(volume,0));
	if (std::isnan(volume) || volume < INT32_MIN)
		volume = INT32_MIN;
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		th->soundChannel->soundTransform->volume = volume;
	}
	else
		wrk->getSystemState()->static_SoundMixer_soundTransform->volume = volume;
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	SoundTransform* st =nullptr;
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		st = th->soundChannel->soundTransform.getPtr();
	}
	else
		st = wrk->getSystemState()->static_SoundMixer_soundTransform.getPtr();
	if (st->leftToLeft == 100)
		asAtomHandler::setInt(ret,wrk,abs(st->rightToRight)-100);
	else
		asAtomHandler::setInt(ret,wrk,100-abs(st->leftToLeft));
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setPan)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	number_t pan;
	ARG_CHECK(ARG_UNPACK(pan));
	SoundTransform* st =nullptr;
	if (std::isnan(pan) || pan < INT32_MIN || pan > INT32_MAX) // it seems everything outside int32 margins is set to INT32_MIN
		pan = INT32_MIN;
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		st = th->soundChannel->soundTransform.getPtr();
	}
	else
		st = wrk->getSystemState()->static_SoundMixer_soundTransform.getPtr();
	if (pan >= 0)
	{
		st->leftToLeft = 100-pan;
		st->rightToRight = 100;
	}
	else
	{
		st->leftToLeft = 100;
		st->rightToRight = 100+pan;
	}
	st->leftToRight = 0;
	st->rightToLeft = 0;
}
ASFUNCTIONBODY_ATOM(AVM1Sound,getTransform)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	ASObject* res = new_asobject(wrk);
	SoundTransform* st =nullptr;
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		st = th->soundChannel->soundTransform.getPtr();
	}
	else
		st = wrk->getSystemState()->static_SoundMixer_soundTransform.getPtr();

	asAtom v;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;

	m.name_s_id=wrk->getSystemState()->getUniqueStringId("ll");
	v = asAtomHandler::fromInt(st->leftToLeft);
	res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

	m.name_s_id=wrk->getSystemState()->getUniqueStringId("lr");
	v = asAtomHandler::fromInt(st->leftToRight);
	res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

	m.name_s_id=wrk->getSystemState()->getUniqueStringId("rl");
	v = asAtomHandler::fromInt(st->rightToLeft);
	res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

	m.name_s_id=wrk->getSystemState()->getUniqueStringId("rr");
	v = asAtomHandler::fromInt(st->rightToRight);
	res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

	ret = asAtomHandler::fromObjectNoPrimitive(res);
}
ASFUNCTIONBODY_ATOM(AVM1Sound,setTransform)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	_NR<ASObject> transformObject;
	ARG_CHECK(ARG_UNPACK(transformObject));

	SoundTransform* st =nullptr;
	if (th->soundChannel)
	{
		if (!th->soundChannel->soundTransform)
			th->soundChannel->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
		st = th->soundChannel->soundTransform.getPtr();
	}
	else
		st = wrk->getSystemState()->static_SoundMixer_soundTransform.getPtr();
	if (transformObject)
	{
		asAtom v = asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ll");
		transformObject->getVariableByMultiname(v,m,NO_INCREF,wrk);
		if (asAtomHandler::isValid(v))
			st->leftToLeft = asAtomHandler::toInt(v);
		v = asAtomHandler::invalidAtom;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("lr");
		transformObject->getVariableByMultiname(v,m,NO_INCREF,wrk);
		if (asAtomHandler::isValid(v))
			st->leftToRight = asAtomHandler::toInt(v);
		v = asAtomHandler::invalidAtom;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rl");
		transformObject->getVariableByMultiname(v,m,NO_INCREF,wrk);
		if (asAtomHandler::isValid(v))
			st->rightToLeft = asAtomHandler::toInt(v);
		v = asAtomHandler::invalidAtom;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rr");
		transformObject->getVariableByMultiname(v,m,NO_INCREF,wrk);
		if (asAtomHandler::isValid(v))
			st->rightToRight = asAtomHandler::toInt(v);
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
ASFUNCTIONBODY_ATOM(AVM1Sound,AVM1_duration)
{
	AVM1Sound* th=asAtomHandler::as<AVM1Sound>(obj);
	if (th->soundChannel)
		ret = asAtomHandler::fromNumber(wrk,::round(th->length),false);
	else
		ret = asAtomHandler::undefinedAtom;
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
