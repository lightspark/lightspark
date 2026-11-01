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

#include "scripting/flash/filters/BlurFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/toplevel/Integer.h"


using namespace std;
using namespace lightspark;


BlurFilter::BlurFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_BLURFILTER),blurX(4.0),blurY(4.0),quality(1)
{
}
BlurFilter::BlurFilter(ASWorker* wrk,Class_base* c,const BLURFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_BLURFILTER),blurX(filter.BlurX),blurY(filter.BlurY),quality(filter.Passes)
{
}

void BlurFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, blurX,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, blurY,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, quality,Integer);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(BlurFilter, blurX, onSetBlurX)
ASFUNCTIONBODY_GETTER_SETTER_CB(BlurFilter, blurY, onSetBlurY)
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, quality)

ASFUNCTIONBODY_ATOM(BlurFilter,_constructor)
{
	BlurFilter *th = asAtomHandler::as<BlurFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->blurX,4.0)(th->blurY,4.0)(th->quality,1))
	th->onSetBlurX(0);
	th->onSetBlurY(0);
}
void BlurFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality);

	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t targetwidth = target->getWidth();
	uint8_t* data = target->getData();
	for (uint32_t i = 0; i < height; i++)
	{
		memcpy(data+((int(ypos)+i)*targetwidth+int(xpos))*4, tmpdata+i*width*4,width*4);
	}
	delete[] tmpdata;
}

bool BlurFilter::compareFILTER(const FILTER& filter) const
{
	return filter.FilterID == FILTER::FILTER_BLUR
			&& filter.BlurFilter.BlurX == this->blurX
			&& filter.BlurFilter.BlurY == this->blurY
			&& filter.BlurFilter.Passes == this->quality;
}
void BlurFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	uint32_t nextstep;
	if (getRenderFilterArgsBlur(args,blurX,blurY,step,quality,nextstep))
		return;
	else
		args[0]=0.0;
}
BitmapFilter* BlurFilter::cloneImpl() const
{
	BlurFilter* cloned = Class<BlurFilter>::getInstanceS(getInstanceWorker());
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->quality = quality;
	return cloned;
}

void BlurFilter::onSetBlurX(number_t oldval)
{
	if (blurX < 0.0)
		blurX = 0.0;
	if (blurX > 255.0)
		blurX = 255.0;
}

void BlurFilter::onSetBlurY(number_t oldval)
{
	if (blurY < 0.0)
		blurY = 0.0;
	if (blurY > 255.0)
		blurY = 255.0;
}

