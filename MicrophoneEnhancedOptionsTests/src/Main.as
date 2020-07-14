package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.MicrophoneEnhancedOptions;
	import flash.media.MicrophoneEnhancedMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var microphoneOptions = new MicrophoneEnhancedOptions();
			microphoneOptions.echoPath = 256;
			microphoneOptions.isVoiceDetected = 1 // -1, 0 or 1
			microphoneOptions.mode = MicrophoneEnhancedMode.FULL_DUPLEX;
			microphoneOptions.nonLinearProcessing = true;
			
			if (microphoneOptions.echoPath == 256)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (microphoneOptions.isVoiceDetected == 1)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (microphoneOptions.mode == MicrophoneEnhancedMode.FULL_DUPLEX)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (microphoneOptions.nonLinearProcessing == true)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		}
		
		private function init(e:Event = null):void 
		{
			removeEventListener(Event.ADDED_TO_STAGE, init);
			// entry point
		}
		
	}
	
}