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

Bitmap::Bitmap(ASWorker* wrk, Class_base* c):
	DisplayObject(wrk,c),TokenContainer(this,&bitmaptokens,1.0),fs(0xff),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
}
Bitmap::Bitmap(ASWorker* wrk, Class_base* c, LoaderInfo* li, std::istream *s, FILE_TYPE type):
	DisplayObject(wrk,c),TokenContainer(this,&bitmaptokens,1.0),fs(0xff),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	setLoaderInfo(li);

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
	setSize(bitmapData->getWidth(), bitmapData->getHeight());
	updatedData();
	afterConstruction();
}

Bitmap::Bitmap(ASWorker* wrk, Class_base* c, _R<BitmapData> data, bool startupload) :
	DisplayObject(wrk,c),TokenContainer(this,&bitmaptokens,1.0),size(data->getWidth(), data->getHeight()),fs(0xff),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	bitmapData = data;
	bitmapData->addUser(this,startupload);
	updatedData();
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
	bitmaptokens.clear();
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
		th->setSize(_bitmapData->getWidth(), _bitmapData->getHeight());
		th->updatedData();
	}
}

void Bitmap::onBitmapData(_NR<BitmapData> old)
{
	if(!old.isNull())
		old->removeUser(this);
	if(!bitmapData.isNull())
	{
		bitmapData->addUser(this);
		setSize(bitmapData->getWidth(), bitmapData->getHeight());
	}
	else
		setSize(Vector2());
	geometryChanged();
	updatedData();
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
void Bitmap::setupTokens()
{
	bitmaptokens.clear();
	if (bitmapData)
	{
		if (!bitmaptokens.filltokens)
			bitmaptokens.filltokens=_MR(new tokenListRef());
		fs.FillStyleType = smoothing ? CLIPPED_BITMAP : NON_SMOOTHED_CLIPPED_BITMAP;
		fs.bitmap = bitmapData->getBitmapContainer();
		bitmaptokens.filltokens->tokens.reserve(14);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(SET_FILL).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(fs).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(MOVE).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(Vector2(0,0)).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(Vector2(number_t(bitmapData->getWidth())/scaling,0)).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(Vector2(number_t(bitmapData->getWidth())/scaling,number_t(bitmapData->getHeight())/scaling)).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(Vector2(0,number_t(bitmapData->getHeight())/scaling)).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(Vector2(0,0)).uval);
		bitmaptokens.filltokens->tokens.push_back(GeomToken(CLEAR_FILL).uval);
		bitmaptokens.boundsRect = RECT(0,0,size.x,size.y);
	}
}
void Bitmap::updatedData()
{
	hasChanged=true;
	setupTokens();
	requestInvalidation(getSystemState());
}
void Bitmap::refreshSurfaceState()
{
	if (bitmapData)
		bitmapData->checkForUpload();
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	if (bitmapData.isNull())
		return false;
	xmin = 0;
	ymin = 0;
	xmax = size.x;
	ymax = size.y;
	return true;
}

_NR<DisplayObject> Bitmap::hitTestImpl(const Vector2f&, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
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
	TokenContainer::requestInvalidation(q,forceTextureRefresh);
}

IDrawable *Bitmap::invalidate(bool smoothing)
{
	if (this->bitmapData)
		this->bitmapData->getBitmapContainer()->flushRenderCalls(getSystemState()->getRenderThread(),nullptr,false);
	return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,false,*this->tokens);
}

void Bitmap::setupTemporaryBitmap(BitmapData* data)
{
	data->incRef();
	bitmapData =_MR<BitmapData>(data);
	setSize(data->getWidth(), data->getHeight());
	hasChanged=true;
	setupTokens();
	resetNeedsTextureRecalculation();
}
