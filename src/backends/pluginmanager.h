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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "compat.h"
#include <iostream>
#include <vector>

#include "interfaces/IPlugin.h"

//convenience typedef for the pointers to the 2 functions we expect to find in the plugin libraries
typedef IPlugin * ( *PLUGIN_FACTORY ) ();
typedef void ( *PLUGIN_CLEANUP ) ( IPlugin * );
typedef struct _GModule GModule;

namespace lightspark
{

class PluginModule;

class PluginManager
{
private:
	std::vector<PluginModule *> pluginsList;
	void findPlugins();
	void addPluginToList ( IPlugin *o_plugin, std::string pathToPlugin );
	void removePluginFromList ( std::string plugin_path );
	int32_t findPluginInList ( std::string desiredname = "", std::string desiredbackend = "", std::string desiredpath = "",
								GModule* hdesiredLoadPlugin = NULL, IPlugin *o_desiredPlugin = NULL );
	void loadPlugin ( uint32_t desiredindex );
	void unloadPlugin ( uint32_t desiredIndex );

public:
	PluginManager();
	std::vector<std::string *> get_backendsList ( PLUGIN_TYPES typeSearched );
	IPlugin *get_plugin ( std::string desiredBackend );
	void release_plugin ( IPlugin *o_plugin );
	~PluginManager();
};

class PluginModule
{
	friend class PluginManager;
protected:
	std::string pluginName;		//plugin name
	PLUGIN_TYPES pluginType;	//plugin type to be able to filter them
	std::string backendName;	//backend (can be something like pulseaudio, opengl, ffmpeg)
	std::string pluginPath;		//full path to the plugin file
	bool enabled;		//should it be enabled (if the audio backend is present)?
	GModule* hLoadedPlugin;	//when loaded, handle to the plugin so we can unload it later
	IPlugin *oLoadedPlugin;	//when instanciated, object to the class

public:
	PluginModule();
	~PluginModule();
};

}

#endif
