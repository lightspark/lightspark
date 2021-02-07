/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_AVM1_AVM1DISPLAY_H
#define SCRIPTING_AVM1_AVM1DISPLAY_H


#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/display/BitmapData.h"

namespace lightspark
{

class AVM1MovieClip: public MovieClip
{
public:
	AVM1MovieClip(Class_base* c):MovieClip(c){}
	AVM1MovieClip(Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID,uint32_t nameID=BUILTIN_STRINGS::EMPTY):MovieClip(c,f,defineSpriteTagID) 
	{
		name=nameID;
	}
	void afterConstruction();
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(startDrag);
	ASFUNCTION_ATOM(stopDrag);
};

class AVM1Shape: public Shape
{
public:
	AVM1Shape(Class_base* c):Shape(c){}
	AVM1Shape(Class_base* c, float scaling, DefineShapeTag* tag):Shape(c,scaling,tag) {}
	static void sinit(Class_base* c);
};

class AVM1SimpleButton: public SimpleButton
{
public:
	AVM1SimpleButton(Class_base* c, DisplayObject *dS = nullptr, DisplayObject *hTS = nullptr,
				 DisplayObject *oS = nullptr, DisplayObject *uS = nullptr, DefineButtonTag* tag = nullptr)
		:SimpleButton(c,dS,hTS,oS,uS,tag) {}
	
	static void sinit(Class_base* c);
};
class AVM1Stage: public Stage
{
public:
	AVM1Stage(Class_base* c):Stage(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getDisplayState);
	ASFUNCTION_ATOM(_setDisplayState);
	ASFUNCTION_ATOM(_getShowMenu);
	ASFUNCTION_ATOM(_setShowMenu);
	ASFUNCTION_ATOM(getAlign);
	ASFUNCTION_ATOM(setAlign);
	ASFUNCTION_ATOM(addResizeListener);
	ASFUNCTION_ATOM(removeResizeListener);
};

class AVM1MovieClipLoader: public Loader
{
private:
	mutable Mutex spinlock;
	std::list<IThreadJob *> jobs;
	std::set<_R<ASObject> > listeners;
public:
	AVM1MovieClipLoader(Class_base* c):Loader(c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(loadClip);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
	bool destruct() override;
};

class AVM1Color: public ASObject
{
	_NR<DisplayObject> target;
public:
	AVM1Color(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(getRGB);
	ASFUNCTION_ATOM(setRGB);
	ASFUNCTION_ATOM(getTransform);
	ASFUNCTION_ATOM(setTransform);
};

class AVM1Broadcaster: public ASObject
{
public:
	AVM1Broadcaster(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(initialize);
	ASFUNCTION_ATOM(broadcastMessage);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
};

class AVM1Bitmap: public Bitmap
{
public:
	AVM1Bitmap(Class_base* c, _NR<LoaderInfo> li=NullRef, std::istream *s = nullptr, FILE_TYPE type=FT_UNKNOWN):Bitmap(c,li,s,type) {}
	AVM1Bitmap(Class_base* c, _R<BitmapData> data):Bitmap(c,data) {}
	static void sinit(Class_base* c);
};

}

#endif // SCRIPTING_AVM1_AVM1DISPLAY_H
