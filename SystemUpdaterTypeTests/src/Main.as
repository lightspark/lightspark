package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.SystemUpdaterType;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (SystemUpdaterType.DRM == "drm")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SystemUpdaterType.SYSTEM == "system")
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