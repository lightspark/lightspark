package
{
	import flash.display.JPEGEncoderOptions;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.ApplicationDomain;
	import flash.system.JPEGLoaderContext;
	import flash.system.SecurityDomain;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var jpegLoaderContext1 = new JPEGLoaderContext(0.0);
			var jpegLoaderContext2 = new JPEGLoaderContext(1, false);
			var jpegLoaderContext3 = new JPEGLoaderContext(40.5, false, new ApplicationDomain());
			var jpegLoaderContext4 = new JPEGLoaderContext(80, false, new ApplicationDomain(),SecurityDomain.currentDomain);
		}
		
	}
	
}