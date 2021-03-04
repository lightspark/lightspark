package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.NumberFormatter;
	import flash.globalization.NumberParseResult;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
	
		public function Main() 
		{
			runDefaultTests();
			runParse();
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4(-1, "-1.00"));
			formatIntList.push(new TestData4(-9999, "-9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1#00"));
			formatIntList.push(new TestData4(9999, "9,999#00"));
			formatIntList.push(new TestData4(-1, "-1#00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(-9999, "-9,999#00"));
			formatUIntList.push(new TestData5(1, "1#00"));
			formatUIntList.push(new TestData5(9999, "9,999#00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(-9999, "-9,999#00"));
			formatIntList.push(new TestData4(1, "9,999.000"));
			formatIntList.push(new TestData4(9999, "9,999#00"));
			formatIntList.push(new TestData4( -9999, "-9,999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.000"));
			formatUIntList.push(new TestData5(9999, "9999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var nf:NumberFormatter = new NumberFormatter("en_US");
			nf.groupingPattern = "3;2;*";
			
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.00"));
			formatIntList.push(new TestData4( -9999, "-9,999.000"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
			
			var formatIntList:Array = new Array();
			formatIntList.push(new TestData4(1, "1.00"));
			formatIntList.push(new TestData4(9999, "9,999.00"));
			formatIntList.push(new TestData4(-1, "-1.00"));
			formatIntList.push(new TestData4(-9999, "-9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatInt(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
			}
			
			var formatUIntList:Array = new Array();
			formatUIntList.push(new TestData5(1, "1.00"));
			formatUIntList.push(new TestData5(9999, "9,999.00"));
			
			for each (var item in formatNumberList)
			{
				var result = nf.formatUint(item.value);
				if (result == item.expected)
				{
					trace("Passed");
				}
				else
				{
					trace("Failed. Got: " + result + ", expected: " + item.expected);
				}
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
		
		private function runParse()
		{
			trace("Parse tests");
			var list:Array = new Array();
			list.push(new ParseResult(1, "(123)", 0, -123, 0, 5));
			list.push(new ParseResult(2, "(123)", 1, 123, 1, 4));
			list.push(new ParseResult(3, "(123)", 2, 123, 1, 4));
			list.push(new ParseResult(4, "(123)", 3, 123, 1, 4));
			list.push(new ParseResult(5, "(123)", 4, 123, 1, 4));
			
			list.push(new ParseResult(6, "( 123 )", 0, -123, 0, 7));
			list.push(new ParseResult(7, "( 123 )", 1, 123, 2, 5));
			list.push(new ParseResult(8, "( 123 )", 2, 123, 2, 5));
			list.push(new ParseResult(9, "( 123 )", 3, 123, 2, 5));
			list.push(new ParseResult(10, "( 123 )", 4, 123, 2, 5));
			
			list.push(new ParseResult(11, "-123", 0, 123, 1, 4));
			list.push(new ParseResult(12, "-123", 1, -123, 0, 4));
			list.push(new ParseResult(13, "-123", 2, -123, 0, 4));
			list.push(new ParseResult(14, "-123", 3, 123, 1, 4));
			list.push(new ParseResult(15, "-123", 4, 123, 1, 4));
			
			list.push(new ParseResult(16, "- 123", 0, 123, 2, 5));
			list.push(new ParseResult(17, "- 123", 1, -123, 0, 5));
			list.push(new ParseResult(18, "- 123", 2, -123, 0, 5));
			list.push(new ParseResult(19, "- 123", 3, 123, 2, 5));
			list.push(new ParseResult(20, "- 123", 4, 123, 2, 5));
			
			list.push(new ParseResult(21, "123-",  0, 123, 0, 3));
			list.push(new ParseResult(22, "123-",  1, 123, 0, 3));
			list.push(new ParseResult(23, "123-",  2, 123, 0, 3));
			list.push(new ParseResult(24, "123-",  3, -123, 0, 4));
			list.push(new ParseResult(25, "123-",  4, -123, 0, 4));
			
			list.push(new ParseResult(26, "123 -", 0, 123, 0, 3));
			list.push(new ParseResult(27, "123 -", 1, 123, 0, 3));
			list.push(new ParseResult(28, "123 -", 2, 123, 0, 3));
			list.push(new ParseResult(29, "123 -", 3, -123, 0, 5));
			list.push(new ParseResult(30, "123 -", 4, -123, 0, 5));
			
			list.push(new ParseResult(31, "1,56 mètre", 1, 156, 0, 4));
			list.push(new ParseResult(32, "1,56 meter", 1, 156, 0, 4));
			
			list.push(new ParseResult(33, "word 1000 word", 1, 1000, 5, 9));
			list.push(new ParseResult(34, "word 1000", 1, 1000, 5, 9));
			list.push(new ParseResult(35, "1000 word", 1, 1000, 0, 4));
			list.push(new ParseResult(36, " word ", 1, NaN, 2147483647, 2147483647));
			
			
			// TODO
			list.push(new ParseResult(37, "word 1000 word", 1, 1000, 5, 9));
			list.push(new ParseResult(38, "word 1000", 1, 1000, 5, 9));
			list.push(new ParseResult(39, "1000 word", 1, 1000, 0, 4));
			list.push(new ParseResult(40, " word ", 1, NaN, 2147483647, 2147483647));
			
			
			list.push(new ParseResult(41, "word 1000 word", 1, 1000, 5, 9));
			list.push(new ParseResult(42, "word 1000", 1, 1000, 5, 9));
			list.push(new ParseResult(43, "1000 word", 1, 1000, 0, 4));
			list.push(new ParseResult(44, " word ", 1, NaN, 2147483647, 2147483647));
			
			
			
			
			for each (var item:ParseResult in list)
			{
				trace("==================")
				
				var nf:NumberFormatter = new NumberFormatter("en_US");
				nf.negativeNumberFormat = item.negativeNumberFormat;
				
				var result:NumberParseResult = nf.parse(item.input);
				
				if (result.value == item.expectedResult)// && result.startIndex == item.expectedStartIndex && result.endIndex == item.expectedEndIndex)
				{
					trace("Passed");
				}
				else
				{
					trace("Index: " + item.index);
					trace("Value to parse: \"" + item.input + "\"");
					trace("Failed: value: " + result.value + ", startIndex: " + result.startIndex + ", endIndex: " + result.endIndex);
					trace("Expected: value: " + item.expectedResult + ", startIndex: " + item.expectedStartIndex + ", endIndex: " + item.expectedEndIndex);
					trace("Negative number value: " + item.negativeNumberFormat);
				}
				trace("==================")
			}
		}
	}
}