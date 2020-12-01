package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.StageVideoAvailabilityReason;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (StageVideoAvailabilityReason.DRIVER_TOO_OLD == "driverTooOld")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (StageVideoAvailabilityReason.NO_ERROR == "noError")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		
			if (StageVideoAvailabilityReason.UNAVAILABLE == "unavailable")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (StageVideoAvailabilityReason.USER_DISABLED == "userDisabled")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (StageVideoAvailabilityReason.WMODE_INCOMPATIBLE == "wModeIncompatible")
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