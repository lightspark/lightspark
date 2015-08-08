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
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRect","",Class<IFunction>::getFunction(drawRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRect","",Class<IFunction>::getFunction(drawRoundRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRectComplex","",Class<IFunction>::getFunction(drawRoundRectComplex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawCircle","",Class<IFunction>::getFunction(drawCircle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawEllipse","",Class<IFunction>::getFunction(drawEllipse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawPath","",Class<IFunction>::getFunction(drawPath),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",Class<IFunction>::getFunction(drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawGraphicsData","",Class<IFunction>::getFunction(drawGraphicsData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("curveTo","",Class<IFunction>::getFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("cubicCurveTo","",Class<IFunction>::getFunction(cubicCurveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineBitmapStyle","",Class<IFunction>::getFunction(lineBitmapStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineGradientStyle","",Class<IFunction>::getFunction(lineGradientStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(lineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(beginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(beginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginBitmapFill","",Class<IFunction>::getFunction(beginBitmapFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",Class<IFunction>::getFunction(endFill),NORMAL_METHOD,true);
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

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	th->owner->tokens.clear();
	th->owner->owner->requestInvalidation(getSys());
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	assert_and_throw(argslen==2);

	int32_t x=args[0]->toInt();
	int32_t y=args[1]->toInt();

	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(x, y)));
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y)));
	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

ASFUNCTIONBODY(Graphics,curveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int controlX=args[0]->toInt();
	int controlY=args[1]->toInt();

	int anchorX=args[2]->toInt();
	int anchorY=args[3]->toInt();

	th->owner->tokens.emplace_back(GeomToken(CURVE_QUADRATIC,
	                        Vector2(controlX, controlY),
	                        Vector2(anchorX, anchorY)));
	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

ASFUNCTIONBODY(Graphics,cubicCurveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==6);
	th->checkAndSetScaling();

	int control1X=args[0]->toInt();
	int control1Y=args[1]->toInt();

	int control2X=args[2]->toInt();
	int control2Y=args[3]->toInt();

	int anchorX=args[4]->toInt();
	int anchorY=args[5]->toInt();

	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(control1X, control1Y),
	                        Vector2(control2X, control2Y),
	                        Vector2(anchorX, anchorY)));
	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

/* KAPPA = 4 * (sqrt2 - 1) / 3
 * This value was found in a Python prompt:
 *
 * >>> 4.0 * (2**0.5 - 1) / 3.0
 *
 * Source: http://whizkidtech.redprince.net/bezier/circle/
 */
const double KAPPA = 0.55228474983079356;

ASFUNCTIONBODY(Graphics,drawRoundRect)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==5 || argslen==6);
	th->checkAndSetScaling();

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double width=args[2]->toNumber();
	double height=args[3]->toNumber();
	double ellipseWidth=args[4]->toNumber();
	double ellipseHeight;
	if (argslen == 6)
		ellipseHeight=args[5]->toNumber();

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

	// D
	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(x+width, y+height-ellipseHeight)));

	// D -> E
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+width, y+height-ellipseHeight+kappaH),
	                        Vector2(x+width-ellipseWidth+kappaW, y+height),
	                        Vector2(x+width-ellipseWidth, y+height)));

	// E -> F
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+ellipseWidth, y+height)));

	// F -> G
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+ellipseWidth-kappaW, y+height),
	                        Vector2(x, y+height-kappaH),
	                        Vector2(x, y+height-ellipseHeight)));

	// G -> H
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y+ellipseHeight)));

	// H -> A
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x, y+ellipseHeight-kappaH),
	                        Vector2(x+ellipseWidth-kappaW, y),
	                        Vector2(x+ellipseWidth, y)));

	// A -> B
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+width-ellipseWidth, y)));

	// B -> C
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+width-ellipseWidth+kappaW, y),
	                        Vector2(x+width, y+kappaH),
	                        Vector2(x+width, y+ellipseHeight)));

	// C -> D
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+width, y+height-ellipseHeight)));

	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawRoundRectComplex)
{
	LOG(LOG_NOT_IMPLEMENTED,"Graphics.drawRoundRectComplex currently draws a normal rect");
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen>=4);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	th->owner->tokens.emplace_back(GeomToken(MOVE, a));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, b));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, c));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, d));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, a));
	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawCircle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==3);
	th->checkAndSetScaling();

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double radius=args[2]->toNumber();

	double kappa = KAPPA*radius;

	// right
	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(x+radius, y)));

	// bottom
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+radius, y+kappa ),
	                        Vector2(x+kappa , y+radius),
	                        Vector2(x       , y+radius)));

	// left
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x-kappa , y+radius),
	                        Vector2(x-radius, y+kappa ),
	                        Vector2(x-radius, y       )));

	// top
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x-radius, y-kappa ),
	                        Vector2(x-kappa , y-radius),
	                        Vector2(x       , y-radius)));

	// back to right
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+kappa , y-radius),
	                        Vector2(x+radius, y-kappa ),
	                        Vector2(x+radius, y       )));

	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawEllipse)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	double left=args[0]->toNumber();
	double top=args[1]->toNumber();
	double width=args[2]->toNumber();
	double height=args[3]->toNumber();

	double xkappa = KAPPA*width/2;
	double ykappa = KAPPA*height/2;

	// right
	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(left+width, top+height/2)));
	
	// bottom
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(left+width , top+height/2+ykappa),
	                        Vector2(left+width/2+xkappa, top+height),
	                        Vector2(left+width/2, top+height)));

	// left
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(left+width/2-xkappa, top+height),
	                        Vector2(left, top+height/2+ykappa),
	                        Vector2(left, top+height/2)));

	// top
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(left, top+height/2-ykappa),
	                        Vector2(left+width/2-xkappa, top),
	                        Vector2(left+width/2, top)));

	// back to right
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(left+width/2+xkappa, top),
	                        Vector2(left+width, top+height/2-ykappa),
	                        Vector2(left+width, top+height/2)));

	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

ASFUNCTIONBODY(Graphics,drawRect)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	th->owner->tokens.emplace_back(GeomToken(MOVE, a));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, b));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, c));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, d));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, a));
	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawPath)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	_NR<Vector> commands;
	_NR<Vector> data;
	tiny_string winding;
	ARG_UNPACK (commands) (data) (winding, "evenOdd");

	if (commands.isNull() || data.isNull())
		throwError<ArgumentError>(kInvalidParamError);

	pathToTokens(commands, data, winding, th->owner->tokens);

	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

void Graphics::pathToTokens(_NR<Vector> commands, _NR<Vector> data,
			    tiny_string winding, std::vector<GeomToken>& tokens)
{
	if (commands.isNull() || data.isNull())
		return;

	if (winding != "evenOdd")
		LOG(LOG_NOT_IMPLEMENTED, "Only event-odd winding implemented in Graphics.drawPath");

	_R<Number> zeroRef = _MR(Class<Number>::getInstanceS(0));
	Number *zero = zeroRef.getPtr();

	int k = 0;
	for (unsigned int i=0; i<commands->size(); i++)
	{
		switch (commands->at(i)->toInt())
		{
			case GraphicsPathCommand::MOVE_TO:
			{
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(MOVE, Vector2(x, y)));
				break;
			}

			case GraphicsPathCommand::LINE_TO:
			{
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y)));
				break;
			}

			case GraphicsPathCommand::CURVE_TO:
			{
				number_t cx = data->at(k++, zero)->toNumber();
				number_t cy = data->at(k++, zero)->toNumber();
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(CURVE_QUADRATIC,
							      Vector2(cx, cy),
							      Vector2(x, y)));
				break;
			}

			case GraphicsPathCommand::WIDE_MOVE_TO:
			{
				k+=2;
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(MOVE, Vector2(x, y)));
				break;
			}

			case GraphicsPathCommand::WIDE_LINE_TO:
			{
				k+=2;
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y)));
				break;
			}

			case GraphicsPathCommand::CUBIC_CURVE_TO:
			{
				number_t c1x = data->at(k++, zero)->toNumber();
				number_t c1y = data->at(k++, zero)->toNumber();
				number_t c2x = data->at(k++, zero)->toNumber();
				number_t c2y = data->at(k++, zero)->toNumber();
				number_t x = data->at(k++, zero)->toNumber();
				number_t y = data->at(k++, zero)->toNumber();
				tokens.emplace_back(GeomToken(CURVE_CUBIC,
							      Vector2(c1x, c1y),
							      Vector2(c2x, c2y),
							      Vector2(x, y)));
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

ASFUNCTIONBODY(Graphics,drawTriangles)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	_NR<Vector> vertices;
	_NR<Vector> indices;
	_NR<Vector> uvtData;
	tiny_string culling;
	ARG_UNPACK (vertices) (indices, NullRef) (uvtData, NullRef) (culling, "none");

	drawTrianglesToTokens(vertices, indices, uvtData, culling, th->owner->tokens);
	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

void Graphics::drawTrianglesToTokens(_NR<Vector> vertices, _NR<Vector> indices, _NR<Vector> uvtData, tiny_string culling, std::vector<GeomToken>& tokens)
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
	tokens.emplace_back(FILL_KEEP_SOURCE);

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
				vertex=indices->at(3*i+j)->toInt();

			x[j]=vertices->at(2*vertex)->toNumber();
			y[j]=vertices->at(2*vertex+1)->toNumber();

			if (has_uvt)
			{
				u[j]=uvtData->at(vertex*uvtElemSize)->toNumber()*texturewidth;
				v[j]=uvtData->at(vertex*uvtElemSize+1)->toNumber()*textureheight;
			}
		}
		
		Vector2 a(x[0], y[0]);
		Vector2 b(x[1], y[1]);
		Vector2 c(x[2], y[2]);

		tokens.emplace_back(GeomToken(MOVE, a));
		tokens.emplace_back(GeomToken(STRAIGHT, b));
		tokens.emplace_back(GeomToken(STRAIGHT, c));
		tokens.emplace_back(GeomToken(STRAIGHT, a));

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
			tokens.emplace_back(GeomToken(FILL_TRANSFORM_TEXTURE, m));
		}
	}
}

ASFUNCTIONBODY(Graphics,drawGraphicsData)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	_NR<Vector> graphicsData;
	ARG_UNPACK(graphicsData);

	for (unsigned int i=0; i<graphicsData->size(); i++)
	{
		IGraphicsData *graphElement = dynamic_cast<IGraphicsData *>(graphicsData->at(i));
		if (!graphElement)
		{
			LOG(LOG_ERROR, "Invalid type in Graphics::drawGraphicsData()");
			continue;
		}

		graphElement->appendToTokens(th->owner->tokens);
	}

	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

ASFUNCTIONBODY(Graphics,lineStyle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	if (argslen == 0)
	{
		th->owner->tokens.emplace_back(CLEAR_STROKE);
		return NULL;
	}
	uint32_t color = 0;
	uint8_t alpha = 255;
	UI16_SWF thickness = UI16_SWF(imax(args[0]->toNumber() * 20, 0));
	if (argslen >= 2)
		color = args[1]->toUInt();
	if (argslen >= 3)
		alpha = uint8_t(args[1]->toNumber() * 255);

	// TODO: pixel hinting, scaling, caps, miter, joints
	
	LINESTYLE2 style(0xff);
	style.Color = RGBA(color, alpha);
	style.Width = thickness;
	th->owner->tokens.emplace_back(GeomToken(SET_STROKE, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineBitmapStyle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_UNPACK (bitmap) (matrix, NullRef) (repeat, true) (smooth, false);

	if (bitmap.isNull())
		return NULL;

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createBitmapFill(bitmap, matrix, repeat, smooth);
	
	th->owner->tokens.emplace_back(GeomToken(SET_STROKE, style));

	return NULL;
}

ASFUNCTIONBODY(Graphics,lineGradientStyle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<Array> ratios;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	ARG_UNPACK (type) (colors) (alphas) (ratios) (matrix, NullRef)
		(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0);

	LINESTYLE2 style(0xff);
	style.Width = th->owner->getCurrentLineWidth();
	style.HasFillFlag = true;
	style.FillType = createGradientFill(type, colors, alphas, ratios, matrix,
					    spreadMethod, interpolationMethod,
					    focalPointRatio);

	th->owner->tokens.emplace_back(GeomToken(SET_STROKE, style));

	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	tiny_string type;
	_NR<Array> colors;
	_NR<Array> alphas;
	_NR<ASObject> ratiosParam;
	_NR<Matrix> matrix;
	tiny_string spreadMethod;
	tiny_string interpolationMethod;
	number_t focalPointRatio;
	ARG_UNPACK (type) (colors) (alphas) (ratiosParam) (matrix, NullRef)
		(spreadMethod, "pad") (interpolationMethod, "rgb") (focalPointRatio, 0);

	//Work around for bug in YouTube player of July 13 2011
	if (!ratiosParam->is<Array>())
		return NULL;
	if (ratiosParam.isNull())
		return NULL;

	ratiosParam->incRef();
	_NR<Array> ratios = _MNR(ratiosParam->as<Array>());

	FILLSTYLE style = createGradientFill(type, colors, alphas, ratios, matrix,
					     spreadMethod, interpolationMethod,
					     focalPointRatio);
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));

	return NULL;
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
		record.Color = RGBA(colors->at(i)->toUInt(), (int)alphas->at(i)->toNumber()*255);
		record.Ratio = UI8(ratios->at(i)->toUInt());
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

ASFUNCTIONBODY(Graphics,beginBitmapFill)
{
	Graphics* th = obj->as<Graphics>();
	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_UNPACK (bitmap) (matrix, NullRef) (repeat, true) (smooth, false);

	if(bitmap.isNull())
		return NULL;

	th->checkAndSetScaling();

	FILLSTYLE style = createBitmapFill(bitmap, matrix, repeat, smooth);
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=args[0]->toUInt();
	if(argslen>=2)
		alpha=(uint8_t(args[1]->toNumber()*0xff));
	FILLSTYLE style = Graphics::createSolidFill(color, alpha);
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,endFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	th->owner->tokens.emplace_back(CLEAR_FILL);
	return NULL;
}

ASFUNCTIONBODY(Graphics,copyFrom)
{
	Graphics* th=static_cast<Graphics*>(obj);
	_NR<Graphics> source;
	ARG_UNPACK(source);
	if (source.isNull())
		return NULL;

	th->owner->tokens.assign(source->owner->tokens.begin(),
				 source->owner->tokens.end());
	return NULL;
}
