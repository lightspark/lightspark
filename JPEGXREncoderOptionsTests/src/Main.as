package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.JPEGXREncoderOptions;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var jpegOptions = new JPEGXREncoderOptions(19, "auto", 2);
			if (jpegOptions.colorSpace == "auto")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (jpegOptions.quantization == 19)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (jpegOptions.trimFlexBits == 2)
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