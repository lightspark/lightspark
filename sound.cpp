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

using namespace lightspark;
using namespace std;

void SoundManager::streamStatusCB(pa_stream* stream, SoundStream* th)
{
	if(pa_stream_get_state(stream)==PA_STREAM_READY)
		th->streamReady=true;
	else if(pa_stream_get_state(stream)==PA_STREAM_TERMINATED)
	{
		assert(stream==th->stream);
		th->streamDead=true;
	}
}

void SoundManager::fillAndSinc(uint32_t id)
{
	//Get buffer size
	assert(streams[id-1]);
	if(streams[id-1]->streamReady==false)
		return;
	pa_stream* stream=streams[id-1]->stream;
	while(1)
	{
		pa_threaded_mainloop_lock(mainLoop);
		size_t frameSize=pa_stream_writable_size(stream);
		if(frameSize==0)
		{
			pa_threaded_mainloop_unlock(mainLoop);
			break;
		}
		int16_t* dest;
		pa_stream_begin_write(stream, (void**)&dest, &frameSize);
		uint32_t retSize=streams[id-1]->decoder->copyFrame(dest, frameSize);
		if(retSize==frameSize || retSize==0) //The buffer has been filled or there is no data currently
		{
			pa_stream_cancel_write(stream);
			pa_threaded_mainloop_unlock(mainLoop);
			break;
		}
		pa_stream_write(stream, dest, retSize, NULL, 0, PA_SEEK_RELATIVE);
		pa_threaded_mainloop_unlock(mainLoop);
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
	assert(streams[id-1]);
	pa_stream* toDelete=streams[id-1]->stream;
	pa_stream_disconnect(toDelete);
	//Do not delete the stream now, let's wait termination
	SoundStream* s=streams[id-1];
	streams[id-1]=NULL;
	pa_threaded_mainloop_unlock(mainLoop);
	while(s->streamDead==false);
	pa_threaded_mainloop_lock(mainLoop);
	pa_stream_unref(s->stream);
	pa_threaded_mainloop_unlock(mainLoop);
	delete s;
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
	if(index==streams.size())
		streams.push_back(new SoundStream(this));
	pa_sample_spec ss;
	ss.format=PA_SAMPLE_S16LE;
	ss.rate=44100;
	ss.channels=2;
	pa_buffer_attr attrs;
	attrs.maxlength=(uint32_t)-1;
	attrs.prebuf=(uint32_t)-1;
	attrs.tlength=pa_usec_to_bytes(40000,&ss);
	attrs.fragsize=(uint32_t)-1;
	attrs.minreq=(uint32_t)-1;
	streams[index]->stream=pa_stream_new(context, "SoundStream", &ss, NULL);
	streams[index]->decoder=decoder;
	pa_stream_set_state_callback(streams[index]->stream, (pa_stream_notify_cb_t)streamStatusCB, streams[index]);
	pa_stream_set_write_callback(streams[index]->stream, (pa_stream_request_cb_t)streamWriteCB, streams[index]);
	pa_stream_connect_playback(streams[index]->stream, NULL, &attrs, 
			(pa_stream_flags)(PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING), NULL, NULL);

	pa_threaded_mainloop_unlock(mainLoop);
	return index+1;
}

void SoundManager::contextStatusCB(pa_context* context, SoundManager* th)
{
	if(pa_context_get_state(context)!=PA_CONTEXT_READY)
		return;
	th->contextReady=true;
}

SoundManager::SoundManager():contextReady(false),stopped(false)
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
