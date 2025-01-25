package
{
	import flash.display.MovieClip;
	import flash.events.Event;

	public class Test extends MovieClip
	{
		public function Test()
		{
			stage.addEventListener("test", onTest);
			stage.addEventListener("test", onTest2);
			stage.dispatchEvent(new TestEvent());
		}

		private function onTest(ev: Event): void
		{
			trace("Test: onTest");
		}

		private function onTest2(ev: Event): void
		{
			trace("Test: onTest2");
		}
	}
}
