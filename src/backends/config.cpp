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

#include <glib.h>

#include "compat.h"
#include <string>
#include <boost/filesystem.hpp>

#include "config.h"

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;

Config::Config():
	parser(NULL),
	//CONFIGURATION FILENAME AND SEARCH DIRECTORIES
	configFilename("lightspark.conf"),
	systemConfigDirectories(g_get_system_config_dirs()),userConfigDirectory(g_get_user_config_dir()),
	//DEFAULT SETTINGS
	defaultCacheDirectory((string) g_get_user_cache_dir() + "/lightspark"),
	cacheDirectory(defaultCacheDirectory),cachePrefix("cache"),
	audioBackend(PULSEAUDIO),audioBackendName("")
{
	audioBackendNames[0] = "pulseaudio";
	audioBackendNames[1] = "openal";
	audioBackendNames[2] = "sdl";
}

Config::~Config()
{
	if(parser != NULL)
		delete parser;
}

void Config::load()
{
	//Try system configs first
	string sysDir;
	const char* const* cursor = systemConfigDirectories;
	while(*cursor != NULL)
	{
		sysDir = *cursor;
		parser = new ConfigParser(sysDir + "/" + configFilename);
		while(parser->read())
			handleEntry();
		delete parser;
		parser = NULL;

		++cursor;
	}

	//Try user config next
	parser = new ConfigParser(userConfigDirectory + "/" + configFilename);
	while(parser->read())
		handleEntry();
	delete parser;
	parser = NULL;

#ifndef _WIN32
	//Expand tilde in path
	if(cacheDirectory.length() > 0 && cacheDirectory[0] == '~')
		cacheDirectory.replace(0, 1, getenv("HOME"));
#endif

	//If cache dir doesn't exist, create it
	path cacheDirectoryP(cacheDirectory);
	if(!is_directory(cacheDirectoryP))
	{
		LOG(LOG_INFO, "Cache directory does not exist, trying to create");
		try
		{
			create_directories(cacheDirectoryP);
		}
		catch(const basic_filesystem_error<path>& e)
		{
			LOG(LOG_INFO, _("Could not create cache directory, falling back to default cache directory: ") <<
					defaultCacheDirectory);
			cacheDirectory = defaultCacheDirectory;
		}
	}

	//Set the audio backend name
	audioBackendName = audioBackendNames[audioBackend];
}

void Config::handleEntry()
{
	string group = parser->getGroup();
	string key = parser->getKey();
	string value = parser->getValue();
	//Audio backend
	if(group == "audio" && key == "backend" && value == audioBackendNames[PULSEAUDIO])
		audioBackend = PULSEAUDIO;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[OPENAL])
		audioBackend = OPENAL;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[SDL])
		audioBackend = SDL;
	//Cache directory
	else if(group == "cache" && key == "directory")
		cacheDirectory = value;
	//Cache prefix
	else if(group == "cache" && key == "prefix")
		cachePrefix = value;
	else
		throw ConfigException((string) _("Invalid entry encountered in configuration file") + ": '" + group + "/" + key + "'='" + value + "'");
}
