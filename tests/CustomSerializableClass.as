package
{
import flash.utils.IExternalizable;
import flash.utils.IDataOutput;
import flash.utils.IDataInput;
public class CustomSerializableClass implements IExternalizable
{
	public var ok:Boolean = false;
	public function writeExternal(output:IDataOutput):void
	{
		output.writeUTF("customstuff");
	}
	public function readExternal(input:IDataInput):void
	{
		var s:String=input.readUTF();
		if(s=="customstuff")
			ok = true;
	}
}
}
