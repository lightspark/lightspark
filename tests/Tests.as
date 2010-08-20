// USAGE:
// assertArrayEquals(arr1, arr2, msg, strict):
// 	Test arrays for equality (objects in array are equal and in same order)
// assertEqualsDelta(expected, actual, delta, msg, strict):
// 	Test difference between expected and actual is within range delta
// 	NOTE: strict also performs same-type checking
// assertEquals(expected, actual, msg, strict):
// 	Test expected equals actual
// assertTrue(cond, msg, strict):
// 	Test cond is true
// assertFalse(cond, msg, strict):
// 	Test cond is false
// assertNull(obj, msg, strict):
// 	Test obj is null
// assertNotNull(obj, msg, strict):
// 	Test obj is not null
// assertUndefined(obj, msg, strict):
// 	Test obj is undefined
// assertNotUndefined(obj, msg, strict):
// 	Test obj is not undefined
// assertDontReach(msg):
// 	Fails when reached
// 
// info(msg1, msg2, ...):
// 	Print messages to stderr (with [INFO] prefix)
// error(msg1, msg2, ...):
// 	Print messages to stderr (with [ERROR] prefix)
//
// report(visual):
//	Print the test report to stderr. Failure messages will be visible in the output.
//	If visual is not null, it will be assumed to be an UIComponent and will be colored red on failure, green on success
//
// NOTE: all assertion methods except assertDontReach take an OPTIONAL strict argument
//       if this argument is true, strict checking is performed with === (same value AND type)
// NOTE: all assert methods take an OPTIONAL message argument (msg)
//       this message will be output in between test output (not with the report)
//
//
// OUTPUT:
// A succesful test outputs '.', a failed test 'F'
// assertArrayEquals outputs 'F' when the lengths of the arrays differ otherwise it outputs '. '
//   it outputs '.' or 'F' for every element checked on the same line
//

package
{
	import flash.system.fscommand;
	public class Tests
	{
		private static var testCount:uint = 0;
		private static var failures:Array = new Array();
		public static function assertArrayEquals(arr1:Array, arr2:Array, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			var outp:String = "";
			if(arr1.length == arr2.length)
			{
				outp += "A ";
				var res:Boolean = true;
				for(var i:uint = 0; i < arr1.length; i++)
				{
					if( (!strict && arr1[i] == arr2[i]) || (strict && arr1[i] === arr2[i]) )
						outp += ".";
					else
					{
						outp += "F";
						res = false;
					}
				}
				trace(outp,"[" + (msg == null ? "assertArrayEquals" : msg) + "]");
				if(res != true && msg == null)
					failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertArrayEquals\n\tArray 1: " + arr1 + "\n\tArray 2: " + arr2);
				else if(res != true && msg != null)
					failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tArray 1: " + arr1 + "\n\tArray 2: " + arr2);
			}
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertArrayEquals" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertArrayEquals\n\tLength array 1: " + arr1.length + "\n\tLength array 2: " + arr2.length);
			}
			else
			{
				trace("F [" + (msg == null ? "assertArrayEquals" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tArray 1: " + arr1 + "\n\tArray 2: " + arr2);
			}
		}
		public static function assertEqualsDelta(expected:*, actual:*, delta:Number, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if(Math.abs(expected-actual) < Math.abs(delta) && (!strict || (typeof expected == typeof actual)))
				trace(". [" + (msg == null ? "assertEqualsDelta" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertEqualsDelta" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertEqualsDelta\n\tExpected: " + expected + "\n\tDelta: " + delta + "\n\tActual: " + actual);
			}
			else
			{
				trace("F [" + (msg == null ? "assertEqualsDelta" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: " + expected + "\n\tDelta: " + delta + "\n\tActual: " + actual);
			}
		}
		public static function assertEquals(expected:*, actual:*, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && expected == actual) || (strict && expected === actual) )
				trace(". [" + (msg == null ? "assertEquals" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertEquals" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertEquals\n\tExpected: " + expected + "\n\tActual: " + actual);
			}
			else
			{
				trace("F [" + (msg == null ? "assertEquals" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: " + expected + "\n\tActual: " + actual);
			}
		}
		public static function assertTrue(cond:*,msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && cond == true) || (strict && cond === true) )
				trace(". [" + (msg == null ? "assertTrue" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertTrue" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertTrue\n\tExpected: true\n\tActual: " + cond);
			}
			else
			{
				trace("F [" + (msg == null ? "assertTrue" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: true\n\tActual: " + cond);
			}
		}
		public static function assertFalse(cond:*,msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && cond == false) || (strict && cond === false) )
				trace(". [" + (msg == null ? "assertFalse" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertFalse" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertFalse\n\tExpected: false\n\tActual: " + cond);
			}
			else
			{
				trace("F [" + (msg == null ? "assertFalse" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: false\n\tActual: " + cond);
			}
		}
		public static function assertNull(obj:*, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && obj == null) || (strict && obj === null) )
				trace(". [" + (msg == null ? "assertNull" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertNull" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertNull\n\tExpected: null\n\tActual: " + obj);
			}
			else
			{
				trace("F [" + (msg == null ? "assertNull" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: null\n\tActual: " + obj);
			}
		}
		public static function assertNotNull(obj:*, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && obj != null) || (strict && obj !== null) )
				trace(". [" + (msg == null ? "assertNotNull" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertNotNull" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertNotNull\n\tExpected: not null\n\tActual: " + obj);
			}
			else
			{
				trace("F [" + (msg == null ? "assertNotNull" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: not null\n\tActual: " + obj);
			}
		}
		public static function assertUndefined(obj:*, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && obj == undefined) || (strict && obj === undefined) )
				trace(". [" + (msg == null ? "assertUndefined" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertUndefined" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertUndefined\n\tExpected: undefined\n\tActual: " + obj);
			}
			else
			{
				trace("F [" + (msg == null ? "assertUndefined" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: undefined\n\tActual: " + obj);
			}
		}
		public static function assertNotUndefined(obj:*, msg:String=null, strict:Boolean=false):void
		{
			testCount++;
			if( (!strict && obj != undefined) || (strict && obj !== undefined) )
				trace(". [" + (msg == null ? "assertNotUndefined" : msg) + "]");
			else if(msg == null)
			{
				trace("F [" + (msg == null ? "assertNotUndefined" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\tassertNotUndefined\n\tExpected: not undefined\n\tActual: " + obj);
			}
			else
			{
				trace("F [" + (msg == null ? "assertNotUndefined" : msg) + "]");
				failures.push("Failed assertion:" + (strict ? " (strict)" : "") + "\n\t" + msg + "\n\tExpected: not undefined\n\tActual: " + obj);
			}
		}
		public static function assertDontReach(msg:String=null):void
		{
			testCount++;
			trace("F [" + (msg == null ? "assertDontReach" : msg) + "]");
			if(msg == null)
				failures.push("Failed assertion:\n\tassertDontReach");
			else
				failures.push("Failed assertion:\n\t" + msg);
		}
		public static function error(...msgs):void
		{
			var lines:Array;
			for(var i:uint = 0; i < msgs.length; i++)
			{
				lines = msgs[i].split("\n");
				for(var j:uint = 0; j < lines.length; j++)
				{
					trace("[ERROR] " + lines[j]);
				}
			}
		}
		public static function info(...msgs):void
		{
			var lines:Array;
			for(var i:uint = 0; i < msgs.length; i++)
			{
				lines = msgs[i].split("\n");
				for(var j:uint = 0; j < lines.length; j++)
				{
					trace("[INFO] " + lines[j]);
				}
			}
		}
		public static function report(visual:*=null, name:String=null, dontQuit:Boolean=false):void
		{
			if(failures.length > 0)
			{
				trace("=====================Failures (" + failures.length + "/" + testCount + ")=====================");
				for(var i:uint = 0; i < failures.length; i++)
				{
					trace( (i > 0 ? "\n" : "") + failures[i]);
				}
				trace("=========================================================");
				trace("FAILURE (" + (name == null ? "unnamed test" : name) + ")");
				if(visual != null && visual.graphics != null)
				{
					visual.graphics.beginFill(0xFF0000);
					visual.graphics.drawRect(0,0,500,30); //Top
					visual.graphics.drawRect(0,375-30,500,375); //Bottom
					visual.graphics.endFill();
				}
			}
			else
			{
				trace("=====================No failures (" + testCount + ")=====================");
				trace("SUCCESS (" + (name == null ? "unnamed test" : name) + ")");
				if(visual != null && visual.graphics != null)
				{
					visual.graphics.beginFill(0x00FF00);
					visual.graphics.drawRect(0,0,500,30); //Top
					visual.graphics.drawRect(0,375-30,500,375); //Bottom
					visual.graphics.endFill();
				}
			}
			if(dontQuit != true)
			{
				terminate();
			}
		}
		public static function terminate():void
		{
			fscommand("quit");
		}
	}
}
