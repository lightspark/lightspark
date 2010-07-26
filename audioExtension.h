/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010 Alexandre Demers (papouta@hotmail.com)

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
 

#ifndef AUDIOEXTENSION_H
#define AUDIOEXTENSION_H

#ifdef AUDIO_BACKEND

#include "compat.h"
#include "decoder.h"
#include "audioExtension.h"

namespace lightspark
{

class AudioExtension
{
private:
	class AudioStream
	{
	public:
		enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
//		pa_stream* stream;
		AudioDecoder* decoder;
		AudioExtension* manager;
		volatile STREAM_STATUS streamStatus;
//		AudioStream(AudioExtension* m):stream(NULL),decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
		AudioStream(AudioExtension* m):decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
	};
//	pa_threaded_mainloop* mainLoop;
//	pa_context* context;
//	static void contextStatusCB(pa_context* context, AudioExtension* th);
//	static void streamStatusCB(pa_stream* stream, AudioStream* th);
//	static void streamWriteCB(pa_stream* stream, size_t nbytes, AudioStream* th);
	std::vector<AudioStream*> streams;
	volatile bool contextReady;
	volatile bool noServer;
	bool stopped;
public:
	AudioExtension();
	uint32_t createStream(AudioDecoder* decoder);
	void freeStream(uint32_t id);
	void fillAndSync(uint32_t id, uint32_t streamTime);
	void stop();
	~AudioExtension();
};

};

#endif
#endif