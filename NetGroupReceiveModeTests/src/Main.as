package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.NetGroupReceiveMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (NetGroupReceiveMode.EXACT == "exact")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (NetGroupReceiveMode.NEAREST == "nearest")
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