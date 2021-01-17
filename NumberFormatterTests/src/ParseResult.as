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
		
		public function ParseResult(input:String, negativeNumberFormat:int, expectedResult:Number, expectedStartIndex:int, expectedEndIndex:int) 
		{
			this.input = input;
			this.negativeNumberFormat = negativeNumberFormat;
			this.expectedResult = expectedResult;
			this.expectedStartIndex = expectedStartIndex;
			this.expectedEndIndex = expectedEndIndex;
		}
	}
}