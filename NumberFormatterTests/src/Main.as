package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.NumberFormatter;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			runDefaultTests();
			runDecimalSeporatorTests();
			runDigitsTypeTests();
			runFractionalDigitsTests();
			runGroupingPattern();
			runGroupSeparatorTests();
			runLeadingZeroTests();
			runNegativeNumberFormatTests();
			runNegativeSymbolTests();
			runTrailingZerosTests();
			runUseGrouping();
		}
		
		public function runDefaultTests()
		{
			trace("Default Tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1.1"));
			parseList.push(new TestData("1.9", "1.9"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "NaN"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			/*for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1",1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runDecimalSeporatorTests()
		{
			trace("Decimal Seporator Tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.decimalSeparator = "#";
			
			// Format int tests
			if (nf.formatInt(1) == "1#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999#00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1#00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1#00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1#10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1#90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1#50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1#40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0#00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000#00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999#00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5#00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5#00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5#00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5#00"));
			formatNumberList.push(new TestData3(new Number(+1), "1#00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", NaN));
			parseNumberList.push(new TestData2("1.9", NaN));
			parseNumberList.push(new TestData2("-1",-1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999.00));
			parseNumberList.push(new TestData2("1111111", 1111111.00));
			parseNumberList.push(new TestData2(" 5.0", NaN));
			parseNumberList.push(new TestData2("5.0 ", NaN));
			parseNumberList.push(new TestData2("5.0", NaN));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", NaN));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runDigitsTypeTests()
		{
			/*trace("Digits Type Tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.digitsType = 1;
			
			// Format int tests
			runIntFormatTests(nf);
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1"));
			formatNumberList.push(new TestData3(new Number(-1), "1"));
			formatNumberList.push(new TestData3(new Number(1.1), "1"));
			formatNumberList.push(new TestData3(new Number(1.9), "1"));
			formatNumberList.push(new TestData3(new Number(1.5), "1"));
			formatNumberList.push(new TestData3(new Number(1.4), "1"));
			formatNumberList.push(new TestData3(new Number(0.001), "1"));
			formatNumberList.push(new TestData3(new Number(100000000), "1"));
			formatNumberList.push(new TestData3(new Number(909999999), "1"));
			formatNumberList.push(new TestData3(new Number("5,000"), "1"));
			formatNumberList.push(new TestData3(new Number(5.0), "1"));
			formatNumberList.push(new TestData3(new Number(5.0), "1"));
			formatNumberList.push(new TestData3(new Number(5.0), "1"));
			formatNumberList.push(new TestData3(new Number(5.0), "1"));
			formatNumberList.push(new TestData3(new Number(+1), "1"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			/*for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", new Number(0)));
			parseNumberList.push(new TestData2("1.1", new Number(1)));
			parseNumberList.push(new TestData2("1.9", new Number(1)));
			parseNumberList.push(new TestData2("-1", new Number(1)));
			parseNumberList.push(new TestData2("+1", new Number(1)));
			parseNumberList.push(new TestData2("9999999", new Number(1)));
			parseNumberList.push(new TestData2("1111111", new Number(1)));
			parseNumberList.push(new TestData2(" 5.0", new Number(1)));
			parseNumberList.push(new TestData2("5.0 ", new Number(1)));
			parseNumberList.push(new TestData2("5.0", new Number(1)));
			parseNumberList.push(new TestData2("5,000", new Number(1)));
			parseNumberList.push(new TestData2("123,567,89,0.254", new Number(1)));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
		}
		
		public function runFractionalDigitsTests()
		{
			trace("Fractional Digits Tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.fractionalDigits = 3;
			
			// Format int tests
			if (nf.formatInt(1) == "1.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.000")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.000"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.000"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.100"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.900"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.500"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.400"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.001"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000.000"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999.000"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.000"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.000"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.000"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.000"));
			formatNumberList.push(new TestData3(new Number(+1), "1.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "123567890.254"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runGroupingPattern()
		{
			trace("Group Pattern tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.groupingPattern = "3;2;*";
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "10,00,00,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "90,99,99,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1.0));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runGroupSeparatorTests()
		{
			trace("Group Seperator Tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.groupingSeparator = "#";
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9#999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9#999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9#999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100#000#000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909#999#999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", NaN));
			parseNumberList.push(new TestData2("123,567,89,0.254", NaN));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runLeadingZeroTests()
		{
			trace("Leading Zero tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.leadingZero = true;
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			// Parse tests
			/*var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runNegativeNumberFormatTests()
		{
			trace("Negative Number Format tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.negativeNumberFormat = 2;
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "- 1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "- 9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "- 1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runNegativeSymbolTests()
		{
			trace("Negative Symbol tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.negativeSymbol = "#";
			
			// Format int tests
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000.00), "100,000,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999.00), "909,999,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runTrailingZerosTests()
		{
			trace("Trailing Zeros tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.trailingZeros = true;
			
			// Format int tests
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9,999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100,000,000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909,999,999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(5.0), "5.00"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1"));
			parseList.push(new TestData("-1", "1"));
			parseList.push(new TestData("1.1", "1"));
			parseList.push(new TestData("1.9", "1"));
			parseList.push(new TestData("1.5", "1"));
			parseList.push(new TestData("1.4", "1"));
			parseList.push(new TestData("0.001", "1"));
			parseList.push(new TestData("100000000", "1"));
			parseList.push(new TestData("909999999", "1"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "1"));
			parseList.push(new TestData("5.0 ", "1"));
			parseList.push(new TestData(" 5.0", "1"));
			parseList.push(new TestData(" 5.0 ", "1"));
			parseList.push(new TestData("+1", "1"));
			parseList.push(new TestData("123,567,89,0.254", "1"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			parseNumberList.push(new TestData2("+1", NaN));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		public function runUseGrouping()
		{
			trace("Use Grouping tests");
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.useGrouping = false;
			
			// Format int tests
			runIntFormatTests(nf);
			
			// Format number tests
			var formatNumberList:Array = new Array();
			formatNumberList.push(new TestData3(new Number(1), "1.00"));
			formatNumberList.push(new TestData3(new Number(-1), "-1.00"));
			formatNumberList.push(new TestData3(new Number(1.1), "1.10"));
			formatNumberList.push(new TestData3(new Number(1.9), "1.90"));
			formatNumberList.push(new TestData3(new Number(1.5), "1.50"));
			formatNumberList.push(new TestData3(new Number(1.4), "1.40"));
			formatNumberList.push(new TestData3(new Number(0.001), "0.00"));
			formatNumberList.push(new TestData3(new Number(100000000), "100000000.00"));
			formatNumberList.push(new TestData3(new Number(909999999), "909999999.00"));
			formatNumberList.push(new TestData3(new Number("5,000"), "NaN"));
			formatNumberList.push(new TestData3(new Number(+1), "1.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			/*// Parse tests
			var parseList:Array = new Array();
			parseList.push(new TestData("1", "1.00"));
			parseList.push(new TestData("-1", "-1.00"));
			parseList.push(new TestData("1.1", "1.10"));
			parseList.push(new TestData("1.9", "1.90"));
			parseList.push(new TestData("1.5", "1.50"));
			parseList.push(new TestData("1.4", "1.40"));
			parseList.push(new TestData("0.001", "0.00"));
			parseList.push(new TestData("100000000", "100000000.00"));
			parseList.push(new TestData("909999999", "909999999.00"));
			parseList.push(new TestData("5,000", "1"));
			parseList.push(new TestData("5.0", "5.0"));
			parseList.push(new TestData("5.0 ", "5.0"));
			parseList.push(new TestData(" 5.0", "5.0"));
			parseList.push(new TestData(" 5.0 ", "5.00"));
			parseList.push(new TestData("+1", "1.00"));
			parseList.push(new TestData("123,567,89,0.254", "1.00"));
			
			for each (var item in parseList)
			{
				var result = nf.parse(item.value).value;
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}*/
			
			// Parse number tests
			var parseNumberList:Array = new Array();
			parseNumberList.push(new TestData2("1", 1));
			parseNumberList.push(new TestData2("1.1", 1.1));
			parseNumberList.push(new TestData2("1.9", 1.9));
			parseNumberList.push(new TestData2("-1", -1));
			//parseNumberList.push(new TestData2("+1", new Number(1)));
			parseNumberList.push(new TestData2("9999999", 9999999));
			parseNumberList.push(new TestData2("1111111", 1111111));
			parseNumberList.push(new TestData2(" 5.0", 5));
			parseNumberList.push(new TestData2("5.0 ", 5));
			parseNumberList.push(new TestData2("5.0", 5));
			parseNumberList.push(new TestData2("5,000", 5000));
			parseNumberList.push(new TestData2("123,567,89,0.254", 123567890.254));
			
			for each (var item in parseNumberList)
			{
				var result = nf.parseNumber(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
		}
		
		private function runIntFormatTests(nf:NumberFormatter)
		{
			// Format int tests
			
			if (nf.formatInt(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(1));
			}
			
			if (nf.formatInt(9999) == "9999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(9999));
			}
			
			if (nf.formatInt(-1) == "-1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-1));
			}
			
			if (nf.formatInt(-9999) == "-9999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatInt(-9999));
			}
			
			// Format UInt tests
			
			if (nf.formatUint(1) == "1.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(1));
			}
			
			if (nf.formatUint(9999) == "9999.00")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed: " + nf.formatUint(9999));
			}
		}
	}
}