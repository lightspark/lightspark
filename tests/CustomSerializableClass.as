package
{
public class CustomSerializableClass implement IExternalizable
{
	var ok:Boolean = true;
	public function writeExternal(output:IDataOutput)
	{
		output.writeUTF("customstuff");
	}
	public function readExternal(input:IDataInput)
	{
		var s:String=input.readUTF();
		trace(s);
		if(s!="customstuff)
			ok = false;
	}
}
