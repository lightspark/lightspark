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

#include <glib.h>

#include "compat.h"
#include <string>
#include <boost/filesystem.hpp>

#include "backends/config.h"

using namespace lightspark;
using namespace std;
using namespace boost::filesystem;

Config* Config::getConfig()
{
	static Config conf;
	return &conf;

}

#ifdef WIN32
const std::string LS_REG_KEY = "SOFTWARE\\MozillaPlugins\\@lightspark.github.com/Lightspark;version=1";
std::string readRegistryEntry(std::string name)
{
	char lszValue[255];
	HKEY hKey;
	LONG returnStatus;
	DWORD dwType=REG_SZ;
	DWORD dwSize=sizeof(lszValue);;
	returnStatus = RegOpenKeyEx(HKEY_CURRENT_USER, LS_REG_KEY.c_str(), 0L, KEY_QUERY_VALUE, &hKey);
	if(returnStatus != ERROR_SUCCESS)
	{
		LOG(LOG_ERROR,"Could not open registry key " << LS_REG_KEY);
		return "";
	}
	returnStatus = RegQueryValueEx(hKey, name.c_str(), NULL, &dwType, (LPBYTE)&lszValue, &dwSize);
	if(returnStatus != ERROR_SUCCESS)
	{
		LOG(LOG_ERROR,"Could not read registry key value " << LS_REG_KEY << "\\" << name);
		return "";
	}
	if(dwType != REG_SZ)
	{
		LOG(LOG_ERROR,"Registry key" << LS_REG_KEY << "\\" << name << " has unexpected type");
		return "";
	}
	RegCloseKey(hKey);
	if(dwSize == 0)
		return "";
	/* strip terminating '\0' - string may or may not have one */
	if(lszValue[dwSize] == '\0')
		dwSize--;
	return std::string(lszValue,dwSize);
}
#endif

Config::Config():
	parser(NULL),
	//CONFIGURATION FILENAME AND SEARCH DIRECTORIES
	configFilename("lightspark.conf"),
	systemConfigDirectories(g_get_system_config_dirs()),userConfigDirectory(g_get_user_config_dir()),
	//DEFAULT SETTINGS
	defaultCacheDirectory((string) g_get_user_cache_dir() + "/lightspark"),
	cacheDirectory(defaultCacheDirectory),cachePrefix("cache"),
	audioBackend(INVALID),audioBackendName(""),
	renderingEnabled(true)
{
#ifdef _WIN32
	const char* exePath = getExectuablePath();
	if(exePath)
		userConfigDirectory = exePath;
#endif
	audioBackendNames[PULSEAUDIO] = "pulseaudio";
	audioBackendNames[SDL] = "sdl";
	audioBackendNames[WINMM] = "winmm";

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
		catch(const filesystem_error& e)
		{
			LOG(LOG_INFO, _("Could not create cache directory, falling back to default cache directory: ") <<
					defaultCacheDirectory);
			cacheDirectory = defaultCacheDirectory;
		}
	}

	/* If no audio backend was specified, use a default */
	if(audioBackend == INVALID)
	{
#ifdef _WIN32
		audioBackend = WINMM;
#else
		audioBackend = PULSEAUDIO;
#endif
	}
	//Set the audio backend name
	audioBackendName = audioBackendNames[audioBackend];

#ifdef _WIN32
	std::string regGnashPath = readRegistryEntry("GnashPath");
	if(regGnashPath.empty())
	{
		const char* s = getExectuablePath();
		if(!s)
			LOG(LOG_ERROR,"Could not get executable path!");
		else
		{
			path gnash_exec_path = s;
			if(is_regular_file(gnash_exec_path / "gtk-gnash.exe"))
			{
				LOG(LOG_INFO,"Found gnash at " << (gnash_exec_path / "gtk-gnash.exe"));
				gnashPath = (gnash_exec_path / "gtk-gnash.exe").string();
			}
			else if(is_regular_file(gnash_exec_path / "sdl-gnash.exe"))
			{
				LOG(LOG_INFO,"Found gnash at " << (gnash_exec_path / "sdl-gnash.exe"));
				gnashPath = (gnash_exec_path / "sdl-gnash.exe").string();
			}
			else
				LOG(LOG_ERROR, "Could not find gnash in " << gnash_exec_path);
		}
	}
	else
	{
		LOG(LOG_INFO, "Read gnash's path from registry: " << regGnashPath);
		gnashPath = regGnashPath;
	}
#else
#	ifndef GNASH_PATH
#	error No GNASH_PATH defined
#	endif
	gnashPath = GNASH_PATH;
#endif
}

Config::~Config()
{
	if(parser != NULL)
		delete parser;
}

/* This is called by the parser for each entry in the configuration file */
void Config::handleEntry()
{
	string group = parser->getGroup();
	string key = parser->getKey();
	string value = parser->getValue();
	//Audio backend
	if(group == "audio" && key == "backend" && value == audioBackendNames[PULSEAUDIO])
		audioBackend = PULSEAUDIO;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[SDL])
		audioBackend = SDL;
	else if(group == "audio" && key == "backend" && value == audioBackendNames[WINMM])
		 audioBackend = WINMM;
	//Rendering
	else if(group == "rendering" && key == "enabled")
		renderingEnabled = atoi(value.c_str());
	//Cache directory
	else if(group == "cache" && key == "directory")
		cacheDirectory = value;
	//Cache prefix
	else if(group == "cache" && key == "prefix")
		cachePrefix = value;
	else
		throw ConfigException((string) _("Invalid entry encountered in configuration file") + ": '" + group + "/" + key + "'='" + value + "'");
}
