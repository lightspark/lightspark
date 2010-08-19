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

#ifndef PULSEPLUGIN_H
#define PULSEPLUGIN_H

#include <pulse/pulseaudio.h>
#include "../AudioPlugin.h"
#include "../../../decoder.h"
#include "../../../compat.h"
#include <iostream>

using namespace std;


class PulsePlugin : public AudioPlugin 
{
  private:
	class AudioStream
	{
	public:
	   enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
	   pa_stream *stream;
	   lightspark::AudioDecoder *decoder;
	   PulsePlugin *manager;
	   volatile STREAM_STATUS streamStatus;
	   AudioStream(PulsePlugin *m):stream(NULL),decoder(NULL),manager(m),streamStatus(STREAM_STARTING){}
	};
	pa_threaded_mainloop *mainLoop;
	pa_context *context;
	static void contextStatusCB(pa_context *context, PulsePlugin *th);
	static void streamStatusCB(pa_stream *stream, AudioStream *th);
	static void streamWriteCB(pa_stream *stream, size_t nbytes, AudioStream *th);
	void start();
	vector<AudioStream*> streams;
  public:
	PulsePlugin(PLUGIN_TYPES init_Type = AUDIO, string init_Name = "Pulse plugin output only",
		    string init_audiobackend = "pulse", bool init_contextReady = false,
		    bool init_noServer = false, bool init_stopped = false);
	uint32_t createStream(lightspark::AudioDecoder *decoder);
	bool Is_Connected();
	void freeStream(uint32_t id);
	void fill(uint32_t id);
	void stop();
	uint32_t getPlayedTime(uint32_t streamId);
	~PulsePlugin();
};

#endif
