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
#include "platforms/engineutils.h"
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
bool AudioStream::init(double volume)
{
	unmutevolume = curvolume = volume;
	mixer_channel = manager->engineData->audio_StreamInit(this);
	if (mixer_channel >= 0)
	{
		isPaused = false;
		return true;
	}
	return false;
}

void AudioStream::deinit()
{
	if (!isdone)
		manager->engineData->audio_StreamDeinit(mixer_channel);
	mixer_channel=-1;
	if (audiobuffer)
		delete[] audiobuffer;
	audiobuffer=nullptr;
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
	return isPaused;
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
	curvolume = volume;
}

void AudioStream::setPanning(uint16_t left, uint16_t right)
{
	panning[0]=(float)left/32768.0f;
	panning[1]=(float)right/32768.0f;
	curvolume=1.0;
}

void AudioStream::setIsDone()
{
	RELEASE_WRITE(isdone,true);
	decoder->skipAll();
}

bool AudioStream::getIsDone() const
{
	return ACQUIRE_READ(isdone);
}

AudioStream::~AudioStream()
{
}

AudioManager::AudioManager(EngineData *engine):muteAllStreams(false),audio_available(false),mixeropened(0),engineData(engine),device(0)
{
	audio_available = engine->audio_ManagerInit();
	mixeropened = 0;
}
void AudioManager::muteAll()
{
	Locker l(streamMutex);
	muteAllStreams = true;
	for (auto it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->mute();
	}
}
void AudioManager::unmuteAll()
{
	Locker l(streamMutex);
	muteAllStreams = false;
	for (auto it = streams.begin();it != streams.end(); ++it )
	{
		(*it)->unmute();
	}
}

void AudioManager::removeStream(AudioStream *s)
{
	streamMutex.lock();
	streams.remove(s);
	s->deinit();
	delete s;
	if (streams.empty())
	{
		streamMutex.unlock();
		managerMutex.lock();
		if (mixeropened)
			engineData->audio_ManagerCloseMixer(this);
		mixeropened = false;
		managerMutex.unlock();
	}
	else
		streamMutex.unlock();
		
}

void AudioManager::stopAllSounds()
{
	// use temporary list of producers to avoid deadlock, as threadAbort() leads to removeStream();
	list<IThreadJob*> producers;
	{
		Locker l(streamMutex);
		for (auto it = streams.begin();it != streams.end(); ++it )
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

AudioStream* AudioManager::createStream(AudioDecoder* decoder, bool startpaused, IThreadJob* producer, int grouptag, uint32_t playedTime, double volume)
{
	if (!audio_available)
		return nullptr;
	managerMutex.lock();
	if (!mixeropened)
	{
		if (!engineData->audio_ManagerOpenMixer(this))
		{
			LOG(LOG_ERROR,"Couldn't open mixer");
			audio_available = 0;
			return nullptr;
		}
		mixeropened = 1;
	}
	managerMutex.unlock();
	Locker l(streamMutex);
	AudioStream *stream = new AudioStream(this,producer,grouptag,playedTime);
	stream->decoder = decoder;
	if (!stream->init(volume))
	{
		delete stream;
		return nullptr;
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
	managerMutex.lock();
	if (mixeropened)
	{
		engineData->audio_ManagerCloseMixer(this);
	}
	if (audio_available)
	{
		engineData->audio_ManagerDeinit();
	}
	managerMutex.unlock();
}
