package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.FullScreenEvent;
	
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (FullScreenEvent.FULL_SCREEN == "fullScreen")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (FullScreenEvent.FULL_SCREEN_INTERACTIVE_ACCEPTED == "fullScreenInteractiveAccepted")
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