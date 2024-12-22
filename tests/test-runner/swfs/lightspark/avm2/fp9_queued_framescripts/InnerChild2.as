package {
	import flash.display.MovieClip;
	import flash.events.Event;
	import test.utils.log;
	import test.on_framescript;

	public class InnerChild2 extends MovieClip {
		var main: Main = root as Main;

		public function InnerChild2() {
			log(this, "constructing");
			addFrameScript(0, framescript, 1, framescript, 2, framescript);
			if (main.init) {
				addEventListener(Event.ENTER_FRAME, enter_frame);
				addEventListener("frameConstructed", frame_constructed);
			}
		}

		function enter_frame(ev: Event) {
			log(this, "enterFrame");
			gotoAndPlay(2);
		}

		function frame_constructed(ev: Event) {
			log(this, "frameConstructed");
		}

		function framescript() {
			on_framescript(this, 3, 2);
		}
	}
}
