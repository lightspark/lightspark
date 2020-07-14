package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.BitmapEncodingColorSpace;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (BitmapEncodingColorSpace.COLORSPACE_4_2_0 == "4:2:0")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (BitmapEncodingColorSpace.COLORSPACE_4_2_2 == "4:2:2")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (BitmapEncodingColorSpace.COLORSPACE_4_4_4 == "4:4:4")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (BitmapEncodingColorSpace.COLORSPACE_AUTO == "auto")
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