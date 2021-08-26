package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.system.Capabilities;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			trace("avHardwareDisable: " + Capabilities.avHardwareDisable); 
			trace("cpuArchitecture: " + Capabilities.cpuArchitecture); 
			trace("hasAccessibility: " + Capabilities.hasAccessibility); 
			trace("hasAudio: " + Capabilities.hasAudio); 
			trace("hasAudioEncoder: " + Capabilities.hasAudioEncoder); 
			trace("hasEmbeddedVideo: " + Capabilities.hasEmbeddedVideo); 
			trace("hasIME: " + Capabilities.hasIME); 
			trace("hasMP3: " + Capabilities.hasMP3); 
			trace("hasPrinting: " + Capabilities.hasPrinting); 
			trace("hasScreenBroadcast: " + Capabilities.hasScreenBroadcast); 
			trace("hasScreenPlayback: " + Capabilities.hasScreenPlayback); 
			trace("hasStreamingAudio: " + Capabilities.hasStreamingAudio); 
			trace("hasStreamingVideo: " + Capabilities.hasStreamingVideo); 
			trace("hasTLS: " + Capabilities.hasTLS); 
			trace("hasVideoEncoder: " + Capabilities.hasVideoEncoder); 
			trace("isDebugger: " + Capabilities.isDebugger); 
			trace("isEmbeddedInAcrobat: " + Capabilities.isEmbeddedInAcrobat); 
			trace("language: " + Capabilities.language); 
			trace("localFileReadDisable: " + Capabilities.localFileReadDisable); 
			trace("manufacturer: " + Capabilities.manufacturer); 
			trace("maxLevelIDC: " + Capabilities.maxLevelIDC); 
			trace("os: " + Capabilities.os);
			trace("pixelAspectRatio: " + Capabilities.pixelAspectRatio); 
			trace("playerType: " + Capabilities.playerType); 
			trace("screenColor: " + Capabilities.screenColor); 
			trace("screenDPI: " + Capabilities.screenDPI); 
			trace("screenResolutionX: " + Capabilities.screenResolutionX); 
			trace("screenResolutionY: " + Capabilities.screenResolutionY); 
			trace("serverString: " + Capabilities.serverString); 
			trace("supports32BitProcesses: " + Capabilities.supports32BitProcesses); 
			trace("supports64BitProcesses: " + Capabilities.supports64BitProcesses); 
			trace("touchscreenType: " + Capabilities.touchscreenType); 
 	 	
			trace("hasMultiChannelAudio: " + Capabilities.hasMultiChannelAudio("DTM"));
		}
		
	}
	
}