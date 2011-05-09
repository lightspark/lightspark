/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "OpenALPlugin.h"

using namespace lightspark;
using namespace std;


OpenALPlugin::OpenALPlugin ( PLUGIN_TYPES init_Type, string init_Name, string init_audiobackend,
                             bool init_contextReady, bool init_noServer, bool init_stopped )
{
	pluginType = init_Type;
	pluginName = init_Name;
	backendName = init_audiobackend;
	contextReady = init_contextReady;
	noServer = init_noServer;
	stopped = init_stopped;
	playbackDevice = NULL;
	captureDevice = NULL;

	start();
}

void OpenALPlugin::start()
{
	/*
	Generate lists (playback and captures)
	Find info on selected devices in config file
	open devices
	create context and activate it
	create buffers
	create sources
	Don't forget to check for errors
	*/
	ALenum error;

	generateDevicesList ( playbackDevicesList, PLAYBACK );
	generateDevicesList ( captureDevicesList, CAPTURE );
	//getConfig() //To be implemented at a later time

	initPlayback ( this );
	initCapture ( this );
}

void OpenALPlugin::initCapture ( OpenALPlugin *th )
{
	th->captureDevice = alcCaptureOpenDevice ( NULL );	//NULL to be changed to the one from the config

	if ()	//verify capture device could be opened
	{
	}
}

void OpenALPlugin::initPlayback ( OpenALPlugin *th )
{
	th->playbackDevice = alcOpenDevice ( NULL );	//NULL to be changed to the one from the config

	if ( th->playbackDevice ) //verify playback device could be opened
	{
		context = alcCreateContext ( th->playbackDevice, NULL );
		alcMakeContextCurrent ( th->context );
	}

	alGetError();	//Clearing error code
	alGenBuffers ( NUM_BUFFERS, th->pbBuffers );
	if ( ( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenBuffers :" << error << endl;
		return;
	}

	alGenSources ( NUM_SOURCES, th->pbSources );
	if ( ( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenSources :" << error << endl;
		return;
	}

}

void OpenALPlugin::streamStatusCB ( pa_stream *stream, AudioStream *th )
{/*
	if(pa_stream_get_state(stream)==PA_STREAM_READY)
		th->streamStatus=AudioStream::STREAM_READY;
	else if(pa_stream_get_state(stream)==PA_STREAM_TERMINATED)
	{
		assert(stream==th->stream);
		th->streamStatus=AudioStream::STREAM_DEAD;
	}
*/}

uint32_t OpenALPlugin::getPlayedTime ( uint32_t id )
{/*
	assert(streams[id-1]);
	assert(noServer==false);

	if(streams[id-1]->streamStatus!=AudioStream::STREAM_READY) //The stream is not yet ready, delay upload
		return 0;
	pa_stream* stream=streams[id-1]->stream;

	pa_threaded_mainloop_lock(mainLoop);
	//Request updated timing info
	pa_operation* timeUpdate=pa_stream_update_timing_info(stream, NULL, NULL);
	pa_threaded_mainloop_unlock(mainLoop);
	while(pa_operation_get_state(timeUpdate)!=PA_OPERATION_DONE);
	pa_threaded_mainloop_lock(mainLoop);
	pa_operation_unref(timeUpdate);

	pa_usec_t time;
	pa_stream_get_time(stream, &time);

	pa_threaded_mainloop_unlock(mainLoop);
	return time/1000;
*/}

void OpenALPlugin::fill ( uint32_t id )
{/*
	assert(streams[id-1]);
	if(noServer==false)
	{
		if(streams[id-1]->streamStatus!=AudioStream::STREAM_READY) //The stream is not yet ready, delay upload
			return;
		pa_threaded_mainloop_lock(mainLoop);
		if(!streams[id-1]->decoder->hasDecodedFrames()) //No decoded data available yet, delay upload
			return;
		pa_stream *stream=streams[id-1]->stream;
		int16_t *dest;
		//Get buffer size
		size_t frameSize=pa_stream_writable_size(stream);
		if(frameSize==0) //The server can't accept any data now
		{
			pa_threaded_mainloop_unlock(mainLoop);
			return;
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

		cout << "Filled " << totalWritten << endl;
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
*/}

void OpenALPlugin::streamWriteCB ( pa_stream *stream, size_t askedData, AudioStream *th )
{/*
	int16_t *dest;
	//Get buffer size
	size_t frameSize=askedData;
	//Write data until we have space on the server and we have data available
	uint32_t totalWritten=0;
	pa_stream_begin_write(stream, (void**)&dest, &frameSize);
	if(frameSize==0) //The server can't accept any data now
		return;
	do
	{
		uint32_t retSize=th->decoder->copyFrame(dest+(totalWritten/2), frameSize);
		if(retSize==0) //There is no more data
			break;
		totalWritten+=retSize;
		frameSize-=retSize;
	}
	while(frameSize);

	if(totalWritten)
		pa_stream_write(stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE);
	else
		pa_stream_cancel_write(stream);
	//If the server asked for more data we have to sent it the inefficient way
	if(totalWritten<askedData)
	{
		uint32_t restBytes=askedData-totalWritten;
		totalWritten=0;
		dest=new int16_t[restBytes/2];
		do
		{
			uint32_t retSize=th->decoder->copyFrame(dest+(totalWritten/2), restBytes);
			if(retSize==0) //There is no more data
				break;
			totalWritten+=retSize;
			restBytes-=retSize;
		}
		while(restBytes);
		pa_stream_write(stream, dest, totalWritten, NULL, 0, PA_SEEK_RELATIVE);
		delete[] dest;
	}
	pa_stream_cork(stream, 0, NULL, NULL); //Start the stream, just in case it's still stopped
*/}

void OpenALPlugin::freeStream ( uint32_t id )
{/*
	pa_threaded_mainloop_lock(mainLoop);
	AudioStream *s=streams[id-1];
	assert(s);
	if(noServer==false)
	{
		pa_stream *toDelete=s->stream;
		pa_stream_disconnect(toDelete);
	}
	//Do not delete the stream now, let's wait termination
	streams[id-1]=NULL;
	pa_threaded_mainloop_unlock(mainLoop);
	while(s->streamStatus!=AudioStream::STREAM_DEAD);
	pa_threaded_mainloop_lock(mainLoop);
	if(s->stream)
		pa_stream_unref(s->stream);
	pa_threaded_mainloop_unlock(mainLoop);
	delete s;
*/}

void overflow_notify()
{
	LOG(LOG_NO_INFO, "AUDIO BACKEND: ____overflow!!!!");
}

void underflow_notify()
{
	LOG(LOG_NO_INFO, "AUDIO BACKEND: ____underflow!!!!");
}

void started_notify()
{
	LOG(LOG_NO_INFO, "AUDIO BACKEND: ____started!!!!");
}

uint32_t OpenALPlugin::createStream ( AudioDecoder *decoder )
{/*
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
		streams.push_back(new AudioStream(this));
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
		streams[index]->stream=pa_stream_new(context, "AudioStream", &ss, NULL);
		pa_stream_set_state_callback(streams[index]->stream, (pa_stream_notify_cb_t)streamStatusCB, streams[index]);
		pa_stream_set_write_callback(streams[index]->stream, (pa_stream_request_cb_t)streamWriteCB, streams[index]);
		pa_stream_set_underflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)underflow_notify, NULL);
		pa_stream_set_overflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)overflow_notify, NULL);
		pa_stream_set_started_callback(streams[index]->stream, (pa_stream_notify_cb_t)started_notify, NULL);
		pa_stream_connect_playback(streams[index]->stream, NULL, &attrs,
			(pa_stream_flags)(PA_STREAM_START_CORKED), NULL, NULL);
	}
	else
	{
		//Create the stream as dead
		streams[index]->streamStatus=AudioStream::STREAM_DEAD;
	}
	pa_threaded_mainloop_unlock(mainLoop);
	return index+1;
*/}

void OpenALPlugin::contextStatusCB ( pa_context *context, OpenALPlugin *th )
{/*
	switch(pa_context_get_state(context))
	{
		case PA_CONTEXT_READY:
			th->contextReady=true;
			break;
		case PA_CONTEXT_FAILED:
			th->noServer=true;
			th->contextReady=false;
			th->stop();
			LOG(LOG_ERROR,"Connection to OpenAL server failed");
			break;
		default:
			break;
	}
*/}

void OpenALPlugin::generateDevicesList ( vector< string* >* devicesList, DEVICE_TYPES desiredType )
{
	if ( alcIsExtensionPresent ( NULL, "ALC_ENUMERATION_EXT" ) == AL_TRUE ) //Check if the extension if found
	{
		ALCchar *devices;
		if ( desiredType == PLAYBACK )
		{
			devices = alcGetString ( NULL, ALC_DEVICE_SPECIFIER );
		}
		else if ( desiredType == CAPTURE )
		{
			devices = alcGetString ( NULL, ALC_CAPTURE_DEVICE_SPECIFIER );
		}

		while () //Split the devices' name and add them to the device list
		{
			addDeviceToList ( devicesList, deviceName );
		}
	}
}

void OpenALPlugin::addDeviceToList ( std::vector< string* >* devicesList, string* deviceName )
{
	uint32_t index = devicesList->size(); //we want to add the plugin to the end of the list
	if ( devicesList->size() == ( uint32_t ) ( index ) )
	{
		devicesList->push_back ( new string ( *deviceName ) );
	}
}

vector< string* >* OpenALPlugin::get_devicesList ( DEVICE_TYPES desiredType )
{
	if ( desiredType == PLAYBACK )
	{
		return playbackDevicesList;
	}
	else if ( desiredType == CAPTURE )
	{
		return captureDevicesList;
	}
}

/**********************
desiredDevice should be empty to use the default. Else, it should be the name of the device.
When setting a device, we should check if there is a context active using the current device
  If so, suspend it, unload previous device, load the new one, update the context and restart it
**********************/
void OpenALPlugin::set_device ( string desiredDeviceName, DEVICE_TYPES desiredType, ALCdevice *pDevice )
{
	ALCdevice *tmpDevice = NULL;

	if ( ( desiredType == PLAYBACK ) && ( selectedPlaybackDevice != desiredDevice ) )
	{
		if ( desiredDevice == "" ) //Find the default device
		{
			if ( desiredType == PLAYBACK )
			{
				desiredDevice = desiredDevicealcGetString ( NULL, ALC_DEFAULT_DEVICE_SPECIFIER );
			}
			else if ( desiredType == CAPTURE )
			{
				desiredDevice = desiredDevicealcGetString ( NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER );
			}
		}

		if ( tmpDevice = alcOpenDevice ( desiredDevice.c_str() ) ) //The device could be opened
		{
			if ( pDevice != NULL ) //Close the old device if opened
			{
				alcCloseDevice ( pDevice );
			}
			pDevice = tmpDevice;
			if ( desiredType == PLAYBACK )
			{
				playbackDeviceName = tmpDevice;
			}
			else if ( desiredType == CAPTURE )
			{
				captureDeviceName = tmpDevice;
			}
		}
	}
}

OpenALPlugin::~OpenALPlugin()
{
	stop();
}

void OpenALPlugin::stop()
{
	if ( !stopped )
	{
		stopped = true;

		// stop context
		// delete sources
		// delete buffers
		// delete context


		// close devices
		if ( ( playbackDevice != NULL ) || ( captureDevice != NULL ) )
		{
			alcCloseDevice ( playback );
			alcCloseDevice ( capture );
		}
	}
}

// Plugin factory function
extern "C" DLL_PUBLIC IPlugin *create()
{
	return new OpenALPlugin();
}

// Plugin cleanup function
extern "C" DLL_PUBLIC void release ( IPlugin *p_plugin )
{
	//delete the previously created object
	delete p_plugin;
}
