package {
	import Tests;
	import flash.display.*;
	import flash.events.*;
	import OrderOfOperationsChild;
	
	public class OrderOfOperations extends MovieClip {

		private var numOp:uint = 0; //number of current operation

		public function OrderOfOperations() {
			this.addEventListener(Event.ENTER_FRAME,onEnterFrame);
			this.addEventListener(Event.FRAME_CONSTRUCTED,onFrameConstructed);
			this.addEventListener(Event.EXIT_FRAME,onExitFrame);
			this.addEventListener(Event.ADDED,onAdded);
			this.addEventListener(Event.ADDED_TO_STAGE,onAddedToStage);
			this.addEventListener(Event.RENDER,onRender);
			this.addEventListener(Event.REMOVED,onRemoved);
			//Not tested events: removedFromStage
			this.addFrameScript(0,frame1)
			this.addFrameScript(1,frame2)
			this.addFrameScript(2,frame3)
			
			this.addFrameScript(3,exit)
		}

		private function frame1():void {
			Tests.assertEquals(3,numOp, "frame script on frame 1 is in right order");
			numOp++;
			Tests.assertEquals(1,this.currentFrame,"frame script on frame 1");
			Tests.assertEquals(0,this.numChildren,"No children yet");
		}

		private function frame2():void {
			//<PlaceObject id="2" depth="1" />
			Tests.assertEquals(8,numOp, "frame script on frame 2 is in right order");
			numOp++;
			Tests.assertEquals(2,this.currentFrame,"frame script on frame 2");
			Tests.assertEquals(1,this.numChildren,"frame2: One child created");
			Tests.assertNotNull(this.getChildAt(0),"Legacy child created and non-null in frameScript");
		}

		private function frame3():void {
			var a:OrderOfOperationsChild = new OrderOfOperationsChild(stage);
		}
		
		private function exit():void {
			Tests.assertEquals(16,numOp,"exit is in right order");
			Tests.report(null, this.name);
		}

		private function onEnterFrame(e:Event):void {
			switch(this.currentFrame)
			{
			case 2:
				//<PlaceObject id="2" depth="1" />
				Tests.assertEquals(5,numOp,"ENTER_FRAME in frame 2 is in right order");
				numOp++;
				
				Tests.assertEquals(1,this.numChildren,"numChildren updated in onEnterFrame of PlaceObject");
				if(this.numChildren == 1)
					Tests.assertNull(this.getChildAt(0),"Legacy child created but null in onEnterFrame of PlaceObject");
				break;
			case 3:
				Tests.assertEquals(10,numOp,"ENTER_FRAME in frame 3 is in right order");
				numOp++;
				break;
			case 4:
				Tests.assertEquals(14,numOp,"ENTER_FRAME in frame 4 is in right order");
				numOp++;
				break;
			case 1: //There is no ENTER_FRAME for frame 1
			default:
				Tests.assertDontReach("ENTER_FRAME is dispatched for wrong frame");
				break;
			}
		}

		private function onFrameConstructed(e:Event):void {
			switch(this.currentFrame)
			{
			case 1:
				Tests.assertEquals(2,numOp,"onFrameConstructed in frame 1 is in right order");
				numOp++;
				break;
			case 2:
				Tests.assertEquals(7,numOp,"onFrameConstructed in frame 2 is in right order");
				numOp++;
				Tests.assertEquals(1,this.numChildren,"numChildren updated in onFrameConstructed of PlaceObject");
				if(this.numChildren == 1)
					Tests.assertNotNull(this.getChildAt(0),"Legacy child created and non-null in onFrameConstructed of PlaceObject");
				break;
			case 3:
				Tests.assertEquals(11,numOp,"onFrameConstructed in frame 3 is in right order");
				numOp++;
				break;
			case 4:
				Tests.assertEquals(15,numOp,"onFrameConstructed in frame 4 is in right order");
				numOp++;
				break;
			default:
				Tests.assertDontReach("frameConstructed is dispatched for wrong frame");
				break;
			}
		}
		
		private function onExitFrame(e:Event):void {
			switch(this.currentFrame)
			{
			case 1:
				Tests.assertEquals(4,numOp,"onExitFrame in frame 1 is in right order");
				numOp++;
				break;
			case 2:
				Tests.assertEquals(9,numOp,"onExitFrame in frame 2 is in right order");
				numOp++;
				break;
			case 3:
				Tests.assertEquals(12,numOp,"onExitFrame in frame 3 is in right order");
				numOp++;
				break;
			case 4:
				break; //this is dispatched after the test results have already been printed
			default:
				Tests.assertDontReach("onExitFrame is dispatched for wrong frame");
				break;
			}
		}
		
		private function onRender(e:Event):void {
			switch(this.currentFrame)
			{
			case 2:
				Tests.assertEquals(3,numOp,"onRender in frame 2 is in right order");
				numOp++;
				break;
			case 3:
				Tests.assertEquals(3,numOp,"onRender in frame 3 is in right order");
				numOp++;
				break;
			case 1:
			default:
				Tests.assertDontReach("onRender is dispatched for wrong frame");
				break;
			}
		}
		
		private function onAdded(e:Event):void {
			if(e.target == this) {
				Tests.assertEquals(0,numOp, "onAdded is in right order");
				numOp++;
				Tests.assertEquals(1,this.currentFrame,"onAdded on right frame");
			} else {
				//This is the event for the child added in frame 2 (it bubbles)
				Tests.assertEquals(6,numOp, "onAdded for child is in right order");
				numOp++;
				Tests.assertEquals(2,this.currentFrame,"onAdded for child on right frame");
			}
		}
		
		private function onRemoved(e:Event):void {
			Tests.assertEquals(true,this != e.target,"onRemoved this != e.target");
			//This is the event for the child added in frame 2 (it bubbles)
			Tests.assertEquals(13,numOp, "onRemoved for child is in right order");
			numOp++;
			Tests.assertEquals(3,this.currentFrame,"onRemoved for child on right frame");
		}
		
		private function onAddedToStage(e:Event):void {
			if(e.target == this) {
				Tests.assertEquals(1,numOp, "onAddedToStage is in right order");
				numOp++;
				Tests.assertEquals(1,this.currentFrame,"onAddedToStage on right frame");
			} else {
				//We won't get this event from the child as it doesn't bubble
				Tests.assertDontReach("onAddedToStage is not dispatched for child");
			}
		}
	}
}
