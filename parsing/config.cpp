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

#include "config.h"
#include "swf.h"

using namespace lightspark;
extern TLSDATA SystemState* sys;

ConfigParser::ConfigParser(const std::string& data):
	valid(true),file(g_key_file_new()),groupCount(0),currentGroup(0),keyCount(0),currentKey(0)
{
	if(!g_key_file_load_from_data(file, data.c_str(), data.length(), G_KEY_FILE_NONE, NULL))
		valid = false;

	groups = g_key_file_get_groups(file, &groupCount);
}

ConfigParser::ConfigParser(const std::string& filename, bool search, const char** dirs):
	valid(true),file(g_key_file_new()),
	groups(NULL),groupCount(0),currentGroup(0),
	keys(NULL),keyCount(0),currentKey(0)
{
	if(!search)
	{
		LOG(LOG_NO_INFO, "trying to parse: " << filename);
		if(!g_key_file_load_from_file(file, filename.c_str(), G_KEY_FILE_NONE, NULL))
			valid = false;
	}
	else if(dirs == NULL)
	{
		char* fullPath;
		if(!g_key_file_load_from_data_dirs(file, filename.c_str(), &fullPath, G_KEY_FILE_NONE, NULL))
			valid = false;
		else
			free(fullPath);
	}
	else
	{
		char* fullPath;
		if(!g_key_file_load_from_dirs(file, filename.c_str(), dirs, &fullPath, G_KEY_FILE_NONE, NULL))
			valid = false;
		else
			free(fullPath);
	}

	if(valid)
		groups = g_key_file_get_groups(file, &groupCount);
}

ConfigParser::~ConfigParser()
{
	if(groups != NULL)
		g_strfreev(groups);
	if(keys != NULL)
		g_strfreev(keys);
	if(file != NULL)
		g_key_file_free(file);
}

bool ConfigParser::read()
{
	//There are no groups or we have already parsed the last key/group pair in the file
	if(!valid || groupCount == 0 || (currentGroup == groupCount && (currentKey == keyCount || keyCount == 0)))
		return false;

	if(currentKey == keyCount && currentGroup == groupCount-1)
		return false;
	else if(currentGroup == 0 && currentKey == 0)
	{
		keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		while(keyCount == 0 && currentGroup < groupCount-1)
		{
			LOG(LOG_NO_INFO, "empty group, trying next");
			++currentGroup;
			g_strfreev(keys);
			keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		}
		//We didn't find any more groups containing keys
		if(keyCount == 0 && currentGroup == groupCount-1)
			return false;
		group = groups[currentGroup];
	}
	else if(currentKey == keyCount && currentGroup < groupCount-1)
	{
		currentKey = 0;
		do {
			++currentGroup;
			g_strfreev(keys);
			keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		}
		while(keyCount == 0 && currentGroup < groupCount-1);
		//We didn't find any more groups containing keys
		if(keyCount == 0 && currentGroup == groupCount-1)
			return false;
		group = groups[currentGroup];
	}


	//We have a valid currentKey/currentGroup pair, lets set the current key
	key = keys[currentKey];

	++currentKey;
	return true;
}
