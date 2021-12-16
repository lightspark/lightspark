package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.text.engine.FontDescription;
	import flash.text.engine.ElementFormat;
	import flash.text.engine.TextBlock;
	import flash.text.engine.TextElement;
	import flash.text.engine.TextLine;
	
	
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		public function tests()
		{
			// Test creating FontDescription object
			var fd1:FontDescription = new FontDescription("arial", flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC, flash.text.engine.FontLookup.EMBEDDED_CFF, flash.text.engine.RenderingMode.CFF, flash.text.engine.CFFHinting.HORIZONTAL_STEM);
			
			if (fd1.fontName == "arial" &&
				fd1.fontWeight == flash.text.engine.FontWeight.BOLD &&
				fd1.fontPosture == flash.text.engine.FontPosture.ITALIC &&
				fd1.fontLookup == flash.text.engine.FontLookup.EMBEDDED_CFF &&
				fd1.renderingMode == flash.text.engine.RenderingMode.CFF &&
				fd1.cffHinting == flash.text.engine.CFFHinting.HORIZONTAL_STEM &&
				fd1.locked == false)
			{
				trace("Passed");
			}
			else
			{
				trace("Fail");
				
				trace(fd1.fontName);
				trace(fd1.fontWeight);
				trace(fd1.fontPosture);
				trace(fd1.fontLookup);
				trace(fd1.renderingMode);
				trace(fd1.cffHinting);
				trace(fd1.locked);
			}
			
			var fd2:FontDescription = new FontDescription("arial", flash.text.engine.FontWeight.NORMAL, flash.text.engine.FontPosture.NORMAL, flash.text.engine.FontLookup.DEVICE, flash.text.engine.RenderingMode.NORMAL, flash.text.engine.CFFHinting.NONE);
			
			if (fd2.fontName == "arial" &&
				fd2.fontWeight == flash.text.engine.FontWeight.NORMAL &&
				fd2.fontPosture == flash.text.engine.FontPosture.NORMAL &&
				fd2.fontLookup == flash.text.engine.FontLookup.DEVICE &&
				fd2.renderingMode == flash.text.engine.RenderingMode.NORMAL &&
				fd2.cffHinting == flash.text.engine.CFFHinting.NONE &&
				fd2.locked == false)
			{
				trace("Passed");
			}
			else
			{
				trace("Fail");
				
				trace(fd2.fontName);
				trace(fd2.fontWeight);
				trace(fd2.fontPosture);
				trace(fd2.fontLookup);
				trace(fd2.renderingMode);
				trace(fd2.cffHinting);
				trace(fd2.locked);
			}
			
			
			
			
			// Test lock feature
			fd2.locked = true;
			
			try
			{
				fd2.fontName = "arial";
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd2.fontWeight = flash.text.engine.FontWeight.NORMAL;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd2.fontPosture = flash.text.engine.FontPosture.NORMAL;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd2.fontLookup = flash.text.engine.FontLookup.EMBEDDED_CFF;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd2.renderingMode = flash.text.engine.RenderingMode.NORMAL;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd2.cffHinting = flash.text.engine.CFFHinting.NONE;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			
			// Test unknown parameters
			
			var text:String = "unknown";
			
			try
			{
				fd1.fontWeight = text;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd1.fontPosture = text;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd1.fontLookup = text;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd1.renderingMode = text;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			try
			{
				fd1.cffHinting = text;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
			
			// Test lock
			try
			{
				fd2.locked = false;
				
				// Line below should not be reached
				trace("Failed");
			}
			catch (e:Error)
			{
				trace("Passed");
			}
		}
		
		public function Main() 
		{
			
			
			if (stage) init();
			else addEventListener(Event.ADDED_TO_STAGE, init);
			
			
            
            tests();
			
			
			var list:Array = new Array();
			list.push(new FontInfo("Palatino", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Palatino", 20, flash.text.engine.FontWeight.NORMAL, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Palatino", 10, flash.text.engine.FontWeight.NORMAL, flash.text.engine.FontPosture.NORMAL));
			
			list.push(new FontInfo("Arial", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Arial", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Arial", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Verdana", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Verdana", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Verdana", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Helvetica", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Helvetica", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Helvetica", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Tahoma", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Tahoma", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Tahoma", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Trebuchet MS", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Trebuchet MS", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Trebuchet MS", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Times New Roman", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Times New Roman", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Times New Roman", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Georgia", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Georgia", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Georgia", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			list.push(new FontInfo("Courier New", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Courier New", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			list.push(new FontInfo("Courier New", 10, flash.text.engine.FontWeight.BOLD, flash.text.engine.FontPosture.ITALIC));
			
			for (var i:int = 0; i < list.length; i++) 
			{
				var spante:Sprite = new Sprite();
				spante.x = 0;
				spante.y =  i*20;
				
				var fd:FontDescription = new FontDescription();
				//fd.fontLookup = flash.text.engine.FontLookup.DEVICE;
				fd.fontName = list[i].fontName;
				fd.fontWeight = list[i].fontWeight;
				fd.fontPosture = list[i].fontPosture;

				var ef1:ElementFormat = new ElementFormat(fd);
				ef1.fontSize = list[i].fontSize;
				ef1.color = 0xFF0000;
				
				var str:String = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in libero quis nulla efficitur pulvinar vitae eu sapien. ";
				var tb:TextBlock = new TextBlock();
				
				
				
				var te1:TextElement = new TextElement(str, ef1);
				tb.content = te1;
				
				var line1:TextLine = tb.createTextLine(null, 600);
				
				spante.addChild(line1);
				
				addChild(spante);
			}
			
			
		}
		
		private function init(e:Event = null):void 
		{
			removeEventListener(Event.ADDED_TO_STAGE, init);
			// entry point
		}
		
	}
	
}