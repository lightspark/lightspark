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

#ifndef BACKENDS_AUDIO_H
#define BACKENDS_AUDIO_H 1


#include "compat.h"
#include "backends/decoder.h"
#include <iostream>

namespace lightspark
{
class AudioStream;
class EngineData;

class AudioManager
{
	friend class AudioStream;
private:
	bool muteAllStreams;
	bool audio_available;
	int mixeropened;
	EngineData* engineData;
	std::list<AudioStream *> streams;
	typedef std::list<AudioStream *>::iterator stream_iterator;
	Mutex streamMutex;
public:
	AudioManager(EngineData* engine);

	AudioStream *createStream(AudioDecoder *decoder, bool startpaused, IThreadJob *producer, uint32_t playedTime);

	void toggleMuteAll() { muteAllStreams ? unmuteAll() : muteAll(); }
	bool allMuted() { return muteAllStreams; }
	void muteAll();
	void unmuteAll();
	void removeStream(AudioStream* s);
	void stopAllSounds();
	~AudioManager();
};

class DLL_PUBLIC AudioStream
{
friend class AudioManager;
friend class NetStream;
private:
	AudioManager* manager;
	AudioDecoder *decoder;
	IThreadJob* producer;
	bool hasStarted;
	bool isPaused;
	bool mixingStarted;
	double curvolume;
	double unmutevolume;
	uint64_t playedtime;
	struct timeval starttime;
	int mixer_channel;
public:
	bool init();
	void startMixing();
	AudioStream(AudioManager* _manager,IThreadJob* _producer,uint64_t _playedtime):manager(_manager),decoder(NULL),producer(_producer),hasStarted(false),isPaused(true),mixingStarted(false),playedtime(_playedtime) { }

	void SetPause(bool pause_on);
	uint32_t getPlayedTime();
	bool ispaused();
	void mute();
	void unmute();
	void pause() { SetPause(true); }
	void resume() { SetPause(false); }
	void setVolume(double volume);
	void setPlayedTime(uint64_t p) { playedtime = p; }
	inline double getVolume() const { return curvolume; }
	inline AudioDecoder *getDecoder() const { return decoder; }
	~AudioStream();
};


}

#endif /* BACKENDS_AUDIO_H */
