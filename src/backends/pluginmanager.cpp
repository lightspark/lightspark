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
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "pluginmanager.h"
#include <iostream>
#include <list>
#include <boost/filesystem.hpp>
#include <pcre.h>
#include "../logger.h"
#include "../exceptions.h"

#include <gmodule.h>

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;

PluginManager::PluginManager()
{
	assert( g_module_supported() );
	findPlugins();
}

/***************************
Find liblightsparkBACKENDplugin libraries
****************************/
void PluginManager::findPlugins()
{
	//Search for all files under ${PRIVATELIBDIR}/plugins
	//Verify if they are audio plugins
	//If true, add to list of audio plugins
	string froot ( PRIVATELIBDIR ), fplugins ( "/plugins/" ); //LS should always look in the plugins folder, nowhere else
	const path plugins_folder = froot + fplugins;
	const string pattern ( "liblightspark+[A-Za-z]+plugin.so" );

	//Stuff used by/for pcre
	const char* patternError;
	int patternErrorOffset;
	pcre* file_pattern = pcre_compile ( pattern.c_str(), 0, &patternError, &patternErrorOffset, NULL );
	if(patternError)
		throw RunTimeException("PluginManager::findPlugins(): can't compile file_pattern");
	//We don't expect any captured substrings, so 3 ints should be enough
	int patternOvector[3];

#if defined DEBUG
	cout << "Looking for plugins under " << plugins_folder << " for pattern " << pattern << endl;
#endif

	if ( !is_directory ( plugins_folder ) )
	{
		LOG ( LOG_ERROR, _ ( ( ( string ) ( "The plugins folder doesn't exists under " + plugins_folder.string() ) ).c_str() ) );
	}
	else
	{
		for ( recursive_directory_iterator itr ( plugins_folder ), end_itr; itr != end_itr; ++itr )
		{
			if ( is_regular_file ( itr.status() ) )   //Is it a real file? This will remove symlink
			{
				string leaf_name = itr->path().filename();
				int rc=pcre_exec(file_pattern, NULL, leaf_name.c_str(), leaf_name.length(), 0, 0, patternOvector, 3);
				if ( rc > 0 )   // Does it answer to the desired pattern?
				{
					string fullpath = plugins_folder.directory_string() + leaf_name;
					//Try to load the file and see if it's an audio plugin
					if ( GModule* h_plugin = g_module_open( fullpath.c_str(), G_MODULE_BIND_LAZY) )
					{
						PLUGIN_FACTORY p_factory_function;
						PLUGIN_CLEANUP p_cleanup_function;

						if ( g_module_symbol(h_plugin, "create", (void**)&p_factory_function)
							&& g_module_symbol(h_plugin, "release", (void**)&p_cleanup_function) )
						{  //Does it contain the LS IPlugin?
							IPlugin *p_plugin = p_factory_function (); //Instanciate the plugin
							LOG ( LOG_INFO, _ ( "A plugin was found. Adding it to the list." ) );
							addPluginToList ( p_plugin, fullpath ); //Add the plugin info to the audio plugins list

							p_cleanup_function ( p_plugin );
							g_module_close ( h_plugin );
						}
						else   //If doesn't implement our IPlugin interface entry points, close it
						{
							g_module_close( h_plugin );
						}
					}
				}
			}
		}
	}
	pcre_free(file_pattern);
}

//return a list of backends  of the appropriated PLUGIN_TYPES
vector<string *> PluginManager::get_backendsList ( PLUGIN_TYPES typeSearched )
{
	vector<string *> retrievedList;
	uint32_t count = 0;
	for ( uint32_t index = 0; index < pluginsList.size(); index++ )
	{
		if ( pluginsList[index]->pluginType == typeSearched )
		{
			if ( count == retrievedList.size() )
			{
				retrievedList.push_back ( new string );
			}
			retrievedList[count] = &pluginsList[index]->backendName;
			count++;
		}
	}
	return retrievedList;
}

//get the desired plugin associated to the backend
IPlugin *PluginManager::get_plugin ( string desiredBackend )
{
	LOG ( LOG_INFO, _ ( ( ( string ) ( "get_plugin: " + desiredBackend ) ).c_str() ) );
	int32_t index = findPluginInList ( "", desiredBackend, "", NULL, NULL );
	if ( index >= 0 )
	{
		loadPlugin ( index );
		return pluginsList[index]->oLoadedPlugin;
	}
	else
	{
		return NULL;
	}
}

/*******************
When a plugin is not needed anymore somewhere, the client that had asked it tells the manager it doesn't
need it anymore. The PluginManager releases it (delete and unload).
*******************/
void PluginManager::release_plugin ( IPlugin* o_plugin )
{
	for ( uint32_t index = 0; index < pluginsList.size(); index++ )
	{
		if ( pluginsList[index]->oLoadedPlugin == o_plugin )
		{
			unloadPlugin ( index );
		}
	}
}

/*********************
Adds the information about a plugin in the plugins list
*********************/
void PluginManager::addPluginToList ( IPlugin *o_plugin, string pathToPlugin )
{
	//Verify if the plugin is already in the list
	int32_t index = findPluginInList ( "", "", pathToPlugin, NULL, NULL );
	if ( index >= 0 )   //If true, plugin is already in the list, we have nothing to do
	{
		return;
	}
	else
	{
		index = ( int32_t ) ( pluginsList.size() ); //we want to add the plugin to the end of the list
		if ( pluginsList.size() == ( uint32_t ) ( index ) )
		{
			pluginsList.push_back ( new PluginModule() );
		}
		pluginsList[index]->pluginName = o_plugin->get_pluginName();
		pluginsList[index]->backendName = o_plugin->get_backendName();
		pluginsList[index]->pluginPath = pathToPlugin;
		pluginsList[index]->enabled = false;
		LOG ( LOG_INFO, _ ( ( ( string ) ( "The plugin " + pluginsList[index]->pluginName + " was added with backend: " + pluginsList[index]->backendName ) ).c_str() ) );
	}
}

/**********************
Removes the information about a plugin from the plugins list.
It's used only by the manager when there are modifications in the plugins folder.
**********************/
void PluginManager::removePluginFromList ( string pluginPath )
{
	int32_t index = findPluginInList ( "", "", pluginPath, NULL, NULL );
	if ( index >= 0 )
	{
		unloadPlugin ( index );
		pluginsList.erase ( pluginsList.begin() + index );
	}
}

/**************************
Looks in the plugins list for the desired entry.
If found, returns the location in the list (index). Else, returns -1 (which can't be an entry in the list)
**************************/
int32_t PluginManager::findPluginInList ( string desiredname, string desiredbackend,
        string desiredpath, GModule* hdesiredloadPlugin, IPlugin* o_desiredPlugin )
{
	for ( uint32_t index = 0; index < pluginsList.size(); index++ )
	{
		if ( ( desiredname != "" ) && ( pluginsList[index]->pluginName == desiredname ) )
		{
			return index;
		}
		if ( ( desiredbackend != "" ) && ( pluginsList[index]->backendName == desiredbackend ) )
		{
			return index;
		}
		if ( ( desiredpath != "" ) && ( pluginsList[index]->pluginPath == desiredpath ) )
		{
			return index;
		}
		if ( ( hdesiredloadPlugin != NULL ) && ( pluginsList[index]->hLoadedPlugin == hdesiredloadPlugin ) )
		{
			return index;
		}
		if ( ( o_desiredPlugin != NULL ) && ( pluginsList[index]->oLoadedPlugin == o_desiredPlugin ) )
		{
			return index;
		}
	}
	LOG ( LOG_ERROR, _ ( "findPluginInList: no plugin found in list" ) );
	return -1;
}


//Takes care to load and instanciate anything related to the plugin
void PluginManager::loadPlugin ( uint32_t desiredindex )
{
	if( (pluginsList[desiredindex]->hLoadedPlugin = g_module_open( pluginsList[desiredindex]->pluginPath.c_str(), G_MODULE_BIND_LAZY)))
	{
		PLUGIN_FACTORY p_factory_function;
		if ( g_module_symbol(pluginsList[desiredindex]->hLoadedPlugin, "create", (void**)&p_factory_function) )
		{   //Does it contain the LS IPlugin?
			pluginsList[desiredindex]->oLoadedPlugin = ( *p_factory_function ) (); //Instanciate the plugin
			pluginsList[desiredindex]->enabled = true;
		}
	}
}

//Takes care of unloading and releasing anything related to the plugin
void PluginManager::unloadPlugin ( uint32_t desiredIndex )
{
	if ( pluginsList[desiredIndex]->oLoadedPlugin || pluginsList[desiredIndex]->hLoadedPlugin )   //If there is already a backend loaded, unload it
	{
		if ( pluginsList[desiredIndex]->oLoadedPlugin != NULL )
		{
			PLUGIN_CLEANUP p_cleanup_function;

			if ( g_module_symbol(pluginsList[desiredIndex]->hLoadedPlugin, "release", (void**)&p_cleanup_function) )
			{
				p_cleanup_function ( pluginsList[desiredIndex]->oLoadedPlugin );
			}
			else
			{
				delete pluginsList[desiredIndex]->oLoadedPlugin;
			}

			pluginsList[desiredIndex]->oLoadedPlugin = NULL;
			g_module_close ( pluginsList[desiredIndex]->hLoadedPlugin );
		}
		pluginsList[desiredIndex]->enabled = false; //Unselecting any entry in the plugins list
	}
}

PluginManager::~PluginManager()
{
	for(auto i = pluginsList.begin(); i != pluginsList.end(); ++i)
		delete *i;
}


PluginModule::PluginModule()
		: pluginName ( "undefined" ), pluginType ( UNDEFINED ), backendName ( "undefined" ), pluginPath ( "" ),
		enabled ( false ), hLoadedPlugin ( NULL ), oLoadedPlugin ( NULL )
{

}

PluginModule::~PluginModule()
{

}
