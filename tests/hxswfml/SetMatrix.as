package {
	import Tests;
	import flash.display.DisplayObject;
	import flash.display.MovieClip;
	import flash.display.Shape;
	import flash.geom.Matrix;
	import flash.events.Event;
	public class SetMatrix extends MovieClip {
		private function onFrame(e:Event):void
		{
			if(this.currentFrame==1)
				return;
			var child:DisplayObject=this.getChildAt(0);
			trace("FRAME "+this.currentFrame);
			trace(child.transform.matrix);
			child.rotation+=10;
		}
		function SetMatrix()
		{
			addEventListener("enterFrame",onFrame);
		}
	}
}
