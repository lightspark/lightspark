package {
	import flash.display.Sprite;

	public class AccessorTest extends Sprite {
		testns var testproperty:Number = 123;

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
