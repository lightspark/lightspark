package {

	import flash.display.Sprite;

	public class rendering_Clipping extends Sprite {

		public function rendering_Clipping() {
			var circle:Sprite = new Sprite();
			circle.graphics.beginFill(0x00FF00);
			//Rendering corner cases
			circle.graphics.drawCircle(0,80,40);
			circle.graphics.drawCircle(80,0,40);
			circle.graphics.drawCircle(0,0,40);
			addChild(circle);
		}

	}

}
