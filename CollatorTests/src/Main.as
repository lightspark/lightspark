package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.Collator;
	//import flash.globalization.LocaleID;
	import flash.globalization.CollatorMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		public var col:Collator;
        public var testMatchData:Array = ["cote", "Cote", "cÃ´te", "cotÃ©"];
        public var wordToMatch:String = "Cote";
        		
		public function Main() 
		{
			col = new Collator( "en-US", CollatorMode.MATCHING );

			/*trace("Normal comparison");
            if (col.equals("hello world!", "hello world!"))
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			
			trace("Ignore case 1");
			col.ignoreSymbols = true;
			if (col.equals("hello world!", "hello world@"))
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreSymbols = false;
			
			trace("Ignore case 2");
			col.ignoreSymbols = true;
			if (col.equals("hello world!", "HELLO WORLD!"))
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreSymbols = false;
			
			trace("Ignore case 3");
			col.ignoreCase = true;
			if (col.equals("hello world!", "HELLO WORLD!"))
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreCase = false;
			
			
			
			trace("Ignore symbols 1");
			col.ignoreSymbols = true;
			if (col.compare("hello world!", "hello world@") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreSymbols = false;
			
			trace("Ignore case 4");
			col.ignoreCase = true;
			if (col.compare("hello world!", "HELLO WORLD!") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreCase = false;
			
			trace("Ignore empty string");
			if (col.compare("", "") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			
			trace("Ignore character width 1");
			// Ignore character width
			col.ignoreCharacterWidth = true;
			if (col.compare("Ａｱ", "Aア") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreCharacterWidth = false;
			
			trace("Ignore character width 2");
			// Check against character width
			if (col.compare("Ａｱ", "Aア") != 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}*/
			
			/*trace("Ignore Kana type 1");
			// Check against Kana type
			col.ignoreKanaType = false
			if (col.compare("カナ", "かな") != 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			trace("Ignore Kana type 2");
			col.ignoreKanaType = true;
			// Check against Kana type
			if (col.compare("カナ", "かな") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreKanaType = false;*/
			
			trace("Ignore symbols 1");
			// Ignore symbols
			col.ignoreSymbols = true;
			// Check against Kana type
			if (col.compare("OBrian", "O'Brian") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			trace("Ignore symbols 2");
			if (col.compare("OBrian", "O Brian") == 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreSymbols = false;
			trace("Ignore symbols 3");
			if (col.compare("OBrian", "O'Brian") != 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			trace("Ignore symbols 4");
			if (col.compare("OBrian", "O Brian") != 0)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			
			
			
			
			
			
			
			// Ignore Diacritics
			
			/*trace("Ignore diacritics 1");
			col.ignoreDiacritics = true;
			if (col.compare("côte", "coté") == 0) // French
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			col.ignoreDiacritics = false;
			
			trace("Ignore diacritics 2");			
			if (col.compare("côte", "coté") != 0) // French
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}*/
        }		
	}
}