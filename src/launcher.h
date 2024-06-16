/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Alessandro Pignotti (a.pignotti@sssup.it)

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


#ifndef LAUNCHER_H
#define LAUNCHER_H 1

#include "tiny_string.h"

class SDL_Window;
namespace lightspark
{

class DLL_PUBLIC Launcher
{
public:
	Launcher();
	tiny_string selectedfile;
	tiny_string baseurl;
	bool needsAIR;
	bool needsAVMplus;
	bool needsnetwork;
	bool needsfilesystem;
	bool start();
	static void setWindowIcon(SDL_Window* window);
};

}

#endif /* LAUNCHER_H */
