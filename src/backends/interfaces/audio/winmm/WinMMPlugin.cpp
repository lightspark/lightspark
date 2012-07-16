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

#include <iostream>
#include "backends/interfaces/audio/winmm/WinMMPlugin.h"
#include "compat.h"
#include "backends/decoder.h"

using namespace lightspark;
using namespace std;

WinMMPlugin::WinMMPlugin() : IAudioPlugin( "Windows WaveOut plugin", "winmm" )
{
}

void WinMMPlugin::set_device( std::string desiredDevice, DEVICE_TYPES desiredType )
{
}

AudioStream *WinMMPlugin::createStream ( AudioDecoder *decoder )
{
	WinMMStream *audioStream = new WinMMStream( decoder, this );
	streams.push_back( audioStream );	//Create new SoundStream
	return audioStream;
}

WinMMPlugin::~WinMMPlugin()
{
	while(!streams.empty())
		delete streams.front();
}

/****************************
Stream's functions
****************************/
WinMMStream::WinMMStream( AudioDecoder* d, WinMMPlugin* m )  :
	AudioStream(d), manager(m), freeBuffers(NUM_BUFFERS), curBuffer(0), threadAborting(false), paused(false)
{
	assert(decoder->sampleRate > 0);
	/* 1/10 seconds per buffer
	 * This should not cause any a/v-desync,
	 * as getPlayedTime() returns the actual playhead
	 * position, not the time we have buffered.
	 */
	bufferSize = decoder->sampleRate * 2 / 10;

	WAVEFORMATEX wfx;
	wfx.nSamplesPerSec = decoder->sampleRate; /* sample rate */
	wfx.wBitsPerSample = 16; /* sample size */
	wfx.nChannels = decoder->channelCount; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if(waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)&waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		LOG(LOG_ERROR,"unable to open WAVE_MAPPER device");

	if(waveOutSetVolume(hWaveOut, 0xFFFFFFFF) != MMSYSERR_NOERROR)
		LOG(LOG_ERROR,"waveOutSetVolume failed");

	memset(buffer, 0, sizeof(buffer));
	for(uint32_t i=0; i < NUM_BUFFERS; ++i)
	{
		aligned_malloc((void**)&buffer[i].lpData, wfx.nBlockAlign, bufferSize);
		buffer[i].dwBufferLength = bufferSize;
	}
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	workerThread = Thread::create(sigc::mem_fun(this,&WinMMStream::worker));
#else
	workerThread = Thread::create(sigc::mem_fun(this,&WinMMStream::worker), true);
#endif
}

WinMMStream::~WinMMStream()
{
	Mutex::Lock l(mutex);
	manager->streams.remove(this);

	threadAborting = true;
	/* wake up worker() */
	freeBuffers.signal();
	workerThread->join();

	/* Stop playing */
	if(waveOutReset(hWaveOut) != MMSYSERR_NOERROR)
		LOG(LOG_ERROR,"waveOutReset failed");

	/* Close handle */
	if(waveOutClose(hWaveOut) != MMSYSERR_NOERROR)
		LOG(LOG_ERROR,"waveOutClose failed");

	for(uint32_t i=0; i < NUM_BUFFERS; ++i)
		aligned_free(buffer[i].lpData);
}

void CALLBACK WinMMStream::waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if(uMsg != WOM_DONE)
		return;
	WinMMStream* stream = (WinMMStream*)dwInstance;
	stream->freeBuffers.signal();
}

void WinMMStream::worker()
{
	while(!threadAborting)
	{
		freeBuffers.wait();
		if(threadAborting)
			return;

		Mutex::Lock l(mutex);
		/* Unprepare previous data */
		if(buffer[curBuffer].dwFlags & WHDR_PREPARED
		  && waveOutUnprepareHeader(hWaveOut, &buffer[curBuffer], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			LOG(LOG_ERROR,"waveOutUnprepareHeader failed");

		/* Copy data from decoder */
		uint32_t freeSize = bufferSize;
		int8_t* curBufPos = (int8_t*)buffer[curBuffer].lpData;
		while(freeSize && !threadAborting)
		{
			/* copy min(freeSize,available) to buffer */
			uint32_t retSize = decoder->copyFrame ( (int16_t*)curBufPos, freeSize );
			freeSize -= retSize;
			curBufPos += retSize;
		}

		buffer[curBuffer].dwBufferLength = bufferSize;
		buffer[curBuffer].dwFlags = 0;

		/* Prepare current data */
		if(waveOutPrepareHeader(hWaveOut, &buffer[curBuffer], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			LOG(LOG_ERROR,"waveOutPrepareHeader failed");
		/* Write it to the device */
		if(waveOutWrite(hWaveOut, &buffer[curBuffer], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			LOG(LOG_ERROR,"waveOutWrite failed");

		curBuffer = (curBuffer + 1) % NUM_BUFFERS;
	}
}

void WinMMStream::pause()
{
	Mutex::Lock l(mutex);
	/* nop if already paused */
	waveOutPause(hWaveOut);
	paused = true;
}

void WinMMStream::resume()
{
	Mutex::Lock l(mutex);
	/* nop if not paused */
	waveOutRestart(hWaveOut);
	paused = false;
}

uint32_t WinMMStream::getPlayedTime( )
{
	Mutex::Lock l(mutex);
	MMTIME mmtime;
	/* TIME_MS does not work with every driver */
	mmtime.wType = TIME_SAMPLES;
	if(waveOutGetPosition(hWaveOut, &mmtime, sizeof(mmtime)) != MMSYSERR_NOERROR)
	{
		LOG(LOG_ERROR,"waveOutGetPosition failed");
		return 0;
	}
	if(mmtime.wType != TIME_SAMPLES)
	{
		LOG(LOG_ERROR,"Could not obtain playback time correct format, is " << mmtime.wType);
		return 0;
	}
	return (double)mmtime.u.sample / (double)decoder->sampleRate * 1000.0;
}

bool WinMMStream::ispaused()
{
	return paused;
}

bool WinMMStream::isValid()
{
	return true;
}

void WinMMStream::mute()
{
	Mutex::Lock l(mutex);
	DWORD curVolume;
	waveOutGetVolume(hWaveOut, &curVolume);
	if(curVolume != 0)
	{
		waveOutSetVolume(hWaveOut, 0);
		preMuteVolume = curVolume;
	}
}

void WinMMStream::unmute()
{
	Mutex::Lock l(mutex);
	DWORD curVolume;
	waveOutGetVolume(hWaveOut, &curVolume);
	if(curVolume == 0)
		waveOutSetVolume(hWaveOut, preMuteVolume);
}

void WinMMStream::setVolume(double vol)
{
	Mutex::Lock l(mutex);
	/* map vol = 0.0 to 0x0000 and vol = 1.0 to 0xFFFF,
	 * lower word is left channel, higher word is right channel */
	DWORD dwvol = MAKELPARAM(0xFFFF*vol, 0xFFFF*vol);
	waveOutSetVolume(hWaveOut, dwvol);
}

// Plugin factory function
extern "C" DLL_PUBLIC IPlugin *create()
{
	return new WinMMPlugin();
}

// Plugin cleanup function
extern "C" DLL_PUBLIC void release ( IPlugin *p_plugin )
{
	//delete the previously created object
	delete p_plugin;
}
