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

#ifndef CURRENCY_H
#define CURRENCY_H 1

#include "tiny_string.h"
#include <unordered_map>

namespace lightspark
{

    class CurrencyManager
    {
        private:
            std::locale requestedLocale;
            std::unordered_map<std::string, std::string> currencySymbols;
            std::unordered_map<std::string, std::string> countryISOSymbols;
        public:
            CurrencyManager();
            std::unordered_map<std::string, std::string> getCurrencySymbols();
            std::unordered_map<std::string, std::string> getCountryISOSymbols();
            std::string getCurrencySymbol(std::string currencyName);
            std::string getCountryISOSymbol(std::string isoCountryCode);
    };
}

#endif // CURRENCY_H
