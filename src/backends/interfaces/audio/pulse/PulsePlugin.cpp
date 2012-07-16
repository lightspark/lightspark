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
#include "backends/interfaces/audio/pulse/PulsePlugin.h"
#include "compat.h"
#include "backends/decoder.h"

#include <clocale>
#include <libintl.h>
#define _(STRING) gettext(STRING)

using namespace lightspark;
using namespace std;

PulsePlugin::PulsePlugin (string init_Name, string init_audiobackend, bool init_contextReady,
			  bool init_noServer, bool init_stopped ) :
		IAudioPlugin ( init_Name, init_audiobackend, init_stopped )
{
	contextReady = init_contextReady;
	noServer = init_noServer;
	stopped = init_stopped;

	start();
}

void PulsePlugin::start()
{
	mainLoop = pa_threaded_mainloop_new();
	pa_threaded_mainloop_start ( mainLoop );

	pulseLock();
	context = pa_context_new ( pa_threaded_mainloop_get_api ( mainLoop ), "Lightspark" );
	pa_context_set_state_callback ( context, ( pa_context_notify_cb_t ) contextStatusCB, this );
	pa_context_connect ( context, NULL, PA_CONTEXT_NOFLAGS, NULL );
	pulseUnlock();
}

void PulsePlugin::set_device ( string desiredDevice, DEVICE_TYPES desiredType )
{
	playbackDeviceName = desiredDevice;
	pulseLock();
	if(desiredType == PLAYBACK)
	{
		//Add code to change playback device
	}
	else if(desiredType == CAPTURE)
	{
		//Add code to change capture device
	}
	pulseUnlock();
}

void PulsePlugin::generateDevicesList ( vector< string* >* devicesList, DEVICE_TYPES desiredType )
{
	pulseLock();
	if(desiredType == PLAYBACK)
	{
		pa_context_get_sink_info_list ( context, playbackListCB, &playbackDevicesList );
	}
	else if(desiredType == CAPTURE)
	{
		pa_context_get_source_info_list ( context, captureListCB, &captureDevicesList );
	}
	pulseUnlock();

}

void PulsePlugin::captureListCB ( pa_context* context, const pa_source_info* list, int eol, void *th )
{
	PulsePlugin *oPlugin = ( PulsePlugin * ) th;
	string deviceName ( list->name );
	if ( !eol && list )   //Device found
	{
		oPlugin->captureDevicesList.push_back( new string (deviceName));
	}
}

void PulsePlugin::playbackListCB ( pa_context* context, const pa_sink_info* list, int eol, void *th )
{
	PulsePlugin *oPlugin = ( PulsePlugin * ) th;
	string deviceName ( list->name );
	if ( !eol && list )   //Device found
	{
		oPlugin->playbackDevicesList.push_back( new string (deviceName) );
	}
}

void PulsePlugin::streamStatusCB ( pa_stream *stream, PulseAudioStream *th )
{
	if ( pa_stream_get_state ( stream ) == PA_STREAM_READY )
	{
		th->streamStatus = PulseAudioStream::STREAM_READY;
		if ( th->decoder->hasDecodedFrames() )
		{
			//Now that the stream is ready fill it
			size_t availableSize = pa_stream_writable_size ( th->stream );
			th->fillStream( availableSize );
		}
	}
	else if ( pa_stream_get_state ( stream ) == PA_STREAM_TERMINATED ||
		pa_stream_get_state ( stream ) == PA_STREAM_FAILED )
	{
		assert ( stream == th->stream );
		th->streamStatus = PulseAudioStream::STREAM_DEAD;
	}
}

void PulsePlugin::streamWriteCB ( pa_stream *stream, size_t askedData, PulseAudioStream *th )
{
	th->fillStream(askedData);
}

bool PulsePlugin::isTimingAvailable() const
{
	return serverAvailable();
}

PulseAudioStream::~PulseAudioStream()
{
	manager->pulseLock();

	if( manager->serverAvailable() )
		pa_stream_disconnect( stream );

	//Do not delete the stream now, let's wait termination. However, removing it from the list.
	manager->streams.remove(this);

	manager->pulseUnlock();

	while( streamStatus != PulseAudioStream::STREAM_DEAD );

	manager->pulseLock();
	if( stream )
		pa_stream_unref ( stream );
	manager->pulseUnlock();
}

void PulsePlugin::streamOverflowCB( pa_stream *p, void *userdata )
{
	LOG(LOG_INFO, "AUDIO BACKEND: Stream overflow");
}

void PulsePlugin::streamUnderflowCB( pa_stream *p, void *userdata )
{
	LOG(LOG_INFO, "AUDIO BACKEND: Stream underflow");
}

void PulsePlugin::streamStartedCB( pa_stream *p, void *userdata )
{
	LOG(LOG_INFO, "AUDIO BACKEND: Stream started");
}

AudioStream *PulsePlugin::createStream ( AudioDecoder *decoder )
{
	PulseAudioStream *audioStream = new PulseAudioStream( this );
	streams.push_back( audioStream );	//Create new SoundStream
	if ( serverAvailable() )
	{
		while ( !contextReady );
		pulseLock();
		assert ( decoder->isValid() );

		audioStream->decoder = decoder;
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
		pa_stream_set_state_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) streamStatusCB, audioStream );
		pa_stream_set_write_callback ( audioStream->stream, ( pa_stream_request_cb_t ) streamWriteCB, audioStream );
		pa_stream_set_underflow_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) streamUnderflowCB, NULL );
		pa_stream_set_overflow_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) streamOverflowCB, NULL );
		pa_stream_set_started_callback ( audioStream->stream, ( pa_stream_notify_cb_t ) streamStartedCB, NULL );
		pa_stream_flags flags = (pa_stream_flags) PA_STREAM_START_CORKED;
		if(muteAllStreams)
			flags = (pa_stream_flags) (flags | PA_STREAM_START_MUTED);
		pa_stream_connect_playback ( audioStream->stream, NULL, &attrs,
		                             flags, NULL, NULL );
		pulseUnlock();
	}
	else
	{
		//Create the stream as dead
		audioStream->streamStatus = PulseAudioStream::STREAM_DEAD;
	}
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
		LOG(LOG_ERROR,_("AUDIO BACKEND: Connection to PulseAudio server failed"));
	case PA_CONTEXT_TERMINATED:
		th->noServer = true;
		th->contextReady = false; //In case something went wrong and the context is not correctly set
		th->stop(); //It should stop if the context can't be set
		break;
	default:
		break;
	}
}
void PulseAudioStream::pause()
{
	if(isValid() && !ispaused())
	{
		pa_stream_cork(stream, 1, NULL, NULL);	//This will stop the stream's time from running
		paused=true;
	}
}

void PulseAudioStream::resume()
{
	if(isValid() && ispaused())
	{
		pa_stream_cork(stream, 0, NULL, NULL);	//This will restart time
		paused=false;
	}
}

void PulsePlugin::pulseLock()
{
	pa_threaded_mainloop_lock(mainLoop);;
}

void PulsePlugin::pulseUnlock()
{
	pa_threaded_mainloop_unlock(mainLoop);
}

bool PulsePlugin::serverAvailable() const
{
	return !noServer;
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
		for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
		{
			delete *it;
		}
		if(serverAvailable())
		{
			pulseLock();
			pa_context_disconnect ( context );
			pa_context_unref ( context );
			pulseUnlock();
			pa_threaded_mainloop_stop ( mainLoop );
			pa_threaded_mainloop_free ( mainLoop );
		}
	}
}

void PulsePlugin::muteAll()
{
	IAudioPlugin::muteAll();
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		((PulseAudioStream*) (*it))->mute();
	}
}
void PulsePlugin::unmuteAll()
{
	IAudioPlugin::unmuteAll();
	for ( stream_iterator it = streams.begin();it != streams.end(); ++it )
	{
		((PulseAudioStream*) (*it))->unmute();
	}
}


/****************************
Stream's functions
****************************/
PulseAudioStream::PulseAudioStream ( PulsePlugin* m )  :
	AudioStream(NULL), paused(false), stream ( NULL ), manager ( m ), streamStatus ( STREAM_STARTING ),
	streamVolume(0.0)
{

}

uint32_t PulseAudioStream::getPlayedTime ( )
{
	if ( streamStatus != STREAM_READY ) //The stream is not yet ready, delay upload
		return 0;

	manager->pulseLock();
	//Request updated timing info
	pa_operation* timeUpdate = pa_stream_update_timing_info ( stream, NULL, NULL );
	manager->pulseUnlock();
	while ( pa_operation_get_state ( timeUpdate ) != PA_OPERATION_DONE );
	manager->pulseLock();
	pa_operation_unref ( timeUpdate );

	pa_usec_t time;
	pa_stream_get_time ( stream, &time );

	manager->pulseUnlock();
	return time / 1000;
}

void PulseAudioStream::fillStream(size_t toSend)
{
	/* Write data until we have space on the server and we have data available.
	 * pa_stream_begin_write will return maximum 65472 bytes, but toSend is usually much bigger
	 * so we loop until it is filled or we have no more decoded data
	 */
	while(toSend)
	{
		int16_t *dest;
		uint32_t totalWritten = 0;
		size_t frameSize = toSend;
		int ret = pa_stream_begin_write ( stream, ( void** ) &dest, &frameSize );
		(void)ret; // silence warning about unused variable
		assert(!ret);
		toSend -= frameSize;
		if (frameSize == 0)
			break;

		/* copy frames from the decoder until the buffer is full */
		do
		{
			uint32_t retSize = decoder->copyFrame ( dest + ( totalWritten / 2 ), frameSize );
			if ( retSize == 0 ) //There is no more data
				break;
			totalWritten += retSize;
			frameSize -= retSize;
		}
		while ( frameSize );

		if ( totalWritten )
		{
			pa_stream_write ( stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE );
		}
		else
		{
			//there was not any decoded data available
			pa_stream_cancel_write ( stream );
			break;
		}
	}

	if(!paused && pa_stream_is_corked(stream))
		pa_stream_cork ( stream, 0, NULL, NULL ); //Start the stream, just in case it's still stopped
}

bool PulseAudioStream::ispaused()
{
	assert_and_throw(isValid());
	return pa_stream_is_corked(stream);
}

bool PulseAudioStream::isValid()
{
	return streamStatus != STREAM_DEAD;
}

void PulseAudioStream::mute()
{
	pa_context_set_sink_input_mute(
			pa_stream_get_context(stream),
			pa_stream_get_index(stream),
			1,
			NULL,
			NULL
			);
}
void PulseAudioStream::unmute()
{
	pa_context_set_sink_input_mute(
			pa_stream_get_context(stream),
			pa_stream_get_index(stream),
			0,
			NULL,
			NULL
			);
}

void PulseAudioStream::sinkInfoForSettingVolumeCB(pa_context* context, const pa_sink_info* i, int eol, PulseAudioStream* stream)
{
	if(eol)
	{
		//The callback is called multiple times even if querying a single device
		return;
	}
	struct pa_cvolume volume;
	pa_sw_cvolume_multiply_scalar(&volume, &i->volume, stream->streamVolume*PA_VOLUME_NORM);

	pa_context_set_sink_input_volume(
			context,
			pa_stream_get_index(stream->stream),
			&volume,
			NULL,
			NULL
			);
}

void PulseAudioStream::setVolume(double vol)
{
	if(vol==streamVolume)
		return;
	streamVolume=vol;
	uint32_t deviceIndex = pa_stream_get_device_index(stream);
	pa_operation* op=pa_context_get_sink_info_by_index(
			pa_stream_get_context(stream),
			deviceIndex,
			(pa_sink_info_cb_t)sinkInfoForSettingVolumeCB,
			this);
	pa_operation_unref(op);
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
