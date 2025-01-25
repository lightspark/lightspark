package
{
	import flash.display.MovieClip;
	import flash.events.Event;

	public class Clip extends MovieClip
	{
		public function Clip()
		{
			stage.addEventListener("test", onTest);
			stage.addEventListener("test", onTest2);
		}

		private function onTest(ev: Event): void
		{
			trace("Clip: onTest");
		}

		private function onTest2(ev: Event): void
		{
			trace("Clip: onTest2");
		}
	}
}
