package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.DateTimeNameStyle;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (DateTimeNameStyle.FULL == "full") {
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (DateTimeNameStyle.LONG_ABBREVIATION == "longAbbreviation") {
				trace("Passed");
			}
			else {
				trace("Failed");
			}
			
			if (DateTimeNameStyle.SHORT_ABBREVIATION == "shortAbbreviation")	{
				trace("Passed");
			}
			else {
				trace("Failed");
			}
		}
	}
}