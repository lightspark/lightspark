package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.desktop.ClipboardFormats;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ClipboardFormats.BITMAP_FORMAT == "air:bitmap")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardFormats.FILE_LIST_FORMAT  == "air:file list")
			{
				trace("Passed");				
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardFormats.HTML_FORMAT == "air:html")
			{
				trace("Passed");				
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardFormats.RICH_TEXT_FORMAT == "air:rtf")
			{
				trace("Passed");				
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardFormats.TEXT_FORMAT == "air:text")
			{
				trace("Passed");				
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardFormats.URL_FORMAT == "air:url")
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