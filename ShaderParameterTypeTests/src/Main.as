package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.display.ShaderParameterType;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (ShaderParameterType.BOOL == "bool")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.BOOL2 == "bool2")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.BOOL3 == "bool3")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.BOOL4 == "bool4")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.FLOAT == "float")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.FLOAT2 == "float2")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.FLOAT3 == "float3")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.FLOAT4 == "float4")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.INT == "int")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.INT2 == "int2")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.INT3 == "int3")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.INT4 == "int4")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.MATRIX2X2 == "matrix2x2")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.MATRIX3X3 == "matrix3x3")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (ShaderParameterType.MATRIX4X4 == "matrix4x4")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		}
		
	}
	
}