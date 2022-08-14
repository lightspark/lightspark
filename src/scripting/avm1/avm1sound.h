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

#ifndef SCRIPTING_AVM1_AVM1SOUND_H
#define SCRIPTING_AVM1_AVM1SOUND_H


#include "asobject.h"
#include "scripting/flash/media/flashmedia.h"

namespace lightspark
{

class AVM1Sound: public Sound
{
private:
	MovieClip* clip;
	bool loading;
	bool isStreaming;
public:
	AVM1Sound(ASWorker* wrk,Class_base* c):Sound(wrk,c),clip(nullptr),loading(false),isStreaming(false){}
	AVM1Sound(ASWorker* wrk,Class_base* c, _R<StreamCache> soundData, AudioFormat format, number_t duration_in_ms):Sound(wrk,c,soundData,format,duration_in_ms),
		clip(nullptr),loading(false),isStreaming(false)
	{
	}
	static void sinit(Class_base* c);
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;

	ASFUNCTION_ATOM(avm1constructor);
	ASFUNCTION_ATOM(attachSound);
	ASFUNCTION_ATOM(getVolume);
	ASFUNCTION_ATOM(setVolume);
	ASFUNCTION_ATOM(getPan);
	ASFUNCTION_ATOM(setPan);
	ASFUNCTION_ATOM(stop);
	ASFUNCTION_ATOM(getPosition);
	ASFUNCTION_ATOM(loadSound);
};

}

#endif // SCRIPTING_AVM1_AVM1SOUND_H
