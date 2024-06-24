/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/filters/DisplacementMapFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/geom/Point.h"

using namespace std;
using namespace lightspark;

DisplacementMapFilter::DisplacementMapFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_DISPLACEMENTFILTER)
{
}

void DisplacementMapFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,alpha);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,componentX);
	REGISTER_GETTER_SETTER(c,componentY);
	REGISTER_GETTER_SETTER(c,mapBitmap);
	REGISTER_GETTER_SETTER(c,mapPoint);
	REGISTER_GETTER_SETTER(c,mode);
	REGISTER_GETTER_SETTER(c,scaleX);
	REGISTER_GETTER_SETTER(c,scaleY);
}

void DisplacementMapFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	xpos *= scalex;
	ypos *= scaley;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint32_t mapx = mapPoint ? uint32_t(mapPoint->getX()) : 0;
	uint32_t mapy = mapPoint ? uint32_t(mapPoint->getX()) : 0;
	if (mapBitmap.isNull() || mapBitmap->getBitmapContainer().isNull() || (mapBitmap->getBitmapContainer()->getWidth()-mapx)*(mapBitmap->getBitmapContainer()->getHeight()-mapy) > size)
		return;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	
	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	uint32_t mapPointstartpos = mapPoint ? (mapx+mapy*mapBitmap->getWidth())*4 : 0;
	uint8_t* mapbitmapdata=mapBitmap->getBitmapContainer()->getData();
	uint32_t mapchannelindexX = 0;
	uint32_t mapchannelindexY = 0;
	switch (componentX)
	{
		case BitmapDataChannel::ALPHA:
			mapchannelindexX=3;
			break;
		case BitmapDataChannel::RED:
			mapchannelindexX=2;
			break;
		case BitmapDataChannel::GREEN:
			mapchannelindexX=1;
			break;
		case BitmapDataChannel::BLUE:
			mapchannelindexX=0;
			break;
	}
	switch (componentY)
	{
		case BitmapDataChannel::ALPHA:
			mapchannelindexY=3;
			break;
		case BitmapDataChannel::RED:
			mapchannelindexY=2;
			break;
		case BitmapDataChannel::GREEN:
			mapchannelindexY=1;
			break;
		case BitmapDataChannel::BLUE:
			mapchannelindexY=0;
			break;
	}

	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
		{
			startpos+=target->getWidth();
			mapPointstartpos+=mapBitmap->getWidth();
		}
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		uint32_t mappointpos = (mapPointstartpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		int32_t alphaResult = tmpdata[i+3 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t redResult   = tmpdata[i+2 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t greenResult = tmpdata[i+1 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t blueResult  = tmpdata[i   + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];

		data[targetpos  ] = max(int32_t(0),min(int32_t(0xff),blueResult ));
		data[targetpos+1] = max(int32_t(0),min(int32_t(0xff),greenResult));
		data[targetpos+2] = max(int32_t(0),min(int32_t(0xff),redResult  ));
		data[targetpos+3] = max(int32_t(0),min(int32_t(0xff),alphaResult));
	}
	delete[] tmpdata;
}

void DisplacementMapFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	LOG(LOG_NOT_IMPLEMENTED,"getRenderFilterArgs not yet implemented for "<<this->toDebugString());
	args[0]=0;
}

void DisplacementMapFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (mapBitmap)
		mapBitmap->prepareShutdown();
	if (mapPoint)
		mapPoint->prepareShutdown();
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,alpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,color)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapBitmap)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapPoint)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mode)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleY)

ASFUNCTIONBODY_ATOM(DisplacementMapFilter,_constructor)
{
	DisplacementMapFilter *th = asAtomHandler::as<DisplacementMapFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->mapBitmap,NullRef)(th->mapPoint,NullRef)(th->componentX,0)(th->componentY,0)(th->scaleX,0.0)(th->scaleY,0.0)(th->mode,"wrap")(th->color,0)(th->alpha,0.0));
}

BitmapFilter* DisplacementMapFilter::cloneImpl() const
{
	DisplacementMapFilter* cloned = Class<DisplacementMapFilter>::getInstanceS(getInstanceWorker());
	cloned->alpha = alpha;
	cloned->color = color;
	cloned->componentX = componentX;
	cloned->componentY = componentY;
	cloned->mapBitmap = mapBitmap;
	cloned->mapPoint = mapPoint;
	cloned->mode = mode;
	cloned->scaleX = scaleX;
	cloned->scaleY = scaleY;
	return cloned;
}
