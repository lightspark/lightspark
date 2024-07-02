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

#include "scripting/flash/filters/ConvolutionFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "backends/rendering.h"
#include "backends/cachedsurface.h"
#include "scripting/toplevel/Array.h"


using namespace std;
using namespace lightspark;


ConvolutionFilter::ConvolutionFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_CONVOLUTIONFILTER),
	alpha(0.0),
	bias(0.0),
	clamp(true),
	color(0),
	divisor(1.0),
	matrixX(0),
	matrixY(0),
	preserveAlpha(true)
{
}
ConvolutionFilter::ConvolutionFilter(ASWorker* wrk,Class_base* c, const CONVOLUTIONFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_CONVOLUTIONFILTER),
	alpha(filter.DefaultColor.af()),
	bias((FLOAT)filter.Bias),
	clamp(filter.Clamp),
	color(RGB(filter.DefaultColor.Red,filter.DefaultColor.Green,filter.DefaultColor.Blue).toUInt()),
	divisor((FLOAT)filter.Divisor),
	matrixX(filter.MatrixX),
	matrixY((uint32_t)filter.MatrixY),
	preserveAlpha(filter.PreserveAlpha)
{
	LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter from Tag");
	if (filter.Matrix.size())
	{
		matrix = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.Matrix.begin();
		while (it != filter.Matrix.end())
		{
			matrix->push(asAtomHandler::fromNumber(wrk,(FLOAT)*it,false));
			it++;
		}
	}
}

void ConvolutionFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,alpha);
	REGISTER_GETTER_SETTER(c,bias);
	REGISTER_GETTER_SETTER(c,clamp);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,divisor);
	REGISTER_GETTER_SETTER(c,matrix);
	REGISTER_GETTER_SETTER(c,matrixX);
	REGISTER_GETTER_SETTER(c,matrixY);
	REGISTER_GETTER_SETTER(c,preserveAlpha);
}

void ConvolutionFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	xpos *= scalex;
	ypos *= scaley;
	// spec is not really clear how this should be implemented, especially when using something different than a 3x3 matrix:
	// "
	// For a 3 x 3 matrix convolution, the following formula is used for each independent color channel:
	// dst (x, y) = ((src (x-1, y-1) * a0 + src(x, y-1) * a1....
	//					  src(x, y+1) * a7 + src (x+1,y+1) * a8) / divisor) + bias
	// "
	uint32_t mX = matrixX;
	uint32_t mY = matrixY;
	number_t* m =new number_t[mX*mY];
	for (uint32_t y=0; y < mY ; y++)
	{
		for (uint32_t x=0; x < mX ; x++)
		{
			m[y*mX+x] = matrix->size() < y*mX+x ? asAtomHandler::toNumber(matrix->at(y*mX+x)) : 0;
		}
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
	int32_t mstartpos=-(mY/2*width*4+mX/2);
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	number_t realdivisor = divisor==0 ? 1.0 : divisor;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		number_t redResult   = 0;
		number_t greenResult = 0;
		number_t blueResult  = 0;
		number_t alphaResult = 0;
		for (uint32_t y=0; y <mY; y++)
		{
			for (uint32_t x=0; x <mX; x++)
			{
				if (
						(i%(width*4) <= mX/2
						|| i%(width*4) >= (width-mX/2)*4
						|| i/(width*4) <= mY/2
						|| i/(width*4) >= height-mY/2))
				{
					if (clamp)
					{
						alphaResult += number_t(tmpdata[i+3])*m[y*mX+x];
						redResult   += number_t(tmpdata[i+2])*m[y*mX+x];
						greenResult += number_t(tmpdata[i+1])*m[y*mX+x];
						blueResult  += number_t(tmpdata[i  ])*m[y*mX+x];
					}
					else
					{
						alphaResult += ((alpha*255)     )*m[y*mX+x];
						redResult   += ((color>>16)&0xff)*m[y*mX+x];
						greenResult += ((color>> 8)&0xff)*m[y*mX+x];
						blueResult  += ((color    )&0xff)*m[y*mX+x];
					}
				}
				else
				{
					alphaResult += number_t(tmpdata[mstartpos + y*mY +x+3])*m[y*mX+x];
					redResult   += number_t(tmpdata[mstartpos + y*mY +x+2])*m[y*mX+x];
					greenResult += number_t(tmpdata[mstartpos + y*mY +x+1])*m[y*mX+x];
					blueResult  += number_t(tmpdata[mstartpos + y*mY +x  ])*m[y*mX+x];
				}
			}
		}
		data[targetpos  ] = (max(int32_t(0),min(int32_t(0xff),int32_t(blueResult  / realdivisor + bias))));
		data[targetpos+1] = (max(int32_t(0),min(int32_t(0xff),int32_t(greenResult / realdivisor + bias))));
		data[targetpos+2] = (max(int32_t(0),min(int32_t(0xff),int32_t(redResult   / realdivisor + bias))));
		if (!preserveAlpha)
			data[targetpos+3] = (max(int32_t(0),min(int32_t(0xff),int32_t(alphaResult / realdivisor + bias))));
		mstartpos+=4;
	}
	delete[] m;
	delete[] tmpdata;
}

void ConvolutionFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (matrix)
		matrix->prepareShutdown();
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,alpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,bias)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,clamp)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,color)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,divisor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrix)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,preserveAlpha)

ASFUNCTIONBODY_ATOM(ConvolutionFilter,_constructor)
{
	ConvolutionFilter *th = asAtomHandler::as<ConvolutionFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->matrixX,0)(th->matrixY,0)(th->matrix,NullRef)(th->divisor,1.0)(th->bias,0.0)(th->preserveAlpha,true)(th->clamp,true)(th->color,0)(th->alpha,0.0));
}

bool ConvolutionFilter::compareFILTER(const FILTER& filter) const
{
	LOG(LOG_NOT_IMPLEMENTED, "comparing ConvolutionFilter");
	return false;
}
void ConvolutionFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	if (step == 0)
	{
		args[0]=float(FILTERSTEP_CONVOLUTION);
		args[1]=bias;
		args[2]=clamp;
		args[3]=divisor == 0.0 ? 1.0 : divisor;
		args[4]=preserveAlpha;
		RGBA c = RGBA(color,0);
		args[5]=c.rf();
		args[6]=c.gf();
		args[7]=c.bf();
		args[8]=alpha;
		float realMatrixX=abs(floor(matrixX));
		float realMatrixY=abs(floor(matrixY));
		if (matrix.isNull() || matrix->size() < realMatrixX*realMatrixY)
		{
			LOG(LOG_ERROR,"ConvolutionFilter invalid matrix");
			realMatrixX=0;
			realMatrixY=0;
		}
		if (realMatrixX*realMatrixX > FILTERDATA_MAXSIZE-13)
		{
			LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter matrix is to big:"<<matrixX<<"*"<<matrixY);
			realMatrixX=0;
			realMatrixY=0;
		}
		args[9]=realMatrixX;
		args[10]=realMatrixY;
		int n = realMatrixX*realMatrixY;
		for (int i=0; i < n; i++)
		{
			args[i+11] = asAtomHandler::toNumber(matrix->at(i));
		}
	}
	else
		args[0]=0;

}
BitmapFilter* ConvolutionFilter::cloneImpl() const
{
	ConvolutionFilter* cloned = Class<ConvolutionFilter>::getInstanceS(getInstanceWorker());
	cloned->alpha = alpha;
	cloned->bias = bias;
	cloned->clamp = clamp;
	cloned->color = color;
	cloned->divisor = divisor;
	cloned->matrix = matrix;
	cloned-> matrixX = matrixX;
	cloned-> matrixY = matrixY;
	cloned->preserveAlpha = preserveAlpha;
	return cloned;
}


