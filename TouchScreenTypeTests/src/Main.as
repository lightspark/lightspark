package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.TouchscreenType;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (TouchscreenType.FINGER == "finger")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (TouchscreenType.NONE == "none")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (TouchscreenType.STYLUS == "stylus")
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