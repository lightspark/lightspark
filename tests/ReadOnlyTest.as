package {
	import flash.display.Sprite;

	public class ReadOnlyTest extends Sprite {
		public function get readOnlyProperty():int {
			return 0;
		}

		// Sprite defines defines getter and setter, we
		// override only the getter.
		public override function get name():String {
			return "";
		}
	}
}
