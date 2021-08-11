package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.text.engine.ContentElement;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ContentElement.GRAPHIC_ELEMENT == 0xFDEF)
			{
				trace("Passed")
			}
			else
			{
				trace("Failed")
			}
		}
	}
	
}