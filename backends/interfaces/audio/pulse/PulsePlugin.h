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

#ifndef PULSEPLUGIN_H
#define PULSEPLUGIN_H

#include <pulse/pulseaudio.h>
#include "../IAudioPlugin.h"
#include "../../../decoder.h"
#include "../../../../compat.h"
#include <iostream>

using namespace std;


class PulsePlugin : public IAudioPlugin
{
private:
        pa_threaded_mainloop *mainLoop;
        pa_context *context;
        static void contextStatusCB ( pa_context *context, PulsePlugin *th );
        void start();
        static void playbackListCB ( pa_context *context, const pa_sink_info *list, int eol, void *th );
        static void captureListCB ( pa_context *context, const pa_source_info *list, int eol, void *th );
        void addDeviceToList ( vector<string *> *devicesList, string *deviceName );
        void generateDevicesList ( vector<string *> *devicesList, DEVICE_TYPES desiredType ); //To populate the devices lists, devicesType must be playback or capture
        static void streamStatusCB ( pa_stream *stream, AudioStream *th );
        static void streamWriteCB ( pa_stream *stream, size_t nbytes, AudioStream *th );
        vector<AudioStream *> streams;
public:
        PulsePlugin ( PLUGIN_TYPES init_Type = AUDIO, string init_Name = "Pulse plugin output only",
                      string init_audiobackend = "pulse", bool init_contextReady = false,
                      bool init_noServer = false, bool init_stopped = false );
        void set_device ( string desiredDevice, DEVICE_TYPES desiredType );
        AudioStream *createStream ( lightspark::AudioDecoder *decoder );
        void freeStream ( AudioStream *audioStream );
        void fill ( AudioStream *audioStream );
        void stop();
        uint32_t getPlayedTime ( AudioStream *audioStream );
        ~PulsePlugin();
};

class AudioStream
{
public:
	enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
	pa_stream *stream;
	lightspark::AudioDecoder *decoder;
	PulsePlugin *manager;
	volatile STREAM_STATUS streamStatus;
	AudioStream ( PulsePlugin *m ) :stream ( NULL ),decoder ( NULL ),manager ( m ),streamStatus ( STREAM_STARTING ) {}
};

#endif
