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

#include "currency.h"

#include "logger.h"

using namespace lightspark;
using namespace std;

CurrencyManager::CurrencyManager()
{
    currencySymbols.reserve(25);
    currencySymbols["USD"] = "$";
    currencySymbols["EUR"] = "€";
    currencySymbols["JPY"] = "¥";
    currencySymbols["AUD"] = "A$";
    currencySymbols["KWD"] = "د.ك";
    currencySymbols["SGD"] = "SN$";
    currencySymbols["CAD"] = "C$";
    currencySymbols["GBP"] = "£";
    currencySymbols["MYR"] = "RM";
    currencySymbols["UAE"] = "د.إ";
    currencySymbols["HKD"] = "HK$";
    currencySymbols["SWD"] = "S£";
    currencySymbols["BR"] = "B$";
    currencySymbols["NZD"] = "NZ$";
    currencySymbols["CYN"] = "元/圆¥";
    currencySymbols["QR"] = "ر.ق";
    currencySymbols["KWON"] = "₩";
    currencySymbols["NT"] = "NT$";
    currencySymbols["BHD"] = ".د.ب";
    currencySymbols["TB"] = "฿";
    currencySymbols["SR"] = "ر.س";
    currencySymbols["PHP"] = "₱ ";

    currencySymbols["Dollar"] = "$";
    currencySymbols["Pound"] = "£";
    currencySymbols["EURO"] = "€";

    countryISOSymbols["GB"]= "GBP"; // United Kingdom
    countryISOSymbols["US"] = "USD"; // United States of America
    countryISOSymbols["CA"] = "CAD"; // Canada

    countryISOSymbols["AT"] = "EUR"; // Austria
    countryISOSymbols["BE"] = "EUR"; // Belgium
    countryISOSymbols["BG"] = "EUR"; // Bulgaria
    countryISOSymbols["HR"] = "EUR"; // Croatia
    countryISOSymbols["CY"] = "EUR"; // Republic of Cyprus
    countryISOSymbols["CZ"] = "EUR"; // Czech Republic
    countryISOSymbols["DK"] = "EUR"; // Demark
    countryISOSymbols["EE"] = "EUR"; // Estonia
    countryISOSymbols["FI"] = "EUR"; // Finland
    countryISOSymbols["FR"] = "EUR"; // France
    countryISOSymbols["DE"] = "EUR"; // Germany
    countryISOSymbols["EL"] = "EUR"; // Greese
    countryISOSymbols["HU"] = "EUR"; // Hungary
    countryISOSymbols["IE"] = "EUR"; // Ireland
    countryISOSymbols["IT"] = "EUR"; // Italy
    countryISOSymbols["LV"] = "EUR"; // Latvia
    countryISOSymbols["LT"] = "EUR"; // Lithuania
    countryISOSymbols["LU"] = "EUR"; // Luxembourg
    countryISOSymbols["MT"] = "EUR"; // Malta
    countryISOSymbols["NL"] = "EUR"; // Netherlands
    countryISOSymbols["PL"] = "EUR"; // Poland
    countryISOSymbols["PT"] = "EUR"; // Portugal
    countryISOSymbols["RO"] = "EUR"; // Romania
    countryISOSymbols["SK"] = "EUR"; // Slovak Republic
    countryISOSymbols["SI"] = "EUR"; // Slovenia
    countryISOSymbols["ES"] = "EUR"; // Spain
    countryISOSymbols["SE"] = "EUR"; // Sweden

    // TODO: Add more country ISO symbols here
}

std::unordered_map<std::string, std::string> CurrencyManager::getCurrencySymbols()
{
    return currencySymbols;
}

std::unordered_map<std::string, std::string> CurrencyManager::getCountryISOSymbols()
{
    return currencySymbols;
}

std::string CurrencyManager::getCurrencySymbol(std::string currencyName)
{
    // TODO: Replace below with find
    for (std::unordered_map<std::string, std::string>::iterator it = currencySymbols.begin(); it != currencySymbols.end(); ++it)
    {
        if ((*it).first == currencyName)
        {
            return (*it).second;
        }
    }
    return "";
}

std::string CurrencyManager::getCountryISOSymbol(std::string isoCountryCode)
{
    // TODO: Replace below with find
    for (std::unordered_map<std::string, std::string>::iterator it = countryISOSymbols.begin(); it != countryISOSymbols.end(); ++it)
    {
        if ((*it).first == isoCountryCode)
        {
            return (*it).second;
        }
    }
    return "";
}

