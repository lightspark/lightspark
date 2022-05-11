/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <string>
#include <sys/stat.h>

#include "backends/config.h"
#include "compat.h"
#include "logger.h"
#include "exceptions.h"

#include <cstring>

using namespace lightspark;
using namespace std;

Config* Config::getConfig()
{
	static Config conf;
	return &conf;

}

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

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
	parser(nullptr),
	//CONFIGURATION FILENAME AND SEARCH DIRECTORIES
	configFilename("lightspark.conf"),
	systemConfigDirectories(g_get_system_config_dirs()),userConfigDirectory(g_get_user_config_dir()),
	//DEFAULT SETTINGS
	defaultCacheDirectory((string) g_get_user_cache_dir() + G_DIR_SEPARATOR_S + "lightspark"),
	cacheDirectory(defaultCacheDirectory),cachePrefix("cache"),
	renderingEnabled(true)
{
#ifdef _WIN32
	const char* exePath = getExectuablePath();
	if(exePath)
		userConfigDirectory = exePath;
#endif

	//Try system configs first
	string sysDir;
	const char* const* cursor = systemConfigDirectories;
	while(*cursor != nullptr)
	{
		sysDir = *cursor;
		parser = new ConfigParser(sysDir + G_DIR_SEPARATOR_S + configFilename);
		while(parser->read())
			handleEntry();
		delete parser;
		parser = nullptr;

		++cursor;
	}

	//Try user config next
	parser = new ConfigParser(userConfigDirectory + G_DIR_SEPARATOR_S + configFilename);
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
	if (g_mkdir_with_parents(cacheDirectory.c_str(),S_IRUSR | S_IWUSR | S_IXUSR))
	{
		LOG(LOG_INFO, "Could not create cache directory, falling back to default cache directory: " <<
				defaultCacheDirectory);
		cacheDirectory = defaultCacheDirectory;
	}
	dataDirectory = cacheDirectory+G_DIR_SEPARATOR_S+"files";
	if (g_mkdir_with_parents(dataDirectory.c_str(),S_IRUSR | S_IWUSR | S_IXUSR))
	{
		LOG(LOG_INFO, "Could not create data directory, storing user data may not be possible");
		dataDirectory = "";
	}
	else
	{
		dataDirectory += G_DIR_SEPARATOR_S;
		dataDirectory +="cXXXXXX";
		char* tmpdir = new char[dataDirectory.length()+100];
		strncpy(tmpdir,dataDirectory.c_str(),dataDirectory.length());
		tmpdir[dataDirectory.length()] = 0x00;
		tmpdir =g_mkdtemp(tmpdir);
		if (!tmpdir)
		{
			LOG(LOG_INFO, "Could not create data directory, storing user data may not be possible");
			dataDirectory = "";
		}
		else
			dataDirectory = tmpdir;
		delete[] tmpdir;
	}

#ifdef _WIN32
	std::string regGnashPath = readRegistryEntry("GnashPath");
	if(regGnashPath.empty())
	{
		const char* s = getExectuablePath();
		if(!s)
			LOG(LOG_ERROR,"Could not get executable path!");
		else
		{
			
			std::string gnash_exec_path = s;
			if(g_file_test(std::string(gnash_exec_path + "gtk-gnash.exe").c_str(),G_FILE_TEST_EXISTS))
			{
				LOG(LOG_INFO,"Found gnash at " << (gnash_exec_path + "gtk-gnash.exe"));
				gnashPath = std::string(gnash_exec_path + "gtk-gnash.exe").c_str();
			}
			else if(g_file_test(std::string(gnash_exec_path + "sdl-gnash.exe").c_str(),G_FILE_TEST_EXISTS))
			{
				LOG(LOG_INFO,"Found gnash at " << gnash_exec_path + "sdl-gnash.exe");
				gnashPath = std::string(gnash_exec_path + "sdl-gnash.exe").c_str();
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
	//Rendering
	if(group == "rendering" && key == "enabled")
		renderingEnabled = atoi(value.c_str());
	//Cache directory
	else if(group == "cache" && key == "directory")
		cacheDirectory = value;
	//Cache prefix
	else if(group == "cache" && key == "prefix")
		cachePrefix = value;
	else
		LOG(LOG_ERROR,"Invalid entry encountered in configuration file" << ": '" << group << "/" << key << "'='" << value << "'");
}
