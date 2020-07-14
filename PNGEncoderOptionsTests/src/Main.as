package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.PNGEncoderOptions;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var pngOptions = new PNGEncoderOptions(true);
			if (pngOptions.fastCompression)
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