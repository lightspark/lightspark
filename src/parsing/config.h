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

#ifndef PARSING_CONFIG_H
#define PARSING_CONFIG_H 1

#include <string>
#include <vector>
#include <glib.h>

namespace lightspark
{
	class ConfigParser
	{
	private:
		bool valid;
		GKeyFile* file;

		char** groups;
		gsize groupCount;
		gsize currentGroup;
		char** keys;
		gsize keyCount;
		gsize currentKey;
		//Status
		char* group;
		char* key;
	public:
		//Load a config from a file
		ConfigParser(const std::string& filename);
		~ConfigParser();

		bool isValid() { return valid; }
		bool read();

		//These properties and their return types are only valid until the next read()
		//They should be copied before use
		std::string getGroup() { return group; }
		std::string getKey() { return key; }
		std::string getValue();
		std::string getValueString();
		std::vector<std::string> getValueStringList();
		bool getValueBoolean() { return (bool)g_key_file_get_boolean(file, group, key, NULL); }
		bool* getValueBooleanList(gsize* length) { return (bool*)g_key_file_get_boolean_list(file, group, key, length, NULL); }
		int getValueInteger() { return g_key_file_get_integer(file, group, key, NULL); }
		const int* getValueIntegerList(gsize* length) { return g_key_file_get_integer_list(file, group, key, length, NULL); }
		double getValueDouble() { return g_key_file_get_double(file, group, key, NULL); }
		const double* getValueDoubleList(gsize* length) { return g_key_file_get_double_list(file, group, key, length, NULL); }
	};
}

#endif /* PARSING_CONFIG_H */
