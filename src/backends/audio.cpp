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
#include <sys/time.h>


using namespace lightspark;
using namespace std;

uint32_t AudioStream::getPlayedTime()
{
	uint32_t ret;
	struct timeval now;
	gettimeofday(&now, nullptr);
	if (!mixingStarted)
		return playedtime;

	ret = playedtime + (now.tv_sec * 1000 + now.tv_usec / 1000) - (starttime.tv_sec * 1000 + starttime.tv_usec / 1000);
	return ret;
}
bool AudioStream::init()
{
	unmutevolume = curvolume = 1.0;
	mixer_channel = manager->engineData->audio_StreamInit(this);
	manager->engineData->audio_StreamSetVolume(mixer_channel, curvolume);
	isPaused = false;
	return true;
}

void AudioStream::startMixing()
{
	if(mixingStarted)
		return;
	mixingStarted=true;
	gettimeofday(&starttime, nullptr);
}

void AudioStream::SetPause(bool pause_on)
{
	if (pause_on)
	{
		playedtime = getPlayedTime();
		isPaused = true;
	}
	else
	{
		mixingStarted=false;
		isPaused = false;
	}
	manager->engineData->audio_StreamPause(mixer_channel,pause_on);
}

bool AudioStream::ispaused()
{
	return 	isPaused;
}

void AudioStream::mute()
{
	unmutevolume = curvolume;
	setVolume(0);
}
void AudioStream::unmute()
{
	setVolume(unmutevolume);
}
void AudioStream::setVolume(double volume)
{
	manager->engineData->audio_StreamSetVolume(mixer_channel, volume);
	curvolume = volume;
}

AudioStream::~AudioStream()
{
	manager->engineData->audio_StreamDeinit(mixer_channel);
	manager->removeStream(this);
}

AudioManager::AudioManager(EngineData *engine):muteAllStreams(false),audio_available(false),mixeropened(0),engineData(engine)
{
	audio_available = engine->audio_ManagerInit();
	mixeropened = 0;
}
void AudioManager::muteAll()
{
	Locker l(streamMutex);
	muteAllStreams = true;
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->mute();
	}
}
void AudioManager::unmuteAll()
{
	Locker l(streamMutex);
	muteAllStreams = false;
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->unmute();
	}
}

void AudioManager::removeStream(AudioStream *s)
{
	Locker l(streamMutex);
	streams.remove(s);
	if (streams.empty())
	{
		engineData->audio_ManagerCloseMixer();
		mixeropened = false;
	}
}

void AudioManager::stopAllSounds()
{
	muteAll();
	// use temporary list of producers to avoid deadlock, as threadAbort() leads to removeStream();
	list<IThreadJob*> producers;
	{
		Locker l(streamMutex);
		for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
		{
			if ((*it)->producer)
				producers.push_back((*it)->producer);
		}
	}
	for (auto it = producers.begin();it != producers.end(); ++it )
	{
		(*it)->threadAbort();
	}
}

AudioStream* AudioManager::createStream(AudioDecoder* decoder, bool startpaused, IThreadJob* producer, uint32_t playedTime)
{
	Locker l(streamMutex);
	if (!audio_available)
		return NULL;
	if (!mixeropened)
	{
		if (!engineData->audio_ManagerOpenMixer())
		{
			LOG(LOG_ERROR,"Couldn't open mixer");
			audio_available = 0;
			return NULL;
		}
		mixeropened = 1;
	}

	AudioStream *stream = new AudioStream(this,producer,playedTime);
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
	Locker l(streamMutex);
	for (stream_iterator it = streams.begin(); it != streams.end(); ++it) {
		delete *it;
	}
	if (mixeropened)
	{
		engineData->audio_ManagerCloseMixer();
	}
	if (audio_available)
	{
		engineData->audio_ManagerDeinit();
	}
}
