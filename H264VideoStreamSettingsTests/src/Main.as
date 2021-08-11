package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.H264VideoStreamSettings;
	import flash.media.H264Profile;
	import flash.media.H264Level;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var settings = new H264VideoStreamSettings();
			
			if (settings.codec == "H264Avc")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (settings.level ==  H264Level.LEVEL_2_1)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (settings.profile == H264Profile.BASELINE)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			settings.setProfileLevel(H264Profile.MAIN, H264Level.LEVEL_5_1);
			if (settings.level ==  H264Level.LEVEL_5_1)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (settings.profile == H264Profile.MAIN)
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