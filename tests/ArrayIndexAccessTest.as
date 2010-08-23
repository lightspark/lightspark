package
{
	import flash.display.Sprite;
	
	public class ArrayIndexAccessTest extends Sprite
	{
		public function ArrayIndexAccessTest()
		{
			var testArray:Array = new Array;
			var obj:Object = new Object;
			testArray[0] = "test1";
			testArray[1] = 3.14;
			testArray[2] = obj;
			testArray[3] = "I'm on a boat.";
			
			//Test iteration
			for each(var str:String in testArray)
			{
				trace(str);
			}
			
			//Test toString
			trace(testArray);
			
			//Test out of bounds access (should be undefined according to the human-AS3-VM, Ben Garney)
			trace(testArray[testArray.length]);
		}
	}
}