package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.AudioOutputChangeReason;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (AudioOutputChangeReason.DEVICE_CHANGE == "userSelection")
			{
				trace("");
			}
			else
			{
				trace("");
			}
			
			if (AudioOutputChangeReason.USER_SELECTION == "userSelection")
			{
				trace("");
			}
			else
			{
				trace("");
			}
		}
		
	}
	
}