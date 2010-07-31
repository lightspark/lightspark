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
#include <string.h>
#include <boost/filesystem.hpp> //will be part of the next C++ release
#include "audioManager.h"

#if defined WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <sys/types.h>
#endif

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;

int find_files(const path &folder, const string &file, path &pathToSend);

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
  //Search for all files under ${DATADIR}/plugins
  //Verify if they are plugins
  //If true, add to list of plugins
  string froot(DATADIR), fplugins("/lightspark/plugins");
  const path plugins_folder = froot + fplugins;
  const string file_name = "liblightspark*plugin";
  path path_found;

  #if defined DEBUG
    cout << "Looking for plugins under " << plugins_folder << " for file_name " << file_name << endl;
  #endif

  find_files(plugins_folder, file_name, path_found);
  
  
/*//       //get the program's directory
//       char dir [MAX_PATH];
//       GetModuleFileName (NULL, dir, MAX_PATH);
 
  //eliminate the file name (to get just the directory)
  char *p = strrchr(dir, '\\');
  *(p + 1) = 0;
 
  //find all libraries in the plugins subdirectory
  char search_parms [MAX_PATH];
  strcpy_s (search_parms, MAX_PATH, dir);
  strcat_s (search_parms, MAX_PATH, "plugins\\*.dll");
 
  WIN32_FIND_DATA find_data;
  HANDLE h_find = ::FindFirstFile (search_parms, &find_data);
  BOOL f_ok = TRUE;
  while (h_find != INVALID_HANDLE_VALUE && f_ok)
  {
*/    //load each library and look for the functions expected to initialize
    char *plugin_full_name;
/*    strcpy_s(plugin_full_name, MAX_PATH, dir);
      strcat_s(plugin_full_name, MAX_PATH, "plugins\\");
      strcat_s(plugin_full_name, MAX_PATH, find_data.cFileName);
*/ 
    HLIB h_plugin = LoadLib(plugin_full_name);
    if (h_plugin != NULL)
    {
      PLUGIN_FACTORY p_factory_function = (PLUGIN_FACTORY) ExtractLibContent(h_plugin, "Create_Plugin");
      PLUGIN_CLEANUP p_cleanup_function = (PLUGIN_CLEANUP) ExtractLibContent(h_plugin, "Release_Plugin");
 
      if (p_factory_function != NULL && p_cleanup_function != NULL)
      {
	//The library has the functions and should be what we are looking for
        //invoke the factory to create the plugin object
        IPlugin *p_plugin = (*p_factory_function)();
 
        //Get some more info about the plugin to be sure what it is
//			   if(!(p_plugin->GetPluginType() == PLUGIN_TYPES UNKNOWN))
        if(true)
	{
//	  printf("Plugin %s is loaded from file %s.\n", p_plugin->Get_PluginName(), find_data.cFileName);
//	  printf("Plugin of type %d\n", p_plugin->Get_PluginType());
	  printf("Plugin %s of type %d\n",p_plugin->get_pluginName(), p_plugin->get_pluginType());
	  this->AddAudioPluginToList(h_plugin, plugin_full_name);
	}
                          
        //done, cleanup the plugin by invoking its cleanup function
        (*p_cleanup_function) (p_plugin);
      }
      else
      {
	printf("Not the kind of plugin the application is looking for.\n");
      }
 
      CloseLib(h_plugin);
    }
 
/*  //go for the next DLL
   f_ok = ::FindNextFile (h_find, &find_data);
 }
 
 return 0;
*/}

void AudioManager::AddAudioPluginToList(const HLIB h_pluginToAdd, const char *pathToPlugin)
{
/*  if(this->AudioPluginsList->FirstAudioPlugin == NULL)
  {
    this->AudioPluginsList->FirstAudioPlugin-> = p_pluginToAdd->;
    this->AudioPluginsList->LastAudioPlugin = this->AudioPluginsList->FirstAudioPlugin;
  }
  else
  {
    this->AudioPluginsList->LastAudioPlugin->NextPluginLib = p_pluginToAdd;
    this->AudioPluginsList->LastAudioPlugin
  }
*/}

int find_files(const path &folder, const string &file, path &pathToSend)
{
    
  if(!exists(folder))
  {
    cout << "The plugins folder doesn't exists under " << folder << endl;
  }
  else
  {
    directory_iterator end_itr; //end of iteration
    for( directory_iterator itr(folder); itr != end_itr; ++itr )
    {
      if(is_directory(itr->status()))
      {
	if(find_files(itr->path(), file, pathToSend))
	{
	  return true;
	}
      }
      else if(itr->leaf() == file) // see below
      {
	pathToSend = itr->path();
	return true;
      }
    }
  }
  return false;
}