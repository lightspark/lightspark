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

#include "config.h"
#include "compat.h"

using namespace lightspark;

ConfigParser::ConfigParser(const std::string& filename):
	valid(true),file(g_key_file_new()),
	groups(NULL),groupCount(0),currentGroup(0),
	keys(NULL),keyCount(0),currentKey(0),
	group(NULL),key(NULL)
{
	if(!g_key_file_load_from_file(file, filename.c_str(), G_KEY_FILE_NONE, NULL))
		valid = false;
	else
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
	//This parser is invalid or there are no groups
	if(!valid || groupCount == 0)
		return false;

	//We encountered the first group or have handled the last key in a group
	if((currentGroup == 0 && currentKey == 0) ||
			(currentKey == keyCount && currentGroup < groupCount-1))
	{
		if(currentGroup == 0 && currentKey == 0)
			keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		else
		{
			currentKey = 0;
			++currentGroup;
			g_strfreev(keys);
			keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		}

		while(keyCount == 0 && currentGroup < groupCount-1)
		{
			++currentGroup;
			g_strfreev(keys);
			keys = g_key_file_get_keys(file, groups[currentGroup], &keyCount, NULL);
		}
		group = groups[currentGroup];
	}

	//We encountered the end of the file
	if(currentGroup == groupCount-1 && currentKey == keyCount)
		return false;

	//We have a valid currentKey/currentGroup pair, lets set the current key
	key = keys[currentKey];

	//Increment currentKey for next round of reading
	++currentKey;
	return true;
}

std::string ConfigParser::getValue() 
{
	char *val = g_key_file_get_value(file, group, key, NULL);
	std::string ret(val);
	free(val);
	return ret;
}

std::string ConfigParser::getValueString()
{
	char *val = g_key_file_get_string(file, group, key, NULL);
	std::string ret(val);
	free(val);
	return ret;
}

std::vector<std::string> ConfigParser::getValueStringList()
{
	std::vector<std::string> ret;
	gsize length;
	gchar **valarr = g_key_file_get_string_list(file, group, key, &length, NULL);
	if(!valarr)
		return ret;

	for(gsize i=0; i<length; i++)
		ret.push_back(std::string(valarr[i]));
	g_strfreev(valarr);
	return ret;
}
