package
{
	import flash.accessibility.Accessibility;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.SystemUpdater;
	import flash.system.SystemUpdaterType;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var systemUpdater:SystemUpdater = new SystemUpdater();
			systemUpdater.update("system");
			systemUpdater.cancel();
		}
		
	}
	
}