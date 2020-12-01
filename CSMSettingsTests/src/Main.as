package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.text.CSMSettings;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var csmSettings:CSMSettings = new CSMSettings(3, 4, 5);
			
			if (csmSettings.fontSize == 3)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (csmSettings.insideCutoff == 4)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (csmSettings.outsideCutoff == 5)
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