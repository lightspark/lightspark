package
{
	import adobe.utils.CustomActions;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.Security;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (Security.APPLICATION == undefined) // "application" only in AIR
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Security.LOCAL_TRUSTED == "localTrusted")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Security.LOCAL_WITH_FILE == "localWithFile")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Security.LOCAL_WITH_NETWORK == "localWithNetwork")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Security.REMOTE == "remote")
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