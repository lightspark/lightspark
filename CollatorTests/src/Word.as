package 
{
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Word 
	{
		var firstWord:String;
		var secondWord:String;
		var expectedResult1:int;
		var expectedResult2:Boolean;
		var ignoreCase:Boolean;
		var ignoreSymbols:Boolean;
		
		public function Word(
			firstWord:String,
			secondWord:String,
			expectedResult1:int,
			expectedResult2:Boolean,
			ignoreCase:Boolean,
			ignoreSymbols:Boolean)
		{
			this.firstWord = firstWord;
			this.secondWord = secondWord;
			this.expectedResult1 = expectedResult1;
			this.expectedResult2 = expectedResult2;
			this.ignoreCase = ignoreCase;
			this.ignoreSymbols = ignoreSymbols;
		}
	}

}