package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.VideoCodec;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (VideoCodec.H264AVC == "H264Avc")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (VideoCodec.SORENSON == "Sorenson")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (VideoCodec.VP6 == "VP6")
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