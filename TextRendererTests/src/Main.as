package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.text.TextRenderer;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{		
			trace(TextRenderer.displayMode);
			
			if (TextRenderer.displayMode == "lcd")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			TextRenderer.maxLevel = 4;
			if (TextRenderer.maxLevel == 4)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			var advancedAntiAliasingTable:Array = new Array();
			advancedAntiAliasingTable.push(new flash.text.CSMSettings(2, 2, 2));
			
			TextRenderer.setAdvancedAntiAliasingTable("", "", "", advancedAntiAliasingTable);
		}
		
	}
	
}