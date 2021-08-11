package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.text.AntiAliasType;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (AntiAliasType.ADVANCED == "advanced")
			{
				trace("Passed")
			}
			else
			{
				trace("Failed")
			}
			
			if (AntiAliasType.NORMAL == "normal")
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