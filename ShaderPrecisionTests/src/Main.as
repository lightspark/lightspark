package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.ShaderPrecision;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ShaderPrecision.FAST == "fast")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderPrecision.FULL == "full")
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