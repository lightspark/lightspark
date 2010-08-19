//http://www.adobe.com/livedocs/flash/9.0/ActionScriptLangRefV3/flash/geom/Point.html
package {
    import flash.display.Sprite;
    import flash.geom.Point;

    public class PointTest extends Sprite {

        public function PointTest() {
			var point1:Point = new Point();
			trace(point1);  // (x=0, y=0)
			
			var point2:Point = new Point(6, 8);
			trace(point2); // (x=6, y=8)
			
			trace(Point.interpolate(point1, point2, 0.5)); // (x=3, y=4)
			
			trace(Point.distance(point1, point2)); // 10
			
			trace(point1.add(point2)); // (x=6, y=8)
			
			var point3:Point = point2.clone();
			trace(point2.equals(point3)); // true
			
			point3.normalize(2.5);
			trace(point3); // (x=1.5, y=2)
			
			trace(point2.subtract(point3)); // (x=4.5, y=6)
			
			trace(point1.offset(2, 3)); // (x=2, y=3)
			
			var angle:Number = Math.PI * 2 * (30 / 360); // 30°
			trace(Point.polar(4, angle)) // (x=3.464101615137755, y=1.9999999999999998)   
        }
    }
}