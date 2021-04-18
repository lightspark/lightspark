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

#ifndef LOCALE_H 
#define LOCALE_H 1

#include "tiny_string.h"
#include <unordered_map>
#include <vector>

namespace lightspark
{
	class LocaleItem
	{
	public:
		LocaleItem(
			std::string name,
			std::string language,
			std::string region,
			std::string script,
			std::string variant,
			bool isRightToLeft
		)
		{
			this->name = name;
			this->language = language;
			this->region = region;
			this->script = script;
			this->variant = variant;
			this->isRightToLeft = isRightToLeft;
		}
		std::string name;
		std::string language;
		std::string region;
		std::string script;
		std::string variant;
		bool isRightToLeft;
	};

	class LocaleManager
	{
		private:
			std::locale requestedLocale;
			std::unordered_map<std::string, LocaleItem*> locales;
			void init();
		public:
			LocaleManager();
			~LocaleManager();
			bool isLocaleAvailableOnSystem(std::string locale);
			std::string getSystemLocaleName(std::string locale);
			LocaleItem* getLocaleId(std::string name);
			std::vector<std::string> getAvailableLocaleIDNames();
	};
}

#endif /* LOCALE_H */
