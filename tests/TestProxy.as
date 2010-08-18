package
{

import flash.utils.Proxy;
import flash.utils.flash_proxy;

public class TestProxy extends Proxy
{
	public var data:int = 0;
	public function func():String
	{
		return "func";
	}
	override flash_proxy function getProperty(name:*):*
 	{
 		if(name=="data2")
			return 1;
		else
			return 2;
 	}
	override flash_proxy function setProperty(name:*, value:*):void
	{
		if(name=="data2")
			data=value;
	}
	override flash_proxy function callProperty(name:*, ... rest):*
	{
		if(name=="func2")
			return "func2";
		
		return "no func";
	}
	override flash_proxy function nextNameIndex (index:int):int
	{
		if(index<3)
			return index+1;
		return 0;
	}
	override flash_proxy function nextName(index:int):String
	{
		return String(10+index);
	}
}

}
