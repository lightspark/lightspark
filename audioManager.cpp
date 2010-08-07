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
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "audioManager.h"
#include <iostream>
#include <string.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#if defined WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <sys/types.h>
#endif

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;
using namespace boost;


void AudioManager::fillAndSyncPlugin(uint32_t id, uint32_t streamTime)
{
  o_AudioPlugin->fillAndSync(id, streamTime);
}

void AudioManager::freeStreamPlugin(uint32_t id)
{
  o_AudioPlugin->freeStream(id);
}

uint32_t AudioManager::createStreamPlugin(AudioDecoder *decoder)
{
  return o_AudioPlugin->createStream(decoder);
}


/****************
AudioManager::AudioManager
*****************
It should search for a list of audio plugin lib files (liblightsparkaudio-AUDIOAPI.so)
If it finds any
  Verify they are valid plugin
  If true, list the audio API in a list of audiobackends
  Else nothing
Else nothing

Then, ut should read a config file containing the user's defined audio API choosen as audio backend
If no file or none selected
  default to none
Else
  Select and load the good audio plugin lib files
*****************/

AudioManager::AudioManager()
{
  FindAudioPlugins();
}

/**************************
stop AudioManager
***************************/
AudioManager::~AudioManager()
{
  delete &o_AudioPlugin;
}

void AudioManager::stopPlugin()
{
  o_AudioPlugin->stop();
}

/***************************
Find liblightsparkAUDIOplugin libraries
Load
****************************/
void AudioManager::FindAudioPlugins()
{
  //Search for all files under ${PRIVATELIBDIR}/plugins
  //Verify if they are audio plugins
  //If true, add to list of audio plugins
  string froot(PRIVATELIBDIR), fplugins("/plugins"); //LS should always look in the plugins folder, nowhere else
  const path plugins_folder = froot + fplugins;
  regex file_pattern("liblightspark*plugin.*"); //pattern of ls plugins

  #if defined DEBUG
    cout << "Looking for plugins under " << plugins_folder << " for file_name " << file_pattern << endl;
  #endif

  if(!is_directory(plugins_folder))
  {
    cout << "The plugins folder doesn't exists under " << plugins_folder << endl;
  }
  else
  {
    for(recursive_directory_iterator itr(plugins_folder), end_itr; itr != end_itr; ++itr)
    {
      if(is_regular_file(itr->status())) //Is it a real file? This will remove symlink
      {
	string leaf_name = itr->path().filename();
	if(regex_match(leaf_name, file_pattern)) // Does it answer to the desired pattern?
	{
	  //Try to load the file and see if it's an audio plugin
	  HMODULE h_plugin = LoadLib(leaf_name.c_str());
	  if(h_plugin != NULL) // Is it a library?
	  {
	    PLUGIN_FACTORY p_factory_function = (PLUGIN_FACTORY) ExtractLibContent(h_plugin, "Create_Plugin");
	    PLUGIN_CLEANUP p_cleanup_function = (PLUGIN_CLEANUP) ExtractLibContent(h_plugin, "Release_Plugin");
	    if (p_factory_function != NULL && p_cleanup_function != NULL) //Does it contain the LS IPlugin?
	    {
	      IPlugin *p_plugin = (*p_factory_function)(); //Instanciate the plugin
	      if(p_plugin->get_pluginType() == AUDIO) //Is it really an AUDIO plugin?
	      {
		IAudioPlugin *p_audioplugin = static_cast<IAudioPlugin *>(p_plugin);
		#if defined DEBUG
		  printf("Plugin %s of type %d\n", p_audioplugin->get_pluginName(), p_audioplugin->get_pluginType());
		#endif
		string e("test");
		AddAudioPluginToList(p_audioplugin->get_audioBackend_name(), itr->path().string()); //Add the filename the audio plugins list
	      }
	    }
	  }
	  CloseLib(h_plugin);
	}
      }
    }
  }
}

void AudioManager::AddAudioPluginToList(string AudioBackend_name, string PathToPlugin)
{
}