package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.CollatorMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (CollatorMode.MATCHING == "matching") {
				trace("Passed");
			}
			else {
				trace("Failed")
			}
			
			if (CollatorMode.SORTING == "sorting") {
				trace("Passed");
			}
			else {
				trace("Failed")
			}
		}
	}
}