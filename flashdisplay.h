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
#include "flashutils.h"
#include "thread_pool.h"

class RootMovieClip;
class DisplayListTag;
class LoaderInfo;

class IDisplayListElem: public EventDispatcher
{
public:
	int Depth;
	UI16 CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;
	IDisplayListElem():root(NULL){}
	RootMovieClip* root;
	virtual void Render()=0;
};

class DisplayObject: public IDisplayListElem
{
protected:
	intptr_t width;
	intptr_t height;
	number_t rotation;
	LoaderInfo* loaderInfo;
public:
	DisplayObject();
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getVisible);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_getRotation);
	ASFUNCTION(_getName);
	ASFUNCTION(_getRoot);
	ASFUNCTION(_getBlendMode);
	ASFUNCTION(_getScale9Grid);
	ASFUNCTION(_setRotation);
	ASFUNCTION(_setName);
	ASFUNCTION(localToGlobal);
	void Render()
	{
		abort();
	}
};

class DisplayObjectContainer: public DisplayObject
{
public:
	DisplayObjectContainer();
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getNumChildren);
};

class Shape: public DisplayObject
{
public:
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class LoaderInfo: public EventDispatcher
{
public:
	LoaderInfo()
	{
	}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(_getLoaderUrl);
	ASFUNCTION(_getUrl);
};

class Loader: public IThreadJob, public DisplayObjectContainer
{
private:
	enum SOURCE { URL, BYTES };
	SOURCE source;
	tiny_string url;
	ByteArray* bytes;
	bool loading;
	bool loaded;
	RootMovieClip* local_root;
	LoaderInfo* contentLoaderInfo;
public:
	Loader():loading(false),local_root(NULL),loaded(false)
	{
	}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	void execute();
	int getDepth() const
	{
		return 0;
	}
	void Render();
};

class Sprite: public DisplayObjectContainer
{
protected:
	intptr_t _x;
	intptr_t _y;
public:
	Sprite();
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(getBounds);
	ASFUNCTION(getX);
	ASFUNCTION(getY);
	ASFUNCTION(_getParent);
	int getDepth() const
	{
		return 0;
	}
	void Render()
	{
		LOG(NOT_IMPLEMENTED,"Sprite Rendering");
	}
};

class MovieClip: public Sprite
{
friend class ParseThread;
protected:
	int _framesloaded;
	int totalFrames;
	std::list < IDisplayListElem* > dynamicDisplayList;
	std::list<std::pair<PlaceInfo, IDisplayListElem*> > displayList;
	Frame cur_frame;
	bool initialized;
public:
	std::vector<Frame> frames;
	RunState state;

public:
	void initialize();
	MovieClip();
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(moveTo);
	ASFUNCTION(lineStyle);
	ASFUNCTION(lineTo);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);
	ASFUNCTION(addFrameScript);
	ASFUNCTION(addChild);
	ASFUNCTION(stop);
	ASFUNCTION(_getCurrentFrame);
	ASFUNCTION(_getTotalFrames);

	virtual void addToFrame(DisplayListTag* r);

	//IRenderObject interface
	void Render();
};

#endif
