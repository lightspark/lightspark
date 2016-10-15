/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "swf.h"
#include "backends/audio.h"
#include "backends/config.h"
#include <iostream>
#include "logger.h"
#include <SDL2/SDL_mixer.h>
#include <sys/time.h>


using namespace lightspark;
using namespace std;

void mixer_effect_ffmpeg_cb(int chan, void * stream, int len, void * udata)
{
	AudioStream *s = (AudioStream*)udata;
	if (!s)
		return;

	uint32_t readcount = 0;
	while (readcount < ((uint32_t)len))
	{
		uint32_t ret = s->getDecoder()->copyFrame((int16_t *)(((unsigned char*)stream)+readcount), ((uint32_t)len)-readcount);
		if (!ret)
			break;
		readcount += ret;
	}
}


uint32_t AudioStream::getPlayedTime()
{
	uint32_t ret;
	struct timeval now;
	gettimeofday(&now, NULL);

	ret = playedtime + (now.tv_sec * 1000 + now.tv_usec / 1000) - (starttime.tv_sec * 1000 + starttime.tv_usec / 1000);
	return ret;
}
bool AudioStream::init()
{
	unmutevolume = curvolume = SDL_MIX_MAXVOLUME;
	playedtime = 0;
	gettimeofday(&starttime, NULL);
	mixer_channel = -1;

	uint32_t len = LIGHTSPARK_AUDIO_SDL_BUFFERSIZE;

	uint8_t *buf = new uint8_t[len];
	memset(buf,0,len);
	Mix_Chunk* chunk = Mix_QuickLoad_RAW(buf, len);


	mixer_channel = Mix_PlayChannel(-1, chunk, -1);
	Mix_RegisterEffect(mixer_channel, mixer_effect_ffmpeg_cb, NULL, this);
	Mix_Resume(mixer_channel);

	return true;
}

void AudioStream::SetPause(bool pause_on)
{
	if (pause_on)
	{
		playedtime = getPlayedTime();
		if (mixer_channel != -1)
			Mix_Pause(mixer_channel);
	}
	else
	{
		gettimeofday(&starttime, NULL);
		if (mixer_channel != -1)
			Mix_Resume(mixer_channel);
	}
}

bool AudioStream::ispaused()
{
	return Mix_Paused(mixer_channel);
}

void AudioStream::mute()
{
	unmutevolume = curvolume;
	curvolume = 0;
}
void AudioStream::unmute()
{
	curvolume = unmutevolume;
}
void AudioStream::setVolume(double volume)
{
	curvolume = SDL_MIX_MAXVOLUME * volume;
	if (mixer_channel != -1)
		Mix_Volume(mixer_channel, curvolume);
}

AudioStream::~AudioStream()
{
	manager->streams.remove(this);
	if (mixer_channel != -1)
		Mix_HaltChannel(mixer_channel);
}

AudioManager::AudioManager():muteAllStreams(false),sdl_available(0),mixeropened(0)
{
	sdl_available = 0;
	if (SDL_WasInit(0)) // some part of SDL already was initialized
		sdl_available = !SDL_InitSubSystem ( SDL_INIT_AUDIO );
	else
		sdl_available = !SDL_Init ( SDL_INIT_AUDIO );
	mixeropened = 0;
}
void AudioManager::muteAll()
{
	muteAllStreams = true;
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->mute();
	}
}
void AudioManager::unmuteAll()
{
	muteAllStreams = false;
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->unmute();
	}
}

AudioStream* AudioManager::createStream(AudioDecoder* decoder, bool startpaused)
{
	if (!sdl_available)
		return NULL;
	if (!mixeropened)
	{
		if (Mix_OpenAudio (LIGHTSPARK_AUDIO_SDL_SAMPLERATE, AUDIO_S16, 2, LIGHTSPARK_AUDIO_SDL_BUFFERSIZE) < 0)
		{
			LOG(LOG_ERROR,"Couldn't open SDL_mixer");
			sdl_available = 0;
			return NULL;
		}
		mixeropened = 1;
	}

	AudioStream *stream = new AudioStream(this);
	stream->decoder = decoder;
	if (!stream->init())
	{
		delete stream;
		return NULL;
	}
	if (startpaused)
		stream->pause();
	else
		stream->hasStarted=true;
	streams.push_back(stream);

	return stream;
}


AudioManager::~AudioManager()
{
	for (stream_iterator it = streams.begin(); it != streams.end(); ++it) {
		delete *it;
	}
	if (mixeropened)
	{
		Mix_CloseAudio();
	}
	if (sdl_available)
	{
		SDL_QuitSubSystem ( SDL_INIT_AUDIO );
		if (!SDL_WasInit(0))
			SDL_Quit ();
	}
}
