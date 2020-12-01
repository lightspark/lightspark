package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.LocaleID;
	
	import flash.globalization.Collator;
	import flash.globalization.CurrencyFormatter;
	import flash.globalization.DateTimeFormatter;
	import flash.globalization.NumberFormatter;
	import flash.globalization.StringTools;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var locales:Array = new Array();
			
			locales.push(new LocaleItem("af-ZA", "af", "ZA", "Latn", "", false));
			locales.push(new LocaleItem("am-ET", "am", "ET", "Ethi", "", false));
			locales.push(new LocaleItem("ar-AE", "ar", "AE", "Arab", "", true));
			locales.push(new LocaleItem("ar-BH", "ar", "BH", "Arab", "", true));
			locales.push(new LocaleItem("ar-DZ", "ar", "DZ", "Arab", "", true));
			locales.push(new LocaleItem("ar-EG", "ar", "EG", "Arab", "", true));
			locales.push(new LocaleItem("ar-IQ", "ar", "IQ", "Arab", "", true));
			locales.push(new LocaleItem("ar-JO", "ar", "JO", "Arab", "", true));
			locales.push(new LocaleItem("ar-KW", "ar", "KW", "Arab", "", true));
			locales.push(new LocaleItem("ar-LB", "ar", "LB", "Arab", "", true));
			locales.push(new LocaleItem("ar-LY", "ar", "LY", "Arab", "", true));
			locales.push(new LocaleItem("ar-MA", "ar", "MA", "Arab", "", true));
			locales.push(new LocaleItem("arn-CL", "arn", "CL", "Latn", "", false));
			locales.push(new LocaleItem("ar-OM", "ar", "OM", "Arab", "", true));
			locales.push(new LocaleItem("ar-QA", "ar", "QA", "Arab", "", true));
			locales.push(new LocaleItem("ar-SA", "ar", "SA", "Arab", "", true));
			locales.push(new LocaleItem("ar-SY", "ar", "SY", "Arab", "", true));
			locales.push(new LocaleItem("ar-TN", "ar", "TN", "Arab", "", true));
			locales.push(new LocaleItem("ar-YE", "ar", "YE", "Arab", "", true));
			locales.push(new LocaleItem("as-IN", "as", "IN", "Beng", "", false));
			locales.push(new LocaleItem("az-Cyrl-AZ", "az", "AZ", "Cyrl", "", false));
			locales.push(new LocaleItem("az-Latn-AZ", "az", "AZ", "Latn", "", false));
			locales.push(new LocaleItem("ba-RU", "ba", "RU", "Cyrl", "", false));
			locales.push(new LocaleItem("be-BY", "be", "BY", "Cyrl", "", false));
			locales.push(new LocaleItem("bg-BG", "bg", "BG", "Cyrl", "", false));
			locales.push(new LocaleItem("bn-BD", "bn", "BD", "Beng", "", false));
			locales.push(new LocaleItem("bn-IN", "bn", "IN", "Beng", "", false));
			locales.push(new LocaleItem("bo-CN", "bo", "CN", "Tibt", "", false));
			locales.push(new LocaleItem("br-FR", "br", "FR", "Latn", "", false));
			locales.push(new LocaleItem("bs-Cyrl-BA", "bs", "BA", "Cyrl", "", false));
			locales.push(new LocaleItem("bs-Latn-BA", "bs", "BA", "Latn", "", false));
			locales.push(new LocaleItem("ca-ES", "ca", "ES", "Latn", "", false));
			locales.push(new LocaleItem("co-FR", "co", "FR", "Latn", "", false));
			locales.push(new LocaleItem("cs-CZ", "cs", "CZ", "Latn", "", false));
			locales.push(new LocaleItem("cy-GB", "cy", "GB", "Latn", "", false));
			locales.push(new LocaleItem("da-DK", "da", "DK", "Latn", "", false));
			locales.push(new LocaleItem("de-AT", "de", "AT", "Latn", "", false));
			locales.push(new LocaleItem("de-CH", "de", "CH", "Latn", "", false));
			locales.push(new LocaleItem("de-DE", "de", "DE", "Latn", "", false));
			locales.push(new LocaleItem("de-LI", "de", "LI", "Latn", "", false));
			locales.push(new LocaleItem("de-LU", "de", "LU", "Latn", "", false));
			locales.push(new LocaleItem("dsb-DE", "dsb", "DE", "Latn", "", false));
			locales.push(new LocaleItem("dv-MV", "dv", "MV", "Thaa", "", true));
			locales.push(new LocaleItem("el-GR", "el", "GR", "Grek", "", false));
			locales.push(new LocaleItem("en-029", "en", "029", "Latn", "", false));
			locales.push(new LocaleItem("en-AU", "en", "AU", "Latn", "", false));
			locales.push(new LocaleItem("en-BZ", "en", "BZ", "Latn", "", false));
			locales.push(new LocaleItem("en-CA", "en", "CA", "Latn", "", false));
			locales.push(new LocaleItem("en-GB", "en", "GB", "Latn", "", false));
			locales.push(new LocaleItem("en-IE", "en", "IE", "Latn", "", false));
			locales.push(new LocaleItem("en-IN", "en", "IN", "Latn", "", false));
			locales.push(new LocaleItem("en-JM", "en", "JM", "Latn", "", false));
			locales.push(new LocaleItem("en-MY", "en", "MY", "Latn", "", false));
			locales.push(new LocaleItem("en-NZ", "en", "NZ", "Latn", "", false));
			locales.push(new LocaleItem("en-PH", "en", "PH", "Latn", "", false));
			locales.push(new LocaleItem("en-SG", "en", "SG", "Latn", "", false));
			locales.push(new LocaleItem("en-TT", "en", "TT", "Latn", "", false));
			locales.push(new LocaleItem("en-US", "en", "US", "Latn", "", false));
			locales.push(new LocaleItem("en-ZA", "en", "ZA", "Latn", "", false));
			locales.push(new LocaleItem("en-ZW", "en", "ZW", "Latn", "", false));
			locales.push(new LocaleItem("es-AR", "es", "AR", "Latn", "", false));
			locales.push(new LocaleItem("es-BO", "es", "BO", "Latn", "", false));
			locales.push(new LocaleItem("es-CL", "es", "CL", "Latn", "", false));
			locales.push(new LocaleItem("es-CO", "es", "CO", "Latn", "", false));
			locales.push(new LocaleItem("es-CR", "es", "CR", "Latn", "", false));
			locales.push(new LocaleItem("es-DO", "es", "DO", "Latn", "", false));
			locales.push(new LocaleItem("es-EC", "es", "EC", "Latn", "", false));
			locales.push(new LocaleItem("es-ES", "es", "ES", "Latn", "", false));
			locales.push(new LocaleItem("es-GT", "es", "GT", "Latn", "", false));
			locales.push(new LocaleItem("es-HN", "es", "HN", "Latn", "", false));
			locales.push(new LocaleItem("es-MX", "es", "MX", "Latn", "", false));
			locales.push(new LocaleItem("es-NI", "es", "NI", "Latn", "", false));
			locales.push(new LocaleItem("es-PA", "es", "PA", "Latn", "", false));
			locales.push(new LocaleItem("es-PE", "es", "PE", "Latn", "", false));
			locales.push(new LocaleItem("es-PR", "es", "PR", "Latn", "", false));
			locales.push(new LocaleItem("es-PY", "es", "PY", "Latn", "", false));
			locales.push(new LocaleItem("es-SV", "es", "SV", "Latn", "", false));
			locales.push(new LocaleItem("es-US", "es", "US", "Latn", "", false));
			locales.push(new LocaleItem("es-UY", "es", "UY", "Latn", "", false));
			locales.push(new LocaleItem("es-VE", "es", "VE", "Latn", "", false));
			locales.push(new LocaleItem("et-EE", "et", "EE", "Latn", "", false));
			locales.push(new LocaleItem("eu-ES", "eu", "ES", "Latn", "", false));
			locales.push(new LocaleItem("fa-IR", "fa", "IR", "Arab", "", true));
			locales.push(new LocaleItem("fi-FI", "fi", "FI", "Latn", "", false));
			locales.push(new LocaleItem("fil-PH", "fil", "PH", "Latn", "", false));
			locales.push(new LocaleItem("fo-FO", "fo", "FO", "Latn", "", false));
			locales.push(new LocaleItem("fr-BE", "fr", "BE", "Latn", "", false));
			locales.push(new LocaleItem("fr-CA", "fr", "CA", "Latn", "", false));
			locales.push(new LocaleItem("fr-CH", "fr", "CH", "Latn", "", false));
			locales.push(new LocaleItem("fr-FR", "fr", "FR", "Latn", "", false));
			locales.push(new LocaleItem("fr-LU", "fr", "LU", "Latn", "", false));
			locales.push(new LocaleItem("fr-MC", "fr", "MC", "Latn", "", false));
			locales.push(new LocaleItem("fy-NL", "fy", "NL", "Latn", "", false));
			locales.push(new LocaleItem("ga-IE", "ga", "IE", "Latn", "", false));
			locales.push(new LocaleItem("gd-GB", "gd", "GB", "Latn", "", false));
			locales.push(new LocaleItem("gl-ES", "gl", "ES", "Latn", "", false));
			locales.push(new LocaleItem("gsw-FR", "gsw", "FR", "Latn", "", false));
			locales.push(new LocaleItem("gu-IN", "gu", "IN", "Gujr", "", false));
			locales.push(new LocaleItem("ha-Latn-NG", "ha", "NG", "Latn", "", false));
			locales.push(new LocaleItem("he-IL", "he", "IL", "Hebr", "", true));
			locales.push(new LocaleItem("hi-IN", "hi", "IN", "Deva", "", false));
			locales.push(new LocaleItem("hr-BA", "hr", "BA", "Latn", "", false));
			locales.push(new LocaleItem("hr-HR", "hr", "HR", "Latn", "", false));
			locales.push(new LocaleItem("hsb-DE", "hsb", "DE", "Latn", "", false));
			locales.push(new LocaleItem("hu-HU", "hu", "HU", "Latn", "", false));
			locales.push(new LocaleItem("hy-AM", "hy", "AM", "Armn", "", false));
			locales.push(new LocaleItem("id-ID", "id", "ID", "Latn", "", false));
			locales.push(new LocaleItem("ig-NG", "ig", "NG", "Latn", "", false));
			locales.push(new LocaleItem("ii-CN", "ii", "CN", "Yiii", "", false));
			locales.push(new LocaleItem("is-IS", "is", "IS", "Latn", "", false));
			locales.push(new LocaleItem("it-CH", "it", "CH", "Latn", "", false));
			locales.push(new LocaleItem("it-IT", "it", "IT", "Latn", "", false));
			locales.push(new LocaleItem("iu-Cans-CA", "iu", "CA", "Cans", "", false));
			locales.push(new LocaleItem("iu-Latn-CA", "iu", "CA", "Latn", "", false));
			locales.push(new LocaleItem("ja-JP", "ja", "JP", "Jpan", "", false));
			locales.push(new LocaleItem("ka-GE", "ka", "GE", "Geor", "", false));
			locales.push(new LocaleItem("kk-KZ", "kk", "KZ", "Cyrl", "", false));
			locales.push(new LocaleItem("kl-GL", "kl", "GL", "Latn", "", false));
			locales.push(new LocaleItem("km-KH", "km", "KH", "Khmr", "", false));
			locales.push(new LocaleItem("kn-IN", "kn", "IN", "Knda", "", false));
			locales.push(new LocaleItem("kok-IN", "kok", "IN", "Deva", "", false));
			locales.push(new LocaleItem("ko-KR", "ko", "KR", "Kore", "", false));
			locales.push(new LocaleItem("ky-KG", "ky", "KG", "Cyrl", "", false));
			locales.push(new LocaleItem("lb-LU", "lb", "LU", "Latn", "", false));
			locales.push(new LocaleItem("lo-LA", "lo", "LA", "Laoo", "", false));
			locales.push(new LocaleItem("lt-LT", "lt", "LT", "Latn", "", false));
			locales.push(new LocaleItem("lv-LV", "lv", "LV", "Latn", "", false));
			locales.push(new LocaleItem("mi-NZ", "mi", "NZ", "Latn", "", false));
			locales.push(new LocaleItem("mk-MK", "mk", "MK", "Cyrl", "", false));
			locales.push(new LocaleItem("ml-IN", "ml", "IN", "Mlym", "", false));
			locales.push(new LocaleItem("mn-MN", "mn", "MN", "Cyrl", "", false));
			locales.push(new LocaleItem("mn-Mong-CN", "mn", "CN", "Mong", "", false));
			locales.push(new LocaleItem("moh-CA", "moh", "CA", "Latn", "", false));
			locales.push(new LocaleItem("mr-IN", "mr", "IN", "Deva", "", false));
			locales.push(new LocaleItem("ms-BN", "ms", "BN", "Latn", "", false));
			locales.push(new LocaleItem("ms-MY", "ms", "MY", "Latn", "", false));
			locales.push(new LocaleItem("mt-MT", "mt", "MT", "Latn", "", false));
			locales.push(new LocaleItem("nb-NO", "nb", "NO", "Latn", "", false));
			locales.push(new LocaleItem("ne-NP", "ne", "NP", "Deva", "", false));
			locales.push(new LocaleItem("nl-BE", "nl", "BE", "Latn", "", false));
			locales.push(new LocaleItem("nl-NL", "nl", "NL", "Latn", "", false));
			locales.push(new LocaleItem("nn-NO", "nn", "NO", "Latn", "", false));
			locales.push(new LocaleItem("nso-ZA", "nso", "ZA", "Latn", "", false));
			locales.push(new LocaleItem("oc-FR", "oc", "FR", "Latn", "", false));
			locales.push(new LocaleItem("or-IN", "or", "IN", "Orya", "", false));
			locales.push(new LocaleItem("pa-IN", "pa", "IN", "Guru", "", false));
			locales.push(new LocaleItem("pl-PL", "pl", "PL", "Latn", "", false));
			locales.push(new LocaleItem("prs-AF", "prs", "AF", "Arab", "", true));
			locales.push(new LocaleItem("ps-AF", "ps", "AF", "Arab", "", true));
			locales.push(new LocaleItem("pt-BR", "pt", "BR", "Latn", "", false));
			locales.push(new LocaleItem("pt-PT", "pt", "PT", "Latn", "", false));
			locales.push(new LocaleItem("qut-GT", "qut", "GT", "Latn", "", false));
			locales.push(new LocaleItem("quz-BO", "quz", "BO", "Latn", "", false));
			locales.push(new LocaleItem("quz-EC", "quz", "EC", "Latn", "", false));
			locales.push(new LocaleItem("quz-PE", "quz", "PE", "Latn", "", false));
			locales.push(new LocaleItem("rm-CH", "rm", "CH", "Latn", "", false));
			locales.push(new LocaleItem("ro-RO", "ro", "RO", "Latn", "", false));
			locales.push(new LocaleItem("ru-RU", "ru", "RU", "Cyrl", "", false));
			locales.push(new LocaleItem("rw-RW", "rw", "RW", "Latn", "", false));
			locales.push(new LocaleItem("sah-RU", "sah", "RU", "Cyrl", "", false));
			locales.push(new LocaleItem("sa-IN", "sa", "IN", "Deva", "", false));
			locales.push(new LocaleItem("se-FI", "se", "FI", "Latn", "", false));
			locales.push(new LocaleItem("se-NO", "se", "NO", "Latn", "", false));
			locales.push(new LocaleItem("se-SE", "se", "SE", "Latn", "", false));
			locales.push(new LocaleItem("si-LK", "si", "LK", "Sinh", "", false));
			locales.push(new LocaleItem("sk-SK", "sk", "SK", "Latn", "", false));
			locales.push(new LocaleItem("sl-SI", "sl", "SI", "Latn", "", false));
			locales.push(new LocaleItem("sma-NO", "sma", "NO", "Latn", "", false));
			locales.push(new LocaleItem("sma-SE", "sma", "SE", "Latn", "", false));
			locales.push(new LocaleItem("smj-NO", "smj", "NO", "Latn", "", false));
			locales.push(new LocaleItem("smj-SE", "smj", "SE", "Latn", "", false));
			locales.push(new LocaleItem("smn-FI", "smn", "FI", "Latn", "", false));
			locales.push(new LocaleItem("sms-FI", "sms", "FI", "Latn", "", false));
			locales.push(new LocaleItem("sq-AL", "sq", "AL", "Latn", "", false));
			locales.push(new LocaleItem("sr-Cyrl-BA", "sr", "BA", "Cyrl", "", false));
			locales.push(new LocaleItem("sr-Cyrl-CS", "sr", "CS", "Cyrl", "", false));
			locales.push(new LocaleItem("sr-Cyrl-ME", "sr", "ME", "Cyrl", "", false));
			locales.push(new LocaleItem("sr-Cyrl-RS", "sr", "RS", "Cyrl", "", false));
			locales.push(new LocaleItem("sr-Latn-BA", "sr", "BA", "Latn", "", false));
			locales.push(new LocaleItem("sr-Latn-CS", "sr", "CS", "Latn", "", false));
			locales.push(new LocaleItem("sr-Latn-ME", "sr", "ME", "Latn", "", false));
			locales.push(new LocaleItem("sr-Latn-RS", "sr", "RS", "Latn", "", false));
			locales.push(new LocaleItem("sv-FI", "sv", "FI", "Latn", "", false));
			locales.push(new LocaleItem("sv-SE", "sv", "SE", "Latn", "", false));
			locales.push(new LocaleItem("sw-KE", "sw", "KE", "Latn", "", false));
			locales.push(new LocaleItem("syr-SY", "syr", "SY", "Syrc", "", true));
			locales.push(new LocaleItem("ta-IN", "ta", "IN", "Taml", "", false));
			locales.push(new LocaleItem("te-IN", "te", "IN", "Telu", "", false));
			locales.push(new LocaleItem("tg-Cyrl-TJ", "tg", "TJ", "Cyrl", "", false));
			locales.push(new LocaleItem("th-TH", "th", "TH", "Thai", "", false));
			locales.push(new LocaleItem("tk-TM", "tk", "TM", "Latn", "", false));
			locales.push(new LocaleItem("tn-ZA", "tn", "ZA", "Latn", "", false));
			locales.push(new LocaleItem("tr-TR", "tr", "TR", "Latn", "", false));
			locales.push(new LocaleItem("tt-RU", "tt", "RU", "Cyrl", "", false));
			locales.push(new LocaleItem("tzm-Latn-DZ", "tzm", "DZ", "Latn", "", false));
			locales.push(new LocaleItem("ug-CN", "ug", "CN", "Arab", "", true));
			locales.push(new LocaleItem("uk-UA", "uk", "UA", "Cyrl", "", false));
			locales.push(new LocaleItem("ur-PK", "ur", "PK", "Arab", "", true));
			locales.push(new LocaleItem("uz-Cyrl-UZ", "uz", "UZ", "Cyrl", "", false));
			locales.push(new LocaleItem("uz-Latn-UZ", "uz", "UZ", "Latn", "", false));
			locales.push(new LocaleItem("vi-VN", "vi", "VN", "Latn", "", false));
			locales.push(new LocaleItem("wo-SN", "wo", "SN", "Latn", "", false));
			locales.push(new LocaleItem("xh-ZA", "xh", "ZA", "Latn", "", false));
			locales.push(new LocaleItem("yo-NG", "yo", "NG", "Latn", "", false));
			locales.push(new LocaleItem("zh-CN", "zh", "CN", "Hans", "", false));
			locales.push(new LocaleItem("zh-HK", "zh", "HK", "Hant", "", false));
			locales.push(new LocaleItem("zh-MO", "zh", "MO", "Hant", "", false));
			locales.push(new LocaleItem("zh-SG", "zh", "SG", "Hans", "", false));
			locales.push(new LocaleItem("zh-TW", "zh", "TW", "Hant", "", false));
			locales.push(new LocaleItem("zu-ZA", "zu", "ZA", "Latn", "", false));
			
			
            for ( var i:int = 0; i < locales.length; i++ ) 
            {
				var locale:LocaleItem = locales[i];
				
                var locID:LocaleID = new LocaleID( locale.name as String );
				
				var valid:Boolean = true;
				
				if (locale.name != locID.name)
				{
					valid = false;
				}
				
				if (locale.language != locID.getLanguage())
				{
					valid = false;
				}
				
				if (locale.script != locID.getScript())
				{
					valid = false;
				}
				
				if (locale.region != locID.getRegion())
				{
					valid = false;
				}
				
				if (locale.variant != locID.getVariant())
				{
					valid = false;
				}
				
				if (locale.isRightToLeft != locID.isRightToLeft())
				{
					valid = false;
				}
				
                if (valid)
				{
					trace(i + ": Passed");
				}
				else
				{
					trace(i + ": Failed");
				}
            }
			
			var locID:LocaleID = new LocaleID("en-GB");
			var keysAndValues:Object = locID.getKeysAndValues();
			var key:String;
			for (key in keysAndValues)
			{
				trace("key: ", key + " value: " + keysAndValues[ key ]);
			}
			
			var localeNames:Array = [
				"zh-CH@collation=pinyin;calendar=chinese;currency=RMB",
				"zh-CH@collation=pinyin;calendar=chinese;currency=RMB;",
				"zh-CH@collation=",
				"zh-CH@collation",
				"zh-CH@",
				"zh-CH@=",
				"zh-CH@=bob",
				"zh-CH@collation=pinyin;calendar=chinese;currency=RMB;bob=bob"
			];
            
            for ( var i:int = 0; i < localeNames.length; i++ ) 
            {
                var locID:LocaleID = new LocaleID( localeNames[i] as String );
                				
                var keysAndValues:Object = locID.getKeysAndValues();
                var key:String;
                for (key in keysAndValues)
                {
                    trace("key: ", key + " value: " + keysAndValues[ key ]);
                }
                trace("");
            }
		}
	}
}98