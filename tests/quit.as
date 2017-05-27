package {

    // nonstandard extensions to ECMAScript
    public function print(...s)
    {
        trace(s)
    }

}
namespace flashsystemns = "flash.system";
flashsystemns::fscommand("quit");
