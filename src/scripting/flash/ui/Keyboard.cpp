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

	c->setVariableAtomByQName("A",nsNameAndKind(),asAtom(AS3KEYCODE_A ),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALTERNATE",nsNameAndKind(),asAtom(AS3KEYCODE_ALTERNATE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUDIO",nsNameAndKind(),asAtom(AS3KEYCODE_AUDIO),CONSTANT_TRAIT);
	c->setVariableAtomByQName("B",nsNameAndKind(),asAtom(AS3KEYCODE_B),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BACK",nsNameAndKind(),asAtom(AS3KEYCODE_BACK),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BACKQUOTE",nsNameAndKind(),asAtom(AS3KEYCODE_BACKQUOTE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BACKSLASH",nsNameAndKind(),asAtom(AS3KEYCODE_BACKSLASH),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BACKSPACE",nsNameAndKind(),asAtom(AS3KEYCODE_BACKSPACE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BLUE",nsNameAndKind(),asAtom(AS3KEYCODE_BLUE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("C",nsNameAndKind(),asAtom(AS3KEYCODE_C),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CAPS_LOCK",nsNameAndKind(),asAtom(AS3KEYCODE_CAPS_LOCK),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHANNEL_DOWN",nsNameAndKind(),asAtom(AS3KEYCODE_CHANNEL_DOWN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHANNEL_UP",nsNameAndKind(),asAtom(AS3KEYCODE_CHANNEL_UP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMMA",nsNameAndKind(),asAtom(AS3KEYCODE_COMMA),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMMAND",nsNameAndKind(),asAtom(AS3KEYCODE_COMMAND),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CONTROL",nsNameAndKind(),asAtom(AS3KEYCODE_CONTROL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("D",nsNameAndKind(),asAtom(AS3KEYCODE_D),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DELETE",nsNameAndKind(),asAtom(AS3KEYCODE_DELETE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DOWN",nsNameAndKind(),asAtom(AS3KEYCODE_DOWN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DVR",nsNameAndKind(),asAtom(AS3KEYCODE_DVR),CONSTANT_TRAIT);
	c->setVariableAtomByQName("E",nsNameAndKind(),asAtom(AS3KEYCODE_E),CONSTANT_TRAIT);
	c->setVariableAtomByQName("END",nsNameAndKind(),asAtom(AS3KEYCODE_END),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ENTER",nsNameAndKind(),asAtom(AS3KEYCODE_ENTER),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EQUAL",nsNameAndKind(),asAtom(AS3KEYCODE_EQUAL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ESCAPE",nsNameAndKind(),asAtom(AS3KEYCODE_ESCAPE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EXIT",nsNameAndKind(),asAtom(AS3KEYCODE_EXIT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F",nsNameAndKind(),asAtom(AS3KEYCODE_F),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F1",nsNameAndKind(),asAtom(AS3KEYCODE_F1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F10",nsNameAndKind(),asAtom(AS3KEYCODE_F10),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F11",nsNameAndKind(),asAtom(AS3KEYCODE_F11),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F12",nsNameAndKind(),asAtom(AS3KEYCODE_F12),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F13",nsNameAndKind(),asAtom(AS3KEYCODE_F13),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F14",nsNameAndKind(),asAtom(AS3KEYCODE_F14),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F15",nsNameAndKind(),asAtom(AS3KEYCODE_F15),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F2",nsNameAndKind(),asAtom(AS3KEYCODE_F2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F3",nsNameAndKind(),asAtom(AS3KEYCODE_F3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F4",nsNameAndKind(),asAtom(AS3KEYCODE_F4),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F5",nsNameAndKind(),asAtom(AS3KEYCODE_F5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F6",nsNameAndKind(),asAtom(AS3KEYCODE_F6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F7",nsNameAndKind(),asAtom(AS3KEYCODE_F7),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F8",nsNameAndKind(),asAtom(AS3KEYCODE_F8),CONSTANT_TRAIT);
	c->setVariableAtomByQName("F9",nsNameAndKind(),asAtom(AS3KEYCODE_F9),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FAST_FORWARD",nsNameAndKind(),asAtom(AS3KEYCODE_FAST_FORWARD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("G",nsNameAndKind(),asAtom(AS3KEYCODE_G),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREEN",nsNameAndKind(),asAtom(AS3KEYCODE_GREEN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GUIDE",nsNameAndKind(),asAtom(AS3KEYCODE_GUIDE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("H",nsNameAndKind(),asAtom(AS3KEYCODE_H),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HELP",nsNameAndKind(),asAtom(AS3KEYCODE_HELP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HOME",nsNameAndKind(),asAtom(AS3KEYCODE_HOME),CONSTANT_TRAIT);
	c->setVariableAtomByQName("I",nsNameAndKind(),asAtom(AS3KEYCODE_I),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INFO",nsNameAndKind(),asAtom(AS3KEYCODE_INFO),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INPUT",nsNameAndKind(),asAtom(AS3KEYCODE_INPUT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INSERT",nsNameAndKind(),asAtom(AS3KEYCODE_INSERT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("J",nsNameAndKind(),asAtom(AS3KEYCODE_J),CONSTANT_TRAIT);
	c->setVariableAtomByQName("K",nsNameAndKind(),asAtom(AS3KEYCODE_K),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_BEGIN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Begin"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_BREAK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Break"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_CLEARDISPLAY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ClrDsp"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_CLEARLINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ClrLn"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_DELETE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Delete"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_DELETECHAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"DelChr"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_DELETELINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"DelLn"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_DOWNARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Down"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_END",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"End"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_EXECUTE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Exec"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F1",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F10",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F10"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F11",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F11"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F12",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F12"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F13",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F13"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F14",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F14"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F15",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F15"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F16",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F16"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F17",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F17"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F18",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F18"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F19",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F19"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F2",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F20",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F20"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F21",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F21"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F22",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F22"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F23",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F23"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F24",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F24"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F25",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F25"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F26",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F26"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F27",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F27"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F28",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F28"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F29",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F29"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F3",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F30",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F30"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F31",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F31"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F32",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F32"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F33",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F33"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F34",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F34"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F35",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F35"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F4",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F5",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F5"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F6",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F6"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F7",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F7"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F8",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F8"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_F9",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"F9"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_FIND",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Find"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_HELP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Help"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_HOME",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Home"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_INSERT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Insert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_INSERTCHAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"InsChr"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_INSERTLINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"InsLn"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_LEFTARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Left"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_MENU",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Menu"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_MODESWITCH",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ModeSw"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_NEXT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Next"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PAGEDOWN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"PgDn"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PAGEUP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"PgUp"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PAUSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Pause"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PREV",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Prev"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PRINT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Print"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_PRINTSCREEN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"PrntScrn"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_REDO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Redo"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_RESET",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Reset"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_RIGHTARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Right"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_SCROLLLOCK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ScrlLck"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_SELECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Select"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_STOP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Stop"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_SYSREQ",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"SysReq"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_SYSTEM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Sys"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_UNDO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Undo"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_UPARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"Up"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYNAME_USER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"User"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("L",nsNameAndKind(),asAtom(AS3KEYCODE_L),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LAST",nsNameAndKind(),asAtom(AS3KEYCODE_LAST),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtom(AS3KEYCODE_LEFT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFTBRACKET",nsNameAndKind(),asAtom(AS3KEYCODE_LEFTBRACKET),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIVE",nsNameAndKind(),asAtom(AS3KEYCODE_LIVE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("M",nsNameAndKind(),asAtom(AS3KEYCODE_M),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MASTER_SHELL",nsNameAndKind(),asAtom(AS3KEYCODE_MASTER_SHELL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MENU",nsNameAndKind(),asAtom(AS3KEYCODE_MENU),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MINUS",nsNameAndKind(),asAtom(AS3KEYCODE_MINUS),CONSTANT_TRAIT);
	c->setVariableAtomByQName("N",nsNameAndKind(),asAtom(AS3KEYCODE_N),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEXT",nsNameAndKind(),asAtom(AS3KEYCODE_NEXT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_0",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_1",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_2",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_3",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_4",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_4),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_5",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_6",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_7",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_7),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_8",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_8),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_9",nsNameAndKind(),asAtom(AS3KEYCODE_NUMBER_9),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_0",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_1",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_2",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_3",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_4",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_4),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_5",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_6",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_7",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_7),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_8",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_8),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_9",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_9),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_ADD",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_ADD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_DECIMAL",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_DECIMAL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_DIVIDE",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_DIVIDE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_ENTER",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_ENTER),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_MULTIPLY",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_MULTIPLY),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMPAD_SUBTRACT",nsNameAndKind(),asAtom(AS3KEYCODE_NUMPAD_SUBTRACT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("O",nsNameAndKind(),asAtom(AS3KEYCODE_O),CONSTANT_TRAIT);
	c->setVariableAtomByQName("P",nsNameAndKind(),asAtom(AS3KEYCODE_P),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PAGE_DOWN",nsNameAndKind(),asAtom(AS3KEYCODE_PAGE_DOWN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PAGE_UP",nsNameAndKind(),asAtom(AS3KEYCODE_PAGE_UP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PAUSE",nsNameAndKind(),asAtom(AS3KEYCODE_PAUSE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PERIOD",nsNameAndKind(),asAtom(AS3KEYCODE_PERIOD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PLAY",nsNameAndKind(),asAtom(AS3KEYCODE_PLAY),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PREVIOUS",nsNameAndKind(),asAtom(AS3KEYCODE_PREVIOUS),CONSTANT_TRAIT);
	c->setVariableAtomByQName("Q",nsNameAndKind(),asAtom(AS3KEYCODE_Q),CONSTANT_TRAIT);
	c->setVariableAtomByQName("QUOTE",nsNameAndKind(),asAtom(AS3KEYCODE_QUOTE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("R",nsNameAndKind(),asAtom(AS3KEYCODE_R),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RECORD",nsNameAndKind(),asAtom(AS3KEYCODE_RECORD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RED",nsNameAndKind(),asAtom(AS3KEYCODE_RED),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REWIND",nsNameAndKind(),asAtom(AS3KEYCODE_REWIND),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtom(AS3KEYCODE_RIGHT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHTBRACKET",nsNameAndKind(),asAtom(AS3KEYCODE_RIGHTBRACKET),CONSTANT_TRAIT);
	c->setVariableAtomByQName("S",nsNameAndKind(),asAtom(AS3KEYCODE_S),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SEARCH",nsNameAndKind(),asAtom(AS3KEYCODE_SEARCH),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SEMICOLON",nsNameAndKind(),asAtom(AS3KEYCODE_SEMICOLON),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SETUP",nsNameAndKind(),asAtom(AS3KEYCODE_SETUP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHIFT",nsNameAndKind(),asAtom(AS3KEYCODE_SHIFT),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SKIP_BACKWARD",nsNameAndKind(),asAtom(AS3KEYCODE_SKIP_BACKWARD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SKIP_FORWARD",nsNameAndKind(),asAtom(AS3KEYCODE_SKIP_FORWARD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SLASH",nsNameAndKind(),asAtom(AS3KEYCODE_SLASH),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SPACE",nsNameAndKind(),asAtom(AS3KEYCODE_SPACE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STOP",nsNameAndKind(),asAtom(AS3KEYCODE_STOP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_BEGIN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_BREAK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_CLEARDISPLAY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_CLEARLINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_DELETE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_DELETECHAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_DELETELINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_DOWNARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_END",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_EXECUTE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F1",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F10",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F11",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F12",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F13",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F14",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F15",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F16",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F17",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F18",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F19",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F2",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F20",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F21",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F22",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F23",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F24",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F25",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F26",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F27",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F28",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F29",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F3",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F30",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F31",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F32",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F33",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F34",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F35",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F4",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F5",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F6",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F7",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F8",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_F9",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_FIND",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_HELP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_HOME",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_INSERT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_INSERTCHAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_INSERTLINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_LEFTARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_MENU",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_MODESWITCH",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_NEXT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PAGEDOWN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PAGEUP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PAUSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PREV",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PRINT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_PRINTSCREEN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_REDO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_RESET",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_RIGHTARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_SCROLLLOCK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_SELECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_STOP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_SYSREQ",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_SYSTEM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_UNDO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_UPARROW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STRING_USER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),""),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUBTITLE",nsNameAndKind(),asAtom(AS3KEYCODE_SUBTITLE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("T",nsNameAndKind(),asAtom(AS3KEYCODE_T),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TAB",nsNameAndKind(),asAtom(AS3KEYCODE_TAB),CONSTANT_TRAIT);
	c->setVariableAtomByQName("U",nsNameAndKind(),asAtom(AS3KEYCODE_U),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UP",nsNameAndKind(),asAtom(AS3KEYCODE_UP),CONSTANT_TRAIT);
	c->setVariableAtomByQName("V",nsNameAndKind(),asAtom(AS3KEYCODE_V),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VOD",nsNameAndKind(),asAtom(AS3KEYCODE_VOD),CONSTANT_TRAIT);
	c->setVariableAtomByQName("W",nsNameAndKind(),asAtom(AS3KEYCODE_W),CONSTANT_TRAIT);
	c->setVariableAtomByQName("X",nsNameAndKind(),asAtom(AS3KEYCODE_X),CONSTANT_TRAIT);
	c->setVariableAtomByQName("Y",nsNameAndKind(),asAtom(AS3KEYCODE_Y),CONSTANT_TRAIT);
	c->setVariableAtomByQName("YELLOW",nsNameAndKind(),asAtom(AS3KEYCODE_YELLOW),CONSTANT_TRAIT);
	c->setVariableAtomByQName("Z",nsNameAndKind(),asAtom(AS3KEYCODE_Z),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_ATOM(Keyboard, capsLock)
{
	SDL_Keymod mod = SDL_GetModState();
	return asAtom((mod & KMOD_CAPS) == KMOD_CAPS);
}

ASFUNCTIONBODY_ATOM(Keyboard, hasVirtualKeyboard)
{
	return asAtom::falseAtom;
}

ASFUNCTIONBODY_ATOM(Keyboard, numLock)
{
	SDL_Keymod mod = SDL_GetModState();
	return asAtom((mod & KMOD_NUM) == KMOD_NUM);
}

ASFUNCTIONBODY_ATOM(Keyboard, physicalKeyboardType)
{
	return asAtom::fromString(sys,"alphanumeric");
}

ASFUNCTIONBODY_ATOM(Keyboard, isAccessible)
{
	return asAtom::falseAtom;
}

void KeyboardType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ALPHANUMERIC",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"alphanumeric"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEYPAD",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"keypad"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}

void KeyLocation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtom(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUM_PAD",nsNameAndKind(),asAtom(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtom(2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD",nsNameAndKind(),asAtom(0),CONSTANT_TRAIT);
}
