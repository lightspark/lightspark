package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.desktop.ClipboardTransferMode;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ClipboardTransferMode.CLONE_ONLY == "cloneOnly")
			{
				trace("Passed")
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardTransferMode.CLONE_PREFERRED == "clonePreferred")
			{
				trace("Passed")
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardTransferMode.ORIGINAL_ONLY == "originalOnly")
			{
				trace("Passed")
			}
			else
			{
				trace("Failed");
			}
			
			if (ClipboardTransferMode.ORIGINAL_PREFERRED  == "originalPreferred")
			{
				trace("Passed")
			}
			else
			{
				trace("Failed");
			}
			
		}
		
	}
	
}