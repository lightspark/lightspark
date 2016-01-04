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

#include "backends/interfaces/audio/sdl/SDLPlugin.h"
#include <SDL.h>

using lightspark::AudioDecoder;
using namespace std;

void mixer_effect_ffmpeg_cb(int chan, void * stream, int len, void * udata)
{
	SDLAudioStream *s = (SDLAudioStream*)udata;
	if (!s)
		return;

	if (!s->decoder->hasDecodedFrames())
		return;
	uint32_t readcount = 0;
	while (readcount < ((uint32_t)len))
	{
		if (!s->decoder->hasDecodedFrames())
			break;
		uint32_t ret = s->decoder->copyFrame((int16_t *)(((unsigned char*)stream)+readcount), ((uint32_t)len)-readcount);
		if (!ret)
			break;
		readcount += ret;
	}
}

SDLPlugin::SDLPlugin(string init_Name, string init_audiobackend, bool init_stopped ) :
		IAudioPlugin ( init_Name, init_audiobackend, init_stopped )
{
	sdl_available = 0;
	if (SDL_WasInit(0)) // some part of SDL already was initialized
		sdl_available = !SDL_InitSubSystem ( SDL_INIT_AUDIO );
	else
		sdl_available = !SDL_Init ( SDL_INIT_AUDIO );
	if (Mix_OpenAudio (LIGHTSPARK_AUDIO_SDL_SAMPLERATE, AUDIO_S16, 2, LIGHTSPARK_AUDIO_SDL_BUFERSIZE) < 0)
	{
		LOG(LOG_ERROR,"Couldn't open SDL_mixer");
		sdl_available = 0;
		SDL_QuitSubSystem ( SDL_INIT_AUDIO );
		if (!SDL_WasInit(0))
			SDL_Quit ();
		return;
	}
	Mix_Pause(-1);
}
void SDLPlugin::set_device(std::string desiredDevice,
		IAudioPlugin::DEVICE_TYPES desiredType)
{
	/* not yet implemented */
}

AudioStream* SDLPlugin::createStream(AudioDecoder* decoder)
{
	if (!sdl_available)
		return NULL;

	SDLAudioStream *stream = new SDLAudioStream(this);
	stream->decoder = decoder;
	if (!stream->init())
	{
		delete stream;
		return NULL;
	}
	streams.push_back(stream);

	return stream;
}


SDLPlugin::~SDLPlugin()
{
	for (stream_iterator it = streams.begin(); it != streams.end(); ++it) {
		delete *it;
	}
	if (sdl_available)
	{
		Mix_CloseAudio();
		SDL_QuitSubSystem ( SDL_INIT_AUDIO );
		if (!SDL_WasInit(0))
			SDL_Quit ();
	}
}


bool SDLPlugin::isTimingAvailable() const
{
	return true;
}
void SDLPlugin::muteAll()
{
	IAudioPlugin::muteAll();
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		((SDLAudioStream*) (*it))->mute();
	}
}
void SDLPlugin::unmuteAll()
{
	IAudioPlugin::unmuteAll();
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		((SDLAudioStream*) (*it))->unmute();
	}
}

/****************************
Stream's functions
****************************/
uint32_t SDLAudioStream::getPlayedTime()
{
	uint32_t ret;
	struct timeval now;
	gettimeofday(&now, NULL);

	ret = playedtime + (now.tv_sec * 1000 + now.tv_usec / 1000) - (starttime.tv_sec * 1000 + starttime.tv_usec / 1000);
	return ret;
}

bool SDLAudioStream::init()
{
	unmutevolume = curvolume = SDL_MIX_MAXVOLUME;
	playedtime = 0;
	gettimeofday(&starttime, NULL);
	mixer_channel = -1;

	uint32_t len = LIGHTSPARK_AUDIO_SDL_BUFERSIZE;

	uint8_t *buf = new uint8_t[len];
	memset(buf,0,len);
	Mix_Chunk* chunk = Mix_QuickLoad_RAW(buf, len);


	mixer_channel = Mix_PlayChannel(-1, chunk, -1);
	Mix_RegisterEffect(mixer_channel, mixer_effect_ffmpeg_cb, NULL, this);
	Mix_Resume(mixer_channel);
	return true;
}

void SDLAudioStream::SetPause(bool pause_on)
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

bool SDLAudioStream::ispaused()
{
	return SDL_GetAudioStatus() != SDL_AUDIO_PLAYING;
}

bool SDLAudioStream::isValid()
{
	return true;
}
void SDLAudioStream::mute()
{
	unmutevolume = curvolume;
	curvolume = 0;
}
void SDLAudioStream::unmute()
{
	curvolume = unmutevolume;
}
void SDLAudioStream::setVolume(double volume)
{
	curvolume = SDL_MIX_MAXVOLUME * volume;
	if (mixer_channel != -1)
		Mix_Volume(mixer_channel, curvolume);
}

SDLAudioStream::~SDLAudioStream()
{
	manager->streams.remove(this);
	if (mixer_channel != -1)
		Mix_HaltChannel(mixer_channel);
}

extern "C" DLL_PUBLIC IPlugin *create()
{
	return new SDLPlugin();
}

extern "C" DLL_PUBLIC void release(IPlugin *plugin)
{
	delete plugin;
}
