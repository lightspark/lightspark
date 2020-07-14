package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.AudioDecoder;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (AudioDecoder.DOLBY_DIGITAL == "DolbyDigital")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (AudioDecoder.DOLBY_DIGITAL_PLUS == "DolbyDigitalPlus")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (AudioDecoder.DTS == "DTS")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (AudioDecoder.DTS_EXPRESS == "DTSExpress")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (AudioDecoder.DTS_HD_HIGH_RESOLUTION_AUDIO == "DTSHDHighResolutionAudio")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (AudioDecoder.DTS_HD_MASTER_AUDIO == "DTSHDMasterAudio")
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