package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.MessageChannelState;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (MessageChannelState.CLOSED == "closed")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MessageChannelState.CLOSING == "closing")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (MessageChannelState.OPEN == "open")
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