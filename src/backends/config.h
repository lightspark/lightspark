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

#ifndef _CONFIG_H
#define _CONFIG_H

#include "parsing/config.h"
#include "compat.h"
#include "swftypes.h"

namespace lightspark
{
	class Config
	{
	private:
		ConfigParser* parser;
		
		void handleEntry();

		//-- CONFIGURATION FILENAME AND SEARCH DIRECTORIES
		const std::string configFilename;
		const char* const* systemConfigDirectories;
		const std::string userConfigDirectory;

		//-- SETTINGS VALUES
		enum AUDIOBACKEND { PULSEAUDIO=0, OPENAL=1, SDL=2 };
		std::string audioBackendNames[3];

		//-- SETTINGS
		//Specifies the default cache directory = "~/.cache/lightspark"
		std::string defaultCacheDirectory;
		//Specifies where files are cached (like downloaded data)
		std::string cacheDirectory;
		//Specifies what prefix the cache files should have, default="cache"
		std::string cachePrefix;

		//Specifies what audio backend should, default=PULSEAUDIO
		AUDIOBACKEND audioBackend;
		std::string audioBackendName;
	public:
		Config();
		~Config();
		void load();

		const std::string& getCacheDirectory() const { return cacheDirectory; }
		const std::string& getCachePrefix() const { return cachePrefix; }

		AUDIOBACKEND getAudioBackend() const { return audioBackend; }
		const std::string& getAudioBackendName() const { return audioBackendName; }
	};
}

#endif
