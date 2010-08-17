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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "compat.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "../threading.h"

#include "interfaces/IPlugin.h"

using namespace std;
using namespace boost::filesystem;

//convenience typedef for the pointers to the 2 functions we expect to find in the plugin libraries
typedef IPlugin *(*PLUGIN_FACTORY)();
typedef void (*PLUGIN_CLEANUP)(IPlugin *);

namespace lightspark
{
  
class PluginModule
{
  protected:
    string plugin_name;		//plugin name
    PLUGIN_TYPES plugin_type;	//plugin type to be able to filter them
    string backend_name;	//backend (can be something like pulseaudio, opengl, ffmpeg)
    string plugin_path;		//full path to the plugin file
    bool enabled;		//should it be enabled (if the audio backend is present)?
    HMODULE h_LoadedPlugin;	//when loaded, handle to the plugin so we can unload it later
    IPlugin *o_LoadedPlugin;	//when instanciated, object to the class

  public:
    PluginModule();
    ~PluginModule();
};

class PluginManager : public PluginModule, public IThreadJob
{
  private:
    vector<PluginModule *>plugins_list;
    void FindPlugins();
    void AddPluginToList(IPlugin *o_plugin, string pathToPlugin);
    void RemovePluginFromList(string plugin_path);
    int32_t FindPluginInList(string desiredname = "", string desiredbackend = "", string desiredpath = "",
			      HMODULE hdesiredLoadPlugin = NULL, IPlugin *o_desiredPlugin = NULL);
    void LoadPlugin(uint32_t desiredindex);
    void UnloadPlugin(uint32_t desiredIndex);

  public:
    PluginManager();
    vector<string *> get_backendsList(PLUGIN_TYPES typeSearched);
    IPlugin *get_plugin(string desiredBackend);
    void release_plugin(IPlugin *o_plugin);
    void startCheck();
    ~PluginManager();
};
}

#endif