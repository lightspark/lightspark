package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.IME;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			IME.enabled = false;
			
			if (!IME.enabled)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			IME.enabled = true;
			if (IME.enabled)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (IME.isSupported || !IME.isSupported)
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			IME.setCompositionString("test");
			IME.compositionAbandoned();
			IME.compositionSelectionChanged(0, 1);
			IME.doConversion();
		}
	}
}