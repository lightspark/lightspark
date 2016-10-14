/**************************************************************************
	Lightspark, a free flash player implementation

	Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)
	Copyright (C) 2011 Ludger Krämer (dbluelle@blau-weissoedingen.de)

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

#ifndef BACKENDS_INTERFACES_AUDIO_SDL_SDLPLUGIN_H
#define BACKENDS_INTERFACES_AUDIO_SDL_SDLPLUGIN_H 1

#include "backends/interfaces/audio/IAudioPlugin.h"
#include "backends/decoder.h"
#include "compat.h"
#include <sys/time.h>
#include <SDL2/SDL_mixer.h>

using lightspark::AudioDecoder;

#define LIGHTSPARK_AUDIO_SDL_BUFERSIZE 4096
#define LIGHTSPARK_AUDIO_SDL_SAMPLERATE 44100

class SDLAudioStream;

class SDLPlugin : public IAudioPlugin
{
friend class SDLAudioStream;
private:
	int sdl_available;
	int mixeropened;
public:
	SDLPlugin ( std::string init_Name = "SDL plugin", std::string init_audiobackend = "sdl", bool init_stopped = false );

	void set_device(std::string desiredDevice, IAudioPlugin::DEVICE_TYPES desiredType);

	AudioStream *createStream(AudioDecoder *decoder);

	void muteAll();
	void unmuteAll();
	int forcedSampleRate() const { return LIGHTSPARK_AUDIO_SDL_SAMPLERATE;}
	int forcedChannelLayout() const { return AV_CH_LAYOUT_STEREO;}

	bool isTimingAvailable() const;
	~SDLPlugin();
};

class SDLAudioStream: public AudioStream
{
private:
	SDLPlugin* manager;
	int curvolume;
	int unmutevolume;
	uint32_t playedtime;
	struct timeval starttime;
	int mixer_channel;
public:
	bool init();
	SDLAudioStream(SDLPlugin* _manager) : manager(_manager) { }

	void SetPause(bool pause_on);
	uint32_t getPlayedTime();
	bool ispaused();
	bool isValid();
	void mute();
	void unmute();
	void pause() { SetPause(true); }
	void resume() { SetPause(false); }
	void setVolume(double volume);
	~SDLAudioStream();
};
#endif /* BACKENDS_INTERFACES_AUDIO_SDL_SDLPLUGIN_H */
