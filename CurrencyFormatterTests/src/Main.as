package
{
	import flash.display.Sprite;
	import flash.events.Event;
	
	import flash.globalization.CurrencyFormatter;
	import flash.globalization.CurrencyParseResult;
	import flash.globalization.LastOperationStatus;
	import flash.globalization.LocaleID;

	public class Main extends Sprite
	{
		public function Main():void
		{
			var cf:CurrencyFormatter = new CurrencyFormatter("en_US");
			cf.fractionalDigits = 2;
			trace("LocaleID requested=" + cf.requestedLocaleIDName 
				+ "; actual=" + cf.actualLocaleIDName);
			trace("Last Operation Status: " + cf.lastOperationStatus );
			
			var input = 1254.56;
			
			var result:String = cf.format(input);
			
			if (cf.lastOperationStatus == LastOperationStatus.NO_ERROR ) {
				
				trace(cf.format(input));
				trace(cf.format(input, true));
			}
			
			cf = new CurrencyFormatter("en_GB");
			cf.fractionalDigits = 2;
			trace("LocaleID requested=" + cf.requestedLocaleIDName 
				+ "; actual=" + cf.actualLocaleIDName);
			trace("Last Operation Status: " + cf.lastOperationStatus );
			
			var input = 12;
			
			var result:String = cf.format(input);
			
			if (cf.lastOperationStatus == LastOperationStatus.NO_ERROR ) {
				
				trace(cf.format(input));
				trace(cf.format(input, true));
				
			}
			
			cf = new CurrencyFormatter("en_GB");
			cf.fractionalDigits = 2;
			trace("LocaleID requested=" + cf.requestedLocaleIDName 
				+ "; actual=" + cf.actualLocaleIDName);
			trace("Last Operation Status: " + cf.lastOperationStatus );
			
			var input = 1.25;
			
			var result:String = cf.format(input);
			
			if (cf.lastOperationStatus == LastOperationStatus.NO_ERROR ) {
				
				trace(cf.format(input));
				trace(cf.format(input, true));
				
			}
			
			var currencyResult:CurrencyParseResult = new CurrencyParseResult(123, "£");
			if (currencyResult.currencyString == "£" &&  currencyResult.value == 123)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		}
	}
}
