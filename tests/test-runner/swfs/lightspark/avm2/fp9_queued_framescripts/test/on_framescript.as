package test {
	import flash.display.MovieClip;
	import test.utils.log;
	
	public function on_framescript(
		obj: MovieClip,
		frame: int = 1,
		goto_frame: int = -1,
		msg: String = null,
		goto_msg: String = null,
		stop: Boolean = false
	): Boolean {
		var suffix: String = (msg !== null) ? (". " + msg) : "";
		goto_frame = (goto_frame <= 0) ? obj.totalFrames : goto_frame;
		log(obj, "framescript " + (obj.currentFrame - 1) + suffix);
		if (obj.currentFrame == goto_frame) {
			suffix = (goto_msg !== null) ? (". " + goto_msg) : "";
			log(obj, "going to frame " + frame + suffix);
			(stop) ? obj.gotoAndStop(frame) : obj.gotoAndPlay(frame);
			return true;
		}
		return false;
	}
}
