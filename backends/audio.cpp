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
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "audio.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

//Needed or not with compat.h and compat.cpp?
#if defined WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <sys/types.h>
#endif

#ifdef ENABLE_SOUND
using namespace lightspark;
using namespace std;
using namespace boost::filesystem;
using namespace boost;


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

AudioManager::AudioManager(PluginManager *sharedPluginManager)
{
  pluginManager = sharedPluginManager;
  selectedAudioBackend = "";
  oAudioPlugin = NULL;
//string DesiredAudio = get_audioConfig(); //Looks for the audio selected in the user's config
  string DesiredAudio = "pulse";
  set_audiobackend(DesiredAudio);
}

void AudioManager::fillPlugin(uint32_t id)
{
  oAudioPlugin->fill(id);
}

void AudioManager::freeStreamPlugin(uint32_t id)
{
  oAudioPlugin->freeStream(id);
}

uint32_t AudioManager::createStreamPlugin(AudioDecoder *decoder)
{
  return oAudioPlugin->createStream(decoder);
}

uint32_t AudioManager::getPlayedTimePlugin(uint32_t streamId)
{
  return oAudioPlugin->getPlayedTime(streamId);
}

bool AudioManager::isTimingAvailablePlugin() const
{
  return oAudioPlugin->isTimingAvailable();
}

void AudioManager::set_audiobackend(string desired_backend)
{
  if(selectedAudioBackend != desired_backend)	//Load the desired backend only if it's not already loaded
  {
    load_audioplugin(desired_backend);
    selectedAudioBackend = desired_backend;
  }
}

void AudioManager::get_audioBackendsList()
{
  audioplugins_list = pluginManager->get_backendsList(AUDIO);
}

void AudioManager::refresh_audioplugins_list()
{
  audioplugins_list.clear();
  get_audioBackendsList();
}

void AudioManager::release_audioplugin()
{
  if(oAudioPlugin != NULL)
  {
    oAudioPlugin->stop();
    pluginManager->release_plugin(oAudioPlugin);
  }
}

void AudioManager::load_audioplugin(string selected_backend)
{
  release_audioplugin();
  oAudioPlugin = static_cast<IAudioPlugin *>(pluginManager->get_plugin(selected_backend));
//#if defined DEBUG
  if(oAudioPlugin != NULL)
  {
  cout << "The following audio plugin has been loaded: " << oAudioPlugin->get_pluginName() << endl;
  }
  else
  {
    cout << "The desired backend (" << selected_backend << ") could not be loaded." << endl;
  }
//#endif
}

/**************************
stop AudioManager
***************************/
AudioManager::~AudioManager()
{
  release_audioplugin();
  pluginManager = NULL;	//The plugin manager is not deleted since it's been created outside of the audio manager
}

void AudioManager::stopPlugin()
{
  oAudioPlugin->stop();
}

#endif
