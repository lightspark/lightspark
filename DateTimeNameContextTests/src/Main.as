package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.DateTimeNameContext;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (DateTimeNameContext.FORMAT == "format")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (DateTimeNameContext.STANDALONE == "standalone")
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