package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.globalization.LocaleID;
	
	import flash.globalization.Collator;
	import flash.globalization.CurrencyFormatter;
	import flash.globalization.DateTimeFormatter;
	import flash.globalization.NumberFormatter;
	import flash.globalization.StringTools;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			var localeNames:Array = ["", "en_GB", "en_AU", "en_CA", "en_DK", "en_IE", "en_IN", "en_NZ", "en_PH", "en_US", "en_ZA"];
            
            for ( var i:int = 0; i < localeNames.length; i++ ) 
            {
                var locID:LocaleID = new LocaleID( localeNames[i] as String );
                trace("name:     " + locID.name);
                trace("language: " + locID.getLanguage());
                trace("script:   " + locID.getScript());
                trace("region:   " + locID.getRegion());
                trace("variant:  " + locID.getVariant());
                trace("isRightToLeft: ", locID.isRightToLeft());
            }
		}
	}
}