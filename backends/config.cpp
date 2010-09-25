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

#include <glib.h>

#include "swf.h"
#include "compat.h"
#include <string>
#include <boost/filesystem.hpp>

#include "config.h"

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;
extern TLSDATA SystemState* sys;

Config::Config():
	parser(NULL),
	//Directories
	configFilename("lightspark.conf"),
	systemConfigDirectories(g_get_system_config_dirs()),userConfigDirectory(g_get_user_config_dir()),
	cacheDirectory((string) g_get_user_cache_dir() + "/lightspark"),cachePrefix("cache"),
	audioBackend(PULSEAUDIO),audioBackendName("")
{
	audioBackendNames = {"pulseaudio", "openal", "alsa"};
}

Config::~Config()
{
}

void Config::load()
{
	//Try system configs first
	string sysDir;
	const char* const* cursor = systemConfigDirectories;
	while(*cursor != NULL)
	{
		sysDir = *cursor;
		parser = new ConfigParser(sysDir + "/" + configFilename, false);
		while(parser->read())
			handleEntry();
		delete parser;

		++cursor;
	}

	//Try user config next
	parser = new ConfigParser(userConfigDirectory + "/" + configFilename, false);
	while(parser->read())
		handleEntry();
	delete parser;

	//If cache dir doesn't exist, create it
	path cacheDirectoryP(cacheDirectory);
	if(!is_directory(cacheDirectoryP))
		create_directory(cacheDirectoryP);

	//Set the audio backend name
	audioBackendName = audioBackendNames[audioBackend];
}

void Config::handleEntry()
{
	string group = parser->getGroup();
	string key = parser->getKey();
	string value = parser->getValue();
	if(group == "audio" && key == "backend" && value == audioBackendNames[PULSEAUDIO])
		audioBackend = PULSEAUDIO;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[OPENAL])
		audioBackend = OPENAL;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[ALSA])
		audioBackend = ALSA;
	else if(group == "cache" && key == "directory")
		cacheDirectory = value;
	else if(group == "cache" && key == "prefix")
		cachePrefix = value;
	else
		throw RunTimeException("Config::handleEntry: invalid entry encountered in config file: '" + group + "/" + key + "'='" + value + "'");
}
