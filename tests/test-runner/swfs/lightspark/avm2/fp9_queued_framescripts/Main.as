package {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.system.fscommand;
	import flash.utils.getTimer;
	import test.utils.log;
	import test.on_framescript;

	public class Main extends MovieClip {
		var prev_time: int = getTimer();
		var old_frame_count: int = -1;
		var frame_count: int = 0;
		var init: Boolean = true;
		var running_framescripts: Boolean = false;

		public function Main() {
			running_framescripts = false;
			log(this, "constructing");

			addFrameScript(0, framescript, 1, framescript, 2, framescript);
			addEventListener(Event.ENTER_FRAME, enter_frame);
			addEventListener("frameConstructed", frame_constructed);
			addEventListener("exitFrame", exit_frame);
		}

		function in_queued_framescript(): Boolean {
			return running_framescripts && old_frame_count == frame_count;
		}

		function frame_constructed(ev: Event) {
			running_framescripts = false;
			log(this, "frameConstructed");

			if (init && frame_count > 0) {
				init = false;
			}
		}

		function exit_frame(ev: Event) {
			running_framescripts = false;
			log(this, "exitFrame");
		}

		function enter_frame(ev: Event) {
			running_framescripts = false;
			log(this, "enterFrame. frame_count: " + frame_count);
			frame_count++;
		}

		function framescript() {
			if (in_queued_framescript()) {
				log(this, "queued framescript");
			}
			running_framescripts = true;
			on_framescript(this, 2);
			old_frame_count = frame_count;
		}
	}
}
