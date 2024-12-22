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
#include "scripting/flash/display/Bitmap.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/geom/Point.h"
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
	c->setDeclaredMethodByQName("draw","",c->getSystemState()->getBuiltinFunction(draw),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawWithQuality","",c->getSystemState()->getBuiltinFunction(drawWithQuality),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel","",c->getSystemState()->getBuiltinFunction(getPixel,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel32","",c->getSystemState()->getBuiltinFunction(getPixel32,2,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel","",c->getSystemState()->getBuiltinFunction(setPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel32","",c->getSystemState()->getBuiltinFunction(setPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyPixels","",c->getSystemState()->getBuiltinFunction(copyPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyPixelsToByteArray","",c->getSystemState()->getBuiltinFunction(copyPixelsToByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fillRect","",c->getSystemState()->getBuiltinFunction(fillRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("generateFilterRect","",c->getSystemState()->getBuiltinFunction(generateFilterRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",c->getSystemState()->getBuiltinFunction(hitTest,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scroll","",c->getSystemState()->getBuiltinFunction(scroll),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<BitmapData>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyChannel","",c->getSystemState()->getBuiltinFunction(copyChannel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lock","",c->getSystemState()->getBuiltinFunction(lock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unlock","",c->getSystemState()->getBuiltinFunction(unlock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("floodFill","",c->getSystemState()->getBuiltinFunction(floodFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("histogram","",c->getSystemState()->getBuiltinFunction(histogram,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getColorBoundsRect","",c->getSystemState()->getBuiltinFunction(getColorBoundsRect,2,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixels","",c->getSystemState()->getBuiltinFunction(getPixels,1,Class<ByteArray>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVector","",c->getSystemState()->getBuiltinFunction(getVector,1,Class<Vector>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixels","",c->getSystemState()->getBuiltinFunction(setPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVector","",c->getSystemState()->getBuiltinFunction(setVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(colorTransform),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("compare","",c->getSystemState()->getBuiltinFunction(compare,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("applyFilter","",c->getSystemState()->getBuiltinFunction(applyFilter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("noise","",c->getSystemState()->getBuiltinFunction(noise),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("perlinNoise","",c->getSystemState()->getBuiltinFunction(perlinNoise),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("threshold","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("merge","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("paletteMap","",c->getSystemState()->getBuiltinFunction(paletteMap),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pixelDissolve","",c->getSystemState()->getBuiltinFunction(pixelDissolve),NORMAL_METHOD,true);

	// properties
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rect","",c->getSystemState()->getBuiltinFunction(getRect,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transparent","",c->getSystemState()->getBuiltinFunction(_getTransparent,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);

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

// needs to be called in renderThread
void BitmapData::checkForUpload()
{
	if (!pixels.isNull() && needsupload)
	{
		pixels->checkTextureForUpload(getSystemState());
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

bool BitmapData::checkDisposed(asAtom& ret)
{
	ret = getInstanceWorker()->needsActionScript3() ? asAtomHandler::undefinedAtom : asAtomHandler::fromInt(-1);
	if(pixels.isNull())
	{
		createError<ArgumentError>(getInstanceWorker(),kInvalidBitmapData);
		return true;
	}
	return false;
}

ASFUNCTIONBODY_ATOM(BitmapData,_constructor)
{
	int32_t width;
	int32_t height;
	bool transparent;
	uint32_t fillColor;
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	ARG_CHECK(ARG_UNPACK(width)(height)(transparent, true)(fillColor, 0xFFFFFFFF));

	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->pixels->isEmpty())
		return;
	if((width<0) || (width> (wrk->AVM1getSwfVersion() >=10 ? 8191 : 2880)) )
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"width");
		return;
	}
	if((height<0) || (height > (wrk->AVM1getSwfVersion() >=10 ? 8191 : 2880)) )
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"height");
		return;
	}
	if(height*width> 16777215 )
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"width/height");
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
			uint32_t alpha = ((fillColor >> 24)&0xff);
			res |= ((((fillColor >> 0 ) &0xff) * alpha +0x7f)/0xff) << 0;
			res |= ((((fillColor >> 8 ) &0xff) * alpha +0x7f)/0xff) << 8;
			res |= ((((fillColor >> 16) &0xff) * alpha +0x7f)/0xff) << 16;
			res |= alpha<<24;
			c= GUINT32_TO_BE(res);
		}
	}
	for(uint32_t i=0; i<(uint32_t)(width*height); i++)
		pixelArray[i]=c;
	th->pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	th->transparent=transparent;
}

ASFUNCTIONBODY_ATOM(BitmapData,dispose)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if (th->checkDisposed(ret))
		return;
	th->pixels.reset();
	th->notifyUsers();
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix, bool smoothing, AS_BLENDMODE blendMode, ColorTransformBase* ct)
{
	d->incRef();
	getSystemState()->getRenderThread()->renderDisplayObjectToBimapContainer(_MNR(d),initialMatrix,smoothing,blendMode,ct,this->pixels);
	this->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,drawWithQuality)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	asAtom blendMode = asAtomHandler::invalidAtom;
	_NR<Rectangle> clipRect;
	bool smoothing;
	asAtom quality = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY))
					(clipRect, NullRef) (smoothing, false)(quality,asAtomHandler::nullAtom));

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass(wrk->getSystemState())) )
	{
		if (wrk->rootClip->needsActionScript3())
			createError<TypeError>(wrk,kCheckTypeFailedError, 
								   drawable->getClassName(),
								   "IBitmapDrawable");
		return;
	}
	if (!asAtomHandler::isNull(quality) && asAtomHandler::toString(quality,wrk)!="high")
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.drawWithQuality parameter quality is ignored:"<<asAtomHandler::toDebugString(quality));

	uint32_t blendModeID = BUILTIN_STRINGS::EMPTY;
	if (asAtomHandler::isValid(blendMode) 
		&& !asAtomHandler::isNull(blendMode)
		&& !asAtomHandler::isUndefined(blendMode))
		blendModeID = asAtomHandler::toStringId(blendMode,wrk);
	if(drawable->is<BitmapData>())
	{
		if(blendModeID != BUILTIN_STRINGS::EMPTY)
			LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support blendMode parameter:"<<drawable->toDebugString()<<" "<<wrk->getSystemState()->getStringFromUniqueId(blendModeID));
		BitmapData* data=drawable->as<BitmapData>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		CairoRenderContext ctxt(th->pixels->getData(), th->pixels->getWidth(), th->pixels->getHeight(),false);
		//Blit the data while transforming it
		if (clipRect.isNull())
		{
			ctxt.transformedBlit(initialMatrix, data->pixels.getPtr(),ctransform.getPtr(),
								smoothing ? CairoRenderContext::FILTER_SMOOTH : CairoRenderContext::FILTER_NONE,0,0,th->pixels->getWidth(), th->pixels->getHeight());
		}
		else
		{
			ctxt.transformedBlit(initialMatrix, data->pixels.getPtr(),ctransform.getPtr(),
								smoothing ? CairoRenderContext::FILTER_SMOOTH : CairoRenderContext::FILTER_NONE,clipRect->x,clipRect->y,clipRect->width,clipRect->height);
		}
	}
	else if(drawable->is<DisplayObject>())
	{
		if(!clipRect.isNull())
			LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support clipRect parameter:"<<drawable->toDebugString()<<" "<<clipRect->x<<"/"<<clipRect->y<<" "<<clipRect->width<<"/"<<clipRect->height);
		DisplayObject* d=drawable->as<DisplayObject>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		AS_BLENDMODE bl = BLENDMODE_NORMAL;
		switch(blendModeID)
		{
			case BUILTIN_STRINGS::STRING_ADD: bl = BLENDMODE_ADD; break;
			case BUILTIN_STRINGS::STRING_ALPHA: bl = BLENDMODE_ALPHA; break;
			case BUILTIN_STRINGS::STRING_DARKEN: bl = BLENDMODE_DARKEN; break;
			case BUILTIN_STRINGS::STRING_DIFFERENCE: bl = BLENDMODE_DIFFERENCE; break;
			case BUILTIN_STRINGS::STRING_ERASE: bl = BLENDMODE_ERASE; break;
			case BUILTIN_STRINGS::STRING_HARDLIGHT: bl = BLENDMODE_HARDLIGHT; break;
			case BUILTIN_STRINGS::STRING_INVERT: bl = BLENDMODE_INVERT; break;
			case BUILTIN_STRINGS::STRING_LAYER: bl = BLENDMODE_LAYER; break;
			case BUILTIN_STRINGS::STRING_LIGHTEN: bl = BLENDMODE_LIGHTEN; break;
			case BUILTIN_STRINGS::STRING_MULTIPLY: bl = BLENDMODE_MULTIPLY; break;
			case BUILTIN_STRINGS::STRING_OVERLAY: bl = BLENDMODE_OVERLAY; break;
			case BUILTIN_STRINGS::STRING_SCREEN: bl = BLENDMODE_SCREEN; break;
			case BUILTIN_STRINGS::STRING_SUBTRACT: bl = BLENDMODE_SUBTRACT; break;
		}
		th->drawDisplayObject(d, initialMatrix,smoothing,bl,ctransform.getPtr());
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support " << drawable->toDebugString());
	th->notifyUsers();
}
ASFUNCTIONBODY_ATOM(BitmapData,draw)
{
	drawWithQuality(ret,wrk,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(BitmapData,getPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	int32_t x;
	int32_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));

	uint32_t pix=th->pixels->getPixel(x, y,false);
	asAtomHandler::setUInt(ret,wrk,pix & 0xffffff);
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixel32)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	int32_t x;
	int32_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));

	if (wrk->needsActionScript3())
	{
		uint32_t pix=th->pixels->getPixel(x, y,false);
		asAtomHandler::setUInt(ret,wrk,pix);
	}
	else
	{
		int32_t pix=(int32_t)th->pixels->getPixel(x, y,false);
		asAtomHandler::setInt(ret,wrk,pix);
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,setPixel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
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
	if(th->checkDisposed(ret))
		return;
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
	if(th->checkDisposed(ret))
		return;
	Rectangle *rect=Class<Rectangle>::getInstanceS(wrk);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,_getHeight)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	asAtomHandler::setInt(ret,wrk,th->getHeight());
}

ASFUNCTIONBODY_ATOM(BitmapData,_getWidth)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	asAtomHandler::setInt(ret,wrk,th->getWidth());
}
ASFUNCTIONBODY_ATOM(BitmapData,_getTransparent)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	asAtomHandler::setBool(ret,th->transparent);
}

ASFUNCTIONBODY_ATOM(BitmapData,fillRect)
{
	BitmapData* th=asAtomHandler::as<BitmapData>(obj);
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(rect)(color));

	if(th->checkDisposed(ret))
		return;
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
			res |= ((((color >> 0 ) &0xff) * alpha +0x7f)/0xff) << 0;
			res |= ((((color >> 8 ) &0xff) * alpha +0x7f)/0xff) << 8;
			res |= ((((color >> 16) &0xff) * alpha +0x7f)/0xff) << 16;
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
	if (th->checkDisposed(ret))
		return;
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapData> alphaBitmapData;
	_NR<Point> alphaPoint;
	bool mergeAlpha;
	ARG_CHECK(ARG_UNPACK(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false));

	if(th->checkDisposed(ret))
		return;
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

ASFUNCTIONBODY_ATOM(BitmapData,copyPixelsToByteArray)
{
	BitmapData* th=asAtomHandler::as<BitmapData>(obj);
	_NR<Rectangle> rect;
	_NR<ByteArray> data;
	ARG_CHECK(ARG_UNPACK(rect)(data));

	if(th->checkDisposed(ret))
		return;
	if (rect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "data");
		return;
	}
	RECT cliprect;
	th->pixels->clipRect(rect->getRect(),cliprect);
	for (int y=cliprect.Ymin; y<cliprect.Ymax; y++)
	{
		for (int x=cliprect.Xmin; x<cliprect.Xmax; x++)
		{
			data->writeUnsignedInt(data->endianIn(th->pixels->getPixel(x,y,false)));
		}
	}
}

ASFUNCTIONBODY_ATOM(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	Rectangle *rect=Class<Rectangle>::getInstanceS(wrk);
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	ret = asAtomHandler::fromObject(rect);
}

ASFUNCTIONBODY_ATOM(BitmapData,hitTest)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	_NR<ASObject> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_CHECK(ARG_UNPACK(firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1));

	asAtomHandler::setBool(ret,false);
	number_t firstPointX=0;
	number_t firstPointY=0;
	if (wrk->needsActionScript3())
	{
		if (!firstPoint->is<Point>())
		{
			createError<ArgumentError>(wrk,kCheckTypeFailedError,
									   th->getClassName(),
									   "firstPoint");
			return;
		}
		firstPointX = firstPoint->as<Point>()->getX();
		firstPointY = firstPoint->as<Point>()->getY();
	}
	else
	{
		if (firstPoint->is<Point>())
		{
			firstPointX = firstPoint->as<Point>()->getX();
			firstPointY = firstPoint->as<Point>()->getY();
		}
		else
		{
			asAtom a= asAtomHandler::invalidAtom;
			list<tiny_string> ns;
			firstPoint->getVariableByMultiname(a,"x",ns,wrk);
			if (asAtomHandler::isValid(a))
				firstPointX = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			a= asAtomHandler::invalidAtom;
			firstPoint->getVariableByMultiname(a,"y",ns,wrk);
			if (asAtomHandler::isValid(a))
				firstPointY = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
		}
	}

	if(secondObject->is<Point>())
	{
		Point* secondPoint = secondObject->as<Point>();
	
		uint32_t pix=th->pixels->getPixel(secondPoint->getX()-firstPointX, secondPoint->getY()-firstPointY);
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
				uint32_t pix=th->pixels->getPixel(r->x+x-firstPointX, r->y+y-firstPointY);
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
				uint32_t pix1=th->pixels->getPixel(firstPointX, firstPointY);
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
		if (secondObject->as<BitmapData>()->checkDisposed(ret))
			return;
		if (secondBitmapDataPoint.isNull())
			LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest with BitmapData as secondObject and no secondBitmapDataPoint");
		else
		{
			Point* secondPoint = secondBitmapDataPoint->as<Point>();
			uint32_t pix1=th->pixels->getPixel(firstPointX, firstPointY);
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
}

ASFUNCTIONBODY_ATOM(BitmapData,scroll)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	int x;
	int y;
	ARG_CHECK(ARG_UNPACK(x) (y));

	if (th->pixels->scroll(x, y))
		th->notifyUsers();

}

ASFUNCTIONBODY_ATOM(BitmapData,clone)
{
	if (!asAtomHandler::is<BitmapData>(obj))
	{
		ret = wrk->needsActionScript3() ? asAtomHandler::undefinedAtom : asAtomHandler::fromInt(-1);
		return;
	}
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	BitmapData* clone = Class<BitmapData>::getInstanceS(wrk,th->getWidth(),th->getHeight());
	clone->transparent = th->transparent;
	if (th->getBitmapContainer())
		th->getBitmapContainer()->clone(clone->getBitmapContainer().getPtr());
	ret = asAtomHandler::fromObject(clone);
}

ASFUNCTIONBODY_ATOM(BitmapData,copyChannel)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
	if(th->checkDisposed(ret))
		return;

	th->locked++;
}

ASFUNCTIONBODY_ATOM(BitmapData,unlock)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
	if(th->checkDisposed(ret))
		return;

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
	if(th->checkDisposed(ret))
		return;

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
	ApplicationDomain* appdomain = wrk->rootClip->applicationDomain.getPtr();
	Template<Vector>::getInstanceS(wrk,v,Template<Vector>::getTemplateInstance(Class<Number>::getClass(wrk->getSystemState()),appdomain).getPtr(),appdomain);
	Vector *result = asAtomHandler::as<Vector>(v);
	int channelOrder[4] = {2, 1, 0, 3}; // red, green, blue, alpha
	for (int j=0; j<4; j++)
	{
		Template<Vector>::getInstanceS(wrk,v,Class<Number>::getClass(wrk->getSystemState()),appdomain);
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
	if(th->checkDisposed(ret))
		return;

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
		bounds->width = (xmax == 0) ? 0 : xmax - xmin + 1;
		bounds->height = (ymax == 0) ? 0 : ymax - ymin + 1;
	}
	ret =asAtomHandler::fromObject(bounds);
}

ASFUNCTIONBODY_ATOM(BitmapData,getPixels)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
	if(th->checkDisposed(ret))
		return;

	_NR<Rectangle> rect;
	ARG_CHECK(ARG_UNPACK(rect));

	if (rect.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "rect");
		return;
	}

	asAtom v=asAtomHandler::invalidAtom;
	ApplicationDomain* appdomain = wrk->rootClip->applicationDomain.getPtr();
	Template<Vector>::getInstanceS(wrk,v,Class<UInteger>::getClass(wrk->getSystemState()),appdomain);
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
	if(th->checkDisposed(ret))
		return;

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
			th->pixels->setPixel(x, y, pixel, th->transparent,false);
		}
	}
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(BitmapData,setVector)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
	if (th->checkDisposed(ret))
		return;

	RECT rect;
	RECT inrect;
	_NR<ColorTransform> inputColorTransform;
	if (wrk->needsActionScript3())
	{
		_NR<Rectangle> inputRect;
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
		inrect=inputRect->getRect();
	}
	else
	{
		_NR<ASObject> inputRect;
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
		if (inputRect->is<Rectangle>())
			inrect=inputRect->as<Rectangle>()->getRect();
		else
		{
			// it seems adobe allows an object with x,y,width,height properties as inputRect
			asAtom a;
			std::list<tiny_string> ns;
			inputRect->getVariableByMultiname(a,"x",ns,wrk);
			inrect.Xmin=asAtomHandler::toInt(a);
			inputRect->getVariableByMultiname(a,"y",ns,wrk);
			inrect.Ymin=asAtomHandler::toInt(a);
			inputRect->getVariableByMultiname(a,"width",ns,wrk);
			inrect.Xmax=inrect.Xmin+asAtomHandler::toInt(a);
			inputRect->getVariableByMultiname(a,"height",ns,wrk);
			inrect.Ymax=inrect.Ymin+asAtomHandler::toInt(a);
		}
	}
	th->pixels->clipRect(inrect, rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);

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
			
			th->pixels->setPixel(x, y, pixel, th->transparent,false);
			i++;
		}
	}
	th->notifyUsers();
}
ASFUNCTIONBODY_ATOM(BitmapData,compare)
{
	bool isAS3 = wrk->needsActionScript3();
	if (!asAtomHandler::is<BitmapData>(obj))
	{
		if (!isAS3)
			asAtomHandler::setInt(ret,wrk,-1);
		return;
	}
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	
	_NR<BitmapData> otherBitmapData;
	ARG_CHECK(ARG_UNPACK(otherBitmapData));

	if (!isAS3 && th->pixels.isNull())
	{
		asAtomHandler::setInt(ret,wrk,-1);
		return;
	}

	if (otherBitmapData.isNull())
	{
		if (isAS3)
			createError<TypeError>(wrk,kNullPointerError, "otherBitmapData");
		else
			asAtomHandler::setInt(ret,wrk,-2);
		return;
	}

	if (!isAS3 && otherBitmapData->pixels.isNull())
	{
		asAtomHandler::setInt(ret,wrk,-2);
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
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect,false);
	vector<uint32_t> otherpixelvec = otherBitmapData->pixels->getPixelVector(rect,false);
	
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
				uint32_t resultpixel= (((pixel & 0xFF000000) - (otherpixel & 0xFF000000))&0xFF000000) | 0x00FFFFFF;
				res->pixels->setPixel(x, y, resultpixel, true,false);
			}
			else 
			{
				different = true;
				uint32_t resultpixel= (((pixel & 0x00FF0000) - (otherpixel & 0x00FF0000)) &0x00FF0000) |
									   (((pixel & 0x0000FF00) - (otherpixel & 0x0000FF00)) &0x0000FF00) |
									   (((pixel & 0x000000FF) - (otherpixel & 0x000000FF)) &0x000000FF);
				resultpixel |= 0xFF000000;
				res->pixels->setPixel(x, y, resultpixel, true,false);
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
	if (th->checkDisposed(ret))
		return;
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

uint32_t LehmerRandom(uint32_t& seed)
{
	seed = (uint64_t(seed) * 16807U) % 2147483647;
	return seed;
}
ASFUNCTIONBODY_ATOM(BitmapData,noise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	int32_t randomSeed;
	uint32_t low;
	uint32_t high;
	uint32_t channelOptions;
	bool grayScale;
	ARG_CHECK(ARG_UNPACK(randomSeed)(low, 0) (high, 255) (channelOptions, 7) (grayScale, false));
	
	uint32_t randomval = randomSeed <= 0 ? -randomSeed+1 : randomSeed;
	if (high < low)
		high=low;
	uint32_t range = (uint8_t)high-(uint8_t)low;
	for (int32_t y=0; y<th->getHeight(); y++)
	{
		for (int32_t x=0; x<th->getWidth(); x++)
		{
			uint32_t pixel = 0;
			
			if (grayScale)
			{
				uint8_t v = (LehmerRandom(randomval) % (range +1) + low) & 0xff;
				pixel |= v<<16 | v<<8 | v;
				if((channelOptions & 0x8) == 0x8) // A
					pixel |= (LehmerRandom(randomval) % (range +1) + low)<<24;
				else
					pixel |= 0xff<<24;
			}
			else
			{
				if((channelOptions & 0x1) == 0x1) // R
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<16;
				if((channelOptions & 0x2) == 0x2) // G
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<8;
				if((channelOptions & 0x4) == 0x4) // B
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff);
				if((channelOptions & 0x8) == 0x8) // A
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<24;
				else
					pixel |= 0xff<<24;
			}
			th->pixels->setPixel(x, y,pixel,true,false);
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,perlinNoise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
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
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
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
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

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
ASFUNCTIONBODY_ATOM(BitmapData,pixelDissolve)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	int32_t randomSeed;
	int32_t numPixels;
	uint32_t fillColor;
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (randomSeed, 0) (numPixels, 0) (fillColor, 0));

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.pixelDissolve not implemented");
}
