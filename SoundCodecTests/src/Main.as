package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.SoundCodec;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (SoundCodec.NELLYMOSER == "NellyMoser")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SoundCodec.PCMA == "pcma")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SoundCodec.PCMU == "pcmu")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SoundCodec.SPEEX == "Speex")
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