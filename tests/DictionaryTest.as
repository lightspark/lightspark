package
{
	import flash.display.Sprite;
	import flash.utils.Dictionary;
	
	public class DictionaryTest extends Sprite
	{
		public function DictionaryTest()
		{
			var testDict:Dictionary = new Dictionary;
			var obj:Object = new Object;
			testDict["foo"] = "test1";
			testDict["bar"] = 3.14;
			testDict["baz"] = obj;
			testDict[obj] = "I'm on a boat.";
			
			//Test iteration
			for each(var str:String in testDict)
			{
				trace(str);
			}
			
			//Test toString
			trace(testDict);
		}
	}
}