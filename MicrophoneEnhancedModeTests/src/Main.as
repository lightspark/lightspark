package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.MicrophoneEnhancedMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (MicrophoneEnhancedMode.FULL_DUPLEX == "fullDuplex")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MicrophoneEnhancedMode.HALF_DUPLEX == "halfDuplex")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MicrophoneEnhancedMode.HEADSET == "headset")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MicrophoneEnhancedMode.OFF == "off")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MicrophoneEnhancedMode.SPEAKER_MUTE == "speakerMute")
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