/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef OPENALPLUGIN_H
#define OPENALPLUGIN_H

#include <AL/al.h>
#include <AL/alc.h>
#include "../IAudioPlugin.h"
#include "../../../decoder.h"
#include "../../../../compat.h"
#include <iostream>

#define NUM_BUFFERS 4
#define NUM_SOURCES 1

class OpenALPlugin : public IAudioPlugin
{
private:
	ALCcontext *context;
	ALCdevice *playbackDevice;
	ALCdevice *captureDevice;
	ALuint	pbBuffers[NUM_BUFFERS];
	ALuint	pbSources[NUM_SOURCES];
	void initPlayback ( OpenALPlugin *th );
	void initCapture ( OpenALPlugin *th );
	static void contextStatusCB ( pa_context *context, OpenALPlugin *th );
	static void streamStatusCB ( pa_stream *stream, AudioStream *th );
	static void streamWriteCB ( pa_stream *stream, size_t nbytes, AudioStream *th );
	void addDeviceToList ( std::vector<std::string *> *devicesList, std::string *deviceName );
	void generateDevicesList ( std::vector<std::string *> *devicesList, DEVICE_TYPES desiredType ); //To populate the devices lists, devicesType must be playback or capture
	void start();
	std::vector<AudioStream*> streams;
public:
	OpenALPlugin ( PLUGIN_TYPES init_Type = AUDIO, std::string init_Name = "OpenAL plugin",
	               std::string init_audiobackend = "openal", bool init_contextReady = false,
	               bool init_noServer = false, bool init_stopped = false );
	void set_device ( std::string desiredDevice, DEVICE_TYPES desiredType );
	uint32_t createStream ( lightspark::AudioDecoder *decoder );
	void freeStream ( uint32_t id );
	void fill ( uint32_t id );
	void stop();
	uint32_t getPlayedTime ( uint32_t streamId );
	~OpenALPlugin();
};

class AudioStream
{
public:
	enum STREAM_STATUS { STREAM_STARTING = 0, STREAM_READY = 1, STREAM_DEAD = 2 };
	pa_stream *stream;
	lightspark::AudioDecoder *decoder;
	OpenALPlugin *manager;
	volatile STREAM_STATUS streamStatus;
	AudioStream ( OpenALPlugin *m ) : stream ( NULL ), decoder ( NULL ), manager ( m ), streamStatus ( STREAM_STARTING ) {}
};
