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

#include "scripting/flash/filters/ColorMatrixFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/toplevel/Array.h"

using namespace std;
using namespace lightspark;

ColorMatrixFilter::ColorMatrixFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_COLORMATRIXFILTER)
{
}
ColorMatrixFilter::ColorMatrixFilter(ASWorker* wrk, Class_base* c, const COLORMATRIXFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_COLORMATRIXFILTER)
{
	matrix = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	for (uint32_t i = 0; i < 20 ; i++)
	{
		FLOAT f = filter.Matrix[i];
		matrix->push(asAtomHandler::fromNumber(wrk,f,false));
	}
}

void ColorMatrixFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, matrix);
}

void ColorMatrixFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	xpos *= scalex;
	ypos *= scaley;
	assert_and_throw(matrix->size() >= 20);
	number_t m[20];
	for (int i=0; i < 20; i++)
	{
		m[i] = asAtomHandler::toNumber(matrix->at(i));
	}
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	
	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		number_t srcA = number_t(tmpdata[i+3]);
		number_t srcR = number_t(tmpdata[i+2])*srcA/255.0;
		number_t srcG = number_t(tmpdata[i+1])*srcA/255.0;
		number_t srcB = number_t(tmpdata[i  ])*srcA/255.0;
		number_t redResult   = (m[0 ]*srcR) + (m[1 ]*srcG) + (m[2 ]*srcB) + (m[3 ]*srcA) + m[4 ];
		number_t greenResult = (m[5 ]*srcR) + (m[6 ]*srcG) + (m[7 ]*srcB) + (m[8 ]*srcA) + m[9 ];
		number_t blueResult  = (m[10]*srcR) + (m[11]*srcG) + (m[12]*srcB) + (m[13]*srcA) + m[14];
		number_t alphaResult = (m[15]*srcR) + (m[16]*srcG) + (m[17]*srcB) + (m[18]*srcA) + m[19];

		data[targetpos  ] = (max(int32_t(0),min(int32_t(0xff),int32_t(blueResult *alphaResult/255.0))));
		data[targetpos+1] = (max(int32_t(0),min(int32_t(0xff),int32_t(greenResult*alphaResult/255.0))));
		data[targetpos+2] = (max(int32_t(0),min(int32_t(0xff),int32_t(redResult  *alphaResult/255.0))));
		data[targetpos+3] = (max(int32_t(0),min(int32_t(0xff),int32_t(alphaResult))));
	}
	delete[] tmpdata;
}

ASFUNCTIONBODY_GETTER_SETTER(ColorMatrixFilter, matrix)

ASFUNCTIONBODY_ATOM(ColorMatrixFilter,_constructor)
{
	ColorMatrixFilter *th = asAtomHandler::as<ColorMatrixFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->matrix,NullRef));
}

bool ColorMatrixFilter::compareFILTER(const FILTER& filter) const
{
	if (filter.FilterID == FILTER::FILTER_COLORMATRIX
		&& this->matrix && this->matrix->size()==20)
	{
		for (uint32_t i =0; i < 20; i++)
		{
			FLOAT f = filter.ColorMatrixFilter.Matrix[i];
			if (f != asAtomHandler::toNumber(this->matrix->at(i)))
				return false;
		}
		return true;
	}
	return false;
}
void ColorMatrixFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	if (step == 0)
	{
		args[0]=float(FILTERSTEP_COLORMATRIX);
		assert_and_throw(matrix->size() >= 20);
		for (int i=0; i < 20; i++)
		{
			args[i+1] = asAtomHandler::toNumber(matrix->at(i));
		}
	}
	else
		args[0]=0;
}

BitmapFilter* ColorMatrixFilter::cloneImpl() const
{
	ColorMatrixFilter *cloned = Class<ColorMatrixFilter>::getInstanceS(getInstanceWorker());
	if (!matrix.isNull())
	{
		matrix->incRef();
		cloned->matrix = matrix;
	}
	return cloned;
}

void ColorMatrixFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (matrix)
		matrix->prepareShutdown();
}
