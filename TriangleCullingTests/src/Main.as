package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.TriangleCulling;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (TriangleCulling.NEGATIVE == "negative")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (TriangleCulling.NONE == "none")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (TriangleCulling.POSITIVE == "positive")
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