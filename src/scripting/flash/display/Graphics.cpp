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

using namespace std;
using namespace lightspark;

void Graphics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRect","",Class<IFunction>::getFunction(c->getSystemState(),drawRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRect","",Class<IFunction>::getFunction(c->getSystemState(),drawRoundRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRectComplex","",Class<IFunction>::getFunction(c->getSystemState(),drawRoundRectComplex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawCircle","",Class<IFunction>::getFunction(c->getSystemState(),drawCircle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawEllipse","",Class<IFunction>::getFunction(c->getSystemState(),drawEllipse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawPath","",Class<IFunction>::getFunction(c->getSystemState(),drawPath),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",Class<IFunction>::getFunction(c->getSystemState(),drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawGraphicsData","",Class<IFunction>::getFunction(c->getSystemState(),drawGraphicsData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(c->getSystemState(),moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("curveTo","",Class<IFunction>::getFunction(c->getSystemState(),curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("cubicCurveTo","",Class<IFunction>::getFunction(c->getSystemState(),cubicCurveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(c->getSystemState(),lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineBitmapStyle","",Class<IFunction>::getFunction(c->getSystemState(),lineBitmapStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineGradientStyle","",Class<IFunction>::getFunction(c->getSystemState(),lineGradientStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(c->getSystemState(),lineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(c->getSystemState(),beginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(c->getSystemState(),beginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginBitmapFill","",Class<IFunction>::getFunction(c->getSystemState(),beginBitmapFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",Class<IFunction>::getFunction(c->getSystemState(),endFill),NORMAL_METHOD,true);
}

void Graphics::buildTraits(ASObject* o)
{
}

//TODO: Add spinlock
void Graphics::checkAndSetScaling()
{
	if(owner->scaling != 1.0f)
	{
		owner->scaling = 1.0f;
		owner->tokens.clear();
	}
}

ASFUNCTIONBODY_ATOM(Graphics,_constructor)
{
}

ASFUNCTIONBODY_ATOM(Graphics,clear)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();
	th->inFilling = false;
	th->hasChanged = false;
	th->owner->tokens.clear();
	th->owner->owner->hasChanged=true;
	th->owner->owner->setNeedsTextureRecalculation();
	th->owner->owner->requestInvalidation(sys);
}

ASFUNCTIONBODY_ATOM(Graphics,moveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();
	assert_and_throw(argslen==2);
	if (th->inFilling)
		th->dorender(true);

	int32_t x=asAtomHandler::toInt(args[0]);
	int32_t y=asAtomHandler::toInt(args[1]);
	th->movex = x;
	th->movey = y;
	if (th->inFilling)
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x, y))));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x, y))));
}

ASFUNCTIONBODY_ATOM(Graphics,lineTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==2);
	th->checkAndSetScaling();

	int x=asAtomHandler::toInt(args[0]);
	int y=asAtomHandler::toInt(args[1]);

	if (th->inFilling)
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y))));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y))));
	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,curveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int controlX=asAtomHandler::toInt(args[0]);
	int controlY=asAtomHandler::toInt(args[1]);

	int anchorX=asAtomHandler::toInt(args[2]);
	int anchorY=asAtomHandler::toInt(args[3]);

	if (th->inFilling)
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_QUADRATIC,
	                        Vector2(controlX, controlY),
	                        Vector2(anchorX, anchorY))));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_QUADRATIC,
								Vector2(controlX, controlY),
								Vector2(anchorX, anchorY))));
	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(false);
}

ASFUNCTIONBODY_ATOM(Graphics,cubicCurveTo)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==6);
	th->checkAndSetScaling();

	int control1X=asAtomHandler::toInt(args[0]);
	int control1Y=asAtomHandler::toInt(args[1]);

	int control2X=asAtomHandler::toInt(args[2]);
	int control2Y=asAtomHandler::toInt(args[3]);

	int anchorX=asAtomHandler::toInt(args[4]);
	int anchorY=asAtomHandler::toInt(args[5]);

	if (th->inFilling)
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
	                        Vector2(control1X, control1Y),
	                        Vector2(control2X, control2Y),
	                        Vector2(anchorX, anchorY))));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(control1X, control1Y),
								Vector2(control2X, control2Y),
								Vector2(anchorX, anchorY))));
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
	th->checkAndSetScaling();

	double x=asAtomHandler::toNumber(args[0]);
	double y=asAtomHandler::toNumber(args[1]);
	double width=asAtomHandler::toNumber(args[2]);
	double height=asAtomHandler::toNumber(args[3]);
	double ellipseWidth=asAtomHandler::toNumber(args[4]);
	double ellipseHeight;
	if (argslen == 6)
		ellipseHeight=asAtomHandler::toNumber(args[5]);

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
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x+width, y+height-ellipseHeight))));
	
		// D -> E
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x+width, y+height-ellipseHeight+kappaH),
								Vector2(x+width-ellipseWidth+kappaW, y+height),
								Vector2(x+width-ellipseWidth, y+height))));
	
		// E -> F
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+ellipseWidth, y+height))));
	
		// F -> G
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x+ellipseWidth-kappaW, y+height),
								Vector2(x, y+height-kappaH),
								Vector2(x, y+height-ellipseHeight))));
	
		// G -> H
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y+ellipseHeight))));
	
		// H -> A
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x, y+ellipseHeight-kappaH),
								Vector2(x+ellipseWidth-kappaW, y),
								Vector2(x+ellipseWidth, y))));
	
		// A -> B
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+width-ellipseWidth, y))));
	
		// B -> C
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x+width-ellipseWidth+kappaW, y),
								Vector2(x+width, y+kappaH),
								Vector2(x+width, y+ellipseHeight))));
	
		// C -> D
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+width, y+height-ellipseHeight))));
	}
	// D
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x+width, y+height-ellipseHeight))));

	// D -> E
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x+width, y+height-ellipseHeight+kappaH),
							Vector2(x+width-ellipseWidth+kappaW, y+height),
							Vector2(x+width-ellipseWidth, y+height))));

	// E -> F
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+ellipseWidth, y+height))));

	// F -> G
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x+ellipseWidth-kappaW, y+height),
							Vector2(x, y+height-kappaH),
							Vector2(x, y+height-ellipseHeight))));

	// G -> H
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y+ellipseHeight))));

	// H -> A
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x, y+ellipseHeight-kappaH),
							Vector2(x+ellipseWidth-kappaW, y),
							Vector2(x+ellipseWidth, y))));

	// A -> B
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+width-ellipseWidth, y))));

	// B -> C
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x+width-ellipseWidth+kappaW, y),
							Vector2(x+width, y+kappaH),
							Vector2(x+width, y+ellipseHeight))));

	// C -> D
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x+width, y+height-ellipseHeight))));
	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawRoundRectComplex)
{
	LOG(LOG_NOT_IMPLEMENTED,"Graphics.drawRoundRectComplex currently draws a normal rect");
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen>=4);
	th->checkAndSetScaling();

	int x=asAtomHandler::toInt(args[0]);
	int y=asAtomHandler::toInt(args[1]);
	int width=asAtomHandler::toInt(args[2]);
	int height=asAtomHandler::toInt(args[3]);

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	if (th->inFilling)
	{
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, a)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, b)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, c)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, d)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, a)));
	}	
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, a)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, b)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, c)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, d)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, a)));
	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawCircle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==3);
	th->checkAndSetScaling();

	double x=asAtomHandler::toNumber(args[0]);
	double y=asAtomHandler::toNumber(args[1]);
	double radius=asAtomHandler::toNumber(args[2]);

	double kappa = KAPPA*radius;

	if (th->inFilling)
	{
		// right
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x+radius, y))));
	
		// bottom
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x+radius, y+kappa ),
								Vector2(x+kappa , y+radius),
								Vector2(x       , y+radius))));
	
		// left
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x-kappa , y+radius),
								Vector2(x-radius, y+kappa ),
								Vector2(x-radius, y       ))));
	
		// top
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x-radius, y-kappa ),
								Vector2(x-kappa , y-radius),
								Vector2(x       , y-radius))));
	
		// back to right
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(x+kappa , y-radius),
								Vector2(x+radius, y-kappa ),
								Vector2(x+radius, y       ))));
	}
	// right
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x+radius, y))));

	// bottom
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x+radius, y+kappa ),
							Vector2(x+kappa , y+radius),
							Vector2(x       , y+radius))));

	// left
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x-kappa , y+radius),
							Vector2(x-radius, y+kappa ),
							Vector2(x-radius, y       ))));

	// top
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x-radius, y-kappa ),
							Vector2(x-kappa , y-radius),
							Vector2(x       , y-radius))));

	// back to right
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(x+kappa , y-radius),
							Vector2(x+radius, y-kappa ),
							Vector2(x+radius, y       ))));
	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawEllipse)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	double left=asAtomHandler::toNumber(args[0]);
	double top=asAtomHandler::toNumber(args[1]);
	double width=asAtomHandler::toNumber(args[2]);
	double height=asAtomHandler::toNumber(args[3]);

	double xkappa = KAPPA*width/2.0;
	double ykappa = KAPPA*height/2.0;

	if (th->inFilling)
	{
		// right
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(left+width, top+height/2.0))));
		
		// bottom
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(left+width , top+height/2.0+ykappa),
								Vector2(left+width/2.0+xkappa, top+height),
								Vector2(left+width/2.0, top+height))));
	
		// left
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(left+width/2.0-xkappa, top+height),
								Vector2(left, top+height/2.0+ykappa),
								Vector2(left, top+height/2.0))));
	
		// top
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(left, top+height/2.0-ykappa),
								Vector2(left+width/2.0-xkappa, top),
								Vector2(left+width/2.0, top))));
	
		// back to right
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
								Vector2(left+width/2.0+xkappa, top),
								Vector2(left+width, top+height/2.0-ykappa),
								Vector2(left+width, top+height/2.0))));
	}
	// right
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(left+width, top+height/2.0))));
	
	// bottom
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(left+width , top+height/2.0+ykappa),
							Vector2(left+width/2.0+xkappa, top+height),
							Vector2(left+width/2.0, top+height))));

	// left
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(left+width/2.0-xkappa, top+height),
							Vector2(left, top+height/2.0+ykappa),
							Vector2(left, top+height/2.0))));

	// top
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(left, top+height/2.0-ykappa),
							Vector2(left+width/2.0-xkappa, top),
							Vector2(left+width/2.0, top))));

	// back to right
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							Vector2(left+width/2.0+xkappa, top),
							Vector2(left+width, top+height/2.0-ykappa),
							Vector2(left+width, top+height/2.0))));
	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawRect)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int x=asAtomHandler::toInt(args[0]);
	int y=asAtomHandler::toInt(args[1]);
	int width=asAtomHandler::toInt(args[2]);
	int height=asAtomHandler::toInt(args[3]);

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	if (th->inFilling)
	{
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, a)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, b)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, c)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, d)));
		th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, a)));
	}
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(MOVE, a)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, b)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, c)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, d)));
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(STRAIGHT, a)));
	th->hasChanged = true;
	th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,drawPath)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	_NR<Vector> commands;
	_NR<Vector> data;
	tiny_string winding;
	ARG_UNPACK_ATOM (commands) (data) (winding, "evenOdd");

	if (commands.isNull() || data.isNull())
		throwError<ArgumentError>(kInvalidParamError);

	if (th->inFilling)
		pathToTokens(commands, data, winding, th->owner->tokens.filltokens);
	pathToTokens(commands, data, winding, th->owner->tokens.stroketokens);
	th->hasChanged = true;
	th->dorender(true);
}

void Graphics::pathToTokens(_NR<Vector> commands, _NR<Vector> data,
			    tiny_string winding, std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>>>& tokens)
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
			case GraphicsPathCommand::MOVE_TO:
			{
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::LINE_TO:
			{
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::CURVE_TO:
			{
				asAtom acx = data->at(k++, zero);
				asAtom acy = data->at(k++, zero);
				number_t cx = asAtomHandler::toNumber(acx);
				number_t cy = asAtomHandler::toNumber(acy);
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(CURVE_QUADRATIC,
							      Vector2(cx, cy),
							      Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::WIDE_MOVE_TO:
			{
				k+=2;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::WIDE_LINE_TO:
			{
				k+=2;
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::CUBIC_CURVE_TO:
			{
				asAtom ac1x = data->at(k++, zero);
				asAtom ac1y = data->at(k++, zero);
				asAtom ac2x = data->at(k++, zero);
				asAtom ac2y = data->at(k++, zero);
				number_t c1x = asAtomHandler::toNumber(ac1x);
				number_t c1y = asAtomHandler::toNumber(ac1y);
				number_t c2x = asAtomHandler::toNumber(ac2x);
				number_t c2y = asAtomHandler::toNumber(ac2y);
				asAtom ax = data->at(k++, zero);
				asAtom ay = data->at(k++, zero);
				number_t x = asAtomHandler::toNumber(ax);
				number_t y = asAtomHandler::toNumber(ay);
				tokens.emplace_back(_MR(new GeomToken(CURVE_CUBIC,
							      Vector2(c1x, c1y),
							      Vector2(c2x, c2y),
							      Vector2(x, y))));
				break;
			}

			case GraphicsPathCommand::NO_OP:
			default:
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
	if (hasChanged)
	{
		if (inFilling && closepath)
		{
			owner->tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(movex, movey))));
			owner->tokens.filltokens.emplace_back(_MR(new GeomToken(CLEAR_FILL)));
		}
		owner->owner->legacy=false;
		owner->owner->hasChanged=true;
		owner->owner->setNeedsTextureRecalculation(true);
		owner->owner->requestInvalidation(getSystemState());
		hasChanged = false;
	}
}

ASFUNCTIONBODY_ATOM(Graphics,drawTriangles)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	_NR<Vector> vertices;
	_NR<Vector> indices;
	_NR<Vector> uvtData;
	tiny_string culling;
	ARG_UNPACK_ATOM (vertices) (indices, NullRef) (uvtData, NullRef) (culling, "none");

	if (th->inFilling)
		drawTrianglesToTokens(vertices, indices, uvtData, culling, th->owner->tokens.filltokens);
	drawTrianglesToTokens(vertices, indices, uvtData, culling, th->owner->tokens.stroketokens);
	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(true);
}

void Graphics::drawTrianglesToTokens(_NR<Vector> vertices, _NR<Vector> indices, _NR<Vector> uvtData, tiny_string culling, std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>> > &tokens)
{
	if (culling != "none")
		LOG(LOG_NOT_IMPLEMENTED, "Graphics.drawTriangles doesn't support culling");

	// Validate the parameters
	if (vertices.isNull())
		return;

	if ((indices.isNull() && (vertices->size() % 6 != 0)) || 
	    (!indices.isNull() && (indices->size() % 3 != 0)))
	{
		throwError<ArgumentError>(kInvalidParamError);
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
			throwError<ArgumentError>(kInvalidParamError);
		}

		TokenContainer::getTextureSize(tokens, &texturewidth, &textureheight);
	}

	// According to testing, drawTriangles first fills the current
	// path and creates a new path, but keeps the source.
	tokens.emplace_back(_MR(new GeomToken(FILL_KEEP_SOURCE)));

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
			x[j]=asAtomHandler::toNumber(ax);
			asAtom ay = vertices->at(2*vertex+1);
			y[j]=asAtomHandler::toNumber(ay);;

			if (has_uvt)
			{
				asAtom au = uvtData->at(vertex*uvtElemSize);
				u[j]=asAtomHandler::toNumber(au)*texturewidth;
				asAtom av = uvtData->at(vertex*uvtElemSize+1);
				v[j]=asAtomHandler::toNumber(av)*textureheight;
			}
		}
		
		Vector2 a(x[0], y[0]);
		Vector2 b(x[1], y[1]);
		Vector2 c(x[2], y[2]);

		tokens.emplace_back(_MR(new GeomToken(MOVE, a)));
		tokens.emplace_back(_MR(new GeomToken(STRAIGHT, b)));
		tokens.emplace_back(_MR(new GeomToken(STRAIGHT, c)));
		tokens.emplace_back(_MR(new GeomToken(STRAIGHT, a)));

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

			MATRIX m(t[1], t[5], t[4], t[2], t[0], t[3]);
			tokens.emplace_back(_MR(new GeomToken(FILL_TRANSFORM_TEXTURE, m)));
		}
	}
}

ASFUNCTIONBODY_ATOM(Graphics,drawGraphicsData)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	_NR<Vector> graphicsData;
	ARG_UNPACK_ATOM(graphicsData);

	for (unsigned int i=0; i<graphicsData->size(); i++)
	{
		IGraphicsData *graphElement = dynamic_cast<IGraphicsData *>(asAtomHandler::getObject(graphicsData->at(i)));
		if (!graphElement)
		{
			LOG(LOG_ERROR, "Invalid type in Graphics::drawGraphicsData()");
			continue;
		}

		graphElement->appendToTokens(th->owner->tokens.filltokens);
	}
	th->hasChanged = true;
	if (!th->inFilling)
		th->dorender(true);
}

ASFUNCTIONBODY_ATOM(Graphics,lineStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	if (argslen == 0)
	{
		th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(CLEAR_STROKE)));
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
	ARG_UNPACK_ATOM(thickness,Number::NaN)(color, 0)(alpha, 1.0)(pixelHinting,false)(scaleMode,"normal")(caps,"")(joints,"")(miterLimit,3);
	UI16_SWF _thickness = UI16_SWF(imax(thickness * 20, 0));
	
	LINESTYLE2 style(0xff);
	style.Color = RGBA(color, ((int)(alpha*255.0))&0xff);
	style.Width = _thickness;
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
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(SET_STROKE, style)));
}

ASFUNCTIONBODY_ATOM(Graphics,lineBitmapStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_UNPACK_ATOM (bitmap) (matrix, NullRef) (repeat, true) (smooth, false);

	if (bitmap.isNull())
		return;

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createBitmapFill(bitmap, matrix, repeat, smooth);
	
	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(SET_STROKE, style)));
}

ASFUNCTIONBODY_ATOM(Graphics,lineGradientStyle)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<Array> ratios;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	ARG_UNPACK_ATOM (type) (colors) (alphas) (ratios) (matrix, NullRef)
		(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0);

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createGradientFill(type, colors, alphas, ratios, matrix,
					    spreadMethod, interpolationMethod,
					    focalPointRatio);

	th->owner->tokens.stroketokens.emplace_back(_MR(new GeomToken(SET_STROKE, style)));
}

ASFUNCTIONBODY_ATOM(Graphics,beginGradientFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<ASObject> ratiosParam;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	if (sys->mainClip->usesActionScript3)
	{
		ARG_UNPACK_ATOM (type) (colors) (alphas) (ratiosParam) (matrix, NullRef)
				(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0);
	}
	else
	{
		_NR<ASObject> mat;
		ARG_UNPACK_ATOM (type) (colors) (alphas) (ratiosParam) (mat, NullRef)
				(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0);
		if (!mat.isNull())
		{
			if (mat->is<Matrix>())
				matrix = _MR(mat->as<Matrix>());
			else 
			{
				multiname m(nullptr);
				m.name_type = multiname::NAME_STRING;
				m.name_s_id = sys->getUniqueStringId("matrixType");
				asAtom a=asAtomHandler::invalidAtom;
				mat->getVariableByMultiname(a,m);
				if (asAtomHandler::toString(a,sys) != "box")
					LOG(LOG_NOT_IMPLEMENTED,"beginGradientFill with Object as Matrix and matrixType "<<asAtomHandler::toDebugString(a));
				else
				{
					matrix = _MR(Class<Matrix>::getInstanceSNoArgs(sys));
					m.name_s_id = sys->getUniqueStringId("x");
					asAtom x=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(x,m);
					m.name_s_id = sys->getUniqueStringId("y");
					asAtom y=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(y,m);
					m.name_s_id = sys->getUniqueStringId("w");
					asAtom w=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(w,m);
					m.name_s_id = sys->getUniqueStringId("h");
					asAtom h=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(h,m);
					m.name_s_id = sys->getUniqueStringId("r");
					asAtom r=asAtomHandler::invalidAtom;
					mat->getVariableByMultiname(r,m);
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
	th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL, style)));
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

	if (NumGradient < 1 || NumGradient > 15)
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
		record.Color = RGBA(asAtomHandler::toUInt(cl), (int)asAtomHandler::toNumber(al)*255);
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
		//Conversion from twips to pixels
		cairo_matrix_scale(&style.Matrix, 1.0f/20.0f, 1.0f/20.0f);
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
	ARG_UNPACK_ATOM (bitmap) (matrix, NullRef) (repeat, true) (smooth, false);

	if(bitmap.isNull())
		return;

	th->checkAndSetScaling();
	th->dorender(true);
	th->inFilling=true;

	FILLSTYLE style=createBitmapFill(bitmap, matrix, repeat, smooth);
	th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL, style)));
}

ASFUNCTIONBODY_ATOM(Graphics,beginFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();
	th->dorender(true);
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=asAtomHandler::toUInt(args[0]);
	if(argslen>=2)
		alpha=(uint8_t(asAtomHandler::toNumber(args[1])*0xff));
	th->inFilling=true;
	FILLSTYLE style = Graphics::createSolidFill(color, alpha);
	th->owner->tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL, style)));
}

ASFUNCTIONBODY_ATOM(Graphics,endFill)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	th->checkAndSetScaling();
	th->dorender(true);

	th->inFilling=false;
	th->movex = 0;
	th->movey = 0;
}

ASFUNCTIONBODY_ATOM(Graphics,copyFrom)
{
	Graphics* th=asAtomHandler::as<Graphics>(obj);
	_NR<Graphics> source;
	ARG_UNPACK_ATOM(source);
	if (source.isNull())
		return;

	th->owner->tokens.filltokens.assign(source->owner->tokens.filltokens.begin(),
				 source->owner->tokens.filltokens.end());
	th->owner->tokens.stroketokens.assign(source->owner->tokens.stroketokens.begin(),
				 source->owner->tokens.stroketokens.end());
	th->hasChanged = true;
}
