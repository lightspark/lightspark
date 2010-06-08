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

void successCB(pa_stream* stream, int success, void* userData)
{
	uintptr_t val=(uintptr_t)userData;
	if(val!=0x1)
		return;

	cout << "Success " << success << endl;
	uint64_t usec=0;
	int neg=0;
	pa_stream_get_latency(stream,&usec,&neg);
	cout << "usec " << usec << endl;
	cout << "neg " << neg << endl;
}

void streamCB(pa_stream* stream, void* c)
{
	if(pa_stream_get_state(stream)!=PA_STREAM_READY)
		return;
	pa_stream_update_timing_info(stream, successCB, (void*)0x1);
	//Create a fake buffer
	char* data=new char[44100*10];
	for(int i=0;i<441000;i++)
		data[i]=sin(i*M_PI/90)*100;
	pa_stream_write(stream, data, 441000, NULL, 0, PA_SEEK_RELATIVE);
}

void statusCB(pa_context* context, void* mainLoop)
{
	if(pa_context_get_state(context)!=PA_CONTEXT_READY)
		return;
	pa_sample_spec ss;
	ss.format=PA_SAMPLE_U8;
	ss.rate=44100;
	ss.channels=1;
	pa_stream* stream=pa_stream_new(context, "puppa", &ss, NULL);
	pa_stream_connect_playback(stream, NULL, NULL, PA_STREAM_NOFLAGS, NULL, NULL);
	pa_stream_set_state_callback(stream, streamCB,NULL);
}

SoundManager::SoundManager()
{
	mainLoop=pa_threaded_mainloop_new();
	pa_threaded_mainloop_start(mainLoop);

	pa_threaded_mainloop_lock(mainLoop);
	pa_context* context=pa_context_new(pa_threaded_mainloop_get_api(mainLoop),"Lightspark");
	pa_context_set_state_callback(context, statusCB, mainLoop);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	pa_threaded_mainloop_unlock(mainLoop);
	/*pa_threaded_mainloop_lock(mainLoop);
	pa_stream_disconnect(stream);
	pa_stream_unref(stream);
	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_threaded_mainloop_unlock(mainLoop);*/
}

SoundManager::~SoundManager()
{
	pa_threaded_mainloop_stop(mainLoop);
	pa_threaded_mainloop_free(mainLoop);
}
