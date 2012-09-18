package {
import flash.display.*;
import flash.external.*;
public class external_ExternalInterface_test extends Sprite {
	internal function returnTest(value:*):*
	{
		return function(... args):* { return value; }
	}
	public function external_ExternalInterface_test() {
		ExternalInterface.call("alert", "hello");
		ExternalInterface.addCallback("returnCompositeTest", returnTest(["string", 3.14, 271, true,  {a : "str key", 2 : "int key"}]));
		ExternalInterface.addCallback("returnTrueTest", returnTest(true));
		ExternalInterface.addCallback("returnFalseTest", returnTest(false));
		ExternalInterface.addCallback("returnNullTest", returnTest(null));
		ExternalInterface.addCallback("returnUndefinedTest", returnTest(undefined));
		ExternalInterface.addCallback("returnNumberTest", returnTest(3.1415));
		ExternalInterface.addCallback("returnIntegerTest", returnTest(271));
		ExternalInterface.addCallback("returnStringTest", returnTest("hello world"));
		ExternalInterface.addCallback("returnArgumentsTest", 
			function(... args):* { return args; });
		ExternalInterface.addCallback("returnCyclicTest", 
			function(... args):* { return args[0]["recursive"]; });

		ExternalInterface.addCallback("nested1Test", 
			function(... args):*
			{
				return ExternalInterface.call("nested1Test");
			}
		);
		ExternalInterface.addCallback("nested2Test", 
			function(... args):*
			{
				return ExternalInterface.call("nested2Test");
			}
		);
		ExternalInterface.addCallback("nested3Test", 
			function(... args):*
			{
				return "You reached level 3";
			}
		);
		ExternalInterface.addCallback("unsupportedCall1Test", 
			function(... args):*
			{
				return ExternalInterface.call("rawJSTest('hello')");
			}
		);
		ExternalInterface.addCallback("unsupportedCall2Test", 
			function(... args):*
			{
				return ExternalInterface.call("function() { return 'hello'; }");
			}
		);
		// This is actually unwanted behaviour, but we can't get around to it.
		ExternalInterface.addCallback("unsupportedCall3Test", 
			function(... args):*
			{
				return ExternalInterface.call("function() { return function() { return 'hello'; }; }");
			}
		);
		ExternalInterface.addCallback("undefinedMethodTest",
			function(... args):*
			{
				return ExternalInterface.call("idontexist");
			}
		);
		ExternalInterface.addCallback("jsExceptionTest",
			function(... args):*
			{
				try { return ExternalInterface.call("jsExceptionTest"); }
				catch(e:*) { return e; }
			}
		);

		trace('Ext avilable' + ExternalInterface.available)
		ExternalInterface.call("ready");
		trace('Ext ready signalled')

		ExternalInterface.call("foo.bar1");
		ExternalInterface.call("foo.bar2()");
		ExternalInterface.call("function() { alert('Executed eval');}");
		ExternalInterface.call("alert('Complex thing'), function() { alert('Inside complex thing');}");
		ExternalInterface.call("alert","Passing arguments");
		ExternalInterface.call("alert",true);
		ExternalInterface.call("alert",2.4);

		var arrayRet:Object=ExternalInterface.call("testArray");
		ExternalInterface.call("trace",arrayRet.toString());

		var obj:Object = new Object();
		obj["prop1"] = "val1";
		obj["prop2"] = true;
		obj["prop3"] = 2.4;

		ExternalInterface.call("alert",obj);
	}
}
}
