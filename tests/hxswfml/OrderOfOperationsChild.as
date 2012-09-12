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
		public function OrderOfOperationsChild(c:DisplayObjectContainer):void
		{
			this.addEventListener("addedToStage",onAddedToStage);
			c.addChild(this);
			Tests.assertTrue(addedToStageReceived, "addedToStage called immediately for 'new'ed objects 2/2");
			constructorEnded = true;
		}
	}
}
