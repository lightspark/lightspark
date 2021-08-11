package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.HTTPStatusEvent;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (HTTPStatusEvent.HTTP_RESPONSE_STATUS == undefined) // Not "httpResponseStatus" as stated in documentation
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (HTTPStatusEvent.HTTP_STATUS == "httpStatus")
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