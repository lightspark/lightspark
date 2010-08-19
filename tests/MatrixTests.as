package
{
	import flash.display.Sprite;
	import flash.geom.Matrix;
	
	public class MatrixTests extends Sprite
	{
		public function MatrixTests()
		{
			//http://www.adobe.com/livedocs/flash/9.0/ActionScriptLangRefV3/flash/geom/Matrix.html
			var myMatrix:Matrix = new Matrix();
			trace(myMatrix.a);  // 1

			myMatrix.a = 2;
			trace(myMatrix.a);  // 2
		}
	}
}