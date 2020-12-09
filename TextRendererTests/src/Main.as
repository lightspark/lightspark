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
			TextRenderer.antiAliasType = "advanced";
			TextRenderer.antiAliasType = "normal";
			TextRenderer.displayMode = "default";
			TextRenderer.displayMode = "crt";
			TextRenderer.displayMode = "lcd";
			
			var advancedAntiAliasingTable:Array = new Array();
			advancedAntiAliasingTable.push(new flash.text.CSMSettings(2, 2, 2));
			
			TextRenderer.setAdvancedAntiAliasingTable("", "", "", advancedAntiAliasingTable);
		}
		
	}
	
}