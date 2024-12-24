﻿package {
	public class Test {
	}
}

import flash.utils.Dictionary;

trace("///var a = new Dictionary()");
var a = new Dictionary();

trace("///a[\"key\"] = 5");
a["key"] = 5;

trace("///a[\"key\"]");
trace(a["key"]);

trace("///a[\"key\"] = 6");
a["key"] = 6;

trace("///var key2 = new Test()");
var key2 = new Test();

trace("///a[key2] = 23");
a[key2] = 23;

trace("///var key3 = new Test()");
var key3 = new Test();

trace('///a[key3] = "Key3 True Value"');
a[key3] = "Key3 True Value";

trace('///a["key3"] = "Key3 False Value"');
a["key3"] = "Key3 False Value";

trace("///var key4 = {\"toString\": function() { return \"key4\"; }}");
var key4 = {"toString": function() { return "key4"; }};

trace('///a[key4] = "Key4 True Value"');
a[key4] = "Key4 True Value";

trace('///a["key4"] = "Key4 False Value"');
a["key4"] = "Key4 False Value";

trace('///a[13] = "i\'ve been found!"');
a[13] = "i've been found!";

trace('///a["13"] = "no I haven\'t"');
a["13"] = "no I haven't";

trace('///a[1.123] = "this violates Rust!"');
a[1.123] = "this violates Rust!";

trace('///a["1.123"] = "this is perfectly acceptable"');
a["1.123"] = "this is perfectly acceptable";

trace('///a[undefined] = "oh no"');
a[undefined] = "oh no";

trace('///a["undefined"] = "uh huh..."');
a["undefined"] = "uh huh...";

trace('///a[null] = "oh YES!"');
a[null] = "oh YES!";

trace('///a["null"] = "yeah sure"');
a["null"] = "yeah sure";

trace('///a[true] = "true"');
a[true] = "true";

trace('///a["true"] = "stringy true"');
a["true"] = "stringy true";

trace('///a[false] = "false"');
a[false] = "false";

trace('///a["false"] = "stringy false"');
a["false"] = "stringy false";

trace('///"key" in a');
trace("key" in a);

trace('///key2 in a');
trace(key2 in a);

trace('///key3 in a');
trace(key3 in a);

trace('///"key3" in a');
trace("key3" in a);

trace('///key4 in a');
trace(key4 in a);

trace('///"key4" in a');
trace("key4" in a);

trace('///13 in a');
trace(13 in a);

trace('///1.123 in a');
trace(1.123 in a);

trace('///"1.123" in a');
trace("1.123" in a);

trace('///undefined in a');
trace(undefined in a);

trace('///"undefined" in a');
trace("undefined" in a);

trace('///null in a');
trace(null in a);

trace('///"null" in a');
trace("null" in a);

trace('///true in a');
trace(true in a);

trace('///"true" in a');
trace("true" in a);

trace('///false in a');
trace(false in a);

trace('///"false" in a');
trace("false" in a);

trace('///a[a] = a');
a[a] = a;

trace('///a in a');
trace(a in a);
