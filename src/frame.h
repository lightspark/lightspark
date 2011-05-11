/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FRAME_H
#define _FRAME_H

#include "compat.h"
#include <list>
#include "swftypes.h"
#include "smartrefs.h"

namespace lightspark
{

class DisplayListTag;
class ControlTag;
class DisplayObject;
class MovieClip;

class PlaceInfo
{
public:
	uint32_t Depth;
	MATRIX Matrix;
	PlaceInfo(uint32_t d):Depth(d){}
};

class Frame
{
private:
	bool initialized;
	bool invalid;
	ACQUIRE_RELEASE_FLAG(constructed);
public:
	typedef std::list<std::pair<PlaceInfo, _R<DisplayObject> > > DisplayListType;
	Frame(const Frame& r);
	Frame(const Frame&& r);
	Frame& operator=(const Frame& r);
	std::list<DisplayListTag*> blueprint;
	DisplayListType displayList;
	//A temporary vector for control tags
	std::vector < ControlTag* > controls;
	Frame():initialized(false),invalid(true),constructed(false){}
	void Render(bool maskEnabled);
	void init(_R<MovieClip> parent, const DisplayListType& d);
	void construct(_R<MovieClip> parent);
	bool isInitialized() const { return initialized; }
	bool isInvalid() const { return invalid; }
	void setInvalid(bool i) { invalid=i; }
	bool isConstructed() const { return ACQUIRE_READ(constructed); }
	void setConstructed() { RELEASE_WRITE(constructed,true); }
};
};

#endif
