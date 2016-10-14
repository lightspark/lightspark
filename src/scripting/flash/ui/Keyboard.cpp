/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/flash/ui/Keyboard.h"
#include "scripting/class.h"
#include "scripting/toplevel/ASString.h"
#include "backends/input.h"
#include "scripting/flash/ui/keycodes.h"

using namespace std;
using namespace lightspark;

void Keyboard::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("capsLock","",Class<IFunction>::getFunction(c->getSystemState(),capsLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hasVirtualKeyboard","",Class<IFunction>::getFunction(c->getSystemState(),hasVirtualKeyboard),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numLock","",Class<IFunction>::getFunction(c->getSystemState(),numLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("physicalKeyboardType","",Class<IFunction>::getFunction(c->getSystemState(),physicalKeyboardType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("isAccessible","",Class<IFunction>::getFunction(c->getSystemState(),isAccessible),NORMAL_METHOD,true);

	c->setVariableByQName("A","",abstract_ui(c->getSystemState(),AS3KEYCODE_A ),CONSTANT_TRAIT);
	c->setVariableByQName("ALTERNATE","",abstract_ui(c->getSystemState(),AS3KEYCODE_ALTERNATE),CONSTANT_TRAIT);
	c->setVariableByQName("AUDIO","",abstract_ui(c->getSystemState(),AS3KEYCODE_AUDIO),CONSTANT_TRAIT);
	c->setVariableByQName("B","",abstract_ui(c->getSystemState(),AS3KEYCODE_B),CONSTANT_TRAIT);
	c->setVariableByQName("BACK","",abstract_ui(c->getSystemState(),AS3KEYCODE_BACK),CONSTANT_TRAIT);
	c->setVariableByQName("BACKQUOTE","",abstract_ui(c->getSystemState(),AS3KEYCODE_BACKQUOTE),CONSTANT_TRAIT);
	c->setVariableByQName("BACKSLASH","",abstract_ui(c->getSystemState(),AS3KEYCODE_BACKSLASH),CONSTANT_TRAIT);
	c->setVariableByQName("BACKSPACE","",abstract_ui(c->getSystemState(),AS3KEYCODE_BACKSPACE),CONSTANT_TRAIT);
	c->setVariableByQName("BLUE","",abstract_ui(c->getSystemState(),AS3KEYCODE_BLUE),CONSTANT_TRAIT);
	c->setVariableByQName("C","",abstract_ui(c->getSystemState(),AS3KEYCODE_C),CONSTANT_TRAIT);
	c->setVariableByQName("CAPS_LOCK","",abstract_ui(c->getSystemState(),AS3KEYCODE_CAPS_LOCK),CONSTANT_TRAIT);
	c->setVariableByQName("CHANNEL_DOWN","",abstract_ui(c->getSystemState(),AS3KEYCODE_CHANNEL_DOWN),CONSTANT_TRAIT);
	c->setVariableByQName("CHANNEL_UP","",abstract_ui(c->getSystemState(),AS3KEYCODE_CHANNEL_UP),CONSTANT_TRAIT);
	c->setVariableByQName("COMMA","",abstract_ui(c->getSystemState(),AS3KEYCODE_COMMA),CONSTANT_TRAIT);
	c->setVariableByQName("COMMAND","",abstract_ui(c->getSystemState(),AS3KEYCODE_COMMAND),CONSTANT_TRAIT);
	c->setVariableByQName("CONTROL","",abstract_ui(c->getSystemState(),AS3KEYCODE_CONTROL),CONSTANT_TRAIT);
	c->setVariableByQName("D","",abstract_ui(c->getSystemState(),AS3KEYCODE_D),CONSTANT_TRAIT);
	c->setVariableByQName("DELETE","",abstract_ui(c->getSystemState(),AS3KEYCODE_DELETE),CONSTANT_TRAIT);
	c->setVariableByQName("DOWN","",abstract_ui(c->getSystemState(),AS3KEYCODE_DOWN),CONSTANT_TRAIT);
	c->setVariableByQName("DVR","",abstract_ui(c->getSystemState(),AS3KEYCODE_DVR),CONSTANT_TRAIT);
	c->setVariableByQName("E","",abstract_ui(c->getSystemState(),AS3KEYCODE_E),CONSTANT_TRAIT);
	c->setVariableByQName("END","",abstract_ui(c->getSystemState(),AS3KEYCODE_END),CONSTANT_TRAIT);
	c->setVariableByQName("ENTER","",abstract_ui(c->getSystemState(),AS3KEYCODE_ENTER),CONSTANT_TRAIT);
	c->setVariableByQName("EQUAL","",abstract_ui(c->getSystemState(),AS3KEYCODE_EQUAL),CONSTANT_TRAIT);
	c->setVariableByQName("ESCAPE","",abstract_ui(c->getSystemState(),AS3KEYCODE_ESCAPE),CONSTANT_TRAIT);
	c->setVariableByQName("EXIT","",abstract_ui(c->getSystemState(),AS3KEYCODE_EXIT),CONSTANT_TRAIT);
	c->setVariableByQName("F","",abstract_ui(c->getSystemState(),AS3KEYCODE_F),CONSTANT_TRAIT);
	c->setVariableByQName("F1","",abstract_ui(c->getSystemState(),AS3KEYCODE_F1),CONSTANT_TRAIT);
	c->setVariableByQName("F10","",abstract_ui(c->getSystemState(),AS3KEYCODE_F10),CONSTANT_TRAIT);
	c->setVariableByQName("F11","",abstract_ui(c->getSystemState(),AS3KEYCODE_F11),CONSTANT_TRAIT);
	c->setVariableByQName("F12","",abstract_ui(c->getSystemState(),AS3KEYCODE_F12),CONSTANT_TRAIT);
	c->setVariableByQName("F13","",abstract_ui(c->getSystemState(),AS3KEYCODE_F13),CONSTANT_TRAIT);
	c->setVariableByQName("F14","",abstract_ui(c->getSystemState(),AS3KEYCODE_F14),CONSTANT_TRAIT);
	c->setVariableByQName("F15","",abstract_ui(c->getSystemState(),AS3KEYCODE_F15),CONSTANT_TRAIT);
	c->setVariableByQName("F2","",abstract_ui(c->getSystemState(),AS3KEYCODE_F2),CONSTANT_TRAIT);
	c->setVariableByQName("F3","",abstract_ui(c->getSystemState(),AS3KEYCODE_F3),CONSTANT_TRAIT);
	c->setVariableByQName("F4","",abstract_ui(c->getSystemState(),AS3KEYCODE_F4),CONSTANT_TRAIT);
	c->setVariableByQName("F5","",abstract_ui(c->getSystemState(),AS3KEYCODE_F5),CONSTANT_TRAIT);
	c->setVariableByQName("F6","",abstract_ui(c->getSystemState(),AS3KEYCODE_F6),CONSTANT_TRAIT);
	c->setVariableByQName("F7","",abstract_ui(c->getSystemState(),AS3KEYCODE_F7),CONSTANT_TRAIT);
	c->setVariableByQName("F8","",abstract_ui(c->getSystemState(),AS3KEYCODE_F8),CONSTANT_TRAIT);
	c->setVariableByQName("F9","",abstract_ui(c->getSystemState(),AS3KEYCODE_F9),CONSTANT_TRAIT);
	c->setVariableByQName("FAST_FORWARD","",abstract_ui(c->getSystemState(),AS3KEYCODE_FAST_FORWARD),CONSTANT_TRAIT);
	c->setVariableByQName("G","",abstract_ui(c->getSystemState(),AS3KEYCODE_G),CONSTANT_TRAIT);
	c->setVariableByQName("GREEN","",abstract_ui(c->getSystemState(),AS3KEYCODE_GREEN),CONSTANT_TRAIT);
	c->setVariableByQName("GUIDE","",abstract_ui(c->getSystemState(),AS3KEYCODE_GUIDE),CONSTANT_TRAIT);
	c->setVariableByQName("H","",abstract_ui(c->getSystemState(),AS3KEYCODE_H),CONSTANT_TRAIT);
	c->setVariableByQName("HELP","",abstract_ui(c->getSystemState(),AS3KEYCODE_HELP),CONSTANT_TRAIT);
	c->setVariableByQName("HOME","",abstract_ui(c->getSystemState(),AS3KEYCODE_HOME),CONSTANT_TRAIT);
	c->setVariableByQName("I","",abstract_ui(c->getSystemState(),AS3KEYCODE_I),CONSTANT_TRAIT);
	c->setVariableByQName("INFO","",abstract_ui(c->getSystemState(),AS3KEYCODE_INFO),CONSTANT_TRAIT);
	c->setVariableByQName("INPUT","",abstract_ui(c->getSystemState(),AS3KEYCODE_INPUT),CONSTANT_TRAIT);
	c->setVariableByQName("INSERT","",abstract_ui(c->getSystemState(),AS3KEYCODE_INSERT),CONSTANT_TRAIT);
	c->setVariableByQName("J","",abstract_ui(c->getSystemState(),AS3KEYCODE_J),CONSTANT_TRAIT);
	c->setVariableByQName("K","",abstract_ui(c->getSystemState(),AS3KEYCODE_K),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_BEGIN","",abstract_s(c->getSystemState(),"Begin"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_BREAK","",abstract_s(c->getSystemState(),"Break"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_CLEARDISPLAY","",abstract_s(c->getSystemState(),"ClrDsp"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_CLEARLINE","",abstract_s(c->getSystemState(),"ClrLn"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_DELETE","",abstract_s(c->getSystemState(),"Delete"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_DELETECHAR","",abstract_s(c->getSystemState(),"DelChr"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_DELETELINE","",abstract_s(c->getSystemState(),"DelLn"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_DOWNARROW","",abstract_s(c->getSystemState(),"Down"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_END","",abstract_s(c->getSystemState(),"End"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_EXECUTE","",abstract_s(c->getSystemState(),"Exec"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F1","",abstract_s(c->getSystemState(),"F1"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F10","",abstract_s(c->getSystemState(),"F10"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F11","",abstract_s(c->getSystemState(),"F11"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F12","",abstract_s(c->getSystemState(),"F12"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F13","",abstract_s(c->getSystemState(),"F13"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F14","",abstract_s(c->getSystemState(),"F14"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F15","",abstract_s(c->getSystemState(),"F15"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F16","",abstract_s(c->getSystemState(),"F16"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F17","",abstract_s(c->getSystemState(),"F17"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F18","",abstract_s(c->getSystemState(),"F18"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F19","",abstract_s(c->getSystemState(),"F19"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F2","",abstract_s(c->getSystemState(),"F2"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F20","",abstract_s(c->getSystemState(),"F20"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F21","",abstract_s(c->getSystemState(),"F21"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F22","",abstract_s(c->getSystemState(),"F22"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F23","",abstract_s(c->getSystemState(),"F23"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F24","",abstract_s(c->getSystemState(),"F24"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F25","",abstract_s(c->getSystemState(),"F25"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F26","",abstract_s(c->getSystemState(),"F26"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F27","",abstract_s(c->getSystemState(),"F27"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F28","",abstract_s(c->getSystemState(),"F28"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F29","",abstract_s(c->getSystemState(),"F29"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F3","",abstract_s(c->getSystemState(),"F3"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F30","",abstract_s(c->getSystemState(),"F30"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F31","",abstract_s(c->getSystemState(),"F31"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F32","",abstract_s(c->getSystemState(),"F32"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F33","",abstract_s(c->getSystemState(),"F33"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F34","",abstract_s(c->getSystemState(),"F34"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F35","",abstract_s(c->getSystemState(),"F35"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F4","",abstract_s(c->getSystemState(),"F4"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F5","",abstract_s(c->getSystemState(),"F5"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F6","",abstract_s(c->getSystemState(),"F6"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F7","",abstract_s(c->getSystemState(),"F7"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F8","",abstract_s(c->getSystemState(),"F8"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_F9","",abstract_s(c->getSystemState(),"F9"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_FIND","",abstract_s(c->getSystemState(),"Find"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_HELP","",abstract_s(c->getSystemState(),"Help"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_HOME","",abstract_s(c->getSystemState(),"Home"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_INSERT","",abstract_s(c->getSystemState(),"Insert"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_INSERTCHAR","",abstract_s(c->getSystemState(),"InsChr"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_INSERTLINE","",abstract_s(c->getSystemState(),"InsLn"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_LEFTARROW","",abstract_s(c->getSystemState(),"Left"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_MENU","",abstract_s(c->getSystemState(),"Menu"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_MODESWITCH","",abstract_s(c->getSystemState(),"ModeSw"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_NEXT","",abstract_s(c->getSystemState(),"Next"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PAGEDOWN","",abstract_s(c->getSystemState(),"PgDn"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PAGEUP","",abstract_s(c->getSystemState(),"PgUp"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PAUSE","",abstract_s(c->getSystemState(),"Pause"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PREV","",abstract_s(c->getSystemState(),"Prev"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PRINT","",abstract_s(c->getSystemState(),"Print"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_PRINTSCREEN","",abstract_s(c->getSystemState(),"PrntScrn"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_REDO","",abstract_s(c->getSystemState(),"Redo"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_RESET","",abstract_s(c->getSystemState(),"Reset"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_RIGHTARROW","",abstract_s(c->getSystemState(),"Right"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_SCROLLLOCK","",abstract_s(c->getSystemState(),"ScrlLck"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_SELECT","",abstract_s(c->getSystemState(),"Select"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_STOP","",abstract_s(c->getSystemState(),"Stop"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_SYSREQ","",abstract_s(c->getSystemState(),"SysReq"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_SYSTEM","",abstract_s(c->getSystemState(),"Sys"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_UNDO","",abstract_s(c->getSystemState(),"Undo"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_UPARROW","",abstract_s(c->getSystemState(),"Up"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYNAME_USER","",abstract_s(c->getSystemState(),"User"),CONSTANT_TRAIT);
	c->setVariableByQName("L","",abstract_ui(c->getSystemState(),AS3KEYCODE_L),CONSTANT_TRAIT);
	c->setVariableByQName("LAST","",abstract_ui(c->getSystemState(),AS3KEYCODE_LAST),CONSTANT_TRAIT);
	c->setVariableByQName("LEFT","",abstract_ui(c->getSystemState(),AS3KEYCODE_LEFT),CONSTANT_TRAIT);
	c->setVariableByQName("LEFTBRACKET","",abstract_ui(c->getSystemState(),AS3KEYCODE_LEFTBRACKET),CONSTANT_TRAIT);
	c->setVariableByQName("LIVE","",abstract_ui(c->getSystemState(),AS3KEYCODE_LIVE),CONSTANT_TRAIT);
	c->setVariableByQName("M","",abstract_ui(c->getSystemState(),AS3KEYCODE_M),CONSTANT_TRAIT);
	c->setVariableByQName("MASTER_SHELL","",abstract_ui(c->getSystemState(),AS3KEYCODE_MASTER_SHELL),CONSTANT_TRAIT);
	c->setVariableByQName("MENU","",abstract_ui(c->getSystemState(),AS3KEYCODE_MENU),CONSTANT_TRAIT);
	c->setVariableByQName("MINUS","",abstract_ui(c->getSystemState(),AS3KEYCODE_MINUS),CONSTANT_TRAIT);
	c->setVariableByQName("N","",abstract_ui(c->getSystemState(),AS3KEYCODE_N),CONSTANT_TRAIT);
	c->setVariableByQName("NEXT","",abstract_ui(c->getSystemState(),AS3KEYCODE_NEXT),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_0","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_0),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_1","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_1),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_2","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_2),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_3","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_3),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_4","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_4),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_5","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_5),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_6","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_6),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_7","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_7),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_8","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_8),CONSTANT_TRAIT);
	c->setVariableByQName("NUMBER_9","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMBER_9),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_0","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_0),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_1","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_1),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_2","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_2),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_3","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_3),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_4","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_4),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_5","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_5),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_6","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_6),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_7","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_7),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_8","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_8),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_9","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_9),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_ADD","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_ADD),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_DECIMAL","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_DECIMAL),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_DIVIDE","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_DIVIDE),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_ENTER","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_ENTER),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_MULTIPLY","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_MULTIPLY),CONSTANT_TRAIT);
	c->setVariableByQName("NUMPAD_SUBTRACT","",abstract_ui(c->getSystemState(),AS3KEYCODE_NUMPAD_SUBTRACT),CONSTANT_TRAIT);
	c->setVariableByQName("O","",abstract_ui(c->getSystemState(),AS3KEYCODE_O),CONSTANT_TRAIT);
	c->setVariableByQName("P","",abstract_ui(c->getSystemState(),AS3KEYCODE_P),CONSTANT_TRAIT);
	c->setVariableByQName("PAGE_DOWN","",abstract_ui(c->getSystemState(),AS3KEYCODE_PAGE_DOWN),CONSTANT_TRAIT);
	c->setVariableByQName("PAGE_UP","",abstract_ui(c->getSystemState(),AS3KEYCODE_PAGE_UP),CONSTANT_TRAIT);
	c->setVariableByQName("PAUSE","",abstract_ui(c->getSystemState(),AS3KEYCODE_PAUSE),CONSTANT_TRAIT);
	c->setVariableByQName("PERIOD","",abstract_ui(c->getSystemState(),AS3KEYCODE_PERIOD),CONSTANT_TRAIT);
	c->setVariableByQName("PLAY","",abstract_ui(c->getSystemState(),AS3KEYCODE_PLAY),CONSTANT_TRAIT);
	c->setVariableByQName("PREVIOUS","",abstract_ui(c->getSystemState(),AS3KEYCODE_PREVIOUS),CONSTANT_TRAIT);
	c->setVariableByQName("Q","",abstract_ui(c->getSystemState(),AS3KEYCODE_Q),CONSTANT_TRAIT);
	c->setVariableByQName("QUOTE","",abstract_ui(c->getSystemState(),AS3KEYCODE_QUOTE),CONSTANT_TRAIT);
	c->setVariableByQName("R","",abstract_ui(c->getSystemState(),AS3KEYCODE_R),CONSTANT_TRAIT);
	c->setVariableByQName("RECORD","",abstract_ui(c->getSystemState(),AS3KEYCODE_RECORD),CONSTANT_TRAIT);
	c->setVariableByQName("RED","",abstract_ui(c->getSystemState(),AS3KEYCODE_RED),CONSTANT_TRAIT);
	c->setVariableByQName("REWIND","",abstract_ui(c->getSystemState(),AS3KEYCODE_REWIND),CONSTANT_TRAIT);
	c->setVariableByQName("RIGHT","",abstract_ui(c->getSystemState(),AS3KEYCODE_RIGHT),CONSTANT_TRAIT);
	c->setVariableByQName("RIGHTBRACKET","",abstract_ui(c->getSystemState(),AS3KEYCODE_RIGHTBRACKET),CONSTANT_TRAIT);
	c->setVariableByQName("S","",abstract_ui(c->getSystemState(),AS3KEYCODE_S),CONSTANT_TRAIT);
	c->setVariableByQName("SEARCH","",abstract_ui(c->getSystemState(),AS3KEYCODE_SEARCH),CONSTANT_TRAIT);
	c->setVariableByQName("SEMICOLON","",abstract_ui(c->getSystemState(),AS3KEYCODE_SEMICOLON),CONSTANT_TRAIT);
	c->setVariableByQName("SETUP","",abstract_ui(c->getSystemState(),AS3KEYCODE_SETUP),CONSTANT_TRAIT);
	c->setVariableByQName("SHIFT","",abstract_ui(c->getSystemState(),AS3KEYCODE_SHIFT),CONSTANT_TRAIT);
	c->setVariableByQName("SKIP_BACKWARD","",abstract_ui(c->getSystemState(),AS3KEYCODE_SKIP_BACKWARD),CONSTANT_TRAIT);
	c->setVariableByQName("SKIP_FORWARD","",abstract_ui(c->getSystemState(),AS3KEYCODE_SKIP_FORWARD),CONSTANT_TRAIT);
	c->setVariableByQName("SLASH","",abstract_ui(c->getSystemState(),AS3KEYCODE_SLASH),CONSTANT_TRAIT);
	c->setVariableByQName("SPACE","",abstract_ui(c->getSystemState(),AS3KEYCODE_SPACE),CONSTANT_TRAIT);
	c->setVariableByQName("STOP","",abstract_ui(c->getSystemState(),AS3KEYCODE_STOP),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_BEGIN","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_BREAK","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_CLEARDISPLAY","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_CLEARLINE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_DELETE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_DELETECHAR","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_DELETELINE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_DOWNARROW","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_END","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_EXECUTE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F1","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F10","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F11","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F12","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F13","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F14","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F15","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F16","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F17","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F18","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F19","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F2","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F20","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F21","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F22","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F23","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F24","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F25","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F26","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F27","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F28","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F29","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F3","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F30","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F31","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F32","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F33","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F34","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F35","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F4","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F5","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F6","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F7","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F8","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_F9","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_FIND","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_HELP","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_HOME","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_INSERT","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_INSERTCHAR","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_INSERTLINE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_LEFTARROW","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_MENU","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_MODESWITCH","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_NEXT","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PAGEDOWN","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PAGEUP","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PAUSE","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PREV","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PRINT","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_PRINTSCREEN","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_REDO","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_RESET","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_RIGHTARROW","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_SCROLLLOCK","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_SELECT","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_STOP","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_SYSREQ","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_SYSTEM","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_UNDO","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_UPARROW","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("STRING_USER","",abstract_s(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableByQName("SUBTITLE","",abstract_ui(c->getSystemState(),AS3KEYCODE_SUBTITLE),CONSTANT_TRAIT);
	c->setVariableByQName("T","",abstract_ui(c->getSystemState(),AS3KEYCODE_T),CONSTANT_TRAIT);
	c->setVariableByQName("TAB","",abstract_ui(c->getSystemState(),AS3KEYCODE_TAB),CONSTANT_TRAIT);
	c->setVariableByQName("U","",abstract_ui(c->getSystemState(),AS3KEYCODE_U),CONSTANT_TRAIT);
	c->setVariableByQName("UP","",abstract_ui(c->getSystemState(),AS3KEYCODE_UP),CONSTANT_TRAIT);
	c->setVariableByQName("V","",abstract_ui(c->getSystemState(),AS3KEYCODE_V),CONSTANT_TRAIT);
	c->setVariableByQName("VOD","",abstract_ui(c->getSystemState(),AS3KEYCODE_VOD),CONSTANT_TRAIT);
	c->setVariableByQName("W","",abstract_ui(c->getSystemState(),AS3KEYCODE_W),CONSTANT_TRAIT);
	c->setVariableByQName("X","",abstract_ui(c->getSystemState(),AS3KEYCODE_X),CONSTANT_TRAIT);
	c->setVariableByQName("Y","",abstract_ui(c->getSystemState(),AS3KEYCODE_Y),CONSTANT_TRAIT);
	c->setVariableByQName("YELLOW","",abstract_ui(c->getSystemState(),AS3KEYCODE_YELLOW),CONSTANT_TRAIT);
	c->setVariableByQName("Z","",abstract_ui(c->getSystemState(),AS3KEYCODE_Z),CONSTANT_TRAIT);
}

ASFUNCTIONBODY(Keyboard, capsLock)
{
	// gdk functions can be called only from the input thread!
	//return abstract_b(gdk_keymap_get_caps_lock_state(gdk_keymap_get_default()));

	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::capsLock");
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, hasVirtualKeyboard)
{
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, numLock)
{
	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::numLock");
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, physicalKeyboardType)
{
	return abstract_s(getSys(),"alphanumeric");
}

ASFUNCTIONBODY(Keyboard, isAccessible)
{
	return abstract_b(getSys(),false);
}

void KeyboardType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ALPHANUMERIC","",abstract_s(c->getSystemState(),"alphanumeric"),CONSTANT_TRAIT);
	c->setVariableByQName("KEYPAD","",abstract_s(c->getSystemState(),"keypad"),CONSTANT_TRAIT);
	c->setVariableByQName("NONE","",abstract_s(c->getSystemState(),"none"),CONSTANT_TRAIT);
}

void KeyLocation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("LEFT","",abstract_ui(c->getSystemState(),1),CONSTANT_TRAIT);
	c->setVariableByQName("NUM_PAD","",abstract_ui(c->getSystemState(),3),CONSTANT_TRAIT);
	c->setVariableByQName("RIGHT","",abstract_ui(c->getSystemState(),2),CONSTANT_TRAIT);
	c->setVariableByQName("STANDARD","",abstract_ui(c->getSystemState(),0),CONSTANT_TRAIT);
}
