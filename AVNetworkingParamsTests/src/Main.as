package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.AVNetworkingParams;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var avNP1:AVNetworkingParams = new AVNetworkingParams(true, true, true);
			
			if (avNP1.forceNativeNetworking && avNP1.readSetCookieHeader && avNP1.useCookieHeaderForAllRequests)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			var avNP2:AVNetworkingParams = new AVNetworkingParams(false, false, false);
			if (!avNP2.forceNativeNetworking && !avNP2.readSetCookieHeader && !avNP2.useCookieHeaderForAllRequests)
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