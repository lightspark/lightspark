/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012 Alessandro Pignotti (a.pignotti@sssup.it)

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

BitmapData::BitmapData(Class_base* c):ASObject(c),BitmapContainer(c->memoryAccount),disposed(false)
{
}

BitmapData::BitmapData(Class_base* c, const BitmapContainer& b):ASObject(c),BitmapContainer(b),disposed(false)
{
}

BitmapData::BitmapData(Class_base* c, uint32_t width, uint32_t height):ASObject(c),BitmapContainer(c->memoryAccount),disposed(false)
{
	assert(width!=0 && height!=0);

	uint32_t *pixels=new uint32_t[width*height];
	memset(pixels,0,width*height*sizeof(uint32_t));
	fromRGB(reinterpret_cast<uint8_t *>(pixels), width, height, true);
}

BitmapData::~BitmapData()
{
	data.clear();
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
	c->setDeclaredMethodByQName("rect","",Class<IFunction>::getFunction(getRect),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("copyPixels","",Class<IFunction>::getFunction(copyPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fillRect","",Class<IFunction>::getFunction(fillRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("generateFilterRect","",Class<IFunction>::getFunction(generateFilterRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(hitTest),NORMAL_METHOD,true);
	REGISTER_GETTER(c,width);
	REGISTER_GETTER(c,height);
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
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	ARG_UNPACK(width, 0)(height, 0)(transparent, true)(fillColor, 0xFFFFFFFF);

	ASObject::_constructor(obj,NULL,0);
	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->data.empty())
		return NULL;

	uint32_t *pixels=new uint32_t[width*height];
	uint32_t c=GUINT32_TO_BE(fillColor); // fromRGB expects big endian data
	if(!transparent)
	{
		uint8_t *alpha=reinterpret_cast<uint8_t *>(&c);
		*alpha=0xFF;
	}
	for(uint32_t i=0; i<width*height; i++)
		pixels[i]=c;
	th->fromRGB(reinterpret_cast<uint8_t *>(pixels), width, height, true);
	th->transparent=transparent;

	return NULL;
}

ASFUNCTIONBODY_GETTER(BitmapData, width);
ASFUNCTIONBODY_GETTER(BitmapData, height);
ASFUNCTIONBODY_GETTER(BitmapData, transparent);

ASFUNCTIONBODY(BitmapData,dispose)
{
	BitmapData* th = obj->as<BitmapData>();
	th->width = 0;
	th->height = 0;
	th->data.clear();
	th->data.shrink_to_fit();
	th->disposed = true;
	th->notifyUsers();
	return NULL;
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix)
{
	//Create an InvalidateQueue to store all the hierarchy of objects that must be drawn
	SoftwareInvalidateQueue queue;
	d->requestInvalidation(&queue);
	CairoRenderContext ctxt(getData(), width, height);
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
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	_NR<ASString> blendMode;
	_NR<Rectangle> clipRect;
	bool smoothing;
	ARG_UNPACK (drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, NullRef)
					(clipRect, NullRef) (smoothing, false);

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass()) )
		throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");

	if(!ctransform.isNull() || !blendMode.isNull() || !clipRect.isNull() || smoothing)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support many parameters");

	if(drawable->is<BitmapData>())
	{
		BitmapData* data=drawable->as<BitmapData>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		CairoRenderContext ctxt(th->getData(), th->width, th->height);
		//Blit the data while transforming it
		ctxt.transformedBlit(initialMatrix, data->getData(),
				data->getWidth(), data->getHeight(),
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

uint32_t BitmapData::getPixelPriv(uint32_t x, uint32_t y)
{
	if ((int)x >= width || (int)y >= height)
		return 0;

	uint32_t *p=reinterpret_cast<uint32_t *>(&data[y*stride + 4*x]);

	return *p;
}

ASFUNCTIONBODY(BitmapData,getPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);
	if(x<0 || x>th->width || y<0 || y>th->width)
		return abstract_ui(0);

	uint32_t pix=th->getPixelPriv(x, y);
	return abstract_ui(pix & 0xffffff);
}

ASFUNCTIONBODY(BitmapData,getPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);
	if(x<0 || x>th->width || y<0 || y>th->width)
		return abstract_ui(0);

	uint32_t pix=th->getPixelPriv(x, y);
	return abstract_ui(pix);
}

void BitmapData::setPixelPriv(uint32_t x, uint32_t y, uint32_t color, bool setAlpha)
{
	if ((int)x >= width || (int)y >= height)
		return;

	uint32_t *p=reinterpret_cast<uint32_t *>(&data[y*stride + 4*x]);
	if(setAlpha)
		*p=color;
	else
		*p=(*p & 0xff000000) | (color & 0x00ffffff);
	notifyUsers();
}

ASFUNCTIONBODY(BitmapData,setPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	uint32_t x;
	uint32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);
	th->setPixelPriv(x, y, color, false);
	return NULL;
}

ASFUNCTIONBODY(BitmapData,setPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	uint32_t x;
	uint32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);
	th->setPixelPriv(x, y, color, th->transparent);
	return NULL;
}

ASFUNCTIONBODY(BitmapData,getRect)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->width;
	rect->height=th->height;
	return rect;
}

ASFUNCTIONBODY(BitmapData,fillRect)
{
	BitmapData* th=obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_UNPACK(rect)(color);

	//Clip rectangle
	int32_t rectX=rect->x;
	int32_t rectY=rect->y;
	int32_t rectW=rect->width;
	int32_t rectH=rect->height;
	if(rectX<0)
	{
		rectW+=rectX;
		rectX = 0;
	}
	if(rectY<0)
	{
		rectH+=rectY;
		rectY = 0;
	}
	if(rectW > th->width)
		rectW = th->width;
	if(rectH > th->height)
		rectH = th->height;

	for(int32_t i=0;i<rectH;i++)
	{
		for(int32_t j=0;j<rectW;j++)
		{
			uint32_t offset=(i+rectY)*th->stride + (j+rectX)*4;
			uint32_t* ptr=(uint32_t*)(th->getData()+offset);
			*ptr=color;
		}
	}
	return NULL;
}

ASFUNCTIONBODY(BitmapData,copyPixels)
{
	BitmapData* th=obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapData> alphaBitmapData;
	_NR<Point> alphaPoint;
	bool mergeAlpha;
	ARG_UNPACK(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false);

	if(!alphaBitmapData.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "BitmapData.copyPixels doesn't support alpha bitmap");

	int srcLeft=imax((int)sourceRect->x, 0);
	int srcTop=imax((int)sourceRect->y, 0);
	int srcRight=imin((int)(sourceRect->x+sourceRect->width), source->width);
	int srcBottom=imin((int)(sourceRect->y+sourceRect->height), source->height);

	int destLeft=(int)destPoint->getX();
	int destTop=(int)destPoint->getY();
	if(destLeft<0)
	{
		srcLeft+=-destLeft;
		destLeft=0;
	}
	if(destTop<0)
	{
		srcTop+=-destTop;
		destTop=0;
	}

	int copyWidth=imin(srcRight-srcLeft, th->width-destLeft);
	int copyHeight=imin(srcBottom-srcTop, th->height-destTop);

	if(copyWidth<=0 || copyHeight<=0)
		return NULL;

	if(mergeAlpha==false)
	{
		//Fast path using memmove
		for(int i=0; i<copyHeight; i++)
		{
			memmove(th->getData() + (destTop+i)*th->stride + 4*destLeft,
				source->getData() + (srcTop+i)*source->stride + 4*srcLeft,
				4*copyWidth);
		}
	}
	else
	{
		//Slow path using Cairo
		CairoRenderContext ctxt(th->getData(), th->width, th->height);
		ctxt.simpleBlit(destLeft, destTop, source->getData(), source->width, source->height,
				srcLeft, srcTop, copyWidth, copyHeight);
	}

	return NULL;
}

ASFUNCTIONBODY(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->width;
	rect->height=th->height;
	return rect;
}

ASFUNCTIONBODY(BitmapData,hitTest)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->disposed)
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData");

	_NR<Point> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_UNPACK (firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1);

	if(!secondObject->getClass() || !secondObject->getClass()->isSubClass(Class<Point>::getClass()))
		throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");

	if(!secondBitmapDataPoint.isNull() || secondAlphaThreshold!=1)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest does not expect some parameters");

	Point* secondPoint = secondObject->as<Point>();

	uint32_t pix=th->getPixelPriv(secondPoint->getX()-firstPoint->getX(), secondPoint->getY()-firstPoint->getY());
	if((pix>>24)>=firstAlphaThreshold)
		return abstract_b(true);
	else
		return abstract_b(false);
}
