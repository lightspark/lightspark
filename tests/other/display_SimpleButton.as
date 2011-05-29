package {
    import flash.display.Sprite;
    import flash.events.*;

    public class display_SimpleButton extends Sprite {
        public function display_SimpleButton() {
            var button:CustomSimpleButton = new CustomSimpleButton();
            button.x = 100;
            button.y = 20;
	    addChild(button);
	    trace("button.upState",button.upState.parent);
            trace(button.hitTestState.parent);
            trace(button.downState.parent);
            trace(button.overState.parent);
	    trace(button);
            button.addEventListener(MouseEvent.MOUSE_DOWN,handleEvent);
        }
	private function handleEvent(e:Event):void {
            trace('SimpleButton event', e.target, e);
	}
    }
}

import flash.events.*;
import flash.display.DisplayObject;
import flash.display.Sprite;
import flash.display.SimpleButton;

class CustomSimpleButton extends SimpleButton {
    private var upColor:uint   = 0xFFCC00;
    private var overColor:uint = 0xCCFF00;
    private var downColor:uint = 0x00CCFF;
    private var size:uint      = 80;

    public function CustomSimpleButton() {
        downState      = new ButtonDisplayState(downColor, size);
        overState      = new ButtonDisplayState(overColor, size);
        upState        = new ButtonDisplayState(upColor, size);
        hitTestState   = new ButtonDisplayState(upColor, size * 2);
        hitTestState.x = -(size / 4);
        hitTestState.y = hitTestState.x;
        useHandCursor  = true;
    }
}

class ButtonDisplayState extends Sprite {
    private var bgColor:uint;
    private var size:uint;

    public function ButtonDisplayState(bgColor:uint, size:uint) {
        this.bgColor = bgColor;
        this.size    = size;
	this.addEventListener(MouseEvent.MOUSE_DOWN, handleEvent);
        draw();
    }

    private function handleEvent(e:Event): void {
        trace('Mouse event in ButtonDisplayState - should never be called');
    }

    private function draw():void {
        graphics.beginFill(bgColor);
        graphics.drawRect(0, 0, size, size);
        graphics.endFill();
    }
}
