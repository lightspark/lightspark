/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013 Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/display/BitmapData.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/geom/flashgeom.h"
#include "backends/rendering_context.h"

using namespace lightspark;
using namespace std;

BitmapData::BitmapData(Class_base* c):ASObject(c),pixels(_MR(new BitmapContainer(c->memoryAccount))),transparent(true)
{
}

BitmapData::BitmapData(Class_base* c, _R<BitmapContainer> b):ASObject(c),pixels(b),transparent(true)
{
}

BitmapData::BitmapData(Class_base* c, const BitmapData& other)
 : ASObject(c),pixels(other.pixels),transparent(other.transparent)
{
}

BitmapData::BitmapData(Class_base* c, uint32_t width, uint32_t height)
 : ASObject(c),pixels(_MR(new BitmapContainer(c->memoryAccount))),transparent(true)
{
	uint32_t *pixelArray=new uint32_t[width*height];
	if (width!=0 && height!=0)
	{
		memset(pixelArray,0,width*height*sizeof(uint32_t));
		pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	}
}

void BitmapData::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass());
	c->setDeclaredMethodByQName("draw","",Class<IFunction>::getFunction(draw),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel","",Class<IFunction>::getFunction(getPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel32","",Class<IFunction>::getFunction(getPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel","",Class<IFunction>::getFunction(setPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel32","",Class<IFunction>::getFunction(setPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyPixels","",Class<IFunction>::getFunction(copyPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fillRect","",Class<IFunction>::getFunction(fillRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("generateFilterRect","",Class<IFunction>::getFunction(generateFilterRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scroll","",Class<IFunction>::getFunction(scroll),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);

	// properties
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rect","",Class<IFunction>::getFunction(getRect),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	REGISTER_GETTER(c,transparent);

	IBitmapDrawable::linkTraits(c);
}

void BitmapData::addUser(Bitmap* b)
{
	users.insert(b);
}

void BitmapData::removeUser(Bitmap* b)
{
	users.erase(b);
}

void BitmapData::notifyUsers() const
{
	for(auto it=users.begin();it!=users.end();it++)
		(*it)->updatedData();
}

ASFUNCTIONBODY(BitmapData,_constructor)
{
	uint32_t width;
	uint32_t height;
	bool transparent;
	uint32_t fillColor;
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	ARG_UNPACK(width, 0)(height, 0)(transparent, true)(fillColor, 0xFFFFFFFF);

	ASObject::_constructor(obj,NULL,0);
	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->pixels->isEmpty())
		return NULL;

	uint32_t *pixelArray=new uint32_t[width*height];
	uint32_t c=GUINT32_TO_BE(fillColor); // fromRGB expects big endian data
	if(!transparent)
	{
		uint8_t *alpha=reinterpret_cast<uint8_t *>(&c);
		*alpha=0xFF;
	}
	for(uint32_t i=0; i<width*height; i++)
		pixelArray[i]=c;
	th->pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	th->transparent=transparent;

	return NULL;
}

ASFUNCTIONBODY_GETTER(BitmapData, transparent);

ASFUNCTIONBODY(BitmapData,dispose)
{
	BitmapData* th = obj->as<BitmapData>();
	th->pixels.reset();
	th->notifyUsers();
	return NULL;
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix)
{
	//Create an InvalidateQueue to store all the hierarchy of objects that must be drawn
	SoftwareInvalidateQueue queue;
	d->requestInvalidation(&queue);
	CairoRenderContext ctxt(pixels->getData(), pixels->getWidth(), pixels->getHeight());
	for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
	{
		DisplayObject* target=(*it).getPtr();
		//Get the drawable from each of the added objects
		IDrawable* drawable=target->invalidate(d, initialMatrix);
		if(drawable==NULL)
			continue;

		//Compute the matrix for this object
		uint8_t* buf=drawable->getPixelBuffer();
		//Construct a CachedSurface using the data
		CachedSurface& surface=ctxt.allocateCustomSurface(target,buf);
		surface.tex.width=drawable->getWidth();
		surface.tex.height=drawable->getHeight();
		surface.xOffset=drawable->getXOffset();
		surface.yOffset=drawable->getYOffset();
		delete drawable;
	}
	d->Render(ctxt);
}

ASFUNCTIONBODY(BitmapData,draw)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	_NR<ASString> blendMode;
	_NR<Rectangle> clipRect;
	bool smoothing;
	ARG_UNPACK (drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, NullRef)
					(clipRect, NullRef) (smoothing, false);

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass()) )
		throwError<TypeError>(kCheckTypeFailedError, 
				      drawable->getClassName(),
				      "IBitmapDrawable");

	if(!ctransform.isNull() || !blendMode.isNull() || !clipRect.isNull() || smoothing)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support many parameters");

	if(drawable->is<BitmapData>())
	{
		BitmapData* data=drawable->as<BitmapData>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		CairoRenderContext ctxt(th->pixels->getData(), th->pixels->getWidth(), th->pixels->getHeight());
		//Blit the data while transforming it
		ctxt.transformedBlit(initialMatrix, data->pixels->getData(),
				data->pixels->getWidth(), data->pixels->getHeight(),
				CairoRenderContext::FILTER_NONE);
	}
	else if(drawable->is<DisplayObject>())
	{
		DisplayObject* d=drawable->as<DisplayObject>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		th->drawDisplayObject(d, initialMatrix);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support " << drawable->toDebugString());

	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,getPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y);
	return abstract_ui(pix & 0xffffff);
}

ASFUNCTIONBODY(BitmapData,getPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y);
	return abstract_ui(pix);
}

ASFUNCTIONBODY(BitmapData,setPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);

	th->pixels->setPixel(x, y, color, false);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,setPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);

	th->pixels->setPixel(x, y, color, th->transparent);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,getRect)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	return rect;
}

ASFUNCTIONBODY(BitmapData,_getHeight)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	return abstract_i(th->getHeight());
}

ASFUNCTIONBODY(BitmapData,_getWidth)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	return abstract_i(th->getWidth());
}

ASFUNCTIONBODY(BitmapData,fillRect)
{
	BitmapData* th=obj->as<BitmapData>();
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_UNPACK(rect)(color);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	th->pixels->fillRectangle(rect->x, rect->y, rect->width, rect->height, color, th->transparent);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,copyPixels)
{
	BitmapData* th=obj->as<BitmapData>();
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapData> alphaBitmapData;
	_NR<Point> alphaPoint;
	bool mergeAlpha;
	ARG_UNPACK(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	if(!alphaBitmapData.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "BitmapData.copyPixels doesn't support alpha bitmap");

	th->pixels->copyRectangle(source->pixels, sourceRect->x, sourceRect->y,
			destPoint->getX(), destPoint->getY(), sourceRect->width,
			sourceRect->height, mergeAlpha);
	th->notifyUsers();

	return NULL;
}

ASFUNCTIONBODY(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	return rect;
}

ASFUNCTIONBODY(BitmapData,hitTest)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	_NR<Point> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_UNPACK (firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1);

	if(!secondObject->getClass() || !secondObject->getClass()->isSubClass(Class<Point>::getClass()))
		throwError<TypeError>(kCheckTypeFailedError, 
				      secondObject->getClassName(),
				      "Point");

	if(!secondBitmapDataPoint.isNull() || secondAlphaThreshold!=1)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest does not expect some parameters");

	Point* secondPoint = secondObject->as<Point>();

	uint32_t pix=th->pixels->getPixel(secondPoint->getX()-firstPoint->getX(), secondPoint->getY()-firstPoint->getY());
	if((pix>>24)>=firstAlphaThreshold)
		return abstract_b(true);
	else
		return abstract_b(false);
}

ASFUNCTIONBODY(BitmapData,scroll)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	int x;
	int y;
	ARG_UNPACK (x) (y);

	if (th->pixels->scroll(x, y))
		th->notifyUsers();

	return NULL;
}

ASFUNCTIONBODY(BitmapData,clone)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	return Class<BitmapData>::getInstanceS(*th);
}
