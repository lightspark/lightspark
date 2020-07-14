package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.geom.Orientation3D;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			
			if (Orientation3D.AXIS_ANGLE == "axisAngle")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Orientation3D.EULER_ANGLES == "eulerAngles")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (Orientation3D.QUATERNION == "quaternion")
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