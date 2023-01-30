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
class AVM1Color;

class AVM1MovieClip: public MovieClip
{
private:
	// this is only needed to make this movieclip the owner of the color (avoids circular references if color is also set as variable)
	_NR<AVM1Color> color;
public:
	AVM1MovieClip(ASWorker* wrk,Class_base* c):MovieClip(wrk,c){}
	AVM1MovieClip(ASWorker* wrk,Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID,uint32_t nameID=BUILTIN_STRINGS::EMPTY):MovieClip(wrk,c,f,defineSpriteTagID) 
	{
		name=nameID;
	}
	void afterConstruction() override;
	bool destruct() override;
	void prepareShutdown() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(startDrag);
	ASFUNCTION_ATOM(stopDrag);
	ASFUNCTION_ATOM(attachAudio);
	void setColor(AVM1Color* c);
};

class AVM1Shape: public Shape
{
public:
	AVM1Shape(ASWorker* wrk,Class_base* c):Shape(wrk,c){}
	static void sinit(Class_base* c);
};

class AVM1SimpleButton: public SimpleButton
{
public:
	AVM1SimpleButton(ASWorker* wrk,Class_base* c, DisplayObject *dS = nullptr, DisplayObject *hTS = nullptr,
				 DisplayObject *oS = nullptr, DisplayObject *uS = nullptr, DefineButtonTag* tag = nullptr)
		:SimpleButton(wrk,c,dS,hTS,oS,uS,tag) {}
	
	static void sinit(Class_base* c);
};
class AVM1Stage: public Stage
{
public:
	AVM1Stage(ASWorker* wrk,Class_base* c):Stage(wrk,c) {}
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
	AVM1MovieClipLoader(ASWorker* wrk,Class_base* c):Loader(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(loadClip);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
	bool destruct() override;
	void load(const tiny_string& url, const tiny_string& method, AVM1MovieClip* target);
};

class AVM1Color: public ASObject
{
	AVM1MovieClip* target;
public:
	AVM1Color(ASWorker* wrk,Class_base* c):ASObject(wrk,c),target(nullptr) {}
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
	AVM1Broadcaster(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(initialize);
	ASFUNCTION_ATOM(broadcastMessage);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
};

class AVM1BitmapData: public BitmapData
{
public:
	AVM1BitmapData(ASWorker* wrk,Class_base* c):BitmapData(wrk,c) {}
	AVM1BitmapData(ASWorker* wrk,Class_base* c, _R<BitmapContainer> b):BitmapData(wrk,c,b) {}
	AVM1BitmapData(ASWorker* wrk,Class_base* c, const BitmapData& other):BitmapData(wrk,c,other) {}
	AVM1BitmapData(ASWorker* wrk,Class_base* c, uint32_t width, uint32_t height):BitmapData(wrk,c,width,height) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(loadBitmap);
};

class AVM1Bitmap: public Bitmap
{
public:
	AVM1Bitmap(ASWorker* wrk,Class_base* c, _NR<LoaderInfo> li=NullRef, std::istream *s = nullptr, FILE_TYPE type=FT_UNKNOWN):Bitmap(wrk,c,li,s,type) {}
	AVM1Bitmap(ASWorker* wrk,Class_base* c, _R<AVM1BitmapData> data):Bitmap(wrk,c,_R<BitmapData>(data)) {}
	static void sinit(Class_base* c);
};


}

#endif // SCRIPTING_AVM1_AVM1DISPLAY_H
