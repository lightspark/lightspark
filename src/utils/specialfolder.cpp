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

#include "utils/specialfolder.h"
#include <SDL2/SDL.h>

using namespace lightspark;
#ifdef _WIN32
std::string SpecialFolder::getSpecialFolder(REFKNOWNFOLDERID folderID)
{
	std::string res;
	LPWSTR wszPath = NULL;
	HRESULT h = SHGetKnownFolderPath(folderID, KF_FLAG_CREATE, NULL, &wszPath);
	if (SUCCEEDED(h))
	{
		int len = wcslen(wszPath);
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wszPath, len, nullptr, 0, nullptr, nullptr);
		res = std::string( size_needed, 0 );
		WideCharToMultiByte(CP_UTF8, 0, wszPath, len, &res[0], size_needed, nullptr,nullptr);
	}
	CoTaskMemFree (wszPath);
	return res;
}
#endif

std::string SpecialFolder::get_user_config_dir()
{
	std::string res = getenv("XDG_CONFIG_HOME");
#ifdef _WIN32
	if (res.empty())
		res = getSpecialFolder(FOLDERID_LocalAppData);
#endif
	return res;
}

std::string SpecialFolder::get_user_cache_dir()
{
	std::string res = getenv("XDG_CACHE_HOME");
#ifdef _WIN32
	if (res.empty())
		res = getSpecialFolder(FOLDERID_InternetCache);
#endif
	return res;
}
std::string SpecialFolder::get_user_data_dir()
{
	std::string res = getenv("XDG_DATA_HOME");
#ifdef _WIN32
	if (res.empty())
		res = getSpecialFolder(FOLDERID_LocalAppData);
#endif
	return res;
}
void SpecialFolder::get_system_config_dirs(std::list<tiny_string>& res)
{
	tiny_string paths = getenv ("XDG_CONFIG_DIRS");
#ifdef _WIN32
	if (paths.empty())
		paths = getSpecialFolder(FOLDERID_ProgramData);
#endif
	res = paths.split(':');
}