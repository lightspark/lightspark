package {
	import Tests;
	import TestSpriteDeclared;
	import flash.display.*;
	import flash.events.*;
	import flash.system.ApplicationDomain;
	
	public class NamedPlacedObjectDelete extends MovieClip {
		private var dynamicChild:MovieClip;
		private var declaredChild:MovieClip;
		private var frame1Count:int = 0;
		private function frame0dynamic():void
		{
			Tests.assertEquals(1, dynamicChild.currentFrame);
			Tests.assertTrue(dynamicChild["testName"] is Shape, "Right class for legacy children dynamic");
		}
		private function frame1dynamic():void
		{
			Tests.assertEquals(2, dynamicChild.currentFrame);
			//We expect null, not undefined. Use strict equality
			Tests.assertEquals(null, dynamicChild["testName"], "Removed legacy children dynamic is null", true);

			frame1Count++;
			if(frame1Count==2)
				Tests.report();
		}
		private function frame0declared():void
		{
			Tests.assertEquals(1, declaredChild.currentFrame);
			Tests.assertTrue(declaredChild["testName"] is Shape, "Right class for legacy children declared");
		}
		private function frame1declared():void
		{
			Tests.assertEquals(2, declaredChild.currentFrame);
			//We expect null, not undefined. Use strict equality
			Tests.assertEquals(null, declaredChild["testName"], "Removed legacy children declared is null", true);

			frame1Count++;
			if(frame1Count==2)
				Tests.report();
		}
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

			//Tests.report();
			dynamicChild = new TestSpriteDynamic();
			dynamicChild.addFrameScript(0, frame0dynamic, 1, frame1dynamic);
			addChild(dynamicChild);
			declaredChild = new TestSpriteDeclared();
			declaredChild.addFrameScript(0, frame0declared, 1, frame1declared);
			addChild(declaredChild);
		}
	}
}
