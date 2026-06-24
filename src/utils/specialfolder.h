/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef UTILS_SPECIALFOLDER_H
#define UTILS_SPECIALFOLDER_H 1

#include <string>
#include "tiny_string.h"
#ifdef _WIN32
#include <shlobj.h>
#endif

namespace lightspark
{
class SpecialFolder
{
private:
#ifdef _WIN32
	static std::string getSpecialFolder(REFKNOWNFOLDERID folderID);
#endif
public:
	static std::string get_user_config_dir();
	static std::string get_user_cache_dir();
	static std::string get_user_data_dir();
	static void get_system_config_dirs(std::list<tiny_string>& res);
};
}
#endif /* UTILS_SPECIALFOLDER_H */
