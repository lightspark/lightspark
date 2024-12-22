package {
	import flash.display.MovieClip;
	import flash.events.Event;
	import test.utils.log;
	import test.on_framescript;

	public class Container extends MovieClip {
		var main: Main = root as Main;
		public function Container() {
			log(this, "constructing");
			addFrameScript(0, framescript, 1, framescript, 2, framescript);
			if (main.init) {
				log(this, "initializing event listeners");
				addEventListener(Event.ENTER_FRAME, enter_frame);
				addEventListener("frameConstructed", frame_constructed);
				addEventListener("exitFrame", exit_frame);
			}
		}

		function enter_frame(ev: Event) {
			log(this, "enterFrame");
		}

		function frame_constructed(ev: Event) {
			log(this, "frameConstructed");
		}

		function exit_frame(ev: Event) {
			log(this, "exitFrame");
		}

		function framescript() {
			if (on_framescript(this, 2, -1, null, "currentFrame: " + currentFrame)) {
				log(this, "after call to gotoAndPlay. currentFrame: " + currentFrame);
			}
		}
	}
}
