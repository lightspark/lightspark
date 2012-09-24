/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/text/flashtextengine.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swf.h"

#define MAX_LINE_WIDTH 1000000

using namespace std;
using namespace lightspark;

void ContentElement::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

ASFUNCTIONBODY(ContentElement, _constructor)
{
	throw Class<ArgumentError>::getInstanceS("Error #2012: ContentElement class cannot be instantiated.");
}

void ElementFormat::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("GRAPHIC_ELEMENT","",abstract_ui(0xFDEF),CONSTANT_TRAIT);
}

ASFUNCTIONBODY(ElementFormat, _constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "ElementFormat class not implemented");
	return NULL;
}

void FontDescription::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

ASFUNCTIONBODY(FontDescription, _constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "FontDescription class not implemented");
	return NULL;
}

void FontWeight::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("BOLD","",Class<ASString>::getInstanceS("bold"),CONSTANT_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),CONSTANT_TRAIT);
}

void TextBlock::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("createTextLine","",Class<IFunction>::getFunction(createTextLine),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, content);
}

ASFUNCTIONBODY_GETTER_SETTER(TextBlock, content);

ASFUNCTIONBODY(TextBlock, _constructor)
{
	TextBlock* th=static_cast<TextBlock*>(obj);
	ARG_UNPACK (th->content, NullRef);
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock constructor ignores some parameters");

	return NULL;
}

ASFUNCTIONBODY(TextBlock, createTextLine)
{
	TextBlock* th=static_cast<TextBlock*>(obj);
	_NR<TextLine> previousLine;
	int32_t width;
	ARG_UNPACK (previousLine, NullRef) (width, MAX_LINE_WIDTH);

	if (argslen > 2)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::createTextLine ignored some parameters");

	if (width <= 0 || width > MAX_LINE_WIDTH)
		throw Class<ArgumentError>::getInstanceS("Invalid width");

	if (!previousLine.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::createTextLine supports a single line only");
		return getSys()->getNullRef();
	}

	th->incRef();
	TextLine *textLine = Class<TextLine>::getInstanceS(th->content, _MNR(th));
	textLine->width = (uint32_t)width;
	textLine->updateSizes();
	return textLine;
}

void TextElement::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ContentElement>::getRef());
	REGISTER_GETTER_SETTER(c, text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextElement, text);

ASFUNCTIONBODY(TextElement, _constructor)
{
	TextElement* th=static_cast<TextElement*>(obj);
	ARG_UNPACK (th->text, "");
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextElement constructor ignores some parameters");

	return NULL;
}

TextLine::TextLine(Class_base* c, _NR<ContentElement> content, _NR<TextBlock> owner)
  : DisplayObjectContainer(c), TextData()
{
	textBlock = owner;

	if (content.isNull() || !content->is<TextElement>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "TextLine supports only TextElements");
		return;
	}

	TextElement *textElement = content->as<TextElement>();
	text = textElement->text;
	updateSizes();
	requestInvalidation(getSys());
}

void TextLine::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	REGISTER_GETTER(c, textBlock);
	c->setVariableByQName("MAX_LINE_WIDTH","",abstract_ui(MAX_LINE_WIDTH),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_GETTER(TextLine, textBlock);

ASFUNCTIONBODY(TextLine, _constructor)
{
	// Should throw ArgumentError when called from AS code
	//throw Class<ArgumentError>::getInstanceS("Error #2012: TextLine class cannot be instantiated.");

	return NULL;
}

void TextLine::updateSizes()
{
	uint32_t w,h,tw,th;
	w = width;
	h = height;
	//Compute (text)width, (text)height
	CairoPangoRenderer::getBounds(*this, w, h, tw, th);
	width = w; //TODO: check the case when w,h == 0
	textWidth = w;
	height = h;
	textHeight = h;
}

bool TextLine::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

void TextLine::requestInvalidation(InvalidateQueue* q)
{
	DisplayObjectContainer::requestInvalidation(q);
	incRef();
	q->addToInvalidateQueue(_MR(this));
}

IDrawable* TextLine::invalidate(DisplayObject* target, const MATRIX& initialMatrix)
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return NULL;
	}

	//Compute the matrix and the masks that are relevant
	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;
	computeMasksAndMatrix(target,masks,totalMatrix);
	totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
	if(width==0 || height==0)
		return NULL;

	return new CairoPangoRenderer(*this,
				      totalMatrix, x, y, width, height, 1.0f,
				      getConcatenatedAlpha(),masks);
}

void TextLine::renderImpl(RenderContext& ctxt) const
{
	defaultRender(ctxt);
}

_NR<InteractiveObject> TextLine::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	number_t xmin,xmax,ymin,ymax;
	boundsRect(xmin,xmax,ymin,ymax);
	if( xmin <= x && x <= xmax && ymin <= y && y <= ymax && isHittable(type))
		return last;
	else
	{
		incRef();
		return DisplayObjectContainer::hitTestImpl(_MR(this), x, y, type);
	}
}
