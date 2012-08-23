/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

#ifndef BACKENDS_INTERFACES_AUDIO_WINMM_WINMMPLUGIN_H
#define BACKENDS_INTERFACES_AUDIO_WINMM_WINMMPLUGIN_H 1

#include <iostream>
#include <windows.h>
#undef RGB //conflicts with swftypes.h
#include <mmsystem.h>
#include "backends/interfaces/audio/IAudioPlugin.h"
#include "backends/decoder.h"
#include "compat.h"
#include "threading.h"

namespace lightspark
{
class WinMMStream;  //Early declaration

#define NUM_BUFFERS (4)

class WinMMPlugin : public IAudioPlugin
{
friend class WinMMStream;
private:

public:
	WinMMPlugin();
	void set_device( std::string desiredDevice, DEVICE_TYPES desiredType );
	AudioStream *createStream( lightspark::AudioDecoder *decoder );
	bool isTimingAvailable() const { return true; };
	~WinMMPlugin();
};

class WinMMStream: public AudioStream
{
friend class WinMMPlugin;
private:
	WinMMPlugin* manager;
	Semaphore freeBuffers;
	WAVEHDR buffer[NUM_BUFFERS];
	uint32_t curBuffer;
	Thread* workerThread;
	volatile bool threadAborting;
	DWORD preMuteVolume;
	bool paused;
	HWAVEOUT hWaveOut;
	uint32_t bufferSize;
	Mutex mutex;
	void worker();
	static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
public:
	WinMMStream(AudioDecoder* d, WinMMPlugin* m );
	uint32_t getPlayedTime();
	bool ispaused();
	bool isValid();
	void pause();
	void resume();
	void mute();
	void unmute();
	void setVolume(double);
	~WinMMStream();
};

}
#endif /* BACKENDS_INTERFACES_AUDIO_WINMM_WINMMPLUGIN_H */
