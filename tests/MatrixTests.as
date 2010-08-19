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
			
			//Test 1
			
			trace(myMatrix.a);  // 1

			myMatrix.a = 2;
			trace(myMatrix.a);  // 2
			
			//Test 2
			
			trace(myMatrix.b);  // 0

			var degrees:Number = 30;
			var radians:Number = (degrees/180) * Math.PI;
			myMatrix.b = Math.tan(radians);
			trace(myMatrix.b);  // 0.5773502691896257
			
			//Test 3
			
			var matrix_1:Matrix = new Matrix();
			trace(matrix_1);  // (a=1, b=0, c=0, d=1, tx=0, ty=0)

			var matrix_2:Matrix = new Matrix(1, 2, 3, 4, 5, 6);
			trace(matrix_2);  // (a=1, b=2, c=3, d=4, tx=5, ty=6)

			//Test 4
			matrix_2.identity();
			trace(matrix_2);  // (a=1, b=0, c=0, d=1, tx=0, ty=0)
		}
	}
}