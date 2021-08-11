package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.TextEvent;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (TextEvent.LINK == "link")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			if (TextEvent.TEXT_INPUT == "textInput")
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