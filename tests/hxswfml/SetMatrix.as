package {
	import Tests;
	import flash.display.MovieClip;
	import flash.display.Shape;
	import flash.geom.Matrix;
	import flash.events.Event;
	public class SetMatrix extends MovieClip {
		private function onFrame(e:Event):void
		{
			if(this.currentFrame%2==1)
				return;
			trace("FRAME "+this.currentFrame);
			trace(this.getChildAt(0).transform.matrix);
			trace(this.getChildAt(0).rotation);
			this.getChildAt(0).rotation+=10;
			trace("frame");
		}
		function SetMatrix()
		{
			trace("test");
			addEventListener("enterFrame",onFrame);
		}
	}
}
