package
{

import flash.utils.Proxy;
import flash.utils.flash_proxy;

public class TestProxy extends Proxy
{
	public var data:int = 0;
	override flash_proxy function setProperty(name:*, value:*):void
	{
		trace("setProperty exec");
		if(name=="data2")
			data=value;
	}
}

}
