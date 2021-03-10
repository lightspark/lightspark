package
{
	import flash.display.Sprite;
	import flash.events.Event;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (flash.display.StageQuality.BEST == "best")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.HIGH == "high")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.HIGH_16X16 == "16x16")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.HIGH_16X16_LINEAR == "16x16linear")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.HIGH_8X8 == "8x8")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.HIGH_8X8_LINEAR == "8x8linear")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.LOW == "low")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (flash.display.StageQuality.MEDIUM == "medium")
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