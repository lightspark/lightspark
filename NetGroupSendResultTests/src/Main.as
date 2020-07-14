package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.NetGroupSendResult;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (NetGroupSendResult.ERROR == "error")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (NetGroupSendResult.NO_ROUTE == "no route")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (NetGroupSendResult.SENT == "sent")
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