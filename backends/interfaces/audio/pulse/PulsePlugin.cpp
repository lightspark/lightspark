/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include "PulsePlugin.h"
#include "../../../../compat.h"
#include "../../../decoder.h"

#include <locale.h>
#include <libintl.h>
#define _(STRING) gettext(STRING)

using namespace lightspark;
using namespace std;

PulsePlugin::PulsePlugin ( PLUGIN_TYPES init_Type, string init_Name, string init_audiobackend,
                           bool init_contextReady, bool init_noServer, bool init_stopped )
{
	pluginType = init_Type;
	pluginName = init_Name;
	backendName = init_audiobackend;
	contextReady = init_contextReady;
	noServer = init_noServer;
	stopped = init_stopped;

	start();
}

void PulsePlugin::start()
{
	mainLoop = pa_threaded_mainloop_new();
	pa_threaded_mainloop_start ( mainLoop );

	pa_threaded_mainloop_lock ( mainLoop );
	context = pa_context_new ( pa_threaded_mainloop_get_api ( mainLoop ), "Lightspark" );
	pa_context_set_state_callback ( context, ( pa_context_notify_cb_t ) contextStatusCB, this );
	pa_context_connect ( context, NULL, PA_CONTEXT_NOFLAGS, NULL );
	pa_threaded_mainloop_unlock ( mainLoop );
}

void PulsePlugin::set_device ( string desiredDevice, DEVICE_TYPES desiredType )
{
	playbackDeviceName = desiredDevice;
	pa_threaded_mainloop_lock ( mainLoop );
	pa_threaded_mainloop_unlock ( mainLoop );
}

void PulsePlugin::generateDevicesList ( std::vector< string* >* devicesList, DEVICE_TYPES desiredType )
{
	pa_threaded_mainloop_lock ( mainLoop );
	pa_context_get_sink_info_list ( context, playbackListCB, &playbackDevicesList );
	pa_context_get_source_info_list ( context, captureListCB, &captureDevicesList );
	pa_threaded_mainloop_unlock ( mainLoop );

}

void PulsePlugin::captureListCB ( pa_context* context, const pa_source_info* list, int eol, void *th )
{
	PulsePlugin *oPlugin = ( PulsePlugin * ) th;
	string deviceName ( list->name );
	if ( !eol && list )   //Device found
	{
		oPlugin->addDeviceToList ( &oPlugin->captureDevicesList, &deviceName );
	}
}

void PulsePlugin::playbackListCB ( pa_context* context, const pa_sink_info* list, int eol, void *th )
{
	PulsePlugin *oPlugin = ( PulsePlugin * ) th;
	string deviceName ( list->name );
	if ( !eol && list )   //Device found
	{
		oPlugin->addDeviceToList ( &oPlugin->playbackDevicesList, &deviceName );
	}
}

void PulsePlugin::addDeviceToList ( vector< string* >* devicesList, string *deviceName )
{
	uint32_t index = devicesList->size(); //we want to add the plugin to the end of the list
	if ( devicesList->size() == ( uint32_t ) ( index ) )
	{
		devicesList->push_back ( new string ( *deviceName ) );
	}
}

void PulsePlugin::streamStatusCB ( pa_stream *stream, AudioStream *th )
{
	if ( pa_stream_get_state ( stream ) == PA_STREAM_READY )
		th->streamStatus = AudioStream::STREAM_READY;
	else if ( pa_stream_get_state ( stream ) == PA_STREAM_TERMINATED )
	{
		assert ( stream == th->stream );
		th->streamStatus = AudioStream::STREAM_DEAD;
	}
}

uint32_t PulsePlugin::getPlayedTime ( AudioStream *audioStream )
{
	assert ( audioStream );
	assert ( noServer == false );

	if ( audioStream->streamStatus != AudioStream::STREAM_READY ) //The stream is not yet ready, delay upload
		return 0;
	pa_stream* stream = audioStream->stream;

	pa_threaded_mainloop_lock ( mainLoop );
	//Request updated timing info
	pa_operation* timeUpdate = pa_stream_update_timing_info ( stream, NULL, NULL );
	pa_threaded_mainloop_unlock ( mainLoop );
	while ( pa_operation_get_state ( timeUpdate ) != PA_OPERATION_DONE );
	pa_threaded_mainloop_lock ( mainLoop );
	pa_operation_unref ( timeUpdate );

	pa_usec_t time;
	pa_stream_get_time ( stream, &time );

	pa_threaded_mainloop_unlock ( mainLoop );
	return time / 1000;
}

void PulsePlugin::fill ( AudioStream *audioStream )
{
	assert ( audioStream );
	if ( noServer == false )
	{
		if ( audioStream->streamStatus != AudioStream::STREAM_READY ) //The stream is not yet ready, delay upload
			return;
		pa_threaded_mainloop_lock ( mainLoop );
		if ( !audioStream->decoder->hasDecodedFrames() ) //No decoded data available yet, delay upload
			return;
		pa_stream *stream = audioStream->stream;
		int16_t *dest;
		//Get buffer size
		size_t frameSize = pa_stream_writable_size ( stream );
		if ( frameSize == 0 ) //The server can't accept any data now
		{
			pa_threaded_mainloop_unlock ( mainLoop );
			return;
		}
		//Write data until we have space on the server and we have data available
		uint32_t totalWritten = 0;
		pa_stream_begin_write ( stream, ( void** ) &dest, &frameSize );
		do
		{
			uint32_t retSize = audioStream->decoder->copyFrame ( dest + ( totalWritten / 2 ), frameSize );
			if ( retSize == 0 ) //There is no more data
				break;
			totalWritten += retSize;
			frameSize -= retSize;
		}
		while ( frameSize );

		cout << "Filled " << totalWritten << endl;
		if ( totalWritten )
		{
			pa_stream_write ( stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE );
			pa_stream_cork ( stream, 0, NULL, NULL ); //Start the stream, just in case it's still stopped
		}
		else
			pa_stream_cancel_write ( stream );

		pa_threaded_mainloop_unlock ( mainLoop );
	}
	else   //No sound server available
	{
		//Just skip all the contents
		audioStream->decoder->skipAll();
	}
}

void PulsePlugin::streamWriteCB ( pa_stream *stream, size_t askedData, AudioStream *th )
{
	int16_t *dest;
	//Get buffer size
	size_t frameSize = askedData;
	//Write data until we have space on the server and we have data available
	uint32_t totalWritten = 0;
	pa_stream_begin_write ( stream, ( void** ) &dest, &frameSize );
	if ( frameSize == 0 ) //The server can't accept any data now
		return;
	do
	{
		uint32_t retSize = th->decoder->copyFrame ( dest + ( totalWritten / 2 ), frameSize );
		if ( retSize == 0 ) //There is no more data
			break;
		totalWritten += retSize;
		frameSize -= retSize;
	}
	while ( frameSize );

	if ( totalWritten )
		pa_stream_write ( stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE );
	else
		pa_stream_cancel_write ( stream );
	//If the server asked for more data we have to sent it the inefficient way
	if ( totalWritten < askedData )
	{
		uint32_t restBytes = askedData - totalWritten;
		totalWritten = 0;
		dest = new int16_t[restBytes/2];
		do
		{
			uint32_t retSize = th->decoder->copyFrame ( dest + ( totalWritten / 2 ), restBytes );
			if ( retSize == 0 ) //There is no more data
				break;
			totalWritten += retSize;
			restBytes -= retSize;
		}
		while ( restBytes );
		pa_stream_write ( stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE );
		delete[] dest;
	}
	pa_stream_cork ( stream, 0, NULL, NULL ); //Start the stream, just in case it's still stopped
}

void PulsePlugin::freeStream ( AudioStream *audioStream )
{
	pa_threaded_mainloop_lock ( mainLoop );
	AudioStream *s = audioStream;
	assert ( s );
	if ( noServer == false )
	{
		pa_stream *toDelete = s->stream;
		pa_stream_disconnect ( toDelete );
	}
	//Do not delete the stream now, let's wait termination
	audioStream = NULL;
	pa_threaded_mainloop_unlock ( mainLoop );
	while ( s->streamStatus != AudioStream::STREAM_DEAD );
	pa_threaded_mainloop_lock ( mainLoop );
	if ( s->stream )
		pa_stream_unref ( s->stream );
	pa_threaded_mainloop_unlock ( mainLoop );
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

AudioStream *PulsePlugin::createStream ( AudioDecoder *decoder )
{
	AudioStream *audioStream =NULL;
	while ( !contextReady );
	pa_threaded_mainloop_lock ( mainLoop );
	uint32_t index = 0;
	for ( ;index < streams.size();index++ )
	{
		if ( streams[index] == NULL )	//Reusing a stream pointer
			break;
	}
	if ( index == streams.size() )	//Creating a new stream since the list is already full
	{
		streams.push_back ( new AudioStream ( this ) );
	}
	
	audioStream = streams[index];
	assert ( decoder->isValid() );
	audioStream->decoder = decoder;
	if ( noServer == false )
	{
		pa_sample_spec ss;
		ss.format = PA_SAMPLE_S16LE;
		ss.rate = decoder->sampleRate;
		ss.channels = decoder->channelCount;
		pa_buffer_attr attrs;
		attrs.maxlength = ( uint32_t ) - 1;
		attrs.prebuf = 0;
		attrs.tlength = ( uint32_t ) - 1;
		attrs.fragsize = ( uint32_t ) - 1;
		attrs.minreq = ( uint32_t ) - 1;
		audioStream->stream = pa_stream_new ( context, "AudioStream", &ss, NULL );
		pa_stream_set_state_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) streamStatusCB, streams[index] );
		pa_stream_set_write_callback ( audioStream->stream, ( pa_stream_request_cb_t ) streamWriteCB, streams[index] );
		pa_stream_set_underflow_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) underflow_notify, NULL );
		pa_stream_set_overflow_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) overflow_notify, NULL );
		pa_stream_set_started_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) started_notify, NULL );
		pa_stream_connect_playback ( audioStream->stream, NULL, &attrs,
		                             ( pa_stream_flags ) ( PA_STREAM_START_CORKED ), NULL, NULL );
	}
	else
	{
		//Create the stream as dead
		audioStream->streamStatus = AudioStream::STREAM_DEAD;
	}
	pa_threaded_mainloop_unlock ( mainLoop );
	return audioStream;
}

void PulsePlugin::contextStatusCB ( pa_context *context, PulsePlugin *th )
{
	switch ( pa_context_get_state ( context ) )
	{
	case PA_CONTEXT_READY:
		th->noServer = false; //In case something went wrong and the context is not correctly set
		th->contextReady = true;
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		th->noServer = true;
		th->contextReady = false; //In case something went wrong and the context is not correctly set
		th->stop(); //It should stop if the context can't be set
		break;
	default:
		break;
	}
}

PulsePlugin::~PulsePlugin()
{
	stop();
}

void PulsePlugin::stop()
{
	if ( !stopped )
	{
		stopped = true;
		pa_threaded_mainloop_lock ( mainLoop );
		for ( uint32_t i = 0;i < streams.size();i++ )
		{
			if ( streams[i] )
			{
				freeStream ( streams[i] );
			}
		}
		pa_context_disconnect ( context );
		pa_context_unref ( context );
		pa_threaded_mainloop_unlock ( mainLoop );
		pa_threaded_mainloop_stop ( mainLoop );
		pa_threaded_mainloop_free ( mainLoop );
	}
}

// Plugin factory function
extern "C" DLL_PUBLIC IPlugin *create()
{
	return new PulsePlugin();
}

// Plugin cleanup function
extern "C" DLL_PUBLIC void release ( IPlugin *p_plugin )
{
	//delete the previously created object
	delete p_plugin;
}
