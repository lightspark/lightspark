/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "locale.h"

#include "logger.h"
#include <locale>

using namespace lightspark;
using namespace std;

LocaleManager::LocaleManager()
{
	locales["en_AU"] = new LocaleItem("english", "Australia",	   "Latn", "AU", "", false);
	locales["en_CA"] = new LocaleItem("english", "Canada",		  "Latn", "CA", "", false);
	locales["en_DK"] = new LocaleItem("english", "Denmark",		 "Latn", "DK", "", false);
	locales["en_GB"] = new LocaleItem("english", "United Kingdom",  "Latn", "GB", "", false);
	locales["en_IE"] = new LocaleItem("english", "Ireland",		 "Latn", "IE", "", false);
	locales["en_IN"] = new LocaleItem("english", "India",		   "Latn", "IN", "", false);
	locales["en_NZ"] = new LocaleItem("english", "New Zealand",	 "Latn", "NZ", "", false);
	locales["en_PH"] = new LocaleItem("english", "Philippines",	 "Latn", "PH", "", false);
	locales["en_US"] = new LocaleItem("english", "United States",   "Latn", "US", "", false);
	locales["en_ZA"] = new LocaleItem("english", "South Africa",	"Latn", "ZA", "", false);
}

LocaleManager::~LocaleManager()
{
	for (std::unordered_map<string, LocaleItem*>::iterator it = locales.begin(); it != locales.end(); ++it)
	{
		delete (*it).second;
	}
}

bool LocaleManager::isLocaleAvailableOnSystem(std::string locale)
{
	try
	{
		if (locale == "" || locale == "LocaleID.DEFAULT")
		{
			return true;
		}
		requestedLocale = std::locale(locale.c_str());
		return true;
	}
	catch (std::runtime_error& e)
	{
		size_t pos = locale.find("-");
		if(pos != string::npos)
		{
			std::string r("_");
			std::string l = locale.replace(pos,1,r);
			try
			{
				// try with "_" instead of "-"
				requestedLocale = std::locale(l.c_str());
				return true;
			}
			catch (std::runtime_error& e)
			{
				try
				{
					// try appending ".UTF-8"
					l += ".UTF-8";
					requestedLocale = std::locale(l.c_str());
					return true;
				}
				catch (std::runtime_error& e)
				{
					return false;
				}
			}
		}
		else
		{
			try
			{
				// try appending ".UTF-8"
				locale += ".UTF-8";
				requestedLocale = std::locale(locale.c_str());
				return true;
			}
			catch (std::runtime_error& e)
			{
				return false;
			}
		}
	}
	return true;
}

std::string LocaleManager::getSystemLocaleName(std::string name)
{
	try
	{
		if (name == "" || name == "LocaleID.DEFAULT")
		{
			return std::locale("").name();
		}
		requestedLocale = std::locale(name.c_str());
		return name;
	}
	catch (std::runtime_error& e)
	{
		size_t pos = name.find("-");
		if(pos != string::npos) // Check this line
		{
			std::string r("_");
			std::string l = name.replace(pos,1,r);
			try
			{
				// try with "_" instead of "-"
				requestedLocale = std::locale(l.c_str());
				return l;
			}
			catch (std::runtime_error& e)
			{
				try
				{
					// try appending ".UTF-8"
					l += ".UTF-8";
					requestedLocale = std::locale(l.c_str());
					return l;
				}
				catch (std::runtime_error& e)
				{
					LOG(LOG_ERROR,"unknown locale:"<<name<<" "<<e.what());
					return "";
				}
			}
		}
		else
		{
			try
			{
				// try appending ".UTF-8"
				name += ".UTF-8";
				requestedLocale = std::locale(name.c_str());
				return name;
			}
			catch (std::runtime_error& e)
			{
				LOG(LOG_ERROR,"unknown locale:"<<name<<" "<<e.what());
				return "";
			}
		}
	}
	return "";
}

LocaleItem* LocaleManager::getLocaleId(std::string name)
{
	return locales[name];
}

std::vector<std::string> LocaleManager::getAvailableLocaleIDNames()
{
	std::vector<std::string> values;
	for (auto it = locales.begin(); it != locales.end(); ++it)
	{
		if (isLocaleAvailableOnSystem((*it).first))
		{
			values.push_back((*it).first);
		}
	}
	return values;
}