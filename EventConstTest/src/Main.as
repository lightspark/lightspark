package
{
	import flash.display.Sprite;
	import flash.events.Event;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
		
		// TODO: Add more consts here...	
			
		if (Event.BROWSER_ZOOM_CHANGE == "browserZoomChange")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		if (Event.CHANNEL_MESSAGE == "channelMessage")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		if (Event.CHANNEL_STATE == "channelState")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		if (Event.FRAME_LABEL == "frameLabel")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		if (Event.VIDEO_FRAME == "videoFrame")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		if (Event.WORKER_STATE == "workerState")
		{
			trace("Passed");
		}
		else
		{
			trace("Passed");
		}
		
		}
	}
	
}