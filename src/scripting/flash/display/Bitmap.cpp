/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <list>

#include "scripting/class.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/argconv.h"
#include "backends/rendering.h"
#include "backends/cachedsurface.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

Bitmap::Bitmap(ASWorker* wrk, Class_base* c, _NR<LoaderInfo> li, std::istream *s, FILE_TYPE type):
	DisplayObject(wrk,c),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	if(li)
		loaderInfo = li;

	bitmapData = _MR(Class<BitmapData>::getInstanceS(wrk));
	bitmapData->addUser(this);
	if(!s)
		return;

	if(type==FT_UNKNOWN)
	{
		// Try to detect the format from the stream
		UI8 Signature[4];
		(*s) >> Signature[0] >> Signature[1] >> Signature[2] >> Signature[3];
		type=ParseThread::recognizeFile(Signature[0], Signature[1],
						Signature[2], Signature[3]);
		s->putback(Signature[3]).putback(Signature[2]).
		   putback(Signature[1]).putback(Signature[0]);
	}

	switch(type)
	{
		case FT_JPEG:
			bitmapData->getBitmapContainer()->fromJPEG(*s);
			break;
		case FT_PNG:
			bitmapData->getBitmapContainer()->fromPNG(*s);
			break;
		case FT_GIF:
			LOG(LOG_NOT_IMPLEMENTED, "GIFs are not yet supported");
			break;
		default:
			LOG(LOG_ERROR,"Unsupported image type");
			break;
	}
	Bitmap::updatedData();
	afterConstruction();
}

Bitmap::Bitmap(ASWorker* wrk, Class_base* c, _R<BitmapData> data, bool startupload) : DisplayObject(wrk,c),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	bitmapData = data;
	bitmapData->addUser(this,startupload);
	Bitmap::updatedData();
}

Bitmap::~Bitmap()
{
}

bool Bitmap::destruct()
{
	if(!bitmapData.isNull())
		bitmapData->removeUser(this);
	bitmapData.reset();
	smoothing = false;
	return DisplayObject::destruct();
}

void Bitmap::finalize()
{
	bitmapData.reset();
	DisplayObject::finalize();
}

void Bitmap::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (bitmapData)
		bitmapData->prepareShutdown();
}

void Bitmap::setOnStage(bool staged, bool force, bool inskipping)
{
	DisplayObject::setOnStage(staged,force,inskipping);
}

void Bitmap::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bitmapData,BitmapData);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,smoothing,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,pixelSnapping,ASString);
}

ASFUNCTIONBODY_ATOM(Bitmap,_constructor)
{
	tiny_string _pixelSnapping;
	_NR<BitmapData> _bitmapData;
	Bitmap* th = asAtomHandler::as<Bitmap>(obj);
	ARG_CHECK(ARG_UNPACK(_bitmapData, NullRef)(_pixelSnapping, "auto")(th->smoothing, false));

	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);

	if(_pixelSnapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support pixelSnapping:"<<_pixelSnapping);
	th->pixelSnapping = _pixelSnapping;

	if(!_bitmapData.isNull())
	{
		th->bitmapData=_bitmapData;
		th->bitmapData->addUser(th);
		th->updatedData();
	}
}

void Bitmap::onBitmapData(_NR<BitmapData> old)
{
	if(!old.isNull())
		old->removeUser(this);
	if(!bitmapData.isNull())
		bitmapData->addUser(this);
	geometryChanged();
	Bitmap::updatedData();
}

void Bitmap::onSmoothingChanged(bool /*old*/)
{
	updatedData();
}

void Bitmap::onPixelSnappingChanged(tiny_string snapping)
{
	if(snapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap doesn't support pixelSnapping:"<<snapping);
	pixelSnapping = snapping;
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,bitmapData,onBitmapData)
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,smoothing,onSmoothingChanged)
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,pixelSnapping,onPixelSnappingChanged)

void Bitmap::updatedData()
{
	hasChanged=true;
	requestInvalidation(getSystemState());
}
void Bitmap::refreshSurfaceState()
{
	if(bitmapData.isNull() || bitmapData->getBitmapContainer().isNull())
	{
		if (cachedSurface->isChunkOwner && cachedSurface->tex)
			cachedSurface->tex->makeEmpty();
		else
			cachedSurface->tex=nullptr;
		return;
	}
	bitmapData->checkForUpload();
	cachedSurface->tex = &bitmapData->getBitmapContainer()->bitmaptexture;
	cachedSurface->isChunkOwner=false;
	cachedSurface->isValid=true;
	if (cachedSurface->getState())
		cachedSurface->getState()->needsFilterRefresh=true;
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	if (bitmapData.isNull())
		return false;
	xmin = 0;
	ymin = 0;
	xmax = bitmapData->getWidth();
	ymax = bitmapData->getHeight();
	return true;
}

_NR<DisplayObject> Bitmap::hitTestImpl(const Vector2f&, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Simple check inside the area, opacity data should not be considered
	//NOTE: on the X axis the 0th line must be ignored, while the one past the width is valid
	//NOTE: on the Y asix the 0th line is valid, while the one past the width is not
	//NOTE: This is tested behaviour!
	//TODO: Add a point intersect function to RECT, and use that instead.
	if(!bitmapData.isNull() && localPoint.x > 0 && localPoint.x <= bitmapData->getWidth() && localPoint.y >=0 && localPoint.y < bitmapData->getHeight())
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

IntSize Bitmap::getBitmapSize() const
{
	if(bitmapData.isNull())
		return IntSize(0, 0);
	else
		return IntSize(bitmapData->getWidth(), bitmapData->getHeight());
}

void Bitmap::requestInvalidation(InvalidateQueue *q, bool forceTextureRefresh)
{
	incRef();
	// texture recalculation is never needed for legacy bitmaps
	resetNeedsTextureRecalculation();
	requestInvalidationFilterParent(q);
	q->addToInvalidateQueue(_MR(this));
}

IDrawable *Bitmap::invalidate(bool smoothing)
{
	number_t bxmin,bxmax,bymin,bymax;
	if(!boundsRectWithoutChildren(bxmin,bxmax,bymin,bymax,false))
	{
		//No contents, nothing to do
		return nullptr;
	}
	if (this->bitmapData->getWidth()==0 || this->bitmapData->getHeight()==0)
		return nullptr;
	return new BitmapRenderer(this->bitmapData->getBitmapContainer()
				, bxmin, bymin, this->bitmapData->getWidth(), this->bitmapData->getHeight()
				, 1, 1
				, this->isMask(),false
				, this->getConcatenatedAlpha()
				, this->colorTransform.getPtr() ? *this->colorTransform.getPtr() :ColorTransformBase()
				, smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE
				, this->getBlendMode(),getMatrix());
}
void Bitmap::invalidateForRenderToBitmap(RenderDisplayObjectToBitmapContainer* container)
{
	DisplayObject::invalidateForRenderToBitmap(container);
}
