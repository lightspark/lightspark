package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.NetGroupReplicationStrategy;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (NetGroupReplicationStrategy.LOWEST_FIRST == "lowestFirst")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (NetGroupReplicationStrategy.RAREST_FIRST == "rarestFirst")
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