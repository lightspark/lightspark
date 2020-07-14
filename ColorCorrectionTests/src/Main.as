package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.ColorCorrection;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ColorCorrection.DEFAULT == "default")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ColorCorrection.OFF == "off")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ColorCorrection.ON == "on")
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