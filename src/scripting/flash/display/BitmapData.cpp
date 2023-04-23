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
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/system/flashsystem.h"
#include "backends/rendering.h"
#include "3rdparty/perlinnoise/PerlinNoise.hpp"

#include <cstdlib> 

using namespace lightspark;
using namespace std;

BitmapData::BitmapData(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),needsupload(true),transparent(true)
{
}

BitmapData::BitmapData(ASWorker* wrk,Class_base* c, _R<BitmapContainer> b):ASObject(wrk,c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(b),locked(0),needsupload(true),transparent(true)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

BitmapData::BitmapData(ASWorker* wrk,Class_base* c, const BitmapData& other)
  : ASObject(wrk,c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(other.pixels),locked(other.locked),needsupload(other.needsupload),transparent(other.transparent)
{
	traitsInitialized = other.traitsInitialized;
	constructIndicator = other.constructIndicator;
	constructorCallComplete = other.constructorCallComplete;
}

BitmapData::BitmapData(ASWorker* wrk,Class_base* c, uint32_t width, uint32_t height)
 : ASObject(wrk,c,T_OBJECT,SUBTYPE_BITMAPDATA),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),needsupload(true),transparent(true)
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
	c->isReusable=true;
	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass(c->getSystemState()));
	c->setDeclaredMethodByQName("draw","",Class<IFunction>::getFunction(c->getSystemState(),draw),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel","",Class<IFunction>::getFunction(c->getSystemState(),getPixel,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel32","",Class<IFunction>::getFunction(c->getSystemState(),getPixel32,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rect","",Class<IFunction>::getFunction(c->getSystemState(),getRect,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,transparent,Boolean);

	IBitmapDrawable::linkTraits(c);
}

bool BitmapData::destruct()
{
	users.clear();
	if (preparedforshutdown)
		pixels.reset();
	else
		pixels = _MR(new BitmapContainer(getClass()->memoryAccount));
	locked = 0;
	transparent = true;
	return ASObject::destruct();
}

void BitmapData::finalize()
{
	users.clear();
	pixels.reset();
	ASObject::finalize();
}

void BitmapData::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	pixels.reset();
	ASObject::prepareShutdown();
}
void BitmapData::addUser(Bitmap* b, bool startupload)
{
	users.insert(b);
	if (!startupload)
		return;
	needsupload=true;
	b->updatedData();
}

void BitmapData::removeUser(Bitmap* b)
{
	users.erase(b);
}

void BitmapData::checkForUpload()
{
	if (!pixels.isNull() && needsupload)
	{
		if (pixels->checkTexture())
			getSystemState()->getRenderThread()->addUploadJob(this->pixels.getPtr());
		needsupload=false;
	}
}

void BitmapData::notifyUsers()
{
	if (locked > 0 || users.empty())
		return;
	needsupload=true;
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	ARG_CHECK(ARG_UNPACK(width, 0)(height, 0)(transparent, true)(fillColor, 0xFFFFFFFF));

	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->pixels->isEmpty())
		return;
	if(width<0 || height<0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"invalid height or width");
		return;
	}
	if(width>8191 || height>8191)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"invalid height or width");
		return;
	}

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

ASFUNCTIONBODY_GETTER(BitmapData, transparent)

ASFUNCTIONBODY_ATOM(BitmapData,dispose)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	th->pixels.reset();
	th->notifyUsers();
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix, bool smoothing, bool forCachedBitmap, AS_BLENDMODE blendMode)
{
	if (forCachedBitmap)
		d->incRef();
	//Create an InvalidateQueue to store all the hierarchy of objects that must be drawn
	SoftwareInvalidateQueue queue(forCachedBitmap ? _MNR(d):NullRef);
	d->hasChanged=true;
	d->requestInvalidation(&queue);
	if (forCachedBitmap)
	{
		uint8_t* p = pixels->getData();
		memset(p,0,pixels->getWidth()*pixels->getHeight()*4);
	}
	CairoRenderContext ctxt(pixels->getData(), pixels->getWidth(), pixels->getHeight(),smoothing);
	for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
	{
		DisplayObject* target=(*it).getPtr();
		//Get the drawable from each of the added objects
		IDrawable* drawable=target->invalidate(d, initialMatrix,smoothing,&queue, nullptr);
		if(drawable==nullptr)
			continue;
		if (forCachedBitmap && !target->isMask())
			target->hasChanged=false;
		//Compute the matrix for this object
		bool isBufferOwner=true;
		uint32_t bufsize=0;
		uint8_t* buf=drawable->getPixelBuffer(&isBufferOwner,&bufsize);
		if (!forCachedBitmap && !target->filters.isNull())
		{
			BitmapContainer bc(nullptr);
			bc.fromRawData(buf,drawable->getWidth(),drawable->getHeight());
			target->applyFilters(&bc,nullptr,RECT(0,bc.getWidth(),0,bc.getHeight()),0,0, drawable->getXScale(), drawable->getYScale());
			memcpy(buf,bc.getData(),bufsize);
		}
		ColorTransform* ct = target->colorTransform.getPtr();
		DisplayObjectContainer* p = target->getParent();
		while (!ct && p && p!= d)
		{
			ct = p->colorTransform.getPtr();
			p = p->getParent();
		}
		if (ct)
		{
			if (!isBufferOwner)
			{
				isBufferOwner=true;
				uint8_t* buf2 = new uint8_t[bufsize];
				memcpy(buf2,buf,bufsize);
				buf=buf2;
			}
			ct->applyTransformation(buf,bufsize);
		}
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
		surface.alpha = drawable->getAlpha();
		surface.xscale = drawable->getXScale();
		surface.yscale = drawable->getYScale();
		surface.tex->xContentScale = drawable->getXContentScale();
		surface.tex->yContentScale = drawable->getYContentScale();
		surface.tex->xOffset = surface.xOffset;
		surface.tex->yOffset = surface.yOffset;
		surface.isMask=drawable->getIsMask();
		surface.mask=drawable->getMask();
		surface.matrix = drawable->getMatrix();
		if (!forCachedBitmap)
			surface.blendmode=blendMode;
		surface.isValid=true;
		surface.isInitialized=true;
		delete drawable;
	}
	if (d->getMask())
		d->getMask()->Render(ctxt,true);
	d->Render(ctxt,true);
}

ASFUNCTIONBODY_ATOM(BitmapData,draw)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	tiny_string blendMode;
	_NR<Rectangle> clipRect;
	bool smoothing;
	ARG_CHECK(ARG_UNPACK(drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, "")
					(clipRect, NullRef) (smoothing, false));

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass(wrk->getSystemState())) )
	{
		if (wrk->rootClip->needsActionScript3())
			createError<TypeError>(wrk,kCheckTypeFailedError, 
								   drawable->getClassName(),
								   "IBitmapDrawable");
		return;
	}

	if(!clipRect.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support clipRect parameter");

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
		if (ctransform)
			ctransform->applyTransformation(data->pixels->getData(),data->getBitmapContainer()->getWidth()*data->getBitmapContainer()->getHeight()*4);
	}
	else if(drawable->is<DisplayObject>())
	{
		DisplayObject* d=drawable->as<DisplayObject>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		AS_BLENDMODE bl = BLENDMODE_NORMAL;
		if (blendMode == "add") bl = BLENDMODE_ADD;
		else if (blendMode == "alpha") bl = BLENDMODE_ALPHA;
		else if (blendMode == "darken") bl = BLENDMODE_DARKEN;
		else if (blendMode == "difference") bl = BLENDMODE_DIFFERENCE;
		else if (blendMode == "erase") bl = BLENDMODE_ERASE;
		else if (blendMode == "hardlight") bl = BLENDMODE_HARDLIGHT;
		else if (blendMode == "invert") bl = BLENDMODE_INVERT;
		else if (blendMode == "layer") bl = BLENDMODE_LAYER;
		else if (blendMode == "lighten") bl = BLENDMODE_LIGHTEN;
		else if (blendMode == "multiply") bl = BLENDMODE_MULTIPLY;
		else if (blendMode == "overlay") bl = BLENDMODE_OVERLAY;
		else if (blendMode == "screen") bl = BLENDMODE_SCREEN;
		else if (blendMode == "subtract") bl = BLENDMODE_SUBTRACT;
		
		d->DrawToBitmap(th,initialMatrix,true,false,bl);
		if (ctransform)
			ctransform->applyTransformation(th->pixels->getData(),th->getBitmapContainer()->getWidth()*th->getBitmapContainer()->getHeight()*4);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support " << drawable->toDebugString());
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	int32_t x;
	int32_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));

	uint32_t pix=th->pixels->getPixel(x, y,false);
	asAtomHandler::setUInt(ret,wrk,pix & 0xffffff);
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixel32)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	int32_t x;
	int32_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));

	uint32_t pix=th->pixels->getPixel(x, y,false);
	asAtomHandler::setUInt(ret,wrk,pix);
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(x)(y)(color));

	th->pixels->setPixel(x, y, color, false,false);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixel32)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(x)(y)(color));

	th->pixels->setPixel(x, y, color, th->transparent,false);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,getRect)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	Rectangle *rect=Class<Rectangle>::getInstanceS(wrk);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,_getHeight)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	asAtomHandler::setInt(ret,wrk,th->getHeight());
}

ASFUNCTIONBODY_ATOM(BitmapData,_getWidth)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	asAtomHandler::setInt(ret,wrk,th->getWidth());
}

ASFUNCTIONBODY_ATOM(BitmapData,fillRect)
{
	BitmapData* th=asAtomHandler::as<BitmapData>(obj);
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(rect)(color));

	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	if (rect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}

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
	ARG_CHECK(ARG_UNPACK(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false));

	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	if (source.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "source");
		return;
	}
	if (sourceRect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "sourceRect");
		return;
	}
	if (destPoint.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "destPoint");
		return;
	}

	if(!alphaBitmapData.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "BitmapData.copyPixels doesn't support alpha bitmap");

	th->pixels->copyRectangle(source->pixels, sourceRect->getRect(),
				  destPoint->getX(), destPoint->getY(),
				  mergeAlpha|| !th->transparent);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	Rectangle *rect=Class<Rectangle>::getInstanceS(wrk);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,hitTest)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	_NR<Point> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_CHECK(ARG_UNPACK(firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1));

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
		createError<TypeError>(wrk,kCheckTypeFailedError, 
				      secondObject->getClassName(),
				      " Point, Rectangle, Bitmap, or BitmapData");
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(BitmapData,scroll)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	int x;
	int y;
	ARG_CHECK(ARG_UNPACK(x) (y));

	if (th->pixels->scroll(x, y))
		th->notifyUsers();

}

ASFUNCTIONBODY_ATOM(BitmapData,clone)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	BitmapData* clone = Class<BitmapData>::getInstanceS(wrk,th->getWidth(),th->getHeight());
	if (th->getBitmapContainer())
		memcpy (clone->getBitmapContainer()->getData(),th->getBitmapContainer()->getData(),th->getWidth()*th->getHeight()*4);
	ret = asAtomHandler::fromObject(clone);
}

ASFUNCTIONBODY_ATOM(BitmapData,copyChannel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	uint32_t sourceChannel;
	uint32_t destChannel;
	ARG_CHECK(ARG_UNPACK(source) (sourceRect) (destPoint) (sourceChannel) (destChannel));

	if (source.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "source");
		return;
	}
	if (sourceRect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "sourceRect");
		return;
	}
	if (destPoint.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "destPoint");
		return;
	}

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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	th->locked++;
}

ASFUNCTIONBODY_ATOM(BitmapData,unlock)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(x) (y) (color));

	if (!th->transparent)
		color = 0xFF000000 | color;

	th->pixels->floodFill(x, y, color);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,histogram)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	_NR<Rectangle> inputRect;
	ARG_CHECK(ARG_UNPACK(inputRect));

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
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Template<Vector>::getTemplateInstance(root,Class<Number>::getClass(wrk->getSystemState()),NullRef).getPtr(),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);
	int channelOrder[4] = {2, 1, 0, 3}; // red, green, blue, alpha
	for (int j=0; j<4; j++)
	{
		Template<Vector>::getInstanceS(wrk,v,root,Class<Number>::getClass(wrk->getSystemState()),NullRef);
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	uint32_t mask;
	uint32_t color;
	bool findColor;
	ARG_CHECK(ARG_UNPACK(mask) (color) (findColor, true));

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

	Rectangle *bounds = Class<Rectangle>::getInstanceS(wrk);
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	
	_NR<Rectangle> rect;
	ARG_CHECK(ARG_UNPACK(rect));

	if (rect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}

	ByteArray *ba = Class<ByteArray>::getInstanceS(wrk);
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	
	_NR<Rectangle> rect;
	ARG_CHECK(ARG_UNPACK(rect));

	if (rect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}

	asAtom v=asAtomHandler::invalidAtom;
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Class<UInteger>::getClass(wrk->getSystemState()),NullRef);
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	
	_NR<Rectangle> inputRect;
	_NR<ByteArray> inputByteArray;
	ARG_CHECK(ARG_UNPACK(inputRect) (inputByteArray));

	if (inputRect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}
	if (inputByteArray.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "inputByteArray");
		return;
	}

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			uint32_t pixel;
			if (!inputByteArray->readUnsignedInt(pixel))
			{
				createError<EOFError>(wrk,kEOFError);
				return;
			}
			th->pixels->setPixel(x, y, pixel, th->transparent);
		}
	}
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,setVector)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}
	
	_NR<Rectangle> inputRect;
	_NR<Vector> inputVector;
	ARG_CHECK(ARG_UNPACK(inputRect) (inputVector));

	if (inputRect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}
	if (inputVector.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "inputVector");
		return;
	}

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			if (i >= inputVector->size())
			{
				createError<RangeError>(wrk,kParamRangeError);
				return;
			}

			asAtom v = inputVector->at(i);
			uint32_t pixel = asAtomHandler::toUInt(v);
			th->pixels->setPixel(x, y, pixel, th->transparent);
			i++;
		}
	}
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,colorTransform)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	
	_NR<Rectangle> inputRect;
	_NR<ColorTransform> inputColorTransform;
	ARG_CHECK(ARG_UNPACK(inputRect) (inputColorTransform));

	if (inputRect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}
	if (inputColorTransform.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "inputColor");
		return;
	}
	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);
	
	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);;

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
	ARG_CHECK(ARG_UNPACK(otherBitmapData));

	if (otherBitmapData.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "otherBitmapData");
		return;
	}

	if (th->getWidth() != otherBitmapData->getWidth())
	{
		asAtomHandler::setInt(ret,wrk,-3);
		return;
	}
	if (th->getHeight() != otherBitmapData->getHeight())
	{
		asAtomHandler::setInt(ret,wrk,-4);
		return;
	}
	RECT rect;
	rect.Xmin = 0;
	rect.Xmax = th->getWidth();
	rect.Ymin = 0;
	rect.Ymax = th->getHeight();
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect);
	vector<uint32_t> otherpixelvec = otherBitmapData->pixels->getPixelVector(rect);
	
	BitmapData* res = Class<BitmapData>::getInstanceS(wrk,rect.Xmax,rect.Ymax);
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
		asAtomHandler::setInt(ret,wrk,0);
	else
		ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(BitmapData,applyFilter)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapFilter> filter;
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect)(destPoint)(filter));
	if (sourceBitmapData.isNull())
		createError<TypeError>(wrk,kNullPointerError,"sourceBitmapData");
	else if (sourceRect.isNull())
		createError<TypeError>(wrk,kNullPointerError,"sourceRect");
	else if (destPoint.isNull())
		createError<TypeError>(wrk,kNullPointerError,"destPoint");
	else if (filter.isNull())
		createError<TypeError>(wrk,kNullPointerError,"filter");
	else
	{
		th->pixels->applyFilter(sourceBitmapData->pixels, sourceRect->getRect(),
				  destPoint->getX(), destPoint->getY(),
				  filter.getPtr());
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,noise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->pixels.isNull())
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	int randomSeed;
	unsigned int low;
	unsigned int high;
	unsigned int channelOptions;
	bool grayScale;
	ARG_CHECK(ARG_UNPACK(randomSeed)(low, 0) (high, 255) (channelOptions, 7) (grayScale, false));
	
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
	{
		createError<ArgumentError>(wrk,2015,"Disposed BitmapData");
		return;
	}

	number_t baseX;
	number_t baseY;
	unsigned int numOctaves;
	int randomSeed;
	bool stitch;
	bool fractalNoise;
	unsigned int channelOptions;
	bool grayScale;
	_NR<Array> offsets;
	ARG_CHECK(ARG_UNPACK(baseX)(baseY)(numOctaves)(randomSeed)(stitch) (fractalNoise) (channelOptions, 7) (grayScale, false) (offsets, NullRef));

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
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect)(destPoint)(operation)(threshold) (color,0) (mask, 0xFFFFFFFF) (copySource, false));

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
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (redMultiplier) (greenMultiplier) (blueMultiplier) (alphaMultiplier));

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
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (redArray, NullRef) (greenArray, NullRef) (blueArray, NullRef) (alphaArray, NullRef));

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.paletteMap not implemented");
}

