package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.AVTagData;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var avTagData = new AVTagData("data", 0);
			
			if (avTagData.data == "data")
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