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
		public function Main() 
		{
			
			
			if (stage) init();
			else addEventListener(Event.ADDED_TO_STAGE, init);
			
			
            
            
			
			
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