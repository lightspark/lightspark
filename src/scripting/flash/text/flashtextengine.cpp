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

#include "scripting/flash/text/flashtextengine.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swf.h"

#define MAX_LINE_WIDTH 1000000

using namespace std;
using namespace lightspark;

void ContentElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,elementFormat);
}
ASFUNCTIONBODY_GETTER_SETTER(ContentElement,elementFormat)

ElementFormat::ElementFormat(Class_base *c): ASObject(c),
	alignmentBaseline("useDominantBaseline"),
	alpha(1.0),
	baselineShift(0.0),
	breakOpportunity("auto"),
	color(0x000000),
	digitCase("default"),
	digitWidth("default"),
	dominantBaseline("roman"),
	fontDescription(NULL),
	fontSize(12.0),
	kerning("on"),
	ligatureLevel("common"),
	locale("en"),
	locked(false),
	textRotation("auto"),
	trackingLeft(0.0),
	trackingRight(0.0),
	typographicCase("default")
{
	
}

void ElementFormat::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("GRAPHIC_ELEMENT","",abstract_ui(0xFDEF),CONSTANT_TRAIT);
	REGISTER_GETTER_SETTER(c,alignmentBaseline);
	REGISTER_GETTER_SETTER(c,alpha);
	REGISTER_GETTER_SETTER(c,baselineShift);
	REGISTER_GETTER_SETTER(c,breakOpportunity);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,digitCase);
	REGISTER_GETTER_SETTER(c,digitWidth);
	REGISTER_GETTER_SETTER(c,dominantBaseline);
	REGISTER_GETTER_SETTER(c,fontDescription);
	REGISTER_GETTER_SETTER(c,fontSize);
	REGISTER_GETTER_SETTER(c,kerning);
	REGISTER_GETTER_SETTER(c,ligatureLevel);
	REGISTER_GETTER_SETTER(c,locale);
	REGISTER_GETTER_SETTER(c,locked);
	REGISTER_GETTER_SETTER(c,textRotation);
	REGISTER_GETTER_SETTER(c,trackingLeft);
	REGISTER_GETTER_SETTER(c,trackingRight);
	REGISTER_GETTER_SETTER(c,typographicCase);
}
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,alignmentBaseline)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,alpha)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,baselineShift)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,breakOpportunity)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,color)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,digitCase)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,digitWidth)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,dominantBaseline)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,fontDescription)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,fontSize)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,kerning)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,ligatureLevel)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,locale)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,locked)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,textRotation)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,trackingLeft)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,trackingRight)
ASFUNCTIONBODY_GETTER_SETTER(ElementFormat,typographicCase)

ASFUNCTIONBODY(ElementFormat, _constructor)
{
	ElementFormat* th=static_cast<ElementFormat*>(obj);
	ARG_UNPACK(th->fontDescription, NullRef)(th->fontSize, 12.0)(th->color, 0x000000) (th->alpha, 1.0)(th->textRotation, "auto")
			(th->dominantBaseline, "roman") (th->alignmentBaseline, "useDominantBaseline") (th->baselineShift, 0.0)(th->kerning, "on")
			(th->trackingRight, 0.0)(th->trackingLeft, 0.0)(th->locale, "en")(th->breakOpportunity, "auto")(th->digitCase, "default")
			(th->digitWidth, "default")(th->ligatureLevel, "common")(th->typographicCase, "default");
	return NULL;
}

void FontLookup::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("DEVICE","",Class<ASString>::getInstanceS("device"),CONSTANT_TRAIT);
	c->setVariableByQName("EMBEDDED_CFF","",Class<ASString>::getInstanceS("embeddedCFF"),CONSTANT_TRAIT);
}

void FontDescription::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,cffHinting);
	REGISTER_GETTER_SETTER(c,fontLookup);
	REGISTER_GETTER_SETTER(c,fontName);
	REGISTER_GETTER_SETTER(c,fontPosture);
	REGISTER_GETTER_SETTER(c,fontWeight);
	REGISTER_GETTER_SETTER(c,locked);
	REGISTER_GETTER_SETTER(c,renderingMode);
}

ASFUNCTIONBODY(FontDescription, _constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "FontDescription class not implemented");
	return NULL;
}
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,cffHinting)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontLookup)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontName)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontPosture)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontWeight)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,locked)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,renderingMode)

void FontWeight::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("BOLD","",Class<ASString>::getInstanceS("bold"),CONSTANT_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),CONSTANT_TRAIT);
}

void FontMetrics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_FINAL);
}
ASFUNCTIONBODY(FontMetrics, _constructor)
{
	//FontMetrics* th=static_cast<FontMetrics*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "FontMetrics is a stub");
	return NULL;
}

void Kerning::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("AUTO","",Class<ASString>::getInstanceS("auto"),CONSTANT_TRAIT);
	c->setVariableByQName("OFF","",Class<ASString>::getInstanceS("off"),CONSTANT_TRAIT);
	c->setVariableByQName("ON","",Class<ASString>::getInstanceS("on"),CONSTANT_TRAIT);
}

void LineJustification::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ALL_BUT_LAST","",Class<ASString>::getInstanceS("allButLast"),CONSTANT_TRAIT);
	c->setVariableByQName("ALL_BUT_MANDATORY_BREAK","",Class<ASString>::getInstanceS("allButMandatoryBreak"),CONSTANT_TRAIT);
	c->setVariableByQName("ALL_INCLUDING_LAST","",Class<ASString>::getInstanceS("allIncludingLast"),CONSTANT_TRAIT);
	c->setVariableByQName("UNJUSTIFIED","",Class<ASString>::getInstanceS("unjustified"),CONSTANT_TRAIT);
}
void TextBaseline::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ASCENT","",Class<ASString>::getInstanceS("ascent"),CONSTANT_TRAIT);
	c->setVariableByQName("DESCENT","",Class<ASString>::getInstanceS("descent"),CONSTANT_TRAIT);
	c->setVariableByQName("IDEOGRAPHIC_BOTTOM","",Class<ASString>::getInstanceS("ideographicBottom"),CONSTANT_TRAIT);
	c->setVariableByQName("IDEOGRAPHIC_CENTER","",Class<ASString>::getInstanceS("ideographicCenter"),CONSTANT_TRAIT);
	c->setVariableByQName("IDEOGRAPHIC_TOP","",Class<ASString>::getInstanceS("ideographicTop"),CONSTANT_TRAIT);
	c->setVariableByQName("ROMAN","",Class<ASString>::getInstanceS("roman"),CONSTANT_TRAIT);
	c->setVariableByQName("USE_DOMINANT_BASELINE","",Class<ASString>::getInstanceS("useDominantBaseline"),CONSTANT_TRAIT);
}

void TextJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, 0);
}
ASFUNCTIONBODY(TextJustifier, _constructor)
{
	throwError<ArgumentError>(kCantInstantiateError, "TextJustifier cannot be instantiated");
	return NULL;
}
void SpaceJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY(SpaceJustifier, _constructor)
{
	//SpaceJustifier* th=static_cast<SpaceJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "SpaceJustifier is a stub");
	return NULL;
}
void EastAsianJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY(EastAsianJustifier, _constructor)
{
	//EastAsianJustifier* th=static_cast<EastAsianJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "EastAsianJustifier is a stub");
	return NULL;
}

void TextBlock::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("createTextLine","",Class<IFunction>::getFunction(createTextLine),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("recreateTextLine","",Class<IFunction>::getFunction(recreateTextLine),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, content);
	REGISTER_GETTER_SETTER(c, textJustifier);
	REGISTER_GETTER_SETTER(c, bidiLevel);
}

ASFUNCTIONBODY_GETTER_SETTER(TextBlock, content)
ASFUNCTIONBODY_GETTER_SETTER(TextBlock, textJustifier)
ASFUNCTIONBODY_GETTER_SETTER(TextBlock, bidiLevel)

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
	number_t lineOffset;
	bool fitSomething;
	ARG_UNPACK (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false);

	if (argslen > 2)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::createTextLine ignored some parameters");

	if (!fitSomething && (width < 0 || width > MAX_LINE_WIDTH))
	{
		throwError<ArgumentError>(kOutOfRangeError,"Invalid width");
	}

	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>() || th->content->as<TextElement>()->text.empty())
		return NULL;
	tiny_string linetext = th->content->as<TextElement>()->text;
	if (fitSomething && linetext == "")
		linetext = " ";
		
	LOG(LOG_NOT_IMPLEMENTED,"splitting textblock in multiple lines not implemented");
	th->content->as<TextElement>()->text = "";
	th->incRef();
	TextLine *textLine = Class<TextLine>::getInstanceS(linetext, _MNR(th));
	textLine->width = (uint32_t)width;
	textLine->previousLine = previousLine;
	textLine->updateSizes();
	if (textLine->width > textLine->textWidth)
	{
		delete textLine;
		th->decRef();
		return NULL;
	}
	if (!previousLine.isNull())
		previousLine->nextLine == textLine;
	return textLine;
}
ASFUNCTIONBODY(TextBlock, recreateTextLine)
{
	TextBlock* th=static_cast<TextBlock*>(obj);
	_NR<TextLine> previousLine;
	_NR<TextLine> textLine;
	int32_t width;
	number_t lineOffset;
	bool fitSomething;
	ARG_UNPACK (textLine) (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false);

	if (argslen > 2)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::recreateTextLine ignored some parameters");
	LOG(LOG_NOT_IMPLEMENTED, "TextBlock::recreateTextLine doesn't check all parameters for validity");

	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>() || th->content->as<TextElement>()->text.empty())
		return NULL;

	if (!fitSomething && (width < 0 || width > MAX_LINE_WIDTH))
	{
		throwError<ArgumentError>(kOutOfRangeError,"Invalid width");
	}

	if (textLine.isNull())
	{
		throwError<ArgumentError>(kInvalidArgumentError,"Invalid argument: textLine");
	}

	if (th != textLine->textBlock.getPtr())
	{
		throwError<ArgumentError>(kInvalidArgumentError,"Invalid argument: textLine is in different textBlock");
	}
	if (fitSomething && textLine->text == "")
		textLine->text = " ";
	textLine->width = (uint32_t)width;
	textLine->previousLine = previousLine;
	textLine->updateSizes();
	if (textLine->width > textLine->textWidth)
	{
		return NULL;
	}
	if (!previousLine.isNull())
		previousLine->nextLine == textLine;
	return textLine.getPtr();
}

void TextElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ContentElement, _constructor, CLASS_FINAL | CLASS_SEALED);
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

TextLine::TextLine(Class_base* c, tiny_string linetext, _NR<TextBlock> owner)
  : DisplayObjectContainer(c), TextData(),nextLine(NULL),previousLine(NULL),userData(NULL)
{
	textBlock = owner;

	text = linetext;
	updateSizes();
	requestInvalidation(getSys());
}

void TextLine::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("MAX_LINE_WIDTH","",abstract_ui(MAX_LINE_WIDTH),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("descent","",Class<IFunction>::getFunction(getDescent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ascent","",Class<IFunction>::getFunction(getAscent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",Class<IFunction>::getFunction(getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",Class<IFunction>::getFunction(getTextHeight),GETTER_METHOD,true);
	REGISTER_GETTER(c, textBlock);
	REGISTER_GETTER(c, nextLine);
	REGISTER_GETTER(c, previousLine);
	REGISTER_GETTER_SETTER(c, validity);
	REGISTER_GETTER_SETTER(c, userData);
}


ASFUNCTIONBODY_GETTER(TextLine, textBlock);
ASFUNCTIONBODY_GETTER(TextLine, nextLine);
ASFUNCTIONBODY_GETTER(TextLine, previousLine);
ASFUNCTIONBODY_GETTER_SETTER(TextLine, validity);
ASFUNCTIONBODY_GETTER_SETTER(TextLine, userData);

ASFUNCTIONBODY(TextLine, _constructor)
{
	// Should throw ArgumentError when called from AS code
	//throw Class<ArgumentError>::getInstanceS("Error #2012: TextLine class cannot be instantiated.");

	return NULL;
}
ASFUNCTIONBODY(TextLine, getDescent)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.descent");
	return abstract_d(0);
}

ASFUNCTIONBODY(TextLine, getAscent)
{
	TextLine* th=static_cast<TextLine*>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.ascent");
	return abstract_d(th->textHeight);
}

ASFUNCTIONBODY(TextLine, getTextWidth)
{
	TextLine* th=static_cast<TextLine*>(obj);
	return abstract_d(th->textWidth);
}

ASFUNCTIONBODY(TextLine, getTextHeight)
{
	TextLine* th=static_cast<TextLine*>(obj);
	return abstract_d(th->textHeight);
}


void TextLine::updateSizes()
{
	uint32_t w,h,tw,th;
	w = width;
	h = height;
	//Compute (text)width, (text)height
	CairoPangoRenderer::getBounds(*this, w, h, tw, th);
	if (w == 0)
		w = tw;
	if (h == 0)
		h = th;
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

_NR<DisplayObject> TextLine::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
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

void TextLineValidity::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("INVALID","",Class<ASString>::getInstanceS("invalid"),CONSTANT_TRAIT);
	c->setVariableByQName("POSSIBLY_INVALID","",Class<ASString>::getInstanceS("possiblyInvalid"),CONSTANT_TRAIT);
	c->setVariableByQName("STATIC","",Class<ASString>::getInstanceS("static"),CONSTANT_TRAIT);
	c->setVariableByQName("VALID","",Class<ASString>::getInstanceS("valid"),CONSTANT_TRAIT);
}

