package
{
	import flash.events.Event;

	public class TestEvent extends Event
	{
		public function TestEvent(bubbles: Boolean = false, cancelable: Boolean = false)
		{
			super("test", bubbles, cancelable);
		}
	}
}
