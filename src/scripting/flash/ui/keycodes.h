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

#ifndef SCRIPTING_FLASH_UI_KEYCODES_H
#define SCRIPTING_FLASH_UI_KEYCODES_H 1

#if GTK_CHECK_VERSION (2,21,8)
#include <gdk/gdkkeysyms-compat.h> 
#else
#include <gdk/gdkkeysyms.h>
#endif 

namespace lightspark {

struct ASNameForGDKKey {
	const char *keyname;
	unsigned gdkKeyval;
};

const ASNameForGDKKey hardwareKeycodes[] = \
  {{"A", GDK_a},
   {"ALTERNATE", GDK_Alt_L},
   {"B", GDK_b},
   {"BACK", GDK_Back},
   {"BACKQUOTE", GDK_grave},
   {"BACKSLASH", GDK_backslash},
   {"BACKSPACE", GDK_BackSpace},
   {"BLUE", GDK_Blue},
   {"C", GDK_c},
   {"CAPS_LOCK", GDK_Caps_Lock},
   {"COMMA", GDK_comma},
   {"CONTROL", GDK_Control_L},
   {"D", GDK_d},
   {"DELETE", GDK_Delete},
   {"DOWN", GDK_Down},
   {"E", GDK_e},
   {"END", GDK_End},
   {"ENTER", GDK_Return},
   {"EQUAL", GDK_equal},
   {"ESCAPE", GDK_Escape},
   {"F", GDK_f},
   {"F1", GDK_F1},
   {"F2", GDK_F2},
   {"F3", GDK_F3},
   {"F4", GDK_F4},
   {"F5", GDK_F5},
   {"F6", GDK_F6},
   {"F7", GDK_F7},
   {"F8", GDK_F8},
   {"F9", GDK_F9},
   {"F10", GDK_F10},
   {"F11", GDK_F11},
   {"F12", GDK_F12},
   {"F13", GDK_F13},
   {"F14", GDK_F14},
   {"F15", GDK_F15},
   {"G", GDK_g},
   {"GREEN", GDK_Green},
   {"H", GDK_h},
   {"HELP", GDK_Help},
   {"HOME", GDK_Home},
   {"I", GDK_i},
   {"INSERT", GDK_Insert},
   {"J", GDK_j},
   {"K", GDK_k},
   {"L", GDK_l},
   {"LEFT", GDK_Left},
   {"LEFTBRACKET", GDK_bracketleft},
   {"M", GDK_m},
   {"MINUS", GDK_minus},
   {"N", GDK_n},
   {"NUMBER_0", GDK_0},
   {"NUMBER_1", GDK_1},
   {"NUMBER_2", GDK_2},
   {"NUMBER_3", GDK_3},
   {"NUMBER_4", GDK_4},
   {"NUMBER_5", GDK_5},
   {"NUMBER_6", GDK_6},
   {"NUMBER_7", GDK_7},
   {"NUMBER_8", GDK_8},
   {"NUMBER_9", GDK_9},
   {"NUMPAD_0", GDK_KP_0},
   {"NUMPAD_1", GDK_KP_1},
   {"NUMPAD_2", GDK_KP_2},
   {"NUMPAD_3", GDK_KP_3},
   {"NUMPAD_4", GDK_KP_4},
   {"NUMPAD_5", GDK_KP_5},
   {"NUMPAD_6", GDK_KP_6},
   {"NUMPAD_7", GDK_KP_7},
   {"NUMPAD_8", GDK_KP_8},
   {"NUMPAD_9", GDK_KP_9},
   {"NUMPAD_ADD", GDK_KP_Add},
   {"NUMPAD_DECIMAL", GDK_KP_Separator},
   {"NUMPAD_DIVIDE", GDK_KP_Divide},
   {"NUMPAD_ENTER", GDK_KP_Enter},
   {"NUMPAD_MULTIPLY", GDK_KP_Multiply},
   {"NUMPAD_SUBTRACT", GDK_KP_Subtract},
   {"O", GDK_o},
   {"P", GDK_p},
   {"PAGE_DOWN", GDK_Page_Down},
   {"PAGE_UP", GDK_Page_Up},
   {"PAUSE", GDK_Pause},
   {"PERIOD", GDK_period},
   {"Q", GDK_q},
   {"QUOTE", GDK_quoteright},
   {"R", GDK_r},
   {"RED", GDK_Red},
   {"RIGHT", GDK_Right},
   {"RIGHTBRACKET", GDK_bracketright},
   {"S", GDK_s},
   {"SEARCH", GDK_Search},
   {"SEMICOLON", GDK_semicolon},
   {"SHIFT", GDK_Shift_L},
   {"SLASH", GDK_slash},
   {"SPACE", GDK_space},
   {"SUBTITLE", GDK_Subtitle},
   {"T", GDK_t},
   {"TAB", GDK_Tab},
   {"U", GDK_u},
   {"UP", GDK_Up},
   {"V", GDK_v},
   {"W", GDK_w},
   {"X", GDK_x},
   {"Y", GDK_y},
   {"YELLOW", GDK_Yellow},
   {"Z", GDK_z},
   {NULL, 0}};
};

// AUDIO
// CHANNEL_DOWN
// CHANNEL_UP
// COMMAND
// DVR
// EXIT
// FAST_FORWARD
// GUIDE
// INFO
// INPUT
// LAST
// LIVE
// MASTER_SHELL
// MENU
// NEXT
// NUMPAD
// PLAY
// PREVIOUS
// RECORD
// REWIND
// SETUP
// SKIP_BACKWARD
// SKIP_FORWARD
// STOP
// VOD

#endif /* SCRIPTING_FLASH_UI_KEYCODES_H */
