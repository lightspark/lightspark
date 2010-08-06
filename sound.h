/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SOUND_H
#define SOUND_H

#ifdef ENABLE_SOUND

#include "compat.h"
#include <pulse/pulseaudio.h>
#include "decoder.h"

namespace lightspark
{

class SoundManager
{
private:
	class SoundStream
	{
	public:
		enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
		pa_stream* stream;
		AudioDecoder* decoder;
		SoundManager* manager;
		volatile STREAM_STATUS streamStatus;
		SoundStream(SoundManager* m):stream(NULL),decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
	};
	pa_threaded_mainloop* mainLoop;
	pa_context* context;
	static void contextStatusCB(pa_context* context, SoundManager* th);
	static void streamStatusCB(pa_stream* stream, SoundStream* th);
	static void streamWriteCB(pa_stream* stream, size_t nbytes, SoundStream* th);
	std::vector<SoundStream*> streams;
	volatile bool contextReady;
	volatile bool noServer;
	bool stopped;
public:
	SoundManager();
	uint32_t createStream(AudioDecoder* decoder);
	void freeStream(uint32_t id);
	void fill(uint32_t id);
	void stop();
	bool isTimingAvailable() const;
	/**
		Get the elapsed time in milliseconds for the stream streamId
	*/
	uint32_t getPlayedTime(uint32_t streamId);
	~SoundManager();
};

};

#endif
#endif
