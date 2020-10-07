package 
{
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class LocaleItem
	{
		public var name:String;
		public var language:String;
		public var region:String;
		public var script:String;
		public var variant:String;
		public var isRightToLeft:Boolean;
		
		public function LocaleItem(
			name:String,
			language:String,
			region:String,
			script:String,
			variant:String,
			isRightToLeft:Boolean)
		{
			this.name = name;
			this.language = language;
			this.region = region;
			this.script = script;
			this.variant = variant;
			this.isRightToLeft = isRightToLeft;
		}
	}

}