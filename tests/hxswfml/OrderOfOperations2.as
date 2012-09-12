package {
	import Tests;
	import flash.display.*;
	import flash.events.*;
	import OrderOfOperationsChild;
	
	public class OrderOfOperations2 extends MovieClip {
		public function OrderOfOperations2() {
			var s:OrderOfOperationsChild = new OrderOfOperationsChild(stage);
			Tests.report(null, this.name);
		}
	}
}
