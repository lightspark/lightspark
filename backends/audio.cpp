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
#include <string.h>
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

AudioManager::AudioManager()
{
  SelectedAudioPlugin = -1; //Make sure it doesn't point anywhere in the list
  FindAudioPlugins();
//string DesiredAudio = get_audioConfig(); //Looks for the audio selected in the user's config
  string DesiredAudio = "pulse";
  select_audiobackend(DesiredAudio);
}

void AudioManager::fillPlugin(uint32_t id)
{
  o_AudioPlugin->fill(id);
}

void AudioManager::freeStreamPlugin(uint32_t id)
{
  o_AudioPlugin->freeStream(id);
}

uint32_t AudioManager::createStreamPlugin(AudioDecoder *decoder)
{
  return o_AudioPlugin->createStream(decoder);
}

uint32_t AudioManager::getPlayedTimePlugin(uint32_t streamId)
{
  return o_AudioPlugin->getPlayedTime(streamId);
}

bool AudioManager::isTimingAvailablePlugin() const
{
  return o_AudioPlugin->isTimingAvailable();
}

void AudioManager::select_audiobackend(string selected_backend)
{
  uint32_t index=0;
  for(;index<AudioPluginsList.size();index++) //Find the desired plugin in the list and load it
  {
    if(AudioPluginsList[index]->audiobackend_name==selected_backend) //If true, backend is available
    {
      if(SelectedAudioPlugin != index) //Unload and/or load plugin only if the plugin is not the same as selected
      {
	//Unload previous plugin and release handle
	UnloadPlugin();
	
	//Load selected plugin and instanciate it
	LoadPlugin(AudioPluginsList[index]->plugin_path, index);
      }
      break;
    }
  }
}

/*//Takes care to load and instanciate anything related to the plugin
void AudioManager::LoadPlugin(string pluginPath, uint32_t index)
{
}
*/

/*
//Takes care of unloading and releasing anything related to the plugin
void AudioManager::UnloadPlugin()
{
  if(o_AudioPlugin && hSelectedAudioPluginLib) //If there is already a backend loaded, unload it
  {
    PLUGIN_CLEANUP p_cleanup_function = (PLUGIN_CLEANUP) ExtractLibContent(hSelectedAudioPluginLib, "release");
    if(p_cleanup_function != NULL)
    {
      p_cleanup_function(o_AudioPlugin);
      o_AudioPlugin = NULL;
    }
    SelectedAudioPlugin = -1; //Unselecting any entry in the plugins list
    CloseLib(hSelectedAudioPluginLib);
  }
}
*/

/**************************
stop AudioManager
***************************/
AudioManager::~AudioManager()
{
  UnloadPlugin(); //Unload, release the plugin if any and close plugin if needed
}

void AudioManager::stopPlugin()
{
  o_AudioPlugin->stop();
}

/***************************
Find liblightsparkAUDIOplugin libraries
****************************/
/*
void AudioManager::FindAudioPlugins()
{
  //Search for all files under ${PRIVATELIBDIR}/plugins
  //Verify if they are audio plugins
  //If true, add to list of audio plugins
  string froot(PRIVATELIBDIR), fplugins("/plugins/"); //LS should always look in the plugins folder, nowhere else
  const path plugins_folder = froot + fplugins;
  const string pattern("liblightspark+[A-Za-z]+plugin.so");
  regex file_pattern(pattern); //pattern of ls plugins

  #if defined DEBUG
    cout << "Looking for plugins under " << plugins_folder << " for file_name " << pattern << endl;
  #endif

  if(!is_directory(plugins_folder))
  {
    cout << "The plugins folder doesn't exists under " << plugins_folder << endl;
  }
  else
  {
    for(recursive_directory_iterator itr(plugins_folder), end_itr; itr != end_itr; ++itr)
    {
      if(is_regular_file(itr.status())) //Is it a real file? This will remove symlink
      {
	string leaf_name = itr->path().filename();
	if(regex_match(leaf_name, file_pattern)) // Does it answer to the desired pattern?
	{
	  string fullpath = plugins_folder.directory_string() + leaf_name;
	  //Try to load the file and see if it's an audio plugin
	  if(HMODULE h_plugin = LoadLib(fullpath))
	  {
	    PLUGIN_FACTORY p_factory_function = (PLUGIN_FACTORY) ExtractLibContent(h_plugin, "create");
	    PLUGIN_CLEANUP p_cleanup_function = (PLUGIN_CLEANUP) ExtractLibContent(h_plugin, "release");
	    
	    if (p_factory_function != NULL && p_cleanup_function != NULL) //Does it contain the LS IPlugin?
	    {
	      IPlugin *p_plugin = (*p_factory_function)(); //Instanciate the plugin
	      if(p_plugin->get_pluginType() == AUDIO) //Is it really an AUDIO plugin?
	      {
		IAudioPlugin *p_audioplugin = static_cast<IAudioPlugin *>(p_plugin);
		#if defined DEBUG
		  printf("Plugin %s of type %d\n", p_audioplugin->get_pluginName(), p_audioplugin->get_pluginType());
		#endif
		AddAudioPluginToList(p_audioplugin, fullpath); //Add the plugin info to the audio plugins list
	      }
	      
	      (*p_cleanup_function)(p_plugin);
	      CloseLib(h_plugin);
	    }
	    else //If doesn't implement our IPlugin interface entry points, close it
	    {
	      CloseLib(h_plugin);
	    }
	  }
	}
      }
    }
  }
}
*/
/*
void AudioManager::AddAudioPluginToList(IAudioPlugin *audioplug, string pathToPlugin)
{
  //Verify if the plugin is already in the list
  uint32_t index=0;
  for(;index<AudioPluginsList.size();index++)
  {
    if(AudioPluginsList[index]->plugin_path==pathToPlugin) //If true, plugin is already in the list, we have nothing to do
    {
      return;
    }
  }
  
  if(index==AudioPluginsList.size())
  {
    AudioPluginsList.push_back(new PluginInfo(this));
  }
  AudioPluginsList[index]->audiobackend_name = audioplug->get_audioBackend_name();
  AudioPluginsList[index]->plugin_name = audioplug->get_pluginName();
  AudioPluginsList[index]->plugin_path = pathToPlugin;
  if(audioplug->Is_Connected())
  {
    AudioPluginsList[index]->enabled = true;
  }
  else //else, the backend is not present, so we disable the plugin
  {
    AudioPluginsList[index]->enabled = false;
  }
#if defined DEBUG
    cout << "This is the plugin " << index  << " added with backend: " << AudioPluginsList[index]->audiobackend_name << endl;
#endif 
}
*/
#endif
