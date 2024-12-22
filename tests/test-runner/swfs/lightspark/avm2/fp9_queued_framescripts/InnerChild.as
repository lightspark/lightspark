package {	
	import flash.display.MovieClip;
	import flash.events.Event;
	import test.utils.log;
	import test.on_framescript;

	public class InnerChild extends MovieClip {
		var main: Main = root as Main;

		public function InnerChild() {
			log(this, "constructing");
			addFrameScript(0, framescript, 1, framescript, 2, framescript);
			if (main.init) {
				addEventListener("frameConstructed", frame_constructed);
			}
		}

		function frame_constructed(ev: Event) {
			log(this, "frameConstructed");
			gotoAndStop(2);
		}

		function framescript() {
			on_framescript(this, 1, 2);
		}
	}
}
