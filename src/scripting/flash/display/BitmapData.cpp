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
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/filters/flashfilters.h"
#include "backends/rendering.h"
#include "3rdparty/perlinnoise/PerlinNoise.hpp"

#include <cstdlib> 

using namespace lightspark;
using namespace std;

BitmapData::BitmapData(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),transparent(true)
{
}

BitmapData::BitmapData(Class_base* c, _R<BitmapContainer> b):ASObject(c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(b),locked(0),transparent(true)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

BitmapData::BitmapData(Class_base* c, const BitmapData& other)
  : ASObject(c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(other.pixels),locked(other.locked),transparent(other.transparent)
{
	traitsInitialized = other.traitsInitialized;
	constructIndicator = other.constructIndicator;
	constructorCallComplete = other.constructorCallComplete;
}

BitmapData::BitmapData(Class_base* c, uint32_t width, uint32_t height)
 : ASObject(c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),transparent(true)
{
	if (width!=0 && height!=0)
	{
		uint32_t *pixelArray=new uint32_t[width*height];
		memset(pixelArray,0,width*height*sizeof(uint32_t));
		pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	}
}

void BitmapData::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass(c->getSystemState()));
	c->setDeclaredMethodByQName("draw","",Class<IFunction>::getFunction(c->getSystemState(),draw),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel","",Class<IFunction>::getFunction(c->getSystemState(),getPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel32","",Class<IFunction>::getFunction(c->getSystemState(),getPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel","",Class<IFunction>::getFunction(c->getSystemState(),setPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel32","",Class<IFunction>::getFunction(c->getSystemState(),setPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyPixels","",Class<IFunction>::getFunction(c->getSystemState(),copyPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fillRect","",Class<IFunction>::getFunction(c->getSystemState(),fillRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("generateFilterRect","",Class<IFunction>::getFunction(c->getSystemState(),generateFilterRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(c->getSystemState(),hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scroll","",Class<IFunction>::getFunction(c->getSystemState(),scroll),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyChannel","",Class<IFunction>::getFunction(c->getSystemState(),copyChannel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lock","",Class<IFunction>::getFunction(c->getSystemState(),lock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unlock","",Class<IFunction>::getFunction(c->getSystemState(),unlock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("floodFill","",Class<IFunction>::getFunction(c->getSystemState(),floodFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("histogram","",Class<IFunction>::getFunction(c->getSystemState(),histogram),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getColorBoundsRect","",Class<IFunction>::getFunction(c->getSystemState(),getColorBoundsRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixels","",Class<IFunction>::getFunction(c->getSystemState(),getPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVector","",Class<IFunction>::getFunction(c->getSystemState(),getVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixels","",Class<IFunction>::getFunction(c->getSystemState(),setPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVector","",Class<IFunction>::getFunction(c->getSystemState(),setVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(c->getSystemState(),colorTransform),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("compare","",Class<IFunction>::getFunction(c->getSystemState(),compare),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("applyFilter","",Class<IFunction>::getFunction(c->getSystemState(),applyFilter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("noise","",Class<IFunction>::getFunction(c->getSystemState(),noise),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("perlinNoise","",Class<IFunction>::getFunction(c->getSystemState(),perlinNoise),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("threshold","",Class<IFunction>::getFunction(c->getSystemState(),threshold),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("merge","",Class<IFunction>::getFunction(c->getSystemState(),threshold),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("paletteMap","",Class<IFunction>::getFunction(c->getSystemState(),paletteMap),NORMAL_METHOD,true);
	// properties
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rect","",Class<IFunction>::getFunction(c->getSystemState(),getRect),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	REGISTER_GETTER(c,transparent);

	IBitmapDrawable::linkTraits(c);
}

void BitmapData::addUser(Bitmap* b)
{
	users.insert(b);
	if (!pixels.isNull())
	{
		if (pixels->checkTexture())
		{
			getSystemState()->getRenderThread()->addUploadJob(this->pixels.getPtr());
		}
	}
	b->updatedData();
}

void BitmapData::removeUser(Bitmap* b)
{
	users.erase(b);
}

void BitmapData::notifyUsers() const
{
	if (locked > 0 || users.empty())
		return;

	if (!pixels.isNull())
	{
		if (pixels->checkTexture())
        {
		    getSystemState()->getRenderThread()->addUploadJob(this->pixels.getPtr());
        }
	}
	for(auto it=users.begin();it!=users.end();it++)
		(*it)->updatedData();
}

ASFUNCTIONBODY_ATOM(BitmapData,_constructor)
{
	int32_t width;
	int32_t height;
	bool transparent;
	uint32_t fillColor;
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	ARG_UNPACK_ATOM(width, 0)(height, 0)(transparent, true)(fillColor, 0xFFFFFFFF);

	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->pixels->isEmpty())
		return;
	if(width<0 || height<0)
		throw Class<ArgumentError>::getInstanceS(sys,"invalid height or width", kInvalidArgumentError);
	if(width>8191 || height>8191)
		throw Class<ArgumentError>::getInstanceS(sys,"invalid height or width", kInvalidArgumentError);

	uint32_t *pixelArray=new uint32_t[width*height];
	uint32_t c=GUINT32_TO_BE(fillColor); // fromRGB expects big endian data
	if(!transparent)
	{
		uint8_t *alpha=reinterpret_cast<uint8_t *>(&c);
		*alpha=0xFF;
	}
	else
	{
		//premultiply alpha
		uint32_t res = 0;
		uint32_t alpha = ((fillColor >> 24)&0xff);
		if (alpha != 0xff)
		{
			res |= ((((fillColor >> 0) &0xff) * alpha)&0xff) << 0;
			res |= ((((fillColor >> 8) &0xff) * alpha)&0xff) << 8;
			res |= ((((fillColor >> 16) &0xff) * alpha)&0xff) << 16;
			res |= alpha<<24;
			c= GUINT32_TO_BE(res);
		}
	}
	for(uint32_t i=0; i<(uint32_t)(width*height); i++)
		pixelArray[i]=c;
	th->pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	th->transparent=transparent;
}

ASFUNCTIONBODY_GETTER(BitmapData, transparent);

ASFUNCTIONBODY_ATOM(BitmapData,dispose)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	th->pixels.reset();
	th->notifyUsers();
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix, bool smoothing)
{
	//Create an InvalidateQueue to store all the hierarchy of objects that must be drawn
	SoftwareInvalidateQueue queue;
	d->hasChanged=true;
	d->requestInvalidation(&queue);
	CairoRenderContext ctxt(pixels->getData(), pixels->getWidth(), pixels->getHeight(),smoothing);
	for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
	{
		DisplayObject* target=(*it).getPtr();
		//Get the drawable from each of the added objects
		IDrawable* drawable=target->invalidate(d, initialMatrix,smoothing);
		if(drawable==nullptr)
			continue;

		//Compute the matrix for this object
		bool isBufferOwner=true;
		uint8_t* buf=drawable->getPixelBuffer(initialMatrix.getScaleX(),initialMatrix.getScaleY(),&isBufferOwner);
		//Construct a CachedSurface using the data
		CachedSurface& surface=ctxt.allocateCustomSurface(target,buf,isBufferOwner);
		surface.tex->width=drawable->getWidth();
		surface.tex->height=drawable->getHeight();
		surface.xOffset=drawable->getXOffset();
		surface.yOffset=drawable->getYOffset();
		surface.xOffsetTransformed=drawable->getXOffsetTransformed();
		surface.yOffsetTransformed=drawable->getYOffsetTransformed();
		surface.widthTransformed=drawable->getWidthTransformed();
		surface.heightTransformed=drawable->getHeightTransformed();
		surface.rotation=drawable->getRotation();
		surface.xscale = drawable->getXScale();
		surface.yscale = drawable->getYScale();
		delete drawable;
	}
	d->Render(ctxt,true);
}

ASFUNCTIONBODY_ATOM(BitmapData,draw)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	tiny_string blendMode;
	_NR<Rectangle> clipRect;
	bool smoothing;
	ARG_UNPACK_ATOM (drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, "")
					(clipRect, NullRef) (smoothing, false);

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass(sys)) )
		throwError<TypeError>(kCheckTypeFailedError, 
				      drawable->getClassName(),
				      "IBitmapDrawable");

	if(!ctransform.isNull() || !(blendMode.empty() || blendMode == "null") || !clipRect.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support many parameters:"<<ctransform.isNull()<<" "<<clipRect.isNull()<<" "<<blendMode);

	if(drawable->is<BitmapData>())
	{
		BitmapData* data=drawable->as<BitmapData>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		CairoRenderContext ctxt(th->pixels->getData(), th->pixels->getWidth(), th->pixels->getHeight(),smoothing);
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
		th->drawDisplayObject(d, initialMatrix,smoothing);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support " << drawable->toDebugString());

	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK_ATOM(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y,false);
	asAtomHandler::setUInt(ret,sys,pix & 0xffffff);
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixel32)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK_ATOM(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y,false);
	asAtomHandler::setUInt(ret,sys,pix);
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK_ATOM(x)(y)(color);

	th->pixels->setPixel(x, y, color, false,false);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixel32)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK_ATOM(x)(y)(color);

	th->pixels->setPixel(x, y, color, th->transparent,false);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,getRect)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS(sys);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,_getHeight)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	asAtomHandler::setInt(ret,sys,th->getHeight());
}

ASFUNCTIONBODY_ATOM(BitmapData,_getWidth)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	asAtomHandler::setInt(ret,sys,th->getWidth());
}

ASFUNCTIONBODY_ATOM(BitmapData,fillRect)
{
	BitmapData* th=asAtomHandler::as<BitmapData>(obj);
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_UNPACK_ATOM(rect)(color);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	if (th->transparent)
	{
		//premultiply alpha
		uint32_t res = 0;
		uint32_t alpha = ((color >> 24)&0xff);
		if (alpha != 0xff)
		{
			res |= ((((color >> 0) &0xff) * alpha)&0xff) << 0;
			res |= ((((color >> 8) &0xff) * alpha)&0xff) << 8;
			res |= ((((color >> 16) &0xff) * alpha)&0xff) << 16;
			res |= alpha<<24;
			color = res;
		}
	}
	th->pixels->fillRectangle(rect->getRect(), color, th->transparent);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,copyPixels)
{
	BitmapData* th=asAtomHandler::as<BitmapData>(obj);
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapData> alphaBitmapData;
	_NR<Point> alphaPoint;
	bool mergeAlpha;
	ARG_UNPACK_ATOM(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	if (source.isNull())
		throwError<TypeError>(kNullPointerError, "source");
	if (sourceRect.isNull())
		throwError<TypeError>(kNullPointerError, "sourceRect");
	if (destPoint.isNull())
		throwError<TypeError>(kNullPointerError, "destPoint");

	if(!alphaBitmapData.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "BitmapData.copyPixels doesn't support alpha bitmap");

	th->pixels->copyRectangle(source->pixels, sourceRect->getRect(),
				  destPoint->getX(), destPoint->getY(),
				  mergeAlpha);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS(sys);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,hitTest)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	_NR<Point> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_UNPACK_ATOM (firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1);

	asAtomHandler::setBool(ret,false);

	if(secondObject->is<Point>())
	{
		Point* secondPoint = secondObject->as<Point>();
	
		uint32_t pix=th->pixels->getPixel(secondPoint->getX()-firstPoint->getX(), secondPoint->getY()-firstPoint->getY());
		if((pix>>24)>=firstAlphaThreshold)
			asAtomHandler::setBool(ret,true);
		else
			asAtomHandler::setBool(ret,false);
	}
	else if (secondObject->is<Rectangle>())
	{
		Rectangle* r = secondObject->as<Rectangle>();
		
		for (uint32_t x=0; x<r->width; x++)
		{
			for (uint32_t y=0; y<r->height; y++)
			{
				uint32_t pix=th->pixels->getPixel(r->x+x-firstPoint->getX(), r->y+y-firstPoint->getY());
				if((pix>>24)>=firstAlphaThreshold)
				{
					asAtomHandler::setBool(ret,true);
					return;
				}
				else
				{
					asAtomHandler::setBool(ret,false);
					return;
				}
			}
		}
	}
	else if (secondObject->is<Bitmap>())
	{
		if (secondBitmapDataPoint.isNull())
			LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest with Bitmap as secondObject and no secondBitmapDataPoint");
		else
		{
			if (secondObject->as<Bitmap>()->bitmapData.isNull())
				LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest with empty Bitmap as secondObject");
			else
			{
				Point* secondPoint = secondBitmapDataPoint->as<Point>();
				uint32_t pix1=th->pixels->getPixel(firstPoint->getX(), firstPoint->getY());
				uint32_t pix2=secondObject->as<Bitmap>()->bitmapData->pixels->getPixel(secondPoint->getX(), secondPoint->getY());
				if(((pix1>>24)>=firstAlphaThreshold) && (pix2>>24)>=secondAlphaThreshold)
					asAtomHandler::setBool(ret,true);
				else
					asAtomHandler::setBool(ret,false);
			}
		}
	}
	else if (secondObject->is<BitmapData>())
	{
		if (secondBitmapDataPoint.isNull())
			LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest with BitmapData as secondObject and no secondBitmapDataPoint");
		else
		{
			Point* secondPoint = secondBitmapDataPoint->as<Point>();
			uint32_t pix1=th->pixels->getPixel(firstPoint->getX(), firstPoint->getY());
			uint32_t pix2=secondObject->as<BitmapData>()->pixels->getPixel(secondPoint->getX(), secondPoint->getY());
			if(((pix1>>24)>=firstAlphaThreshold) && (pix2>>24)>=secondAlphaThreshold)
				asAtomHandler::setBool(ret,true);
			else
				asAtomHandler::setBool(ret,false);
		}
	}
	else
		throwError<TypeError>(kCheckTypeFailedError, 
				      secondObject->getClassName(),
				      " Point, Rectangle, Bitmap, or BitmapData");
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(BitmapData,scroll)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	int x;
	int y;
	ARG_UNPACK_ATOM (x) (y);

	if (th->pixels->scroll(x, y))
		th->notifyUsers();

}

ASFUNCTIONBODY_ATOM(BitmapData,clone)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	ret = asAtomHandler::fromObject(Class<BitmapData>::getInstanceS(sys,*th));
}

ASFUNCTIONBODY_ATOM(BitmapData,copyChannel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	uint32_t sourceChannel;
	uint32_t destChannel;
	ARG_UNPACK_ATOM (source) (sourceRect) (destPoint) (sourceChannel) (destChannel);

	if (source.isNull())
		throwError<TypeError>(kNullPointerError, "source");
	if (sourceRect.isNull())
		throwError<TypeError>(kNullPointerError, "sourceRect");
	if (destPoint.isNull())
		throwError<TypeError>(kNullPointerError, "destPoint");

	unsigned int sourceShift = BitmapDataChannel::channelShift(sourceChannel);
	unsigned int destShift = BitmapDataChannel::channelShift(destChannel);

	RECT clippedSourceRect;
	int32_t clippedDestX;
	int32_t clippedDestY;
	th->pixels->clipRect(source->pixels, sourceRect->getRect(),
				 destPoint->getX(), destPoint->getY(),
				 clippedSourceRect, clippedDestX, clippedDestY);
	int regionWidth = clippedSourceRect.Xmax - clippedSourceRect.Xmin;
	int regionHeight = clippedSourceRect.Ymax - clippedSourceRect.Ymin;

	if (regionWidth < 0 || regionHeight < 0)
		return;

	uint32_t constantChannelsMask = ~(0xFF << destShift);
	for (int32_t y=0; y<regionHeight; y++)
	{
		for (int32_t x=0; x<regionWidth; x++)
		{
			int32_t sx = clippedSourceRect.Xmin+x;
			int32_t sy = clippedSourceRect.Ymin+y;
			uint32_t sourcePixel = source->pixels->getPixel(sx, sy,false);
			uint32_t channel = (sourcePixel >> sourceShift) & 0xFF;
			int32_t dx = clippedDestX + x;
			int32_t dy = clippedDestY + y;
			uint32_t oldPixel = th->pixels->getPixel(dx, dy,sourceChannel == BitmapDataChannel::ALPHA);
			uint32_t destChannelValue = channel << destShift;

			uint32_t newColor = ((oldPixel & constantChannelsMask) | destChannelValue);
			th->pixels->setPixel(dx, dy, newColor, true,sourceChannel == BitmapDataChannel::ALPHA);
		}
	}

	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,lock)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	th->locked++;
}

ASFUNCTIONBODY_ATOM(BitmapData,unlock)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	if (th->locked > 0)
	{
		th->locked--;
		if (th->locked == 0)
			th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,floodFill)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK_ATOM (x) (y) (color);

	if (!th->transparent)
		color = 0xFF000000 | color;

	th->pixels->floodFill(x, y, color);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,histogram)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	_NR<Rectangle> inputRect;
	ARG_UNPACK_ATOM (inputRect);

	RECT rect;
	if (inputRect.isNull()) {
		rect = RECT(0, th->getWidth(), 0, th->getHeight());
	} else {
		th->pixels->clipRect(inputRect->getRect(), rect);
	}

	unsigned int counts[4][256] = {{0}};
	for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
	{
		for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);
			for (int i=0; i<4; i++)
			{
				counts[i][(pixel >> (8*i)) & 0xFF]++;
			}
		}
	}

	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(v,sys,Template<Vector>::getTemplateInstance(sys,Class<Number>::getClass(sys),NullRef).getPtr(),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);
	int channelOrder[4] = {2, 1, 0, 3}; // red, green, blue, alpha
	for (int j=0; j<4; j++)
	{
		Template<Vector>::getInstanceS(v,sys,Class<Number>::getClass(sys),NullRef);
		Vector *histogram = asAtomHandler::as<Vector>(v);
		for (int level=0; level<256; level++)
		{
			asAtom v = asAtomHandler::fromUInt(counts[channelOrder[j]][level]);
			histogram->append(v);
		}
		asAtom v = asAtomHandler::fromObject(histogram);
		result->append(v);
	}

	ret = asAtomHandler::fromObject(result);
}

ASFUNCTIONBODY_ATOM(BitmapData,getColorBoundsRect)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	uint32_t mask;
	uint32_t color;
	bool findColor;
	ARG_UNPACK_ATOM (mask) (color) (findColor, true);

	int xmin = th->getWidth();
	int xmax = 0;
	int ymin = th->getHeight();
	int ymax = 0;
	for (int32_t x=0; x<th->getWidth(); x++)
	{
		for (int32_t y=0; y<th->getHeight(); y++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);
			if ((findColor && ((pixel & mask) == color)) ||
			    (!findColor && ((pixel & mask) != color)))
			{
				if (x < xmin)
					xmin = x;
				if (x > xmax)
					xmax = x;
				if (y < ymin)
					ymin = y;
				if (y > ymax)
					ymax = y;
			}
		}
	}

	Rectangle *bounds = Class<Rectangle>::getInstanceS(sys);
	if ((xmin <= xmax) && (ymin <= ymax))
	{
		bounds->x = xmin;
		bounds->y = ymin;
		bounds->width = xmax - xmin + 1;
		bounds->height = ymax - ymin + 1;
	}
	ret =asAtomHandler::fromObject(bounds);
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixels)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	
	_NR<Rectangle> rect;
	ARG_UNPACK_ATOM (rect);

	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	ByteArray *ba = Class<ByteArray>::getInstanceS(sys);
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect->getRect());
	vector<uint32_t>::const_iterator it;
	for (it=pixelvec.begin(); it!=pixelvec.end(); ++it)
		ba->writeUnsignedInt(ba->endianIn(*it));
	ret = asAtomHandler::fromObject(ba);
}

ASFUNCTIONBODY_ATOM(BitmapData,getVector)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	
	_NR<Rectangle> rect;
	ARG_UNPACK_ATOM (rect);

	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(v,sys,Class<UInteger>::getClass(sys),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect->getRect());
	vector<uint32_t>::const_iterator it;
	for (it=pixelvec.begin(); it!=pixelvec.end(); ++it)
	{
		asAtom v = asAtomHandler::fromUInt(*it);
		result->append(v);
	}
	ret = asAtomHandler::fromObject(result);
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixels)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	
	_NR<Rectangle> inputRect;
	_NR<ByteArray> inputByteArray;
	ARG_UNPACK_ATOM (inputRect) (inputByteArray);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputByteArray.isNull())
		throwError<TypeError>(kNullPointerError, "inputByteArray");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			uint32_t pixel;
			if (!inputByteArray->readUnsignedInt(pixel))
				throwError<EOFError>(kEOFError);
			th->pixels->setPixel(x, y, pixel, th->transparent);
		}
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,setVector)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);
	
	_NR<Rectangle> inputRect;
	_NR<Vector> inputVector;
	ARG_UNPACK_ATOM (inputRect) (inputVector);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputVector.isNull())
		throwError<TypeError>(kNullPointerError, "inputVector");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			if (i >= inputVector->size())
				throwError<RangeError>(kParamRangeError);

			asAtom v = inputVector->at(i);
			uint32_t pixel = asAtomHandler::toUInt(v);
			th->pixels->setPixel(x, y, pixel, th->transparent);
			i++;
		}
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,colorTransform)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	
	_NR<Rectangle> inputRect;
	_NR<ColorTransform> inputColorTransform;
	ARG_UNPACK_ATOM (inputRect) (inputColorTransform);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputColorTransform.isNull())
		throwError<TypeError>(kNullPointerError, "inputVector");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{

			uint32_t pixel = pixelvec[i];

			int a, r, g, b;
			a = ((pixel >> 24 )&0xff) * inputColorTransform->alphaMultiplier + inputColorTransform->alphaOffset;
			if (a > 255) a = 255;
			if (a < 0) a = 0;
			r = ((pixel >> 16 )&0xff) * inputColorTransform->redMultiplier + inputColorTransform->redOffset;
			if (r > 255) r = 255;
			if (r < 0) r = 0;
			g = ((pixel >> 8 )&0xff) * inputColorTransform->greenMultiplier + inputColorTransform->greenOffset;
			if (g > 255) g = 255;
			if (g < 0) g = 0;
			b = ((pixel )&0xff) * inputColorTransform->blueMultiplier + inputColorTransform->blueOffset;
			if (b > 255) b = 255;
			if (b < 0) b = 0;
			
			pixel = (a<<24) | (r<<16) | (g<<8) | b;
			
			th->pixels->setPixel(x, y, pixel, th->transparent);
			i++;
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,compare)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	
	_NR<BitmapData> otherBitmapData;
	ARG_UNPACK_ATOM (otherBitmapData);

	if (otherBitmapData.isNull())
		throwError<TypeError>(kNullPointerError, "otherBitmapData");

	if (th->getWidth() != otherBitmapData->getWidth())
	{
		asAtomHandler::setInt(ret,sys,-3);
		return;
	}
	if (th->getHeight() != otherBitmapData->getHeight())
	{
		asAtomHandler::setInt(ret,sys,-4);
		return;
	}
	RECT rect;
	rect.Xmin = 0;
	rect.Xmax = th->getWidth();
	rect.Ymin = 0;
	rect.Ymax = th->getHeight();
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect);
	vector<uint32_t> otherpixelvec = otherBitmapData->pixels->getPixelVector(rect);
	
	BitmapData* res = Class<BitmapData>::getInstanceS(sys,rect.Xmax,rect.Ymax);
	unsigned int i = 0;
	bool different = false;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{

			uint32_t pixel = pixelvec[i];
			uint32_t otherpixel = otherpixelvec[i];
			if (pixel == otherpixel)
				res->pixels->setPixel(x, y, 0, true);
			else if ((pixel & 0x00FFFFFF) == (otherpixel & 0x00FFFFFF))
			{
				different = true;
				res->pixels->setPixel(x, y, ((pixel & 0xFF000000) - (otherpixel & 0xFF000000)) | 0x00FFFFFF , true);
			}
			else 
			{
				different = true;
				res->pixels->setPixel(x, y, ((pixel & 0x00FFFFFF) - (otherpixel & 0x00FFFFFF)), true);
			}
			i++;
		}
	}
	if (!different)
		asAtomHandler::setInt(ret,sys,0);
	else
		ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(BitmapData,applyFilter)
{
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapFilter> filter;
	ARG_UNPACK_ATOM (sourceBitmapData)(sourceRect)(destPoint)(filter);
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.applyFilter not implemented");
}

ASFUNCTIONBODY_ATOM(BitmapData,noise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	int randomSeed;
	unsigned int low;
	unsigned int high;
	unsigned int channelOptions;
	bool grayScale;
	ARG_UNPACK_ATOM(randomSeed)(low, 0) (high, 255) (channelOptions, 7) (grayScale, false);
	
	srand(randomSeed);

	uint32_t range = high-low;

	for (int32_t x=0; x<th->getWidth(); x++)
	{
		for (int32_t y=0; y<th->getHeight(); y++)
		{
			uint32_t pixel = 0x000000ff;
			
			if (grayScale)
			{
				uint8_t v = (rand() % range + low) & 0xff;
				pixel |= v<<24 | v<<16 | v<<8;
			}
			else
			{
				if((channelOptions & 0x1) == 0x1) // R
					pixel |= ((rand() % range + low) & 0xff)<<24;
				if((channelOptions & 0x2) == 0x2) // G
					pixel |= ((rand() % range + low) & 0xff)<<16;
				if((channelOptions & 0x4) == 0x4) // B
					pixel |= ((rand() % range + low) & 0xff)<<8;
				if((channelOptions & 0x8) == 0x8) // A
					pixel |= ((rand() % range + low) & 0xff);
			}
			th->pixels->setPixel(x, y,pixel,true,true);
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,perlinNoise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS(sys,"Disposed BitmapData", 2015);

	number_t baseX;
	number_t baseY;
	unsigned int numOctaves;
	int randomSeed;
	bool stitch;
	bool fractalNoise;
	unsigned int channelOptions;
	bool grayScale;
	_NR<Array> offsets;
	ARG_UNPACK_ATOM(baseX)(baseY)(numOctaves)(randomSeed)(stitch) (fractalNoise) (channelOptions, 7) (grayScale, false) (offsets, NullRef);

	if (stitch)
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter stitch is ignored");
	if (fractalNoise)
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter fractalNoise is ignored");
	if (!offsets.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter offsets is ignored");

	const siv::PerlinNoise perlin(randomSeed);
	for (int32_t x=0; x<th->getWidth(); x++)
	{
		for (int32_t y=0; y<th->getHeight(); y++)
		{
			uint32_t pixel = 0x000000ff;
			number_t v1 = perlin.octaveNoise0_1(x / baseX, y / baseY, numOctaves);
			if (grayScale)
			{
				uint8_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint8_t>(v1 * 255.0 + 0.5);
				pixel |= v<<24 | v<<16 | v<<8;
			}
			else
			{
				if((channelOptions & 0x1) == 0x1) // R
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0xff000000;
				}
				if((channelOptions & 0x2) == 0x2) // G
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x00ff0000;
				}
				if((channelOptions & 0x4) == 0x4) // B
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x0000ff00;
				}
				if((channelOptions & 0x8) == 0x8) // A
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x000000ff;
				}
			}
			th->pixels->setPixel(x, y,pixel,true,true);
			//LOG(LOG_INFO,"perlinnoise pixel:"<<x<<" "<<y<<" "<<hex<<th->pixels->getPixel(x,y)<<" "<<grayScale);
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,threshold)
{
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	tiny_string operation;
	uint32_t threshold;
	uint32_t color;
	uint32_t mask;
	bool copySource;
	ARG_UNPACK_ATOM(sourceBitmapData)(sourceRect)(destPoint)(operation)(threshold) (color,0) (mask, 0xFFFFFFFF) (copySource, false);

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.threshold not implemented");
}
ASFUNCTIONBODY_ATOM(BitmapData,merge)
{
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	uint32_t redMultiplier;
	uint32_t greenMultiplier;
	uint32_t blueMultiplier;
	uint32_t alphaMultiplier;
	ARG_UNPACK_ATOM(sourceBitmapData)(sourceRect) (destPoint) (redMultiplier) (greenMultiplier) (blueMultiplier) (alphaMultiplier);

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.merge not implemented");
}
ASFUNCTIONBODY_ATOM(BitmapData,paletteMap)
{
//	BitmapData* th = asAtomHandler::as<BitmapData>(obj);

	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<Array> redArray;
	_NR<Array> greenArray;
	_NR<Array> blueArray;
	_NR<Array> alphaArray;
	ARG_UNPACK_ATOM(sourceBitmapData)(sourceRect) (destPoint) (redArray, NullRef) (greenArray, NullRef) (blueArray, NullRef) (alphaArray, NullRef);

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.paletteMap not implemented");
}

