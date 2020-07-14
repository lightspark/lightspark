package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.StringTools;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	 
	public class Main extends Sprite 
	{
		
		
		public function Main() 
		{
			var localeName:String= "en_US";
			
			var strTool:StringTools = new StringTools(localeName);
			
            var turkishStr:String = "iI Ä±Ä°";
            var greekStr:String = "Î£Ï‚ÏƒÎ²Î°Î�Î£";
            var germanStr:String = "ÃŸ";
			var englishStr:String = "Hello";
			
			trace("Got:" + strTool.toUpperCase(turkishStr) + ", Expected: " + "II Ä±Ä");
			trace("Got:" + strTool.toUpperCase(greekStr) + ", Expected: " + "Î£Ï‚ÏƒÎ²Î°Î�Î£");
			trace("Got:" + strTool.toUpperCase(germanStr) + ", Expected: " + "ÃŸ");
			trace("Got:" + strTool.toUpperCase(englishStr) + ", Expected: " + "HELLO");
			
			trace("Got:" + strTool.toLowerCase(turkishStr) + ", Expected: " + "ii ä±ä");
			trace("Got:" + strTool.toLowerCase(greekStr) + ", Expected: " + "î£ï‚ïƒî²î°î�î£, ");
			trace("Got:" + strTool.toLowerCase(germanStr) + ", Expected: " + "ãÿ");		
			trace("Got:" + strTool.toLowerCase(englishStr) + ", Expected: " + "hello");
		}
	}
}