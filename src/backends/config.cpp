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

#include <string>
#include <sys/stat.h>

#include "backends/config.h"
#include "compat.h"
#include "logger.h"
#include "exceptions.h"
#include "utils/specialfolder.h"
#include "utils/filesystem.h"

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

Config::Config()
	//CONFIGURATION FILENAME AND SEARCH DIRECTORIES
	:configFilename("lightspark.conf")
	,userConfigDirectory(SpecialFolder::get_user_config_dir())
	//DEFAULT SETTINGS
	,defaultCacheDirectory((Path(SpecialFolder::get_user_cache_dir()) / Path("lightspark")).getStr())
	,cacheDirectory(defaultCacheDirectory),cachePrefix("cache")
	,userDataDirectory((Path( SpecialFolder::get_user_data_dir()) / Path("lightspark")).getStr())
	,renderingEnabled(true)
{
	std::list<tiny_string> systemConfigDirectories;
	SpecialFolder::get_system_config_dirs(systemConfigDirectories);
#ifdef _WIN32
	const char* exePath = getExectuablePath();
	if(exePath)
		userConfigDirectory = exePath;
#endif

	//Try system configs first
	tiny_string sysDir;
	auto it = systemConfigDirectories.begin();
	while(it != systemConfigDirectories.end())
	{
		sysDir = *it;
		ini::IniFile parser;
		parser.setMultiLineValues(true);
		parser.load((Path(sysDir) / Path(configFilename)).getStr());
		handleEntries(parser);
		++it;
	}

	//Try user config next
	ini::IniFile parser;
	parser.setMultiLineValues(true);
	parser.load((Path(userConfigDirectory) / Path(configFilename)).getStr());
	handleEntries(parser);

#ifndef _WIN32
	//Expand tilde in path
	if(cacheDirectory.length() > 0 && cacheDirectory[0] == '~')
		cacheDirectory.replace(0, 1, getenv("HOME"));
#endif

	//If cache dir doesn't exist, create it
	if (FileSystem::createDirs(Path(cacheDirectory),FileSystem::Perms::OwnerAll))
	{
		LOG(LOG_INFO, "Could not create cache directory, falling back to default cache directory: " <<
				defaultCacheDirectory);
		cacheDirectory = defaultCacheDirectory;
	}
    dataDirectory = (Path(cacheDirectory) / "files").getStr();
	if (FileSystem::createDirs(Path(dataDirectory),FileSystem::Perms::OwnerAll))
	{
		LOG(LOG_INFO, "Could not create data directory, storing user data may not be possible");
		dataDirectory = "";
	}
	else
	{
        dataDirectory = (Path(dataDirectory) / "cXXXXXX").getStr();
		char* tmpdir = new char[dataDirectory.length()+100];
		strncpy(tmpdir,dataDirectory.c_str(),dataDirectory.length());
		tmpdir[dataDirectory.length()] = 0x00;
        tmpdir =mkdtemp(tmpdir);
		if (!tmpdir)
		{
			LOG(LOG_INFO, "Could not create data directory, storing user data may not be possible");
			dataDirectory = "";
		}
		else
			dataDirectory = tmpdir;
		delete[] tmpdir;
	}
}

Config::~Config()
{
}

/* This is called by the parser for each entry in the configuration file */
void Config::handleEntries(ini::IniFile& parser)
{
	for(const auto &sectionPair : parser)
	{
		const std::string &group = sectionPair.first;

		for(const auto &fieldPair : sectionPair.second)
		{
			const std::string &key = fieldPair.first;
			const ini::IniField &field = fieldPair.second;
			const std::string &value = field.as<string>();
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
	}
}