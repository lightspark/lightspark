package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.Collator;
	import flash.system.WorkerDomain;
	//import flash.globalization.LocaleID;
	import flash.globalization.CollatorMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	 
	public class Main extends Sprite 
	{
		
		
		public function Main() 
		{
			var list:Array = new Array();
			
			list.push(new Word("hello world!", "hello world!", 0, true, false, false)); // 1
			list.push(new Word("hello world!", "hello world!", 0, true, true, false));  // 2
			list.push(new Word("hello world!", "hello world!", 0, true, false, true));  // 3
			list.push(new Word("hello world!", "hello world!", 0, true, true, true));   // 4
			
			list.push(new Word("hello world!", "hello world@",-1, 0, false, false)); // 5
			list.push(new Word("hello world!", "hello world@", -1, 0, true, false)); // 6
			list.push(new Word("hello world!", "hello world@", 0, true, false, true));  // 7
			list.push(new Word("hello world!", "hello world@", 0, true, true, true));   // 8
			
			list.push(new Word("hello world!", "hello world@", -1, 0, false, false)); // 9
			list.push(new Word("hello world!", "hello world@", -1, 0, true, false)); // 10
			list.push(new Word("hello world!", "hello world@", 0, true, false, true)); // 11
			list.push(new Word("hello world!", "hello world@", 0, true, true, true)); // 12
			
			list.push(new Word("hello world!", "HELLO WORLD!", -1, 0, false, false)); // 13
			list.push(new Word("hello world!", "HELLO WORLD!", 0, true, true, false)); // 14
			list.push(new Word("hello world!", "HELLO WORLD!", -1, 0, false, true)); // 15
			list.push(new Word("hello world!", "HELLO WORLD!", 0, true, true, true)); // 16
			
			list.push(new Word("", "", 0, true, false, false)); // 17
			list.push(new Word("", "", 0, true, true, false));  // 18
			list.push(new Word("", "", 0, true, false, true));  // 19
			list.push(new Word("", "", 0, true, true, true));  // 20
			
			list.push(new Word(" ", " ", 0, true, false, false)); // 21
			list.push(new Word(" ", " ", 0, true, true, false)); // 22
			list.push(new Word(" ", " ", 0, true, false, true)); // 23
			list.push(new Word(" ", " ", 0, true, true, true)); // 24
			
			list.push(new Word("-", " ", -1, 0, false, false)); // 25
			list.push(new Word("-", " ", -1, 0, true, false)); // 26
			list.push(new Word("-", " ", 0, true, false, true)); // 27
			list.push(new Word("-", " ", 0, true, true, true)); // 28
			
			list.push(new Word("-", "", 1, 0, false, false)); // 29
			list.push(new Word("-", "", 1, 0, true, false)); // 30
			list.push(new Word("-", "", 1, 0, false, true)); // 31
			list.push(new Word("-", "", 1, 0, true, true)); // 32
			
			list.push(new Word("Ａｱ", "Ａｱ", 0, true, false, false)); // 33
			list.push(new Word("Ａｱ", "Ａｱ", 0, true, true, false)); // 34
			list.push(new Word("Ａｱ", "Ａｱ", 0, true, false, true)); // 35
			list.push(new Word("Ａｱ", "Ａｱ", 0, true, true, true)); // 36
			
			
			list.push(new Word("カナ", "カナ", 0, true, false, false)); // 37
			list.push(new Word("カナ", "カナ", 0, true, true, false)); // 38
			list.push(new Word("カナ", "カナ", 0, true, false, true)); // 39
			list.push(new Word("カナ", "カナ", 0, true, true, true)); // 40
			
			list.push(new Word("OBrian", "O'Brian", -1, 0, false, false)); // 41
			list.push(new Word("OBrian", "O'Brian", -1, 0, true, false)); // 42
			list.push(new Word("OBrian", "O'Brian", 0, true, false, true)); // 43
			list.push(new Word("OBrian", "O'Brian", 0, true, true, true)); // 44
				
			
			list.push(new Word("OBrian", "O Brian", 1, 0, false, false)); // 45
			list.push(new Word("OBrian", "O Brian", 1, 0, true, false)); // 46
			list.push(new Word("OBrian", "O Brian", 0, true, false, true)); // 47
			list.push(new Word("OBrian", "O Brian", 0, true, true, true)); // 48
			
			list.push(new Word("côte", "côte", 0, true, false, false)); // 49
			list.push(new Word("côte", "côte", 0, true, true, false)); // 50
			list.push(new Word("côte", "côte", 0, true, false, true)); // 51
			list.push(new Word("côte", "côte", 0, true, true, true)); // 52
			
			list.push(new Word("hello", "hello world!", -1, false, false, false)); // 53
			list.push(new Word("hello", "hello world!", -1, false, true, false)); // 54
			list.push(new Word("hello", "hello world!", -1, false, false, true)); // 55
			list.push(new Word("hello", "hello world!", -1, false, true, true)); // 56
			
			list.push(new Word("hello world!", "hello", 1, false, false, false)); // 57
			list.push(new Word("hello world!", "hello", 1, false, true, false)); // 58
			list.push(new Word("hello world!", "hello", 1, false, false, true)); // 59
			list.push(new Word("hello world!", "hello", 1, false, true, true)); // 60
			
			var i = 1;
			
			for each (var item in list){
				var collator:Collator = new Collator("LocaleID.DEFAULT");
				collator.ignoreCase = item.ignoreCase;
				collator.ignoreSymbols = item.ignoreSymbols;
				var result = collator.compare(item.firstWord, item.secondWord)
				if (result == item.expectedResult1)
				{
					trace("Passed");
				}
				else
				{
					//trace("Failed");
					trace(i + " " + item.firstWord + ":" + collator.compare(item.firstWord, item.secondWord));
				}
				i++;
			}
			i = 1;
			for each (var item in list){
				var collator:Collator = new Collator("LocaleID.DEFAULT");
				collator.ignoreCase = item.ignoreCase;
				collator.ignoreSymbols = item.ignoreSymbols;
				var result = collator.equals(item.firstWord, item.secondWord);
				if (result == item.expectedResult2)
				{
					trace("Passed");
				}
				else
				{
					//trace("Failed");
					trace(i + " " + item.firstWord + ":" + result);
				}
				i++;
			}
        }		
	}
}