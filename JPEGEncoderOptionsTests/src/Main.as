package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.JPEGEncoderOptions;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var jpegOptions = new JPEGEncoderOptions(70);
			if (jpegOptions.quality == 70)
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