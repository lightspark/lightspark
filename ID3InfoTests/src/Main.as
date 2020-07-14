package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.media.ID3Info;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var song = new ID3Info();
			song.album = "Album";
			song.artist = "Artist";
			song.comment = "Comment";
			song.genre = "Genre";
			song.songName = "Song name";
			song.track = "Track";
			song.year = "2020";
			
			if (song.album == "Album")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.artist == "Artist")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.comment == "Comment")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.genre == "Genre")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.songName == "Song name")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.track == "Track")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (song.year == "2020")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		}
		
		private function init(e:Event = null):void 
		{
			removeEventListener(Event.ADDED_TO_STAGE, init);
			// entry point
		}
		
	}
	
}