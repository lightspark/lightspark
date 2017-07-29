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
#include "scripting/toplevel/Vector.h"
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

ElementFormat::ElementFormat(Class_base *c): ASObject(c,T_OBJECT,SUBTYPE_ELEMENTFORMAT),
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
	c->setVariableAtomByQName("GRAPHIC_ELEMENT",nsNameAndKind(),asAtom((uint32_t)0xFDEF),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),_clone),NORMAL_METHOD,true);

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

ASFUNCTIONBODY_ATOM(ElementFormat, _constructor)
{
	ElementFormat* th=obj.as<ElementFormat>();
	ARG_UNPACK_ATOM(th->fontDescription, NullRef)(th->fontSize, 12.0)(th->color, 0x000000) (th->alpha, 1.0)(th->textRotation, "auto")
			(th->dominantBaseline, "roman") (th->alignmentBaseline, "useDominantBaseline") (th->baselineShift, 0.0)(th->kerning, "on")
			(th->trackingRight, 0.0)(th->trackingLeft, 0.0)(th->locale, "en")(th->breakOpportunity, "auto")(th->digitCase, "default")
			(th->digitWidth, "default")(th->ligatureLevel, "common")(th->typographicCase, "default");
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(ElementFormat, _clone)
{
	ElementFormat* th=obj.as<ElementFormat>();

	ElementFormat* newformat = Class<ElementFormat>::getInstanceS(sys);
	newformat->fontDescription = th->fontDescription;
	newformat->fontSize = th->fontSize;
	newformat->color = th->color;
	newformat->alpha = th->alpha;
	newformat->textRotation = th->textRotation;
	newformat->dominantBaseline = th->dominantBaseline;
	newformat->alignmentBaseline = th->alignmentBaseline;
	newformat->baselineShift = th->baselineShift;
	newformat->kerning = th->kerning;
	newformat->trackingRight = th->trackingRight;
	newformat->trackingLeft = th->trackingLeft;
	newformat->locale = th->locale;
	newformat->breakOpportunity = th->breakOpportunity;
	newformat->digitCase = th->digitCase;
	newformat->digitWidth = th->digitWidth;
	newformat->ligatureLevel = th->ligatureLevel;
	newformat->typographicCase = th->typographicCase;
	newformat->locked = false;
	return asAtom::fromObject(newformat);
}

void FontLookup::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEVICE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"device"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EMBEDDED_CFF",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"embeddedCFF"),CONSTANT_TRAIT);
}

void FontDescription::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),_clone),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,cffHinting);
	REGISTER_GETTER_SETTER(c,fontLookup);
	REGISTER_GETTER_SETTER(c,fontName);
	REGISTER_GETTER_SETTER(c,fontPosture);
	REGISTER_GETTER_SETTER(c,fontWeight);
	REGISTER_GETTER_SETTER(c,locked);
	REGISTER_GETTER_SETTER(c,renderingMode);
}

ASFUNCTIONBODY_ATOM(FontDescription, _constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "FontDescription class not implemented");
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,cffHinting)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontLookup)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontName)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontPosture)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,fontWeight)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,locked)
ASFUNCTIONBODY_GETTER_SETTER(FontDescription,renderingMode)

ASFUNCTIONBODY_ATOM(FontDescription, _clone)
{
	FontDescription* th=obj.as<FontDescription>();

	FontDescription* newfontdescription = Class<FontDescription>::getInstanceS(sys);
	newfontdescription->cffHinting = th->cffHinting;
	newfontdescription->fontLookup = th->fontLookup;
	newfontdescription->fontName = th->fontName;
	newfontdescription->fontPosture = th->fontPosture;
	newfontdescription->fontWeight = th->fontWeight;
	newfontdescription->renderingMode = th->renderingMode;
	newfontdescription->locked = false;
	return asAtom::fromObject(newfontdescription);
}

void FontPosture::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ITALIC",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"italic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}
void FontWeight::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("BOLD",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"bold"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

void FontMetrics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(FontMetrics, _constructor)
{
	//FontMetrics* th=static_cast<FontMetrics*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "FontMetrics is a stub");
	return asAtom::invalidAtom;
}

void Kerning::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OFF",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"off"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ON",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"on"),CONSTANT_TRAIT);
}

void LineJustification::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ALL_BUT_LAST",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"allButLast"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALL_BUT_MANDATORY_BREAK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"allButMandatoryBreak"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALL_INCLUDING_LAST",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"allIncludingLast"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNJUSTIFIED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"unjustified"),CONSTANT_TRAIT);
}
void TextBaseline::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ASCENT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ascent"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESCENT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"descent"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_BOTTOM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ideographicBottom"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_CENTER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ideographicCenter"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_TOP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ideographicTop"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROMAN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"roman"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USE_DOMINANT_BASELINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"useDominantBaseline"),CONSTANT_TRAIT);
}

void TextJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, 0);
}
ASFUNCTIONBODY_ATOM(TextJustifier, _constructor)
{
	throwError<ArgumentError>(kCantInstantiateError, "TextJustifier cannot be instantiated");
	return asAtom::invalidAtom;
}
void SpaceJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(SpaceJustifier, _constructor)
{
	//SpaceJustifier* th=static_cast<SpaceJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "SpaceJustifier is a stub");
	return asAtom::invalidAtom;
}
void EastAsianJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(EastAsianJustifier, _constructor)
{
	//EastAsianJustifier* th=static_cast<EastAsianJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "EastAsianJustifier is a stub");
	return asAtom::invalidAtom;
}


TextBlock::TextBlock(Class_base *c): ASObject(c,T_OBJECT,SUBTYPE_TEXTBLOCK)
  ,applyNonLinearFontScaling(true),baselineFontSize(12),baselineZero("roman"),bidiLevel(0),firstLine(NullRef),lastLine(NullRef),lineRotation("rotate0")
{
}

void TextBlock::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("createTextLine","",Class<IFunction>::getFunction(c->getSystemState(),createTextLine),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("recreateTextLine","",Class<IFunction>::getFunction(c->getSystemState(),recreateTextLine),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("releaseLines","",Class<IFunction>::getFunction(c->getSystemState(),releaseLines),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, applyNonLinearFontScaling);
	REGISTER_GETTER_SETTER(c, baselineFontDescription);
	REGISTER_GETTER_SETTER(c, baselineFontSize);
	REGISTER_GETTER_SETTER(c, baselineZero);
	REGISTER_GETTER_SETTER(c, bidiLevel);
	REGISTER_GETTER_SETTER(c, content);
	REGISTER_GETTER(c, firstInvalidLine );
	REGISTER_GETTER(c, firstLine);
	REGISTER_GETTER(c, lastLine);
	REGISTER_GETTER_SETTER(c, lineRotation);
	REGISTER_GETTER_SETTER(c, textJustifier);
	REGISTER_GETTER_SETTER(c, tabStops);
	REGISTER_GETTER(c, textLineCreationResult);
	REGISTER_GETTER_SETTER(c, userData);
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, applyNonLinearFontScaling);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineFontDescription);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineFontSize);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineZero);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, bidiLevel);
ASFUNCTIONBODY_GETTER_SETTER(TextBlock, content);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, firstInvalidLine );
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, firstLine);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, lastLine);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, lineRotation);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, textJustifier);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, tabStops);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, textLineCreationResult);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, userData);

ASFUNCTIONBODY_ATOM(TextBlock, _constructor)
{
	TextBlock* th=obj.as<TextBlock>();
	ARG_UNPACK_ATOM (th->content, NullRef);
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock constructor ignores some parameters");

	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(TextBlock, createTextLine)
{
	TextBlock* th=obj.as<TextBlock>();
	_NR<TextLine> previousLine;
	int32_t width;
	number_t lineOffset;
	bool fitSomething;
	ARG_UNPACK_ATOM (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false);

	if (argslen > 2)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::createTextLine ignored some parameters");

	if (!fitSomething && (width < 0 || width > MAX_LINE_WIDTH))
	{
		throwError<ArgumentError>(kOutOfRangeError,"Invalid width");
	}

	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>() || th->content->as<TextElement>()->text.empty())
		return asAtom::invalidAtom;
	tiny_string linetext = th->content->as<TextElement>()->text;
	if (fitSomething && linetext == "")
		linetext = " ";
		
	LOG(LOG_NOT_IMPLEMENTED,"splitting textblock in multiple lines not implemented");
	th->content->as<TextElement>()->text = "";
	th->incRef();
	_NR<TextLine> textLine = _NR<TextLine>(Class<TextLine>::getInstanceS(sys,linetext, _MNR(th)));
	textLine->width = (uint32_t)width;
	textLine->previousLine = previousLine;
	textLine->updateSizes();
	if (textLine->width > textLine->textWidth)
	{
		textLine->decRef();
		th->decRef();
		return asAtom::invalidAtom;
	}
	if (previousLine.isNull())
	{
		textLine->incRef();
		th->firstLine = textLine;
		if (th->lastLine.isNull())
		{
			textLine->incRef();
			th->lastLine = textLine;
		}
	}
	else
	{
		if (th->lastLine == previousLine)
		{
			th->lastLine->decRef();
			textLine->incRef();
			th->lastLine = textLine;
		}
		textLine->incRef();
		previousLine->nextLine = textLine;
	}
	
	return asAtom::fromObject(textLine.getPtr());
}
ASFUNCTIONBODY_ATOM(TextBlock, recreateTextLine)
{
	TextBlock* th=obj.as<TextBlock>();
	_NR<TextLine> previousLine;
	_NR<TextLine> textLine;
	int32_t width;
	number_t lineOffset;
	bool fitSomething;
	ARG_UNPACK_ATOM (textLine) (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false);

	if (argslen > 2)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::recreateTextLine ignored some parameters");
	LOG(LOG_NOT_IMPLEMENTED, "TextBlock::recreateTextLine doesn't check all parameters for validity");

	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>() || th->content->as<TextElement>()->text.empty())
		return asAtom::invalidAtom;

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
		return asAtom::invalidAtom;
	}
	if (!previousLine.isNull())
		previousLine->nextLine == textLine;
	textLine->incRef();
	return asAtom::fromObject(textLine.getPtr());
}

ASFUNCTIONBODY_ATOM(TextBlock, releaseLines)
{
	TextBlock* th=obj.as<TextBlock>();
	_NR<TextLine> firstLine;
	_NR<TextLine> lastLine;
	ARG_UNPACK_ATOM (firstLine) (lastLine);


	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>() || th->content->as<TextElement>()->text.empty())
		return asAtom::invalidAtom;

	if (firstLine.isNull() || firstLine->textBlock != th)
	{
		throwError<ArgumentError>(kInvalidArgumentError,"Invalid argument: firstLine");
	}
	if (lastLine.isNull() || lastLine->textBlock != th)
	{
		throwError<ArgumentError>(kInvalidArgumentError,"Invalid argument: lastLine");
	}

	bool afterlast = false;
	_NR<TextLine> tmpLine;
	_NR<TextLine> tmpLine2;
	while (!firstLine.isNull())
	{
		firstLine->validity = "invalid";
		tmpLine2 = firstLine->nextLine;
		if (!afterlast)
		{
			if (!firstLine->previousLine.isNull())
			{
				tmpLine = firstLine->previousLine;
				firstLine->previousLine = NullRef;
				tmpLine->decRef();
			}
			firstLine->textBlock = NullRef;
			firstLine->nextLine = NullRef;
		}
		if (firstLine == lastLine)
			afterlast = true;
		firstLine = tmpLine2;
	}
	
	return asAtom::invalidAtom;
}

void TextElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ContentElement, _constructor, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextElement, text);

ASFUNCTIONBODY_ATOM(TextElement, _constructor)
{
	TextElement* th=obj.as<TextElement>();
	ARG_UNPACK_ATOM (th->text, "");
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextElement constructor ignores some parameters");

	return asAtom::invalidAtom;
}

void GroupElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ContentElement, _constructor, CLASS_FINAL | CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(GroupElement, _constructor)
{
	//GroupElement* th=static_cast<GroupElement*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "GroupElement constructor not implemented");
	return asAtom::invalidAtom;
}

TextLine::TextLine(Class_base* c, tiny_string linetext, _NR<TextBlock> owner)
  : DisplayObjectContainer(c), TextData(),nextLine(NULL),previousLine(NULL),userData(NULL)
  ,hasGraphicElement(false),hasTabs(false),rawTextLength(0),specifiedWidth(0),textBlockBeginIndex(0)
{
	textBlock = owner;

	text = linetext;
	updateSizes();
	requestInvalidation(getSys());
}

void TextLine::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("MAX_LINE_WIDTH",nsNameAndKind(),asAtom((uint32_t)MAX_LINE_WIDTH),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("getBaselinePosition","",Class<IFunction>::getFunction(c->getSystemState(),getBaselinePosition),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descent","",Class<IFunction>::getFunction(c->getSystemState(),getDescent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ascent","",Class<IFunction>::getFunction(c->getSystemState(),getAscent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",Class<IFunction>::getFunction(c->getSystemState(),getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",Class<IFunction>::getFunction(c->getSystemState(),getTextHeight),GETTER_METHOD,true);
	REGISTER_GETTER(c, textBlock);
	REGISTER_GETTER(c, nextLine);
	REGISTER_GETTER(c, previousLine);
	REGISTER_GETTER_SETTER(c, validity);
	REGISTER_GETTER_SETTER(c, userData);
	REGISTER_GETTER(c, hasGraphicElement);
	REGISTER_GETTER(c, hasTabs);
	REGISTER_GETTER(c, rawTextLength);
	REGISTER_GETTER(c, specifiedWidth);
	REGISTER_GETTER(c, textBlockBeginIndex);
}


ASFUNCTIONBODY_GETTER(TextLine, textBlock);
ASFUNCTIONBODY_GETTER(TextLine, nextLine);
ASFUNCTIONBODY_GETTER(TextLine, previousLine);
ASFUNCTIONBODY_GETTER_SETTER(TextLine, validity);
ASFUNCTIONBODY_GETTER_SETTER(TextLine, userData);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, hasGraphicElement);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, hasTabs);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, rawTextLength);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, specifiedWidth);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, textBlockBeginIndex);

ASFUNCTIONBODY_ATOM(TextLine, _constructor)
{
	// Should throw ArgumentError when called from AS code
	//throw Class<ArgumentError>::getInstanceS("Error #2012: TextLine class cannot be instantiated.");

	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(TextLine, getBaselinePosition)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.getBaselinePosition");
	return asAtom(0);
}

ASFUNCTIONBODY_ATOM(TextLine, getDescent)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.descent");
	return asAtom(0);
}

ASFUNCTIONBODY_ATOM(TextLine, getAscent)
{
	TextLine* th=obj.as<TextLine>();
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.ascent");
	return asAtom(th->textHeight);
}

ASFUNCTIONBODY_ATOM(TextLine, getTextWidth)
{
	TextLine* th=obj.as<TextLine>();
	return asAtom(th->textWidth);
}

ASFUNCTIONBODY_ATOM(TextLine, getTextHeight)
{
	TextLine* th=obj.as<TextLine>();
	return asAtom(th->textHeight);
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

void TabStop::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(TabStop, _constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "TabStop constructor not implemented");
	return asAtom::invalidAtom;
}
void BreakOpportunity::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ALL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"all"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"any"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}
void CFFHinting::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("HORIZONTAL_STEM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"horizontalStem"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}
void DigitCase::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"default"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINING",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"lining"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OLD_STYLE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"oldStyle"),CONSTANT_TRAIT);
}
void DigitWidth::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"default"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PROPORTIONAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"proportional"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TABULAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"tabular"),CONSTANT_TRAIT);
}
void JustificationStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("PRIORITIZE_LEAST_ADJUSTMENT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"prioritizeLeastAdjustment"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PUSH_IN_KINSOKU",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"pushInKinsoku"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PUSH_OUT_ONLY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"pushOutOnly"),CONSTANT_TRAIT);
}
void LigatureLevel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("COMMON",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"common"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EXOTIC",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"exotic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MINIMUM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"minimum"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNCOMMON",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"uncommon"),CONSTANT_TRAIT);
}
void RenderingMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CFF",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"cff"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}
void TabAlignment::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CENTER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"center"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DECIMAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"decimal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("END",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"end"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("START",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"start"),CONSTANT_TRAIT);
}
void TextLineValidity::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("INVALID",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"invalid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("POSSIBLY_INVALID",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"possiblyInvalid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STATIC",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"static"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VALID",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"valid"),CONSTANT_TRAIT);
}
void TextRotation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_0",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rotate0"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_180",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rotate180"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_270",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rotate270"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_90",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rotate90"),CONSTANT_TRAIT);
}
