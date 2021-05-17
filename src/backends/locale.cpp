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
	init();
}
void LocaleManager::init()
{
	locales["af-ZA"] = new LocaleItem("af-ZA", "af", "ZA", "Latn", "", false);
	locales["am-ET"] = new LocaleItem("am-ET", "am", "ET", "Ethi", "", false);
	locales["ar-AE"] = new LocaleItem("ar-AE", "ar", "AE", "Arab", "", true);
	locales["ar-BH"] = new LocaleItem("ar-BH", "ar", "BH", "Arab", "", true);
	locales["ar-DZ"] = new LocaleItem("ar-DZ", "ar", "DZ", "Arab", "", true);
	locales["ar-EG"] = new LocaleItem("ar-EG", "ar", "EG", "Arab", "", true);
	locales["ar-IQ"] = new LocaleItem("ar-IQ", "ar", "IQ", "Arab", "", true);
	locales["ar-JO"] = new LocaleItem("ar-JO", "ar", "JO", "Arab", "", true);
	locales["ar-KW"] = new LocaleItem("ar-KW", "ar", "KW", "Arab", "", true);
	locales["ar-LB"] = new LocaleItem("ar-LB", "ar", "LB", "Arab", "", true);
	locales["ar-LY"] = new LocaleItem("ar-LY", "ar", "LY", "Arab", "", true);
	locales["ar-MA"] = new LocaleItem("ar-MA", "ar", "MA", "Arab", "", true);
	locales["arn-CL"] = new LocaleItem("arn-CL", "arn", "CL", "Latn", "", false);
	locales["ar-OM"] = new LocaleItem("ar-OM", "ar", "OM", "Arab", "", true);
	locales["ar-QA"] = new LocaleItem("ar-QA", "ar", "QA", "Arab", "", true);
	locales["ar-SA"] = new LocaleItem("ar-SA", "ar", "SA", "Arab", "", true);
	locales["ar-SY"] = new LocaleItem("ar-SY", "ar", "SY", "Arab", "", true);
	locales["ar-TN"] = new LocaleItem("ar-TN", "ar", "TN", "Arab", "", true);
	locales["ar-YE"] = new LocaleItem("ar-YE", "ar", "YE", "Arab", "", true);
	locales["as-IN"] = new LocaleItem("as-IN", "as", "IN", "Beng", "", false);
	locales["az-Cyrl-AZ"] = new LocaleItem("az-Cyrl-AZ", "az", "AZ", "Cyrl", "", false);
	locales["az-Latn-AZ"] = new LocaleItem("az-Latn-AZ", "az", "AZ", "Latn", "", false);
	locales["ba-RU"] = new LocaleItem("ba-RU", "ba", "RU", "Cyrl", "", false);
	locales["be-BY"] = new LocaleItem("be-BY", "be", "BY", "Cyrl", "", false);
	locales["bg-BG"] = new LocaleItem("bg-BG", "bg", "BG", "Cyrl", "", false);
	locales["bn-BD"] = new LocaleItem("bn-BD", "bn", "BD", "Beng", "", false);
	locales["bn-IN"] = new LocaleItem("bn-IN", "bn", "IN", "Beng", "", false);
	locales["bo-CN"] = new LocaleItem("bo-CN", "bo", "CN", "Tibt", "", false);
	locales["br-FR"] = new LocaleItem("br-FR", "br", "FR", "Latn", "", false);
	locales["bs-Cyrl-BA"] = new LocaleItem("bs-Cyrl-BA", "bs", "BA", "Cyrl", "", false);
	locales["bs-Latn-BA"] = new LocaleItem("bs-Latn-BA", "bs", "BA", "Latn", "", false);
	locales["ca-ES"] = new LocaleItem("ca-ES", "ca", "ES", "Latn", "", false);
	locales["co-FR"] = new LocaleItem("co-FR", "co", "FR", "Latn", "", false);
	locales["cs-CZ"] = new LocaleItem("cs-CZ", "cs", "CZ", "Latn", "", false);
	locales["cy-GB"] = new LocaleItem("cy-GB", "cy", "GB", "Latn", "", false);
	locales["da-DK"] = new LocaleItem("da-DK", "da", "DK", "Latn", "", false);
	locales["de-AT"] = new LocaleItem("de-AT", "de", "AT", "Latn", "", false);
	locales["de-CH"] = new LocaleItem("de-CH", "de", "CH", "Latn", "", false);
	locales["de-DE"] = new LocaleItem("de-DE", "de", "DE", "Latn", "", false);
	locales["de-LI"] = new LocaleItem("de-LI", "de", "LI", "Latn", "", false);
	locales["de-LU"] = new LocaleItem("de-LU", "de", "LU", "Latn", "", false);
	locales["dsb-DE"] = new LocaleItem("dsb-DE", "dsb", "DE", "Latn", "", false);
	locales["dv-MV"] = new LocaleItem("dv-MV", "dv", "MV", "Thaa", "", true);
	locales["el-GR"] = new LocaleItem("el-GR", "el", "GR", "Grek", "", false);
	locales["en-029"] = new LocaleItem("en-029", "en", "029", "Latn", "", false);
	locales["en-AU"] = new LocaleItem("en-AU", "en", "AU", "Latn", "", false);
	locales["en-BZ"] = new LocaleItem("en-BZ", "en", "BZ", "Latn", "", false);
	locales["en-CA"] = new LocaleItem("en-CA", "en", "CA", "Latn", "", false);
	locales["en-GB"] = new LocaleItem("en-GB", "en", "GB", "Latn", "", false);
	locales["en-IE"] = new LocaleItem("en-IE", "en", "IE", "Latn", "", false);
	locales["en-IN"] = new LocaleItem("en-IN", "en", "IN", "Latn", "", false);
	locales["en-JM"] = new LocaleItem("en-JM", "en", "JM", "Latn", "", false);
	locales["en-MY"] = new LocaleItem("en-MY", "en", "MY", "Latn", "", false);
	locales["en-NZ"] = new LocaleItem("en-NZ", "en", "NZ", "Latn", "", false);
	locales["en-PH"] = new LocaleItem("en-PH", "en", "PH", "Latn", "", false);
	locales["en-SG"] = new LocaleItem("en-SG", "en", "SG", "Latn", "", false);
	locales["en-TT"] = new LocaleItem("en-TT", "en", "TT", "Latn", "", false);
	locales["en-US"] = new LocaleItem("en-US", "en", "US", "Latn", "", false);
	locales["en-ZA"] = new LocaleItem("en-ZA", "en", "ZA", "Latn", "", false);
	locales["en-ZW"] = new LocaleItem("en-ZW", "en", "ZW", "Latn", "", false);
	locales["es-AR"] = new LocaleItem("es-AR", "es", "AR", "Latn", "", false);
	locales["es-BO"] = new LocaleItem("es-BO", "es", "BO", "Latn", "", false);
	locales["es-CL"] = new LocaleItem("es-CL", "es", "CL", "Latn", "", false);
	locales["es-CO"] = new LocaleItem("es-CO", "es", "CO", "Latn", "", false);
	locales["es-CR"] = new LocaleItem("es-CR", "es", "CR", "Latn", "", false);
	locales["es-DO"] = new LocaleItem("es-DO", "es", "DO", "Latn", "", false);
	locales["es-EC"] = new LocaleItem("es-EC", "es", "EC", "Latn", "", false);
	locales["es-ES"] = new LocaleItem("es-ES", "es", "ES", "Latn", "", false);
	locales["es-GT"] = new LocaleItem("es-GT", "es", "GT", "Latn", "", false);
	locales["es-HN"] = new LocaleItem("es-HN", "es", "HN", "Latn", "", false);
	locales["es-MX"] = new LocaleItem("es-MX", "es", "MX", "Latn", "", false);
	locales["es-NI"] = new LocaleItem("es-NI", "es", "NI", "Latn", "", false);
	locales["es-PA"] = new LocaleItem("es-PA", "es", "PA", "Latn", "", false);
	locales["es-PE"] = new LocaleItem("es-PE", "es", "PE", "Latn", "", false);
	locales["es-PR"] = new LocaleItem("es-PR", "es", "PR", "Latn", "", false);
	locales["es-PY"] = new LocaleItem("es-PY", "es", "PY", "Latn", "", false);
	locales["es-SV"] = new LocaleItem("es-SV", "es", "SV", "Latn", "", false);
	locales["es-US"] = new LocaleItem("es-US", "es", "US", "Latn", "", false);
	locales["es-UY"] = new LocaleItem("es-UY", "es", "UY", "Latn", "", false);
	locales["es-VE"] = new LocaleItem("es-VE", "es", "VE", "Latn", "", false);
	locales["et-EE"] = new LocaleItem("et-EE", "et", "EE", "Latn", "", false);
	locales["eu-ES"] = new LocaleItem("eu-ES", "eu", "ES", "Latn", "", false);
	locales["fa-IR"] = new LocaleItem("fa-IR", "fa", "IR", "Arab", "", true);
	locales["fi-FI"] = new LocaleItem("fi-FI", "fi", "FI", "Latn", "", false);
	locales["fil-PH"] = new LocaleItem("fil-PH", "fil", "PH", "Latn", "", false);
	locales["fo-FO"] = new LocaleItem("fo-FO", "fo", "FO", "Latn", "", false);
	locales["fr-BE"] = new LocaleItem("fr-BE", "fr", "BE", "Latn", "", false);
	locales["fr-CA"] = new LocaleItem("fr-CA", "fr", "CA", "Latn", "", false);
	locales["fr-CH"] = new LocaleItem("fr-CH", "fr", "CH", "Latn", "", false);
	locales["fr-FR"] = new LocaleItem("fr-FR", "fr", "FR", "Latn", "", false);
	locales["fr-LU"] = new LocaleItem("fr-LU", "fr", "LU", "Latn", "", false);
	locales["fr-MC"] = new LocaleItem("fr-MC", "fr", "MC", "Latn", "", false);
	locales["fy-NL"] = new LocaleItem("fy-NL", "fy", "NL", "Latn", "", false);
	locales["ga-IE"] = new LocaleItem("ga-IE", "ga", "IE", "Latn", "", false);
	locales["gd-GB"] = new LocaleItem("gd-GB", "gd", "GB", "Latn", "", false);
	locales["gl-ES"] = new LocaleItem("gl-ES", "gl", "ES", "Latn", "", false);
	locales["gsw-FR"] = new LocaleItem("gsw-FR", "gsw", "FR", "Latn", "", false);
	locales["gu-IN"] = new LocaleItem("gu-IN", "gu", "IN", "Gujr", "", false);
	locales["ha-Latn-NG"] = new LocaleItem("ha-Latn-NG", "ha", "NG", "Latn", "", false);
	locales["he-IL"] = new LocaleItem("he-IL", "he", "IL", "Hebr", "", true);
	locales["hi-IN"] = new LocaleItem("hi-IN", "hi", "IN", "Deva", "", false);
	locales["hr-BA"] = new LocaleItem("hr-BA", "hr", "BA", "Latn", "", false);
	locales["hr-HR"] = new LocaleItem("hr-HR", "hr", "HR", "Latn", "", false);
	locales["hsb-DE"] = new LocaleItem("hsb-DE", "hsb", "DE", "Latn", "", false);
	locales["hu-HU"] = new LocaleItem("hu-HU", "hu", "HU", "Latn", "", false);
	locales["hy-AM"] = new LocaleItem("hy-AM", "hy", "AM", "Armn", "", false);
	locales["id-ID"] = new LocaleItem("id-ID", "id", "ID", "Latn", "", false);
	locales["ig-NG"] = new LocaleItem("ig-NG", "ig", "NG", "Latn", "", false);
	locales["ii-CN"] = new LocaleItem("ii-CN", "ii", "CN", "Yiii", "", false);
	locales["is-IS"] = new LocaleItem("is-IS", "is", "IS", "Latn", "", false);
	locales["it-CH"] = new LocaleItem("it-CH", "it", "CH", "Latn", "", false);
	locales["it-IT"] = new LocaleItem("it-IT", "it", "IT", "Latn", "", false);
	locales["iu-Cans-CA"] = new LocaleItem("iu-Cans-CA", "iu", "CA", "Cans", "", false);
	locales["iu-Latn-CA"] = new LocaleItem("iu-Latn-CA", "iu", "CA", "Latn", "", false);
	locales["ja-JP"] = new LocaleItem("ja-JP", "ja", "JP", "Jpan", "", false);
	locales["ka-GE"] = new LocaleItem("ka-GE", "ka", "GE", "Geor", "", false);
	locales["kk-KZ"] = new LocaleItem("kk-KZ", "kk", "KZ", "Cyrl", "", false);
	locales["kl-GL"] = new LocaleItem("kl-GL", "kl", "GL", "Latn", "", false);
	locales["km-KH"] = new LocaleItem("km-KH", "km", "KH", "Khmr", "", false);
	locales["kn-IN"] = new LocaleItem("kn-IN", "kn", "IN", "Knda", "", false);
	locales["kok-IN"] = new LocaleItem("kok-IN", "kok", "IN", "Deva", "", false);
	locales["ko-KR"] = new LocaleItem("ko-KR", "ko", "KR", "Kore", "", false);
	locales["ky-KG"] = new LocaleItem("ky-KG", "ky", "KG", "Cyrl", "", false);
	locales["lb-LU"] = new LocaleItem("lb-LU", "lb", "LU", "Latn", "", false);
	locales["lo-LA"] = new LocaleItem("lo-LA", "lo", "LA", "Laoo", "", false);
	locales["lt-LT"] = new LocaleItem("lt-LT", "lt", "LT", "Latn", "", false);
	locales["lv-LV"] = new LocaleItem("lv-LV", "lv", "LV", "Latn", "", false);
	locales["mi-NZ"] = new LocaleItem("mi-NZ", "mi", "NZ", "Latn", "", false);
	locales["mk-MK"] = new LocaleItem("mk-MK", "mk", "MK", "Cyrl", "", false);
	locales["ml-IN"] = new LocaleItem("ml-IN", "ml", "IN", "Mlym", "", false);
	locales["mn-MN"] = new LocaleItem("mn-MN", "mn", "MN", "Cyrl", "", false);
	locales["mn-Mong-CN"] = new LocaleItem("mn-Mong-CN", "mn", "CN", "Mong", "", false);
	locales["moh-CA"] = new LocaleItem("moh-CA", "moh", "CA", "Latn", "", false);
	locales["mr-IN"] = new LocaleItem("mr-IN", "mr", "IN", "Deva", "", false);
	locales["ms-BN"] = new LocaleItem("ms-BN", "ms", "BN", "Latn", "", false);
	locales["ms-MY"] = new LocaleItem("ms-MY", "ms", "MY", "Latn", "", false);
	locales["mt-MT"] = new LocaleItem("mt-MT", "mt", "MT", "Latn", "", false);
	locales["nb-NO"] = new LocaleItem("nb-NO", "nb", "NO", "Latn", "", false);
	locales["ne-NP"] = new LocaleItem("ne-NP", "ne", "NP", "Deva", "", false);
	locales["nl-BE"] = new LocaleItem("nl-BE", "nl", "BE", "Latn", "", false);
	locales["nl-NL"] = new LocaleItem("nl-NL", "nl", "NL", "Latn", "", false);
	locales["nn-NO"] = new LocaleItem("nn-NO", "nn", "NO", "Latn", "", false);
	locales["nso-ZA"] = new LocaleItem("nso-ZA", "nso", "ZA", "Latn", "", false);
	locales["oc-FR"] = new LocaleItem("oc-FR", "oc", "FR", "Latn", "", false);
	locales["or-IN"] = new LocaleItem("or-IN", "or", "IN", "Orya", "", false);
	locales["pa-IN"] = new LocaleItem("pa-IN", "pa", "IN", "Guru", "", false);
	locales["pl-PL"] = new LocaleItem("pl-PL", "pl", "PL", "Latn", "", false);
	locales["prs-AF"] = new LocaleItem("prs-AF", "prs", "AF", "Arab", "", true);
	locales["ps-AF"] = new LocaleItem("ps-AF", "ps", "AF", "Arab", "", true);
	locales["pt-BR"] = new LocaleItem("pt-BR", "pt", "BR", "Latn", "", false);
	locales["pt-PT"] = new LocaleItem("pt-PT", "pt", "PT", "Latn", "", false);
	locales["qut-GT"] = new LocaleItem("qut-GT", "qut", "GT", "Latn", "", false);
	locales["quz-BO"] = new LocaleItem("quz-BO", "quz", "BO", "Latn", "", false);
	locales["quz-EC"] = new LocaleItem("quz-EC", "quz", "EC", "Latn", "", false);
	locales["quz-PE"] = new LocaleItem("quz-PE", "quz", "PE", "Latn", "", false);
	locales["rm-CH"] = new LocaleItem("rm-CH", "rm", "CH", "Latn", "", false);
	locales["ro-RO"] = new LocaleItem("ro-RO", "ro", "RO", "Latn", "", false);
	locales["ru-RU"] = new LocaleItem("ru-RU", "ru", "RU", "Cyrl", "", false);
	locales["rw-RW"] = new LocaleItem("rw-RW", "rw", "RW", "Latn", "", false);
	locales["sah-RU"] = new LocaleItem("sah-RU", "sah", "RU", "Cyrl", "", false);
	locales["sa-IN"] = new LocaleItem("sa-IN", "sa", "IN", "Deva", "", false);
	locales["se-FI"] = new LocaleItem("se-FI", "se", "FI", "Latn", "", false);
	locales["se-NO"] = new LocaleItem("se-NO", "se", "NO", "Latn", "", false);
	locales["se-SE"] = new LocaleItem("se-SE", "se", "SE", "Latn", "", false);
	locales["si-LK"] = new LocaleItem("si-LK", "si", "LK", "Sinh", "", false);
	locales["sk-SK"] = new LocaleItem("sk-SK", "sk", "SK", "Latn", "", false);
	locales["sl-SI"] = new LocaleItem("sl-SI", "sl", "SI", "Latn", "", false);
	locales["sma-NO"] = new LocaleItem("sma-NO", "sma", "NO", "Latn", "", false);
	locales["sma-SE"] = new LocaleItem("sma-SE", "sma", "SE", "Latn", "", false);
	locales["smj-NO"] = new LocaleItem("smj-NO", "smj", "NO", "Latn", "", false);
	locales["smj-SE"] = new LocaleItem("smj-SE", "smj", "SE", "Latn", "", false);
	locales["smn-FI"] = new LocaleItem("smn-FI", "smn", "FI", "Latn", "", false);
	locales["sms-FI"] = new LocaleItem("sms-FI", "sms", "FI", "Latn", "", false);
	locales["sq-AL"] = new LocaleItem("sq-AL", "sq", "AL", "Latn", "", false);
	locales["sr-Cyrl-BA"] = new LocaleItem("sr-Cyrl-BA", "sr", "BA", "Cyrl", "", false);
	locales["sr-Cyrl-CS"] = new LocaleItem("sr-Cyrl-CS", "sr", "CS", "Cyrl", "", false);
	locales["sr-Cyrl-ME"] = new LocaleItem("sr-Cyrl-ME", "sr", "ME", "Cyrl", "", false);
	locales["sr-Cyrl-RS"] = new LocaleItem("sr-Cyrl-RS", "sr", "RS", "Cyrl", "", false);
	locales["sr-Latn-BA"] = new LocaleItem("sr-Latn-BA", "sr", "BA", "Latn", "", false);
	locales["sr-Latn-CS"] = new LocaleItem("sr-Latn-CS", "sr", "CS", "Latn", "", false);
	locales["sr-Latn-ME"] = new LocaleItem("sr-Latn-ME", "sr", "ME", "Latn", "", false);
	locales["sr-Latn-RS"] = new LocaleItem("sr-Latn-RS", "sr", "RS", "Latn", "", false);
	locales["sv-FI"] = new LocaleItem("sv-FI", "sv", "FI", "Latn", "", false);
	locales["sv-SE"] = new LocaleItem("sv-SE", "sv", "SE", "Latn", "", false);
	locales["sw-KE"] = new LocaleItem("sw-KE", "sw", "KE", "Latn", "", false);
	locales["syr-SY"] = new LocaleItem("syr-SY", "syr", "SY", "Syrc", "", true);
	locales["ta-IN"] = new LocaleItem("ta-IN", "ta", "IN", "Taml", "", false);
	locales["te-IN"] = new LocaleItem("te-IN", "te", "IN", "Telu", "", false);
	locales["tg-Cyrl-TJ"] = new LocaleItem("tg-Cyrl-TJ", "tg", "TJ", "Cyrl", "", false);
	locales["th-TH"] = new LocaleItem("th-TH", "th", "TH", "Thai", "", false);
	locales["tk-TM"] = new LocaleItem("tk-TM", "tk", "TM", "Latn", "", false);
	locales["tn-ZA"] = new LocaleItem("tn-ZA", "tn", "ZA", "Latn", "", false);
	locales["tr-TR"] = new LocaleItem("tr-TR", "tr", "TR", "Latn", "", false);
	locales["tt-RU"] = new LocaleItem("tt-RU", "tt", "RU", "Cyrl", "", false);
	locales["tzm-Latn-DZ"] = new LocaleItem("tzm-Latn-DZ", "tzm", "DZ", "Latn", "", false);
	locales["ug-CN"] = new LocaleItem("ug-CN", "ug", "CN", "Arab", "", true);
	locales["uk-UA"] = new LocaleItem("uk-UA", "uk", "UA", "Cyrl", "", false);
	locales["ur-PK"] = new LocaleItem("ur-PK", "ur", "PK", "Arab", "", true);
	locales["uz-Cyrl-UZ"] = new LocaleItem("uz-Cyrl-UZ", "uz", "UZ", "Cyrl", "", false);
	locales["uz-Latn-UZ"] = new LocaleItem("uz-Latn-UZ", "uz", "UZ", "Latn", "", false);
	locales["vi-VN"] = new LocaleItem("vi-VN", "vi", "VN", "Latn", "", false);
	locales["wo-SN"] = new LocaleItem("wo-SN", "wo", "SN", "Latn", "", false);
	locales["xh-ZA"] = new LocaleItem("xh-ZA", "xh", "ZA", "Latn", "", false);
	locales["yo-NG"] = new LocaleItem("yo-NG", "yo", "NG", "Latn", "", false);
	locales["zh-CN"] = new LocaleItem("zh-CN", "zh", "CN", "Hans", "", false);
	locales["zh-HK"] = new LocaleItem("zh-HK", "zh", "HK", "Hant", "", false);
	locales["zh-MO"] = new LocaleItem("zh-MO", "zh", "MO", "Hant", "", false);
	locales["zh-SG"] = new LocaleItem("zh-SG", "zh", "SG", "Hans", "", false);
	locales["zh-TW"] = new LocaleItem("zh-TW", "zh", "TW", "Hant", "", false);
	locales["zu-ZA"] = new LocaleItem("zu-ZA", "zu", "ZA", "Latn", "", false);
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
