/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include <SDL_audio.h>

using lightspark::AudioDecoder;
using namespace std;


SDLPlugin::SDLPlugin(string init_Name, string init_audiobackend, bool init_stopped ) :
		IAudioPlugin ( init_Name, init_audiobackend, init_stopped )
{
	sdl_available = 0;
	if (SDL_WasInit(0)) // some part of SDL already was initialized 
		sdl_available = !SDL_InitSubSystem ( SDL_INIT_AUDIO );
	else
		sdl_available = !SDL_Init ( SDL_INIT_AUDIO );

	/* We use SDL_OpenAudio/SDL_CloseAudio in SDLAudioStream's constructor/destructor
	 * to access the device directly. We _should_ use SDL_mixer instead.
	 */
	LOG(LOG_NOT_IMPLEMENTED,"The SDL audio plugin does only support one concurrent stream!");
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
	SDL_AudioSpec fmt;
	fmt.freq = decoder->sampleRate;
	fmt.format = AUDIO_S16;
	fmt.channels = decoder->channelCount;
	fmt.samples = 4096;
	fmt.callback = async_callback;
	fmt.userdata = this;
	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		LOG(LOG_ERROR, "Unable to open SDL audio:" << SDL_GetError());
		return false;
	}
	unmutevolume = curvolume = SDL_MIX_MAXVOLUME;
	playedtime = 0;
	gettimeofday(&starttime, NULL);
	SDL_PauseAudio(0);
	return true;
}
void SDLAudioStream::async_callback(void *unused, uint8_t *stream, int len)
{
	SDLAudioStream *s = static_cast<SDLAudioStream*>(unused);

	if (!s->decoder->hasDecodedFrames())
		return;

	uint8_t *buf = new uint8_t[len];
	int readcount = 0;
	while (readcount < len)
	{
		if (!s->decoder->hasDecodedFrames())
			break;
		uint32_t ret = s->decoder->copyFrame((int16_t *)(buf+readcount), len-readcount);
		if (!ret)
			break;
		readcount += ret;
	}
	SDL_LockAudio();
	SDL_MixAudio(stream, buf, readcount, s->curvolume);
	SDL_UnlockAudio();
	delete[] buf;
}

void SDLAudioStream::SetPause(bool pause_on)
{
	if (pause_on)
	{
		playedtime = getPlayedTime();
	}
	else
	{
		gettimeofday(&starttime, NULL);
	}
	SDL_PauseAudio(pause_on);
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
}

SDLAudioStream::~SDLAudioStream()
{
	manager->streams.remove(this);
	SDL_CloseAudio();
}

extern "C" DLL_PUBLIC IPlugin *create()
{
	return new SDLPlugin();
}

extern "C" DLL_PUBLIC void release(IPlugin *plugin)
{
	delete plugin;
}
