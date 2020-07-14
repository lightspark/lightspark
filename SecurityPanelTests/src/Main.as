package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.SecurityPanel;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (SecurityPanel.CAMERA == "camera")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SecurityPanel.DEFAULT == "default")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SecurityPanel.DISPLAY == "display")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SecurityPanel.LOCAL_STORAGE == "localStorage")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}

			if (SecurityPanel.MICROPHONE == "microphone")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SecurityPanel.PRIVACY == "privacy")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (SecurityPanel.SETTINGS_MANAGER == "settingsManager")
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