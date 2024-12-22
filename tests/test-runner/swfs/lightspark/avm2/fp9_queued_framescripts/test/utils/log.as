package test.utils {
	import flash.utils.getQualifiedClassName;
	
	public function log(obj: Object, msg: String) {
		trace(getQualifiedClassName(obj) + ": " + msg);
	}
}
