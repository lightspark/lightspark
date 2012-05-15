package
{

import flash.events.IEventDispatcher;
import flash.events.Event;
import flash.events.EventDispatcher;

public class TestDispatcher implements IEventDispatcher
{
    public var dispatcher:EventDispatcher;
               
    public function TestDispatcher() {
        dispatcher = new EventDispatcher(this);
    }
           
    public function addEventListener(type:String, listener:Function, useCapture:Boolean = false, priority:int = 0, useWeakReference:Boolean = false):void{
        dispatcher.addEventListener(type, listener, useCapture, priority);
    }
           
    public function dispatchEvent(evt:Event):Boolean{
        return dispatcher.dispatchEvent(evt);
    }
    
    public function hasEventListener(type:String):Boolean{
        return dispatcher.hasEventListener(type);
    }
    
    public function removeEventListener(type:String, listener:Function, useCapture:Boolean = false):void{
        dispatcher.removeEventListener(type, listener, useCapture);
    }
                   
    public function willTrigger(type:String):Boolean {
        return dispatcher.willTrigger(type);
    }
}

}
