package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.H264Profile;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (H264Profile.BASELINE == "baseline")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (H264Profile.MAIN == "main")
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