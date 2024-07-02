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

#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/flash/display/TokenContainer.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/IGraphicsData.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/argconv.h"
#include "swf.h"

#define TWIPS_FACTOR 20.0f
using namespace std;
using namespace lightspark;

void Graphics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable=true;
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRect","",c->getSystemState()->getBuiltinFunction(drawRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRect","",c->getSystemState()->getBuiltinFunction(drawRoundRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRectComplex","",c->getSystemState()->getBuiltinFunction(drawRoundRectComplex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawCircle","",c->getSystemState()->getBuiltinFunction(drawCircle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawEllipse","",c->getSystemState()->getBuiltinFunction(drawEllipse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawPath","",c->getSystemState()->getBuiltinFunction(drawPath),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",c->getSystemState()->getBuiltinFunction(drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawGraphicsData","",c->getSystemState()->getBuiltinFunction(drawGraphicsData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",c->getSystemState()->getBuiltinFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("curveTo","",c->getSystemState()->getBuiltinFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("cubicCurveTo","",c->getSystemState()->getBuiltinFunction(cubicCurveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",c->getSystemState()->getBuiltinFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineBitmapStyle","",c->getSystemState()->getBuiltinFunction(lineBitmapStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineGradientStyle","",c->getSystemState()->getBuiltinFunction(lineGradientStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",c->getSystemState()->getBuiltinFunction(lineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",c->getSystemState()->getBuiltinFunction(beginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",c->getSystemState()->getBuiltinFunction(beginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginBitmapFill","",c->getSystemState()->getBuiltinFunction(beginBitmapFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",c->getSystemState()->getBuiltinFunction(endFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readGraphicsData","",c->getSystemState()->getBuiltinFunction(readGraphicsData,1,Class<Vector>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	
}

ASFUNCTIONBODY_ATOM(Graphics,_constructor)
{
}

ASFUNCTIONBODY_ATOM(Graphics,clear)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->inFilling = false;
	th->hasChanged = false;
	th->currentrenderindex=1-th->currentrenderindex;
	th->tokens[th->currentrenderindex].clear();
	th->fillStyles[th->currentrenderindex].clear();
	th->lineStyles[th->currentrenderindex].clear();
	th->needsRefresh=false;
	th->tokensHaveChanged=false;
}

ASFUNCTIONBODY_ATOM(Graphics,moveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==2);
	if (th->inFilling)
		th->dorender(true);

	int32_t x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int32_t y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(x,y);
	th->movex = x;
	th->movey = y;
	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(Vector2(x, y)));
	}
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(Vector2(x, y)));
}

ASFUNCTIONBODY_ATOM(Graphics,lineTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==2);

	int x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(x,y);

	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(x, y)));
	}
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(x, y)));

	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,curveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);
	int controlX=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int controlY=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;

	int anchorX=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	int anchorY=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(controlX,controlY);
	th->tokens[th->currentrenderindex].updateTokenBounds(anchorX,anchorY);

	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(CURVE_QUADRATIC));
		th->AddFillToken(GeomToken(Vector2(controlX, controlY)));
		th->AddFillToken(GeomToken(Vector2(anchorX, anchorY)));
	}
	th->AddStrokeToken(GeomToken(CURVE_QUADRATIC));
	th->AddStrokeToken(GeomToken(Vector2(controlX, controlY)));
	th->AddStrokeToken(GeomToken(Vector2(anchorX, anchorY)));

	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,cubicCurveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==6);

	int control1X=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int control1Y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;

	int control2X=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	int control2Y=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;

	int anchorX=asAtomHandler::toNumber(args[4])*TWIPS_FACTOR;
	int anchorY=asAtomHandler::toNumber(args[5])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(control1X,control1Y);
	th->tokens[th->currentrenderindex].updateTokenBounds(control2X,control2Y);
	th->tokens[th->currentrenderindex].updateTokenBounds(anchorX,anchorY);

	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(control1X, control1Y)));
		th->AddFillToken(GeomToken(Vector2(control2X, control2Y)));
		th->AddFillToken(GeomToken(Vector2(anchorX, anchorY)));
	}
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(control1X, control1Y)));
	th->AddStrokeToken(GeomToken(Vector2(control2X, control2Y)));
	th->AddStrokeToken(GeomToken(Vector2(anchorX, anchorY)));

	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(false);
}

/* KAPPA = 4 * (sqrt2 - 1) / 3
 * This value was found in a Python prompt:
 *
 * >>> 4.0 * (2**0.5 - 1) / 3.0
 *
 * Source: http://whizkidtech.redprince.net/bezier/circle/
 */
const double KAPPA = 0.55228474983079356;

ASFUNCTIONBODY_ATOM(Graphics,drawRoundRect)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==5 || argslen==6);

	double x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	double y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	double width=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	double height=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;
	double ellipseWidth=asAtomHandler::toNumber(args[4])*TWIPS_FACTOR;
	double ellipseHeight=numeric_limits<double>::quiet_NaN();
	th->tokens[th->currentrenderindex].updateTokenBounds(x,y);
	th->tokens[th->currentrenderindex].updateTokenBounds(x+width,y+height);
	if (argslen == 6)
		ellipseHeight=asAtomHandler::toNumber(args[5])*TWIPS_FACTOR;

	if (argslen == 5 || std::isnan(ellipseHeight))
		ellipseHeight=ellipseWidth;

	ellipseHeight /= 2;
	ellipseWidth  /= 2;

	double kappaW = KAPPA * ellipseWidth;
	double kappaH = KAPPA * ellipseHeight;

	/*
	 *    A-----B
	 *   /       \
	 *  H         C
	 *  |         |
	 *  G         D
	 *   \       /
	 *    F-----E
	 * 
	 * Flash starts and stops the pen at 'D', so we will too.
	 */

	if (th->inFilling)
	{
		// D
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(Vector2(x+width, y+height-ellipseHeight)));
	
		// D -> E
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x+width, y+height-ellipseHeight+kappaH)));
		th->AddFillToken(GeomToken(Vector2(x+width-ellipseWidth+kappaW, y+height)));
		th->AddFillToken(GeomToken(Vector2(x+width-ellipseWidth, y+height)));
	
		// E -> F
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(x+ellipseWidth, y+height)));
	
		// F -> G
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x+ellipseWidth-kappaW, y+height)));
		th->AddFillToken(GeomToken(Vector2(x, y+height-kappaH)));
		th->AddFillToken(GeomToken(Vector2(x, y+height-ellipseHeight)));

		// G -> H
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(x, y+ellipseHeight)));
	
		// H -> A
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x, y+ellipseHeight-kappaH)));
		th->AddFillToken(GeomToken(Vector2(x+ellipseWidth-kappaW, y)));
		th->AddFillToken(GeomToken(Vector2(x+ellipseWidth, y)));

		// A -> B
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(x+width-ellipseWidth, y)));
	
		// B -> C
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x+width-ellipseWidth+kappaW, y)));
		th->AddFillToken(GeomToken(Vector2(x+width, y+kappaH)));
		th->AddFillToken(GeomToken(Vector2(x+width, y+ellipseHeight)));

		// C -> D
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(x+width, y+height-ellipseHeight)));
	}
	// D
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(Vector2(x+width, y+height-ellipseHeight)));

	// D -> E
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x+width, y+height-ellipseHeight+kappaH)));
	th->AddStrokeToken(GeomToken(Vector2(x+width-ellipseWidth+kappaW, y+height)));
	th->AddStrokeToken(GeomToken(Vector2(x+width-ellipseWidth, y+height)));

	// E -> F
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(x+ellipseWidth, y+height)));

	// F -> G
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x+ellipseWidth-kappaW, y+height)));
	th->AddStrokeToken(GeomToken(Vector2(x, y+height-kappaH)));
	th->AddStrokeToken(GeomToken(Vector2(x, y+height-ellipseHeight)));

	// G -> H
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(x, y+ellipseHeight)));

	// H -> A
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x, y+ellipseHeight-kappaH)));
	th->AddStrokeToken(GeomToken(Vector2(x+ellipseWidth-kappaW, y)));
	th->AddStrokeToken(GeomToken(Vector2(x+ellipseWidth, y)));

	// A -> B
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(x+width-ellipseWidth, y)));

	// B -> C
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x+width-ellipseWidth+kappaW, y)));
	th->AddStrokeToken(GeomToken(Vector2(x+width, y+kappaH)));
	th->AddStrokeToken(GeomToken(Vector2(x+width, y+ellipseHeight)));

	// C -> D
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(x+width, y+height-ellipseHeight)));

	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawRoundRectComplex)
{
	LOG(LOG_NOT_IMPLEMENTED,"Graphics.drawRoundRectComplex currently draws a normal rect");
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen>=4);

	int x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	int width=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	int height=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(x,y);
	th->tokens[th->currentrenderindex].updateTokenBounds(x+width,y+height);

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(a));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(b));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(c));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(d));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(a));
	}
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(a));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(b));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(c));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(d));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(a));

	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawCircle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==3);

	double x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	double y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	double radius=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(x+radius,y+radius);
	th->tokens[th->currentrenderindex].updateTokenBounds(x-radius,y-radius);

	double kappa = KAPPA*radius;

	if (th->inFilling)
	{
		// right
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(Vector2(x+radius, y)));

		// bottom
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x+radius, y+kappa )));
		th->AddFillToken(GeomToken(Vector2(x+kappa , y+radius)));
		th->AddFillToken(GeomToken(Vector2(x       , y+radius)));

		// left
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x-kappa , y+radius)));
		th->AddFillToken(GeomToken(Vector2(x-radius, y+kappa )));
		th->AddFillToken(GeomToken(Vector2(x-radius, y       )));

		// top
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x-radius, y-kappa )));
		th->AddFillToken(GeomToken(Vector2(x-kappa , y-radius)));
		th->AddFillToken(GeomToken(Vector2(x       , y-radius)));

		// back to right
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(x+kappa , y-radius)));
		th->AddFillToken(GeomToken(Vector2(x+radius, y-kappa )));
		th->AddFillToken(GeomToken(Vector2(x+radius, y       )));
	}
	// right
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(Vector2(x+radius, y)));

	// bottom
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x+radius, y+kappa )));
	th->AddStrokeToken(GeomToken(Vector2(x+kappa , y+radius)));
	th->AddStrokeToken(GeomToken(Vector2(x       , y+radius)));

	// left
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x-kappa , y+radius)));
	th->AddStrokeToken(GeomToken(Vector2(x-radius, y+kappa )));
	th->AddStrokeToken(GeomToken(Vector2(x-radius, y       )));

	// top
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x-radius, y-kappa )));
	th->AddStrokeToken(GeomToken(Vector2(x-kappa , y-radius)));
	th->AddStrokeToken(GeomToken(Vector2(x       , y-radius)));

	// back to right
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(x+kappa , y-radius)));
	th->AddStrokeToken(GeomToken(Vector2(x+radius, y-kappa )));
	th->AddStrokeToken(GeomToken(Vector2(x+radius, y       )));

	th->hasChanged = true;
	th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,drawEllipse)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);

	double left=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	double top=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	double width=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	double height=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(left,top);
	th->tokens[th->currentrenderindex].updateTokenBounds(left+width,top+height);

	double xkappa = KAPPA*width/2.0;
	double ykappa = KAPPA*height/2.0;

	if (th->inFilling)
	{
		// right
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(Vector2(left+width, top+height/2.0)));

		// bottom
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(left+width , top+height/2.0+ykappa)));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0+xkappa, top+height)));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0, top+height)));

		// left
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0-xkappa, top+height)));
		th->AddFillToken(GeomToken(Vector2(left, top+height/2.0+ykappa)));
		th->AddFillToken(GeomToken(Vector2(left, top+height/2.0)));

		// top
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(left, top+height/2.0-ykappa)));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0-xkappa, top)));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0, top)));

		// back to right
		th->AddFillToken(GeomToken(CURVE_CUBIC));
		th->AddFillToken(GeomToken(Vector2(left+width/2.0+xkappa, top)));
		th->AddFillToken(GeomToken(Vector2(left+width, top+height/2.0-ykappa)));
		th->AddFillToken(GeomToken(Vector2(left+width, top+height/2.0)));
	}
	// right
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(Vector2(left+width, top+height/2.0)));

	// bottom
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(left+width , top+height/2.0+ykappa)));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0+xkappa, top+height)));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0, top+height)));

	// left
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0-xkappa, top+height)));
	th->AddStrokeToken(GeomToken(Vector2(left, top+height/2.0+ykappa)));
	th->AddStrokeToken(GeomToken(Vector2(left, top+height/2.0)));

	// top
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(left, top+height/2.0-ykappa)));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0-xkappa, top)));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0, top)));

	// back to right
	th->AddStrokeToken(GeomToken(CURVE_CUBIC));
	th->AddStrokeToken(GeomToken(Vector2(left+width/2.0+xkappa, top)));
	th->AddStrokeToken(GeomToken(Vector2(left+width, top+height/2.0-ykappa)));
	th->AddStrokeToken(GeomToken(Vector2(left+width, top+height/2.0)));

	th->hasChanged = true;
	th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,drawRect)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);

	int x=asAtomHandler::toNumber(args[0])*TWIPS_FACTOR;
	int y=asAtomHandler::toNumber(args[1])*TWIPS_FACTOR;
	int width=asAtomHandler::toNumber(args[2])*TWIPS_FACTOR;
	int height=asAtomHandler::toNumber(args[3])*TWIPS_FACTOR;
	th->tokens[th->currentrenderindex].updateTokenBounds(x,y);
	th->tokens[th->currentrenderindex].updateTokenBounds(x+width,y+height);

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	if (th->inFilling)
	{
		th->AddFillToken(GeomToken(MOVE));
		th->AddFillToken(GeomToken(Vector2(a)));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(b)));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(c)));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(d)));
		th->AddFillToken(GeomToken(STRAIGHT));
		th->AddFillToken(GeomToken(Vector2(a)));
	}
	th->AddStrokeToken(GeomToken(MOVE));
	th->AddStrokeToken(GeomToken(Vector2(a)));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(b)));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(c)));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(d)));
	th->AddStrokeToken(GeomToken(STRAIGHT));
	th->AddStrokeToken(GeomToken(Vector2(a)));

	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawPath)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	_NR<Vector> commands;
	_NR<Vector> data;
	tiny_string winding;
	ARG_CHECK(ARG_UNPACK(commands) (data) (winding, "evenOdd"));

	if (commands.isNull() || data.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}

	th->pathToTokens(commands, data, winding,th->tokens[th->currentrenderindex]);
	th->tokensHaveChanged=true;
	th->hasChanged = true;
	th->dorender(true);
}

void Graphics::pathToTokens(_NR<Vector> commands, _NR<Vector> data,
			    tiny_string winding, tokensVector& tokens)
{
	if (commands.isNull() || data.isNull())
		return;

	if (winding != "evenOdd")
		LOG(LOG_NOT_IMPLEMENTED, "Only event-odd winding implemented in Graphics.drawPath");

	asAtom zero = asAtomHandler::fromInt(0);

	int k = 0;
	for (unsigned int i=0; i<commands->size(); i++)
	{
		asAtom c = commands->at(i);
		switch (asAtomHandler::toInt(c))
		{
			case GRAPHICSPATH_COMMANDTYPE::MOVE_TO:
			{
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(MOVE).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::LINE_TO:
			{
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(STRAIGHT).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::CURVE_TO:
			{
				asAtom acx = data->at(k++, zero);
				asAtom acy = data->at(k++, zero);
				number_t cx = asAtomHandler::toNumber(acx)*TWIPS_FACTOR;
				number_t cy = asAtomHandler::toNumber(acy)*TWIPS_FACTOR;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(CURVE_QUADRATIC).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(cx, cy)).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(cx,cy);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::WIDE_MOVE_TO:
			{
				k+=2;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(MOVE).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::WIDE_LINE_TO:
			{
				k+=2;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(STRAIGHT).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::CUBIC_CURVE_TO:
			{
				asAtom ac1x = data->at(k++, zero);
				asAtom ac1y = data->at(k++, zero);
				asAtom ac2x = data->at(k++, zero);
				asAtom ac2y = data->at(k++, zero);
				number_t c1x = asAtomHandler::toNumber(ac1x)*TWIPS_FACTOR;
				number_t c1y = asAtomHandler::toNumber(ac1y)*TWIPS_FACTOR;
				number_t c2x = asAtomHandler::toNumber(ac2x)*TWIPS_FACTOR;
				number_t c2y = asAtomHandler::toNumber(ac2y)*TWIPS_FACTOR;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
				number_t y = asAtomHandler::toNumber(ay)*TWIPS_FACTOR;
				tokens.filltokens.emplace_back(GeomToken(CURVE_CUBIC).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(c1x, c1y)).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(c2x, c2y)).uval);
				tokens.filltokens.emplace_back(GeomToken(Vector2(x, y)).uval);
				tokens.updateTokenBounds(c1x,c1y);
				tokens.updateTokenBounds(c2x,c2y);
				tokens.updateTokenBounds(x,y);
				break;
			}

			case GRAPHICSPATH_COMMANDTYPE::NO_OP:
			default:
				LOG(LOG_NOT_IMPLEMENTED,"pathToTokens:"<<asAtomHandler::toInt(c));
				break;
		}
	}
}

/* Solve for c in the matrix equation
 *
 * [ 1 x1 y1 ] [ c[0] ]   [ u1 ]
 * [ 1 x2 y2 ] [ c[1] ] = [ u2 ]
 * [ 1 x3 y3 ] [ c[2] ]   [ u3 ]
 *
 * The result will be put in the output parameter c.
 */
void Graphics::solveVertexMapping(double x1, double y1,
				  double x2, double y2,
				  double x3, double y3,
				  double u1, double u2, double u3,
				  double c[3])
{
	double eps = 1e-15;
	double det = fabs(x2*y3 + x1*y2 + y1*x3 - y2*x3 - x1*y3 - y1*x2);

	if (det < eps)
	{
		// Degenerate matrix
		c[0] = c[1] = c[2] = 0;
		return;
	}

	// Symbolic solution of the equation by Gaussian elimination
	if (fabs(x1-x2) < eps)
	{
		c[2] = (u2-u1)/(y2-y1);
		c[1] = (u3 - u1 - (y3-y1)*c[2])/(x3-x1);
		c[0] = u1 - x1*c[1] - y1*c[2];
	}
	else
	{
		c[2] = ((x2-x1)*(u3-u1) - (x3-x1)*(u2-u1))/((y3-y1)*(x2-x1) - (x3-x1)*(y2-y1));
		c[1] = (u2 - u1 - (y2-y1)*c[2])/(x2-x1);
		c[0] = u1 - x1*c[1] - y1*c[2];
	}
}

void Graphics::dorender(bool closepath)
{
	needsRefresh = true;
	if (hasChanged)
	{
		if (inFilling)
		{
			if (closepath)
			{
				AddFillToken(GeomToken(STRAIGHT));
				AddFillToken(GeomToken(Vector2(movex, movey)));
			}
			AddFillToken(GeomToken(CLEAR_FILL));
		}
		hasChanged = false;
		if (!tokensHaveChanged && tokens[currentrenderindex].size()==tokens[1-currentrenderindex].size())
			return;
		owner->owner->legacy=false;
		owner->owner->hasChanged=true;
		owner->owner->geometryChanged();
		owner->owner->requestInvalidation(getSystemState());
	}
}

void Graphics::startDrawJob()
{
	drawMutex.lock();
}

void Graphics::endDrawJob()
{
	drawMutex.unlock();
}

bool Graphics::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	Locker l(drawMutex);
	if (this->tokens[this->currentrenderindex].empty())
		return false;
	xmin = float(this->tokens[this->currentrenderindex].boundsRect.Xmin)/TWIPS_FACTOR;
	xmax = float(this->tokens[this->currentrenderindex].boundsRect.Xmax)/TWIPS_FACTOR;
	ymin = float(this->tokens[this->currentrenderindex].boundsRect.Ymin)/TWIPS_FACTOR;
	ymax = float(this->tokens[this->currentrenderindex].boundsRect.Ymax)/TWIPS_FACTOR;
	return true;
}
bool Graphics::hitTest(const Vector2f& point)
{
	Locker l(drawMutex);
	return CairoTokenRenderer::hitTest(this->tokens[this->currentrenderindex], 1.0/TWIPS_FACTOR, point,true);
}

bool Graphics::destruct()
{
	Locker l(drawMutex);
	fillStyles[0].clear();
	lineStyles[0].clear();
	fillStyles[1].clear();
	lineStyles[1].clear();
	tokens[0].clear();
	tokens[1].clear();
	owner=nullptr;
	movex=0;
	movey=0;
	inFilling=false;
	hasChanged=false;
	needsRefresh = true;
	tokensHaveChanged=false;
	return ASObject::destruct();
}

// this has to be embedded inside calls to startDrawJob/endDrawJob
void Graphics::refreshTokens()
{
	if (needsRefresh)
	{
		owner->tokens.filltokens = tokens[currentrenderindex].filltokens;
		owner->tokens.stroketokens = tokens[currentrenderindex].stroketokens;
		owner->tokens.boundsRect = tokens[currentrenderindex].boundsRect;
		owner->owner->setNeedsTextureRecalculation(true);
		tokensHaveChanged=false;
		needsRefresh = false;
	}
}

bool Graphics::hasTokens() const
{
	return !tokens[currentrenderindex].empty();
}

void Graphics::AddFillToken(const GeomToken& token)
{
	if (!tokensHaveChanged && (
		tokens[1-currentrenderindex].filltokens.size()<=tokens[currentrenderindex].filltokens.size()
		|| tokens[1-currentrenderindex].filltokens[tokens[currentrenderindex].size()] != token.uval))
		tokensHaveChanged=true;
	tokens[currentrenderindex].filltokens.push_back(token.uval);
}
void Graphics::AddFillStyleToken(const GeomToken& token)
{
	if (!tokensHaveChanged && (
		tokens[1-currentrenderindex].filltokens.size()<=tokens[currentrenderindex].filltokens.size()
		|| GeomToken(tokens[1-currentrenderindex].stroketokens[tokens[currentrenderindex].size()-1]).type != GeomToken(SET_FILL).type
		|| !(*GeomToken(tokens[1-currentrenderindex].stroketokens[tokens[currentrenderindex].size()],false).fillStyle == *(token.fillStyle))))
		tokensHaveChanged=true;
	tokens[currentrenderindex].filltokens.push_back(token.uval);
}

void Graphics::AddStrokeToken(const GeomToken& token)
{
	if (!tokensHaveChanged && (
		tokens[1-currentrenderindex].stroketokens.size()<=tokens[currentrenderindex].stroketokens.size()
		|| tokens[1-currentrenderindex].stroketokens[tokens[currentrenderindex].size()] != token.uval))
		tokensHaveChanged=true;
	tokens[currentrenderindex].stroketokens.push_back(token.uval);
}
void Graphics::AddLineStyleToken(const GeomToken& token)
{
	if (!tokensHaveChanged && (
		tokens[1-currentrenderindex].stroketokens.size()<=tokens[currentrenderindex].stroketokens.size()
		|| GeomToken(tokens[1-currentrenderindex].stroketokens[tokens[currentrenderindex].size()-1],false).type != GeomToken(SET_STROKE).type
		|| !(*GeomToken(tokens[1-currentrenderindex].stroketokens[tokens[currentrenderindex].size()],false).lineStyle == *(token.lineStyle))))
		tokensHaveChanged=true;
	tokens[currentrenderindex].stroketokens.push_back(token.uval);
}


ASFUNCTIONBODY_ATOM(Graphics,drawTriangles)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	_NR<Vector> vertices;
	_NR<Vector> indices;
	_NR<Vector> uvtData;
	tiny_string culling;
	ARG_CHECK(ARG_UNPACK(vertices) (indices, NullRef) (uvtData, NullRef) (culling, "none"));

	drawTrianglesToTokens(vertices, indices, uvtData, culling, th->tokens[th->currentrenderindex]);
	th->hasChanged = true;
	th->tokensHaveChanged=true; // TODO check if tokens really have changed
	if (!th->inFilling)
		th->dorender(true);
}

void Graphics::drawTrianglesToTokens(_NR<Vector> vertices, _NR<Vector> indices, _NR<Vector> uvtData, tiny_string culling, tokensVector& tokens)
{
	if (culling != "none")
		LOG(LOG_NOT_IMPLEMENTED, "Graphics.drawTriangles doesn't support culling");

	// Validate the parameters
	if (vertices.isNull())
		return;

	if ((indices.isNull() && (vertices->size() % 6 != 0)) || 
	    (!indices.isNull() && (indices->size() % 3 != 0)))
	{
		createError<ArgumentError>(getWorker(),kInvalidParamError);
		return;
	}

	unsigned int numvertices=vertices->size()/2;
	unsigned int numtriangles;
	bool has_uvt=false;
	int uvtElemSize=2;
	int texturewidth=0;
	int textureheight=0;

	if (indices.isNull())
		numtriangles=numvertices/3;
	else
		numtriangles=indices->size()/3;

	if (!uvtData.isNull())
	{
		if (uvtData->size()==2*numvertices)
		{
			has_uvt=true;
			uvtElemSize=2; /* (u, v) */
		}
		else if (uvtData->size()==3*numvertices)
		{
			has_uvt=true;
			uvtElemSize=3; /* (u, v, t), t is ignored */
			LOG(LOG_NOT_IMPLEMENTED, "Graphics.drawTriangles doesn't support t in uvtData parameter");
		}
		else
		{
			createError<ArgumentError>(getWorker(),kInvalidParamError);
			return;
		}

		TokenContainer::getTextureSize(tokens.filltokens, &texturewidth, &textureheight);
	}

	// According to testing, drawTriangles first fills the current
	// path and creates a new path, but keeps the source.
	tokens.filltokens.emplace_back(GeomToken(FILL_KEEP_SOURCE).uval);

	if (has_uvt && (texturewidth==0 || textureheight==0))
		return;

	// Construct the triangles
	for (unsigned int i=0; i<numtriangles; i++)
	{
		double x[3], y[3], u[3]={0}, v[3]={0};
		for (unsigned int j=0; j<3; j++)
		{
			unsigned int vertex;
			if (indices.isNull())
				vertex=3*i+j;
			else
			{
				asAtom a =indices->at(3*i+j);
				vertex=asAtomHandler::toInt(a);
			}

			asAtom ax = vertices->at(2*vertex);
			x[j]=asAtomHandler::toNumber(ax)*TWIPS_FACTOR;
			asAtom ay = vertices->at(2*vertex+1);
			y[j]=asAtomHandler::toNumber(ay)*TWIPS_FACTOR;

			if (has_uvt)
			{
				asAtom au = uvtData->at(vertex*uvtElemSize);
				u[j]=asAtomHandler::toNumber(au)*texturewidth*TWIPS_FACTOR;
				asAtom av = uvtData->at(vertex*uvtElemSize+1);
				v[j]=asAtomHandler::toNumber(av)*textureheight*TWIPS_FACTOR;
			}
		}
		
		Vector2 a(x[0], y[0]);
		Vector2 b(x[1], y[1]);
		Vector2 c(x[2], y[2]);
		tokens.updateTokenBounds(x[0],y[0]);
		tokens.updateTokenBounds(x[1],y[1]);
		tokens.updateTokenBounds(x[2],y[2]);

		tokens.filltokens.emplace_back(GeomToken(MOVE).uval);
		tokens.filltokens.emplace_back(GeomToken(a).uval);
		tokens.filltokens.emplace_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens.emplace_back(GeomToken(b).uval);
		tokens.filltokens.emplace_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens.emplace_back(GeomToken(c).uval);
		tokens.filltokens.emplace_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens.emplace_back(GeomToken(a).uval);

		if (has_uvt)
		{
			double t[6];

			// Use the known (x, y) and (u, v)
			// correspondences to compute a transformation
			// t from (x, y) space into (u, v) space
			// (cairo needs the mapping in this
			// direction).
			//
			// u = t[0] + t[1]*x + t[2]*y
			// v = t[3] + t[4]*x + t[5]*y
			//
			// u and v parts can be solved separately.
			solveVertexMapping(x[0], y[0], x[1], y[1], x[2], y[2],
					   u[0], u[1], u[2], t);
			solveVertexMapping(x[0], y[0], x[1], y[1], x[2], y[2],
					   v[0], v[1], v[2], &t[3]);

			tokens.filltokens.emplace_back(GeomToken(FILL_TRANSFORM_TEXTURE).uval);
			tokens.filltokens.emplace_back(GeomToken(t[1]).uval);
			tokens.filltokens.emplace_back(GeomToken(t[5]).uval);
			tokens.filltokens.emplace_back(GeomToken(t[4]).uval);
			tokens.filltokens.emplace_back(GeomToken(t[2]).uval);
			tokens.filltokens.emplace_back(GeomToken(t[0]).uval);
			tokens.filltokens.emplace_back(GeomToken(t[3]).uval);
		}
	}
}

ASFUNCTIONBODY_ATOM(Graphics,drawGraphicsData)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	_NR<Vector> graphicsData;
	ARG_CHECK(ARG_UNPACK(graphicsData));

	for (unsigned int i=0; i<graphicsData->size(); i++)
	{
		IGraphicsData *graphElement = dynamic_cast<IGraphicsData *>(asAtomHandler::getObject(graphicsData->at(i)));
		if (!graphElement)
		{
			LOG(LOG_ERROR, "Invalid type in Graphics::drawGraphicsData()");
			continue;
		}

		graphElement->appendToTokens(th->tokens[th->currentrenderindex],th);
	}
	th->hasChanged = true;
	th->tokensHaveChanged=true; // TODO check if tokens really have changed
	if (!th->inFilling)
		th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,lineStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	if (argslen == 0)
	{
		th->AddStrokeToken(GeomToken(CLEAR_STROKE));
		return;
	}
	number_t thickness;
	uint32_t color;
	number_t alpha;
	bool pixelHinting;
	tiny_string scaleMode;
	tiny_string caps;
	tiny_string joints;
	number_t miterLimit;
	ARG_CHECK(ARG_UNPACK(thickness,Number::NaN)(color, 0)(alpha, 1.0)(pixelHinting,false)(scaleMode,"normal")(caps,"")(joints,"")(miterLimit,3));
	UI16_SWF _thickness = UI16_SWF(imax(thickness, 0));
	
	LINESTYLE2 style(0xff);
	style.Color = RGBA(color, ((int)(alpha*255.0))&0xff);
	style.Width = _thickness*TWIPS_FACTOR;
	style.PixelHintingFlag = pixelHinting? 1: 0;

	if (scaleMode == "normal")
	{
		style.NoHScaleFlag = false;
		style.NoVScaleFlag = false;
	}
	else if (scaleMode == "none")
	{
		style.NoHScaleFlag = true;
		style.NoVScaleFlag = true;
	}
	else if (scaleMode == "horizontal")
	{
		style.NoHScaleFlag = false;
		style.NoVScaleFlag = true;
	}
	else if (scaleMode == "vertical")
	{
		style.NoHScaleFlag = true;
		style.NoVScaleFlag = false;
	}
	
	if (caps == "none")
		style.StartCapStyle = 1;
	else if (caps == "round")
		style.StartCapStyle = 0;
	else if (caps == "square")
		style.StartCapStyle = 2;

	if (joints == "round")
		style.JointStyle = 0;
	else if (joints == "bevel")
		style.JointStyle = 1;
	else if (joints == "miter")
		style.JointStyle = 2;
	style.MiterLimitFactor = miterLimit;
	th->AddStrokeToken(GeomToken(SET_STROKE));
	th->AddLineStyleToken(GeomToken(th->addLineStyle(style)));
}

ASFUNCTIONBODY_ATOM(Graphics,lineBitmapStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_CHECK(ARG_UNPACK (bitmap) (matrix, NullRef) (repeat, true) (smooth, false));

	if (bitmap.isNull())
		return;

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createBitmapFill(bitmap, matrix, repeat, smooth);
	

	th->AddStrokeToken(GeomToken(SET_STROKE));
	th->AddLineStyleToken(GeomToken(th->addLineStyle(style)));
}

ASFUNCTIONBODY_ATOM(Graphics,lineGradientStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<Array> ratios;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	ARG_CHECK(ARG_UNPACK (type) (colors) (alphas) (ratios) (matrix, NullRef)
		(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0));

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createGradientFill(type, colors, alphas, ratios, matrix,
					    spreadMethod, interpolationMethod,
					    focalPointRatio);

	th->AddStrokeToken(GeomToken(SET_STROKE));
	th->AddLineStyleToken(GeomToken(th->addLineStyle(style)));
}

ASFUNCTIONBODY_ATOM(Graphics,beginGradientFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<ASObject> ratiosParam;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	if (wrk->getSystemState()->mainClip->usesActionScript3)
	{
		ARG_CHECK(ARG_UNPACK (type) (colors) (alphas) (ratiosParam) (matrix, NullRef)
				(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0));
	}
	else
	{
		_NR<ASObject> mat;
		ARG_CHECK(ARG_UNPACK (type) (colors) (alphas) (ratiosParam) (mat, NullRef)
				(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0));
		if (!mat.isNull())
		{
			if (mat->is<Matrix>())
			{
				mat->incRef();
				matrix = _MR(mat->as<Matrix>());
			}
			else 
			{
				multiname m(nullptr);
				m.name_type = multiname::NAME_STRING;
				m.name_s_id = wrk->getSystemState()->getUniqueStringId("matrixType");
				asAtom a=asAtomHandler::invalidAtom;
				mat->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::toString(a,wrk) != "box")
					LOG(LOG_NOT_IMPLEMENTED,"beginGradientFill with Object as Matrix and matrixType "<<asAtomHandler::toDebugString(a));
				else
				{
					matrix = _MR(Class<Matrix>::getInstanceSNoArgs(wrk));
					m.name_s_id = wrk->getSystemState()->getUniqueStringId("x");
					asAtom x=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(x,m,GET_VARIABLE_OPTION::NONE,wrk);
					m.name_s_id = wrk->getSystemState()->getUniqueStringId("y");
					asAtom y=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(y,m,GET_VARIABLE_OPTION::NONE,wrk);
					m.name_s_id = wrk->getSystemState()->getUniqueStringId("w");
					asAtom w=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(w,m,GET_VARIABLE_OPTION::NONE,wrk);
					m.name_s_id = wrk->getSystemState()->getUniqueStringId("h");
					asAtom h=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(h,m,GET_VARIABLE_OPTION::NONE,wrk);
					m.name_s_id = wrk->getSystemState()->getUniqueStringId("r");
					asAtom r=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(r,m,GET_VARIABLE_OPTION::NONE,wrk);
					matrix->_createBox(asAtomHandler::toNumber(w), asAtomHandler::toNumber(h), asAtomHandler::toNumber(r), asAtomHandler::toNumber(x), asAtomHandler::toNumber(y));
				}
			}
		}
	}

	//Work around for bug in YouTube player of July 13 2011
	if (!ratiosParam->is<Array>())
		return;
	if (ratiosParam.isNull())
		return;
	th->dorender(true);

	ratiosParam->incRef();
	_NR<Array> ratios = _MNR(ratiosParam->as<Array>());

	FILLSTYLE style=createGradientFill(type, colors, alphas, ratios, matrix,
					     spreadMethod, interpolationMethod,
					     focalPointRatio);
	th->inFilling=true;
	th->AddFillToken(GeomToken(SET_FILL));
	th->AddFillStyleToken(GeomToken(th->addFillStyle(style)));
}

FILLSTYLE Graphics::createGradientFill(const tiny_string& type,
				       _NR<Array> colors,
				       _NR<Array> alphas,
				       _NR<Array> ratios,
				       _NR<Matrix> matrix,
				       const tiny_string& spreadMethod,
				       const tiny_string& interpolationMethod,
				       number_t focalPointRatio)
{
	FILLSTYLE style(0xff);

	if (colors.isNull() || alphas.isNull() || ratios.isNull())
		return style;

	int NumGradient = colors->size();
	if (NumGradient != (int)alphas->size() || NumGradient != (int)ratios->size())
		return style;

	if ((NumGradient < 1) || (NumGradient > 15))
		return style;

	if(type == "linear")
		style.FillStyleType=LINEAR_GRADIENT;
	else if(type == "radial")
		style.FillStyleType=RADIAL_GRADIENT;
	else
		return style;

	// Don't support FOCALGRADIENT for now.
	GRADIENT grad(0xff);
	for(int i = 0; i < NumGradient; i ++)
	{
		GRADRECORD record(0xff);
		asAtom al = alphas->at(i);
		asAtom cl = colors->at(i);
		asAtom ra = ratios->at(i);
		record.Color = RGBA(asAtomHandler::toUInt(cl), (int)(asAtomHandler::toNumber(al)*255.0));
		record.Ratio = asAtomHandler::toUInt(ra);
		grad.GradientRecords.push_back(record);
	}

	if(matrix.isNull())
	{
		cairo_matrix_scale(&style.Matrix, 100.0/16384.0, 100.0/16384.0);
	}
	else
	{
		style.Matrix = matrix->getMATRIX();
	}

	if (spreadMethod == "pad")
		grad.SpreadMode = 0;
	else if (spreadMethod == "reflect")
		grad.SpreadMode = 1;
	else if (spreadMethod == "repeat")
		grad.SpreadMode = 2;
	else
		grad.SpreadMode = 0; // should not be reached

	if (interpolationMethod == "rgb")
		grad.InterpolationMode = 0;
	else if (interpolationMethod == "linearRGB")
		grad.InterpolationMode = 1;
	else
		grad.InterpolationMode = 0; // should not be reached

	style.Gradient = grad;
	return style;
}

FILLSTYLE Graphics::createBitmapFill(_R<BitmapData> bitmap, _NR<Matrix> matrix, bool repeat, bool smooth)
{
	FILLSTYLE style(0xff);
	if(repeat && smooth)
		style.FillStyleType = REPEATING_BITMAP;
	else if(repeat && !smooth)
		style.FillStyleType = NON_SMOOTHED_REPEATING_BITMAP;
	else if(!repeat && smooth)
		style.FillStyleType = CLIPPED_BITMAP;
	else
		style.FillStyleType = NON_SMOOTHED_CLIPPED_BITMAP;

	if(!matrix.isNull())
		style.Matrix = matrix->getMATRIX();
	MATRIX m;
	m.scale(TWIPS_FACTOR,TWIPS_FACTOR);
	style.Matrix=style.Matrix.multiplyMatrix(m);

	style.bitmap = bitmap->getBitmapContainer();

	return style;
}

FILLSTYLE Graphics::createSolidFill(uint32_t color, uint8_t alpha)
{
	FILLSTYLE style(0xff);
	style.FillStyleType = SOLID_FILL;
	style.Color = RGBA(color, alpha);
	return style;
}

ASFUNCTIONBODY_ATOM(Graphics,beginBitmapFill)
{
	Graphics* th = asAtomHandler::as<Graphics>(obj);
	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_CHECK(ARG_UNPACK (bitmap) (matrix, NullRef) (repeat, true) (smooth, false));

	if(bitmap.isNull())
		return;

	th->dorender(true);
	th->inFilling=true;
	FILLSTYLE style=createBitmapFill(bitmap, matrix, repeat, smooth);
	th->AddFillToken(GeomToken(SET_FILL));
	th->AddFillStyleToken(GeomToken(th->addFillStyle(style)));
}

ASFUNCTIONBODY_ATOM(Graphics,beginFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->dorender(true);
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=asAtomHandler::toUInt(args[0]);
	if(argslen>=2)
		alpha=(uint8_t(asAtomHandler::toNumber(args[1])*0xff));
	th->inFilling=true;
	FILLSTYLE style = Graphics::createSolidFill(color, alpha);
	th->AddFillToken(GeomToken(SET_FILL));
	th->AddFillStyleToken(GeomToken(th->addFillStyle(style)));
}

ASFUNCTIONBODY_ATOM(Graphics,endFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->dorender(true);

	th->inFilling=false;
	th->movex = 0;
	th->movey = 0;
}

ASFUNCTIONBODY_ATOM(Graphics,copyFrom)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	_NR<Graphics> source;
	ARG_CHECK(ARG_UNPACK(source));
	if (source.isNull())
		return;

	th->tokens[th->currentrenderindex].filltokens.assign(source->tokens[source->currentrenderindex].filltokens.begin(),
				 source->tokens[source->currentrenderindex].filltokens.end());
	th->tokens[th->currentrenderindex].stroketokens.assign(source->tokens[source->currentrenderindex].stroketokens.begin(),
				 source->tokens[source->currentrenderindex].stroketokens.end());
	th->tokensHaveChanged=true; // TODO check if tokens really have changed
	th->hasChanged = true;
}

ASFUNCTIONBODY_ATOM(Graphics,readGraphicsData)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);

	bool recurse;
	ARG_CHECK(ARG_UNPACK(recurse,true));

	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,ret,root,InterfaceClass<IGraphicsData>::getClass(wrk->getSystemState()),NullRef);
	Vector *graphicsData = asAtomHandler::as<Vector>(ret);
	th->owner->owner->fillGraphicsData(graphicsData,recurse);
}

