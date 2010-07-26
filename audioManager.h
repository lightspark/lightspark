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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "compat.h"
#include "decoder.h"
#include "plugins/audio/audioPlugin.h"

//convenience typedef for the pointers to the 2 functions we expect to find in the plugins
typedef IPlugin* (*PLUGIN_FACTORY)();
typedef void (*PLUGIN_CLEANUP)(IPlugin*);

namespace lightspark
{

class AudioManager
{
private:
	class AudioStream
	{
	public:
		enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
//		pa_stream* stream;
		AudioDecoder* decoder;
		AudioManager* manager;
		volatile STREAM_STATUS streamStatus;
//		AudioStream(AudioManager* m):stream(NULL),decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
		AudioStream(AudioManager* m):decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
	};
//	pa_threaded_mainloop* mainLoop;
//	pa_context* context;
//	static void contextStatusCB(pa_context* context, AudioManager* th);
//	static void streamStatusCB(pa_stream* stream, AudioStream* th);
//	static void streamWriteCB(pa_stream* stream, size_t nbytes, AudioStream* th);
	std::vector<AudioStream*> streams;
	volatile bool contextReady;
	volatile bool noServer;
	bool stopped;
public:
	AudioManager();
	uint32_t createStream(AudioDecoder* decoder);
	void freeStream(uint32_t id);
	void fillAndSync(uint32_t id, uint32_t streamTime);
	void stop();
	~AudioManager();
};

};

#endif
