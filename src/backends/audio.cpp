/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "swf.h"
#include "backends/audio.h"
#include "backends/config.h"
#include <iostream>
#include "logger.h"

//Needed or not with compat.h and compat.cpp?
#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#	include <sys/types.h>
#endif

using namespace lightspark;
using namespace std;

/****************
AudioManager::AudioManager
*****************
It should search for a list of audio plugin lib files (liblightsparkAUDIOAPIplugin.so)
Then, it should read a config file containing the user's defined audio API choosen as audio backend
If no file or none selected
  default to none
Else
  Select and load the good audio plugin lib files
*****************/

AudioManager::AudioManager ( PluginManager *sharedPluginManager ) :
	oAudioPlugin(NULL), selectedAudioBackend(""), pluginManager(sharedPluginManager)
{
//	  string DesiredAudio = get_audioConfig(); //Looks for the audio selected in the user's config
	string DesiredAudio = Config::getConfig()->getAudioBackendName();
	set_audiobackend ( DesiredAudio );
}

bool AudioManager::pluginLoaded() const
{
	return oAudioPlugin != NULL;
}

AudioStream *AudioManager::createStreamPlugin ( AudioDecoder *decoder )
{
	if ( pluginLoaded() )
	{
		return oAudioPlugin->createStream ( decoder );
	}
	else
	{
		LOG ( LOG_ERROR, _ ( "No audio plugin loaded, can't create stream" ) );
		return NULL;
	}
}

bool AudioManager::isTimingAvailablePlugin() const
{
	if ( pluginLoaded() )
	{
		return oAudioPlugin->isTimingAvailable();
	}
	else
	{
		LOG ( LOG_ERROR, _ ( "isTimingAvailablePlugin: No audio plugin loaded" ) );
		return false;
	}
}

void AudioManager::set_audiobackend ( string desired_backend )
{
	if ( selectedAudioBackend != desired_backend )  	//Load the desired backend only if it's not already loaded
	{
		load_audioplugin ( desired_backend );
		selectedAudioBackend = desired_backend;
	}
}

void AudioManager::get_audioBackendsList()
{
	audioplugins_list = pluginManager->get_backendsList ( AUDIO );
}

void AudioManager::refresh_audioplugins_list()
{
	audioplugins_list.clear();
	get_audioBackendsList();
}

void AudioManager::release_audioplugin()
{
	if ( pluginLoaded() )
	{
		pluginManager->release_plugin ( oAudioPlugin );
	}
}

void AudioManager::load_audioplugin ( string selected_backend )
{
	LOG ( LOG_INFO, _ ( ( ( string ) ( "the selected backend is: " + selected_backend ) ).c_str() ) );
	release_audioplugin();
	oAudioPlugin = static_cast<IAudioPlugin *> ( pluginManager->get_plugin ( selected_backend ) );

	if ( !pluginLoaded() )
	{
		LOG ( LOG_INFO, _ ( "Could not load the audiobackend" ) );
	}
}

/**************************
stop AudioManager
***************************/
AudioManager::~AudioManager()
{
	release_audioplugin();
	pluginManager = NULL;	//The plugin manager is not deleted since it's been created outside of the audio manager
}
