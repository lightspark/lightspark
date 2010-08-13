package
{

import flash.utils.Proxy;
import flash.utils.flash_proxy;

public class TestProxy extends Proxy
{
	public var data:int = 0;
	override flash_proxy function getProperty(name:*):*
	{
		trace("getProperty get existent "+data);
		
		if(name=="data2")
			return 1;
		else
			return 2;
	}
}

}
