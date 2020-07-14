package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.ColorCorrectionSupport;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (stage) init();
			else addEventListener(Event.ADDED_TO_STAGE, init);
		}
		
		private function init(e:Event = null):void 
		{
			removeEventListener(Event.ADDED_TO_STAGE, init);
			// entry point
			if (ColorCorrectionSupport.DEFAULT_OFF == "defaultOff")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ColorCorrectionSupport.DEFAULT_ON == "defaultOn")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ColorCorrectionSupport.UNSUPPORTED == "unsupported")
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