/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef _FLASH_DISPLAY_H
#define _FLASH_DISPLAY_H

#include "swftypes.h"
#include "flashevents.h"
#include "thread_pool.h"

class RootMovieClip;

class LoaderInfo: public EventDispatcher
{
public:
	LoaderInfo()
	{
		constructor=new Function(_constructor);
	}
	ASObject parameters;

	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
};

class Loader: public ASObject, public IThreadJob, public IDisplayListElem
{
private:
	std::string url;
	bool loading;
	bool loaded;
	RootMovieClip* local_root;
public:
	Loader():loading(false),local_root(NULL),loaded(false)
	{
		constructor=new Function(_constructor);
	}
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	void execute();
	int getDepth() const
	{
		return 0;
	}
	void Render();
	ISWFObject* clone()
	{
		return new Loader(*this);
	}
};

class Sprite: public EventDispatcher, public IDisplayListElem
{
protected:
	Integer _x;
	Integer _y;
	Integer _visible;
	Integer _width;
	Integer _height;
	Number rotation;
public:
	Sprite();
	ASFUNCTION(_constructor);
	ASFUNCTION(getBounds);
	ASFUNCTION(_getParent);
	int getDepth() const
	{
		return 0;
	}
	void Render()
	{
		LOG(NOT_IMPLEMENTED,"Sprite Rendering");
	}
	ISWFObject* clone()
	{
		return new Sprite(*this);
	}
};

class DisplayObject: public ASObject
{
public:
	DisplayObject();
	ASFUNCTION(_call);
};

class MovieClip: public Sprite, public IRenderObject
{
friend class ParseThread;
private:
	static bool list_orderer(const IDisplayListElem* a, int d);
protected:
	Integer _framesloaded;
	Integer _totalframes;
	std::list < IDisplayListElem* > dynamicDisplayList;
	std::list < IDisplayListElem* > displayList;
public:
	std::vector<Frame> frames;
	RunState state;

public:
	int displayListLimit;

	MovieClip();
	ASFUNCTION(_constructor);
	ASFUNCTION(moveTo);
	ASFUNCTION(lineStyle);
	ASFUNCTION(lineTo);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);
	ASFUNCTION(addFrameScript);
	ASFUNCTION(addChild);

	virtual void addToDisplayList(IDisplayListElem* r);

	//IRenderObject interface
	void Render();
	ISWFObject* clone()
	{
		return new MovieClip(*this);
	}
};

#endif
