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

#ifndef BACKENDS_CONFIG_H
#define BACKENDS_CONFIG_H 1

#include "parsing/config.h"
#include "compat.h"

namespace lightspark
{
	class DLL_PUBLIC Config
	{
	private:
		ConfigParser* parser;

		void handleEntry();

		//-- CONFIGURATION FILENAME AND SEARCH DIRECTORIES
		const std::string configFilename;
		const char* const* systemConfigDirectories;
		std::string userConfigDirectory;

		//-- SETTINGS
		//Specifies the default cache directory = "~/.cache/lightspark"
		std::string defaultCacheDirectory;
		//Specifies where files are cached (like downloaded data)
		std::string cacheDirectory;
		//Specifies what prefix the cache files should have, default="cache"
		std::string cachePrefix;
		//Specifies the filename including full path of the gnash executable
		std::string gnashPath;
		//Specifies the directory where the app can store files
		std::string dataDirectory;

		//Specifies if rendering should be done
		bool renderingEnabled;
		Config();
		~Config();
	public:
		/* Returns the singleton config object */
		static Config* getConfig();

		const std::string& getCacheDirectory() const { return cacheDirectory; }
		const std::string& getCachePrefix() const { return cachePrefix; }
		const std::string& getDataDirectory() const { return dataDirectory; }
		
		const std::string& getGnashPath() const { return gnashPath; }

		bool isRenderingEnabled() const { return renderingEnabled; }
	};
}

#endif /* BACKENDS_CONFIG_H */
