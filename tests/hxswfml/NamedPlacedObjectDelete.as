package {
	import Tests;
	import TestSpriteDeclared;
	import flash.display.*;
	import flash.events.*;
	import flash.system.ApplicationDomain;
	
	public class NamedPlacedObjectDelete extends MovieClip {
		public function NamedPlacedObjectDelete() {
			var TestSpriteDynamic:Class = ApplicationDomain.currentDomain.getDefinition("TestSpriteDynamic") as Class;
			var testSprite:Sprite = new TestSpriteDynamic();
			Tests.assertTrue(testSprite["testName"] is Shape, "Right class for legacy children dynamic");
			var ret:Boolean = (delete testSprite["testName"]);
			Tests.assertEquals(true, ret, "Deletion of legacy children dynamic");
			Tests.assertEquals(undefined, testSprite["testName"], "Legacy children dynamic after deletion");

			var testSprite2:Sprite = new TestSpriteDeclared();
			Tests.assertTrue(testSprite2["testName"] is Shape, "Right class for legacy children declared");
			var ret2:Boolean = (delete testSprite2["testName"]);
			Tests.assertEquals(false, ret2, "Deletion of legacy children declared");
			Tests.assertTrue(testSprite2["testName"] is Shape, "Legacy children declared after deletion");

			Tests.report();
		}
	}
}
