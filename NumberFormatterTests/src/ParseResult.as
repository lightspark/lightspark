package 
{
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class ParseResult 
	{
		public var input:String;
		public var negativeNumberFormat:int;
		public var expectedResult:Number;
		public var expectedStartIndex:int;
		public var expectedEndIndex:int;
		public var index:int;
		
		public function ParseResult(index:int, input:String, negativeNumberFormat:int, expectedResult:Number, expectedStartIndex:int, expectedEndIndex:int) 
		{
			this.index = index;
			this.input = input;
			this.negativeNumberFormat = negativeNumberFormat;
			this.expectedResult = expectedResult;
			this.expectedStartIndex = expectedStartIndex;
			this.expectedEndIndex = expectedEndIndex;
		}
	}
}