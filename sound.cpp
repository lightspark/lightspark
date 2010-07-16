/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <string.h>
#include "sound.h"

#ifdef ENABLE_SOUND
using namespace lightspark;
using namespace std;

void SoundManager::streamStatusCB(pa_stream* stream, SoundStream* th)
{
	if(pa_stream_get_state(stream)==PA_STREAM_READY)
		th->streamStatus=SoundStream::STREAM_READY;
	else if(pa_stream_get_state(stream)==PA_STREAM_TERMINATED)
	{
		assert(stream==th->stream);
		th->streamStatus=SoundStream::STREAM_DEAD;
	}
}

void SoundManager::fillAndSync(uint32_t id, uint32_t streamTime)
{
	assert(streams[id-1]);
	if(noServer==false)
	{
		if(streams[id-1]->streamStatus!=SoundStream::STREAM_READY) //The stream is not yet ready, delay upload
			return;
		if(!streams[id-1]->decoder->hasDecodedFrames()) //No decoded data available yet, delay upload
			return;
		pa_stream* stream=streams[id-1]->stream;
		pa_threaded_mainloop_lock(mainLoop);
		int16_t* dest;
		//Get buffer size
		size_t frameSize=pa_stream_writable_size(stream);
		if(frameSize==0) //The server can't accept any data now
		{
			pa_threaded_mainloop_unlock(mainLoop);
			return;
		}
		//Request updated timing info
		pa_operation* timeUpdate=pa_stream_update_timing_info(stream, NULL, NULL);
		pa_threaded_mainloop_unlock(mainLoop);
		while(pa_operation_get_state(timeUpdate)!=PA_OPERATION_DONE);
		pa_threaded_mainloop_lock(mainLoop);
		pa_operation_unref(timeUpdate);

		//Get the latency of the stream, in usecs
		pa_usec_t latency;
		int negative;
		int ret=pa_stream_get_latency(stream, &latency, &negative);
		if(ret!=0) //Latency update failed, abort upload
		{
			LOG(LOG_ERROR, "Upload of samples to pulse failed");
			pa_threaded_mainloop_unlock(mainLoop);
			return;
		}
		assert(negative==0);
		//Transform latency in milliseconds
		latency/=1000; //This is the time that will pass before the first written samples will be played
		uint32_t frontTime=streams[id-1]->decoder->getFrontTime();
		int32_t frontLatency=frontTime-streamTime; //This is the time that should pass before playing the first available samples
		//The first sample we write will be played after latency ms. So we want to skip until streamTime+latency
		int32_t gapTime=latency-frontLatency; //This is the gap in the two latencies
		//The idea is to skip or add some samples to adapt to the expected latency
		//Currently the correction unit is an arbitrary 150 usecs. Would be way better to use a complex poles based control
		//and distributing a bit new/removed samples
		if(gapTime==0);
		else if(gapTime>0)
			streams[id-1]->decoder->skipUntil(frontTime, 150);
		else
		{
			uint32_t bytesNeededToFillTheGap=0;
			bytesNeededToFillTheGap=150*streams[id-1]->decoder->getBytesPerMSec()/1000;
			bytesNeededToFillTheGap&=0xfffffffe;
			int16_t* tmp=new int16_t[bytesNeededToFillTheGap/2];
			memset(tmp,0,bytesNeededToFillTheGap);
			pa_stream_write(stream, tmp, bytesNeededToFillTheGap, NULL, 0, PA_SEEK_RELATIVE);
			delete[] tmp;
		}

		//Write data until we have space on the server and we have data available
		uint32_t totalWritten=0;
		pa_stream_begin_write(stream, (void**)&dest, &frameSize);
		do
		{
			uint32_t retSize=streams[id-1]->decoder->copyFrame(dest+(totalWritten/2), frameSize);
			if(retSize==0) //There is no more data
				break;
			totalWritten+=retSize;
			frameSize-=retSize;
		}
		while(frameSize);

		if(totalWritten)
		{
			pa_stream_write(stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE);
			pa_stream_cork(stream, 0, NULL, NULL); //Start the stream, just in case it's still stopped
		}
		else
			pa_stream_cancel_write(stream);

		pa_threaded_mainloop_unlock(mainLoop);
	}
	else //No sound server available
	{
		//Just skip all the contents
		streams[id-1]->decoder->skipAll();
	}
}

void SoundManager::streamWriteCB(pa_stream* stream, size_t frameSize, SoundStream* th)
{
	//Get buffer size
	int16_t* dest;
	pa_stream_begin_write(stream, (void**)&dest, &frameSize);
	uint32_t retSize=th->decoder->copyFrame(dest, frameSize);
	pa_stream_write(stream, dest, retSize, NULL, 0, PA_SEEK_RELATIVE);
}

void SoundManager::freeStream(uint32_t id)
{
	pa_threaded_mainloop_lock(mainLoop);
	SoundStream* s=streams[id-1];
	assert(s);
	if(noServer==false)
	{
		pa_stream* toDelete=s->stream;
		pa_stream_disconnect(toDelete);
	}
	//Do not delete the stream now, let's wait termination
	streams[id-1]=NULL;
	pa_threaded_mainloop_unlock(mainLoop);
	while(s->streamStatus!=SoundStream::STREAM_DEAD);
	pa_threaded_mainloop_lock(mainLoop);
	if(s->stream)
		pa_stream_unref(s->stream);
	pa_threaded_mainloop_unlock(mainLoop);
	delete s;
}

void overflow_notify()
{
	cout << "____overflow!!!!" << endl;
}

void underflow_notify()
{
	cout << "____underflow!!!!" << endl;
}

void started_notify()
{
	cout << "____started!!!!" << endl;
}

uint32_t SoundManager::createStream(AudioDecoder* decoder)
{
	while(!contextReady);
	pa_threaded_mainloop_lock(mainLoop);
	uint32_t index=0;
	for(;index<streams.size();index++)
	{
		if(streams[index]==NULL)
			break;
	}
	assert(decoder->isValid());
	if(index==streams.size())
		streams.push_back(new SoundStream(this));
	streams[index]->decoder=decoder;
	if(noServer==false)
	{
		pa_sample_spec ss;
		ss.format=PA_SAMPLE_S16LE;
		ss.rate=decoder->sampleRate;
		ss.channels=decoder->channelCount;
		pa_buffer_attr attrs;
		attrs.maxlength=(uint32_t)-1;
		attrs.prebuf=0;
		attrs.tlength=(uint32_t)-1;
		attrs.fragsize=(uint32_t)-1;
		attrs.minreq=(uint32_t)-1;
		streams[index]->stream=pa_stream_new(context, "SoundStream", &ss, NULL);
		pa_stream_set_state_callback(streams[index]->stream, (pa_stream_notify_cb_t)streamStatusCB, streams[index]);
		//pa_stream_set_write_callback(streams[index]->stream, (pa_stream_request_cb_t)streamWriteCB, streams[index]);
		pa_stream_set_underflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)underflow_notify, NULL);
		pa_stream_set_overflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)overflow_notify, NULL);
		pa_stream_set_started_callback(streams[index]->stream, (pa_stream_notify_cb_t)started_notify, NULL);
		pa_stream_connect_playback(streams[index]->stream, NULL, &attrs, 
			(pa_stream_flags)(PA_STREAM_START_CORKED), NULL, NULL);
	}
	else
	{
		//Create the stream as dead
		streams[index]->streamStatus=SoundStream::STREAM_DEAD;
	}
	pa_threaded_mainloop_unlock(mainLoop);
	return index+1;
}

void SoundManager::contextStatusCB(pa_context* context, SoundManager* th)
{
	switch(pa_context_get_state(context))
	{
		case PA_CONTEXT_READY:
			th->contextReady=true;
			break;
		case PA_CONTEXT_FAILED:
			th->noServer=true;
			th->contextReady=true;
			LOG(LOG_ERROR,"Connection to PulseAudio server failed");
			break;
		default:
			break;
	}
}

SoundManager::SoundManager():contextReady(false),noServer(false),stopped(false)
{
	mainLoop=pa_threaded_mainloop_new();
	pa_threaded_mainloop_start(mainLoop);

	pa_threaded_mainloop_lock(mainLoop);
	context=pa_context_new(pa_threaded_mainloop_get_api(mainLoop),"Lightspark");
	pa_context_set_state_callback(context, (pa_context_notify_cb_t)contextStatusCB, this);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	pa_threaded_mainloop_unlock(mainLoop);
}

SoundManager::~SoundManager()
{
	stop();
}

void SoundManager::stop()
{
	if(!stopped)
	{
		stopped=true;
		pa_threaded_mainloop_lock(mainLoop);
		for(uint32_t i=0;i<streams.size();i++)
		{
			if(streams[i])
				freeStream(i+1);
		}
		pa_context_disconnect(context);
		pa_context_unref(context);
		pa_threaded_mainloop_unlock(mainLoop);
		pa_threaded_mainloop_stop(mainLoop);
		pa_threaded_mainloop_free(mainLoop);
	}
}
#endif
