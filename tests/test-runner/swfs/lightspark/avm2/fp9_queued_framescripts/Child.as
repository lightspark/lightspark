package {
	import flash.display.DisplayObjectContainer;
	import flash.display.MovieClip;
	import flash.events.Event;
	import test.utils.log;
	import test.on_framescript;

	public class Child extends MovieClip {
		var main: Main = root as Main;

		public function Child() {
			log(this, "constructing");
			if (main.init) {
				addEventListener(Event.ENTER_FRAME, enter_frame);
				addEventListener("frameConstructed", frame_constructed);
			}
			addFrameScript(0, framescript, 1, framescript, 2, framescript);
		}

		function framescript() {
			var parent_clip: MovieClip = parent as MovieClip;
			on_framescript(this, 1, -1, null, "parent.currentFrame: " + parent_clip.currentFrame);
		}

		function enter_frame(ev: Event) {
			log(this, "enterFrame");
		}

		function frame_constructed(ev: Event) {
			log(this, "frameConstructed");
			gotoAndStop(3);
		}
	}
}
