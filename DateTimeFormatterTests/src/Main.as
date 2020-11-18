package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.DateTimeFormatter;
	import flash.globalization.LocaleID;
	import flash.globalization.DateTimeStyle;
	import flash.utils.Dictionary;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var locale = "en-US";
			
			var date:Date = new Date(2020, 10, 20, 1, 2, 3);
			var date2:Date = new Date(1999, 1, 1, 23, 59, 59);
			
			trace(date);
			trace(date2);

			var longDf:DateTimeFormatter = new DateTimeFormatter(locale, DateTimeStyle.LONG, DateTimeStyle.LONG);
			var mediumDf:DateTimeFormatter = new DateTimeFormatter(locale, DateTimeStyle.MEDIUM, DateTimeStyle.MEDIUM);
			var shortDf:DateTimeFormatter = new DateTimeFormatter(locale, DateTimeStyle.SHORT, DateTimeStyle.SHORT);

			var longDate:String = longDf.format(date);
			var mediumDate:String = mediumDf.format(date);
			var shortDate:String = shortDf.format(date);
			
			var expectedDate = "Friday, November 20, 2020 1:02:03 AM";
			
			if (longDate == expectedDate)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
				trace("Got: " + longDate + ", expected: " + expectedDate);
			}

			expectedDate = "Friday, November 20, 2020 1:02:03 AM";
			if (mediumDate == expectedDate)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
				trace("Got: " + mediumDate + ", expected: " + expectedDate);
			}
			
			expectedDate = "11/20/2020 1:02 AM";
			if (shortDate == expectedDate)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
				trace("Got: " + shortDate + ", expected: " + expectedDate);
			}
			
			var dtf:DateTimeFormatter = new DateTimeFormatter(locale);
			dtf.setDateTimePattern("yyyy-MM-dd 'at' hh:mm:ssa");
			var customDate = dtf.format(date);
			expectedDate = "2020-11-20 at 01:02:03AM";
			if (customDate == "2020-11-20 at 01:02:03AM")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
				trace("Got: " + customDate + ", expected: " + expectedDate);
			}
			
			if (longDf.getDateStyle() == "long")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (mediumDf.getDateStyle() == "medium")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			if (shortDf.getDateStyle() == "short")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (longDf.getTimeStyle() == "long")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			if (mediumDf.getTimeStyle() == "medium")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			if (shortDf.getTimeStyle() == "short")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			var tests2:Dictionary = new Dictionary();
			tests2["G"] = "A.D.";
			tests2["GG"] = "A.D.";
			tests2["GGG"] = "A.D.";
			tests2["GGGG"] = "A.D.";
			tests2["GGGGG"] = "A.D.";
			tests2["y"] = "2020";
			tests2["yy"] = "20";
			tests2["yyyy"] = "2020";
			tests2["yyyyy"] = "2020";
			tests2["M"] = "11";
			tests2["MM"] = "11";
			tests2["MMM"] = "Nov";
			tests2["MMMM"] = "November";
			tests2["MMMMM"] = "November";
			tests2["d"] = "20";
			tests2["dd"] = "20";
			tests2["E"] = "Fri";
			tests2["EE"] = "Fri";
			tests2["EEEE"] = "Friday";
			tests2["EEEEE"] = "Friday";
			tests2["Q"] = "";
			tests2["QQ"] = "";
			tests2["QQQ"] = "";
			tests2["QQQQ"] = "";
			tests2["w"] = "";
			tests2["ww"] = "";
			tests2["W"] = "";
			tests2["D"] = "";
			tests2["DD"] = "";
			tests2["DDD"] = "";
			tests2["F"] = "";
			tests2["a"] = "AM";
			//tests2["p"] = "PM";
			tests2["h"] = "1";
			tests2["hh"] = "01";
			tests2["hhh"] = "01";
			tests2["H"] = "1";
			tests2["HH"] = "01";
			tests2["HHH"] = "01";
			tests2["K"] = "1";
			tests2["KK"] = "01";
			tests2["k"] = "1";
			tests2["kk"] = "01";
			tests2["m"] = "2";
			tests2["mm"] = "02";
			tests2["s"] = "3";
			tests2["ss"] = "03";
			tests2["S"] = "";
			tests2["SS"] = "";
			tests2["SSS"] = "";
			tests2["SSSS"] = "";
			tests2["SSSSS"] = "";
			tests2["z"] = "";
			tests2["zz"] = "";
			tests2["zzz"] = "";
			tests2["zzzz"] = "";
			tests2["Z"] = "";
			tests2["ZZ"] = "";
			tests2["ZZZ"] = "";
			tests2["ZZZZ"] = "";
			tests2["v"] = "";
			tests2["vvvv"] = "";
			
			for (var k:String in tests2) {
				var value:String = tests2[k];
				var key:String = k;
				
				var dtf:DateTimeFormatter = new DateTimeFormatter(locale);
				dtf.setDateTimePattern(key);
				
				if (dtf.format(date) == value && dtf.lastOperationStatus == "noError")
				{
					trace("Passed");
				}
				else
				{
					trace("Failed");
					trace(key + ": Got" + ": " + dtf.format(date) + " expected: " + value);
					trace("");
				}
			}
			
			var tests:Dictionary = new Dictionary();
			tests["G"] = "A.D.";
			tests["GG"] = "A.D.";
			tests["GGG"] = "A.D.";
			tests["GGGG"] = "A.D.";
			tests["GGGGG"] = "A.D.";
			tests["y"] = "1999";
			tests["yy"] = "99";
			tests["yyyy"] = "1999";
			tests["yyyyy"] = "1999";
			tests["M"] = "2";
			tests["MM"] = "02";
			tests["MMM"] = "Feb";
			tests["MMMM"] = "February";
			tests["MMMMM"] = "February";
			tests["d"] = "1";
			tests["dd"] = "01";
			tests["E"] = "Mon";
			tests["EE"] = "Mon";
			tests["EEEE"] = "Monday";
			tests["EEEEE"] = "Monday";
			tests["Q"] = "";
			tests["QQ"] = "";
			tests["QQQ"] = "";
			tests["QQQQ"] = "";
			tests["w"] = "";
			tests["ww"] = "";
			tests["W"] = "";
			tests["D"] = "";
			tests["DD"] = "";
			tests["DDD"] = "";
			tests["F"] = "";
			tests["a"] = "PM";
			//tests["p"] = "PM";
			tests["h"] = "11";
			tests["hh"] = "11";
			tests["hhh"] = "11";
			tests["H"] = "23";
			tests["HH"] = "23";
			tests["HHH"] = "23";
			tests["K"] = "11";
			tests["KK"] = "11";
			tests["k"] = "23";
			tests["kk"] = "23";
			tests["m"] = "59";
			tests["mm"] = "59";
			tests["s"] = "59";
			tests["ss"] = "59";
			tests["S"] = "";
			tests["SS"] = "";
			tests["SSS"] = "";
			tests["SSSS"] = "";
			tests["SSSSS"] = "";
			tests["z"] = "";
			tests["zz"] = "";
			tests["zzz"] = "";
			tests["zzzz"] = "";
			tests["Z"] = "";
			tests["ZZ"] = "";
			tests["ZZZ"] = "";
			tests["ZZZZ"] = "";
			tests["v"] = "";
			tests["vvvv"] = "";
			tests[""] = "";
			//tests[" "] = "";
			tests["-"] = "-";
			tests["/"] = "/";
			//tests[":"] = ":";
			
			
			for (var k:String in tests) {
				var value:String = tests[k];
				var key:String = k;
				
				var dtf:DateTimeFormatter = new DateTimeFormatter(locale);
				dtf.setDateTimePattern(key);
				
				if (dtf.format(date2) == value && dtf.lastOperationStatus == "noError")
				{
					trace("Passed");
				}
				else
				{
					trace("Failed");
					trace(key + ": Got" + ": " + dtf.format(date2) + " expected: " + value);
					trace("");
				}
			}
		}	
	}
}