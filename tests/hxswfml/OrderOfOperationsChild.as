package
{
	import flash.display.DisplayObjectContainer;
	import flash.display.Sprite;
	import flash.events.Event;
	import Tests;
	public class OrderOfOperationsChild extends Sprite
	{
		private var constructorEnded:Boolean = false;
		private var addedToStageReceived:Boolean = false;
		private function onAddedToStage(e:Event):void
		{
			Tests.assertFalse(constructorEnded, "addedToStage called immediately for 'new'ed objects 1/2");
			addedToStageReceived = true;
		}
		private function legacyAddedToStage(e:Event):void
		{
			Tests.assertTrue(constructorEnded, "addedToStage called after constructor for legacy objects 2/2");
			addedToStageReceived = true;
		}
		public function OrderOfOperationsChild(c:DisplayObjectContainer = null):void
		{
			Tests.assertEquals(1, this.numChildren, "Legacy children already in parent before their constructor is run");
			if(c)
			{
				this.addEventListener("addedToStage",onAddedToStage);
				c.addChild(this);
				Tests.assertTrue(addedToStageReceived, "addedToStage called immediately for 'new'ed objects 2/2");
			}
			else
			{
				Tests.assertNotNull(parent, "Parent already available in constructor of legacy children");
				Tests.assertEquals(2, parent.numChildren, "Legacy children already in parent before their constructor is run");
				this.addEventListener("addedToStage",legacyAddedToStage);
				Tests.assertFalse(addedToStageReceived, "addedToStage called after constructor for legacy objects 1/2");
			}
			constructorEnded = true;
		}
	}
}
