package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.NetGroupSendMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (NetGroupSendMode.NEXT_DECREASING == "nextDecreasing")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (NetGroupSendMode.NEXT_INCREASING == "nextIncreasing")
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