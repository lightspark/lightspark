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

class AudioManager
{
	friend class AudioStream;
private:
	bool muteAllStreams;
	int sdl_available;
	int mixeropened;
	std::list<AudioStream *> streams;
	typedef std::list<AudioStream *>::iterator stream_iterator;
	Mutex streamMutex;
public:
	AudioManager();

	AudioStream *createStream(AudioDecoder *decoder, bool startpaused);

	void toggleMuteAll() { muteAllStreams ? unmuteAll() : muteAll(); }
	bool allMuted() { return muteAllStreams; }
	void muteAll();
	void unmuteAll();
	void removeStream(AudioStream* s);
	~AudioManager();
};

class AudioStream
{
friend class AudioManager;
friend class NetStream;
private:
	AudioManager* manager;
	AudioDecoder *decoder;
	bool hasStarted;
	int curvolume;
	int unmutevolume;
	uint32_t playedtime;
	struct timeval starttime;
	int mixer_channel;
public:
	bool init();
	AudioStream(AudioManager* _manager):manager(_manager),decoder(NULL),hasStarted(false) { }

	void SetPause(bool pause_on);
	uint32_t getPlayedTime();
	bool ispaused();
	void mute();
	void unmute();
	void pause() { SetPause(true); }
	void resume() { SetPause(false); }
	void setVolume(double volume);
	inline AudioDecoder *getDecoder() const { return decoder; }
	~AudioStream();
};


}

#endif /* BACKENDS_AUDIO_H */
