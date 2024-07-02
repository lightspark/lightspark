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
#include "scripting/flash/text/flashtext.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/class.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"
#include "swf.h"
#include "platforms/engineutils.h"

#define MAX_LINE_WIDTH 1000000

using namespace std;
using namespace lightspark;

void ContentElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED);
	c->isReusable=true;
	REGISTER_GETTER_RESULTTYPE(c,rawText,ASString);
	REGISTER_GETTER_SETTER(c,elementFormat);
	REGISTER_GETTER_RESULTTYPE(c,groupElement,GroupElement);
	REGISTER_GETTER_RESULTTYPE(c,textBlockBeginIndex,Integer);
	
	c->setVariableAtomByQName("GRAPHIC_ELEMENT",nsNameAndKind(),asAtomHandler::fromUInt(0xFDEF),CONSTANT_TRAIT);
}

void ContentElement::finalize()
{
	ASObject::finalize();
	textBlockBeginIndex=-1;
	elementFormat.reset();
	groupElement.reset();
}

bool ContentElement::destruct()
{
	textBlockBeginIndex=-1;
	elementFormat.reset();
	groupElement.reset();
	rawText.clear();
	return ASObject::destruct();
}

void ContentElement::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();

	if (elementFormat)
		elementFormat->prepareShutdown();
	if (groupElement)
		groupElement->prepareShutdown();
}

bool ContentElement::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (elementFormat)
		ret = elementFormat->countAllCylicMemberReferences(gcstate) || ret;
	if (groupElement)
		ret = groupElement->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}
ASFUNCTIONBODY_GETTER_SETTER(ContentElement,elementFormat)
ASFUNCTIONBODY_GETTER(ContentElement,rawText)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(ContentElement,groupElement)
ASFUNCTIONBODY_GETTER(ContentElement,textBlockBeginIndex)


ElementFormat::ElementFormat(ASWorker* wrk, Class_base *c): ASObject(wrk,c,T_OBJECT,SUBTYPE_ELEMENTFORMAT),
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
	c->isReusable=true;
	c->setVariableAtomByQName("GRAPHIC_ELEMENT",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)0xFDEF),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(_clone,0,Class<ElementFormat>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);

	REGISTER_GETTER_SETTER_RESULTTYPE(c,alignmentBaseline,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,alpha,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,baselineShift,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,breakOpportunity,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,color,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,digitCase,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,digitWidth,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,dominantBaseline,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,fontDescription,FontDescription);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,fontSize,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,kerning,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,ligatureLevel,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,locale,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,locked,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,textRotation,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,trackingLeft,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,trackingRight,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,typographicCase,ASString);
}

void ElementFormat::finalize()
{
	ASObject::finalize();
	fontDescription.reset();
}

bool ElementFormat::destruct()
{
	alignmentBaseline= "useDominantBaseline";
	alpha = 1.0;
	baselineShift = 0.0;
	breakOpportunity = "auto";
	color = 0x000000;
	digitCase = "default";
	digitWidth = "default";
	dominantBaseline = "roman";
	fontSize = 12.0;
	kerning = "on";
	ligatureLevel = "common";
	locale = "en";
	locked = false;
	textRotation = "auto";
	trackingLeft = 0.0;
	trackingRight = 0.0;
	typographicCase = "default";
	fontDescription.reset();
	return ASObject::destruct();
}

void ElementFormat::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	
	if (fontDescription)
		fontDescription->prepareShutdown();
}

bool ElementFormat::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (fontDescription)
		ret = fontDescription->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
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

ASFUNCTIONBODY_ATOM(ElementFormat,_constructor)
{
	ElementFormat* th=asAtomHandler::as<ElementFormat>(obj);
	ARG_CHECK(ARG_UNPACK(th->fontDescription, NullRef)(th->fontSize, 12.0)(th->color, 0x000000) (th->alpha, 1.0)(th->textRotation, "auto")
			(th->dominantBaseline, "roman") (th->alignmentBaseline, "useDominantBaseline") (th->baselineShift, 0.0)(th->kerning, "on")
			(th->trackingRight, 0.0)(th->trackingLeft, 0.0)(th->locale, "en")(th->breakOpportunity, "auto")(th->digitCase, "default")
			(th->digitWidth, "default")(th->ligatureLevel, "common")(th->typographicCase, "default"));
}
ASFUNCTIONBODY_ATOM(ElementFormat, _clone)
{
	ElementFormat* th=asAtomHandler::as<ElementFormat>(obj);

	ElementFormat* newformat = Class<ElementFormat>::getInstanceS(wrk);
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
	ret = asAtomHandler::fromObject(newformat);
}

void FontLookup::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEVICE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"device"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EMBEDDED_CFF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"embeddedCFF"),CONSTANT_TRAIT);
}

void FontDescription::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(_clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isFontCompatible","",c->getSystemState()->getBuiltinFunction(isFontCompatible),NORMAL_METHOD,false);

    c->setDeclaredMethodByQName("fontName","",c->getSystemState()->getBuiltinFunction(_getFontName),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("fontName","",c->getSystemState()->getBuiltinFunction(_setFontName),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("fontWeight","",c->getSystemState()->getBuiltinFunction(_getFontWeight),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("fontWeight","",c->getSystemState()->getBuiltinFunction(_setFontWeight),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("fontPosture","",c->getSystemState()->getBuiltinFunction(_getFontPosture),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("fontPosture","",c->getSystemState()->getBuiltinFunction(_setFontPosture),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("fontLookup","",c->getSystemState()->getBuiltinFunction(_getFontLookup),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("fontLookup","",c->getSystemState()->getBuiltinFunction(_setFontLookup),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("renderingMode","",c->getSystemState()->getBuiltinFunction(_getRenderingMode),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("renderingMode","",c->getSystemState()->getBuiltinFunction(_setRenderingMode),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("cffHinting","",c->getSystemState()->getBuiltinFunction(_getCffHinting),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("cffHinting","",c->getSystemState()->getBuiltinFunction(_setCffHinting),SETTER_METHOD,true);

    c->setDeclaredMethodByQName("locked","",c->getSystemState()->getBuiltinFunction(_getLocked),GETTER_METHOD,true);
    c->setDeclaredMethodByQName("locked","",c->getSystemState()->getBuiltinFunction(_setLocked),SETTER_METHOD,true);
}

bool FontDescription::destruct()
{
	cffHinting = "horizontalStem";
	fontLookup = "device";
	fontName = "_serif";
	fontPosture = "normal";
	fontWeight = "normal";
	locked = false;
	renderingMode = "cff";
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(FontDescription,_constructor)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ARG_CHECK(ARG_UNPACK(th->fontName, "_serif")(th->fontWeight, "normal")(th->fontPosture, "normal")(th->fontLookup, "device")(th->renderingMode, "cff")(th->cffHinting, "horizontalStem"));
}

ASFUNCTIONBODY_ATOM(FontDescription, _clone)
{
	FontDescription* th=asAtomHandler::as<FontDescription>(obj);

	FontDescription* newfontdescription = Class<FontDescription>::getInstanceS(wrk);
	newfontdescription->cffHinting = th->cffHinting;
	newfontdescription->fontLookup = th->fontLookup;
	newfontdescription->fontName = th->fontName;
	newfontdescription->fontPosture = th->fontPosture;
	newfontdescription->fontWeight = th->fontWeight;
	newfontdescription->renderingMode = th->renderingMode;
	newfontdescription->locked = false;
	ret = asAtomHandler::fromObject(newfontdescription);
}

ASFUNCTIONBODY_ATOM(FontDescription, isFontCompatible)
{
	tiny_string fontName;
	tiny_string fontWeight;
	tiny_string fontPosture;
	ARG_CHECK(ARG_UNPACK(fontName)(fontWeight)(fontPosture));
	bool italic = false;
	bool bold = false;
	if (fontWeight == "bold")
		bold = true;
	else if (fontWeight != "normal")
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"fontWeight");
		return;
	}
	if (fontPosture == "italic")
		italic = true;
	else if (fontPosture != "normal")
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"fontPosture");
		return;
	}
	tiny_string fontStyle = bold ? (italic ? "boldItalic" : "bold") : (italic ? "italic" : "regular");
	std::vector<asAtom>* flist = ASFont::getFontList();
	auto it = flist->begin();
	while (it != flist->end())
	{
		ASFont* f = asAtomHandler::as<ASFont>(*it);
		if (f->fontName == fontName &&
			f->fontType == "embeddedCFF" &&
			f->fontStyle == fontStyle)
		{
			asAtomHandler::setBool(ret,true);
			return;
		}
		it++;
	}
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(FontDescription,isDeviceFontCompatible)
{
	// Similar to isFontCompatible but without embeddedCFF check.

    tiny_string fontName;
    tiny_string fontWeight;
    tiny_string fontPosture;
    ARG_CHECK(ARG_UNPACK(fontName)(fontWeight)(fontPosture));
    bool italic = false;
    bool bold = false;
    if (fontWeight == "bold")
        bold = true;
    else if (fontWeight != "normal")
	{
        createError<ArgumentError>(wrk,kInvalidArgumentError,"fontWeight");
		return;
	}
    if (fontPosture == "italic")
        italic = true;
    else if (fontPosture != "normal")
	{
        createError<ArgumentError>(wrk,kInvalidArgumentError,"fontPosture");
		return;
	}
    tiny_string fontStyle = bold ? (italic ? "boldItalic" : "bold") : (italic ? "italic" : "regular");
    std::vector<asAtom>* flist = ASFont::getFontList();
    auto it = flist->begin();
    while (it != flist->end())
    {
        ASFont* f = asAtomHandler::as<ASFont>(*it);
        if (f->fontName == fontName &&
            f->fontStyle == fontStyle)
        {
            asAtomHandler::setBool(ret,true);
            return;
        }
        it++;
    }
    asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(FontDescription,_getLocked)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromBool(th->locked);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setLocked)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        bool value;
        ARG_CHECK(ARG_UNPACK(value));
        th->locked = value;
    }
    else
    {
        createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
    }
}

ASFUNCTIONBODY_ATOM(FontDescription,_getFontName)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->fontName);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setFontName)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
        th->fontName = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

ASFUNCTIONBODY_ATOM(FontDescription,_getFontWeight)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->fontWeight);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setFontWeight)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
		if (value != "bold" && value != "normal")
		{
			createError<ArgumentError>(wrk,kInvalidEnumError, "fontWeight");
			return;
		}
        th->fontWeight = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

ASFUNCTIONBODY_ATOM(FontDescription,_getFontPosture)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->fontPosture);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setFontPosture)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
		if (value != "italic" && value != "normal")
		{
			createError<ArgumentError>(wrk,kInvalidEnumError, "fontPosture");
			return;
		}
        th->fontPosture = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

ASFUNCTIONBODY_ATOM(FontDescription,_getFontLookup)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->fontLookup);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setFontLookup)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
		if (value != "device" && value != "embeddedCFF")
		{
			createError<ArgumentError>(wrk,kInvalidEnumError, "fontLookup");
			return;
		}
        th->fontLookup = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

ASFUNCTIONBODY_ATOM(FontDescription,_getRenderingMode)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->renderingMode);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setRenderingMode)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
		if (value != "cff" && value != "normal")
		{
			createError<ArgumentError>(wrk,kInvalidEnumError, "renderingMode");
			return;
		}
        th->renderingMode = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

ASFUNCTIONBODY_ATOM(FontDescription,_getCffHinting)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    ret = asAtomHandler::fromString(wrk->getSystemState(),th->cffHinting);
}

ASFUNCTIONBODY_ATOM(FontDescription,_setCffHinting)
{
    FontDescription* th=asAtomHandler::as<FontDescription>(obj);
    if (!th->locked)
    {
        tiny_string value;
        ARG_CHECK(ARG_UNPACK(value));
		if (value != "horizontalStem" && value != "none")
		{
			createError<ArgumentError>(wrk,kInvalidEnumError, "cffHinting");
			return;
		}
        th->cffHinting = value;
    }
	else
	{
		createError<ArgumentError>(wrk,kCantInstantiateError, "The FontDescription object is locked and cannot be modified.");
	}
}

void FontPosture::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ITALIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"italic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}
void FontWeight::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("BOLD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bold"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

void FontMetrics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(FontMetrics,_constructor)
{
	//FontMetrics* th=static_cast<FontMetrics*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "FontMetrics is a stub");
}

void Kerning::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OFF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"off"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ON",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"on"),CONSTANT_TRAIT);
}

void LineJustification::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ALL_BUT_LAST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"allButLast"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALL_BUT_MANDATORY_BREAK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"allButMandatoryBreak"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALL_INCLUDING_LAST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"allIncludingLast"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNJUSTIFIED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unjustified"),CONSTANT_TRAIT);
}
void TextBaseline::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ASCENT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ascent"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESCENT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"descent"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_BOTTOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ideographicBottom"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_CENTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ideographicCenter"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IDEOGRAPHIC_TOP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ideographicTop"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROMAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"roman"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USE_DOMINANT_BASELINE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"useDominantBaseline"),CONSTANT_TRAIT);
}

void TextJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, 0);
}
ASFUNCTIONBODY_ATOM(TextJustifier,_constructor)
{
	createError<ArgumentError>(wrk,kCantInstantiateError, "TextJustifier cannot be instantiated");
}
void SpaceJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(SpaceJustifier,_constructor)
{
	//SpaceJustifier* th=static_cast<SpaceJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "SpaceJustifier is a stub");
}
void EastAsianJustifier::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextJustifier, _constructor, CLASS_FINAL);
}
ASFUNCTIONBODY_ATOM(EastAsianJustifier,_constructor)
{
	//EastAsianJustifier* th=static_cast<EastAsianJustifier*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "EastAsianJustifier is a stub");
}


TextBlock::TextBlock(ASWorker* wrk, Class_base *c): ASObject(wrk,c,T_OBJECT,SUBTYPE_TEXTBLOCK)
  ,applyNonLinearFontScaling(true),baselineFontSize(12),baselineZero("roman"),bidiLevel(0),firstLine(NullRef),lastLine(NullRef),lineRotation("rotate0")
{
}

void TextBlock::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("createTextLine","",c->getSystemState()->getBuiltinFunction(createTextLine,0,Class<TextLine>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("recreateTextLine","",c->getSystemState()->getBuiltinFunction(recreateTextLine,0,Class<TextLine>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("releaseLines","",c->getSystemState()->getBuiltinFunction(releaseLines),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, applyNonLinearFontScaling, Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, baselineFontDescription, FontDescription);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, baselineFontSize, Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, baselineZero, ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, bidiLevel, Integer);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, content, ContentElement);
	REGISTER_GETTER_RESULTTYPE(c, firstInvalidLine, TextLine);
	REGISTER_GETTER_RESULTTYPE(c, firstLine, TextLine);
	REGISTER_GETTER_RESULTTYPE(c, lastLine, TextLine);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, lineRotation, ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, textJustifier, TextJustifier);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, tabStops, Vector);
	REGISTER_GETTER_RESULTTYPE(c, textLineCreationResult, ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, userData, ASObject);
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, applyNonLinearFontScaling)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineFontDescription)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineFontSize)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, baselineZero)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, bidiLevel)
ASFUNCTIONBODY_GETTER_SETTER(TextBlock, content)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, firstInvalidLine )
ASFUNCTIONBODY_GETTER(TextBlock, firstLine)
ASFUNCTIONBODY_GETTER(TextBlock, lastLine)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, lineRotation)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, textJustifier)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, tabStops)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextBlock, textLineCreationResult)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextBlock, userData)

ASFUNCTIONBODY_ATOM(TextBlock,_constructor)
{
	TextBlock* th=asAtomHandler::as<TextBlock>(obj);
	ARG_CHECK(ARG_UNPACK (th->content, NullRef));
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock constructor ignores some parameters");
}

bool TextBlock::fillTextLine(_NR<TextLine> textLine, bool fitSomething, _NR<TextLine> previousLine)
{
	tiny_string linetext = asAtomHandler::toString(content->as<TextElement>()->text,getInstanceWorker());
	uint32_t startpos = 0;
	if (!previousLine.isNull())
		startpos += previousLine->textBlockBeginIndex + previousLine->getText().numChars();
	if (linetext.numChars() <= startpos)
		linetext.clear();
	else
		linetext = linetext.substr(startpos,linetext.numChars()-startpos);
	if (fitSomething && linetext.empty())
		linetext = " ";
	textLine->setText(linetext.raw_buf(),true);
	textLine->checkEmbeddedFont(textLine.getPtr());
	textLine->textBlockBeginIndex = startpos;
	textLine->rawTextLength=textLine->getText().numChars();
	return textLine->rawTextLength>0;
}
ASFUNCTIONBODY_ATOM(TextBlock, createTextLine)
{
	TextBlock* th=asAtomHandler::as<TextBlock>(obj);
	_NR<TextLine> previousLine;
	int32_t width;
	number_t lineOffset;
	bool fitSomething;
	ARG_CHECK(ARG_UNPACK (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false));

	if (lineOffset != 0.0)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::createTextLine ignores parameter lineOffset");

	if (!fitSomething && ((width < 0) || (width > MAX_LINE_WIDTH)))
	{
		createError<ArgumentError>(wrk,kOutOfRangeError,"Invalid width");
		return;
	}

	if (th->content.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}
	if (!th->content->is<TextElement>())
	{
		LOG(LOG_NOT_IMPLEMENTED,"TextBlock.createTextLine support for content other than TextElement not yet implemented:"<<th->content->toDebugString());
		asAtomHandler::setNull(ret);
		return;
	}
	if (asAtomHandler::isNull(th->content->as<TextElement>()->text))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	th->incRef();
	_NR<TextLine> textLine = _NR<TextLine>(Class<TextLine>::getInstanceS(wrk, _MNR(th)));
	if (!th->fillTextLine(textLine, fitSomething, previousLine))
	{
		textLine.reset();
		// it's not in the specs but it seems that we have to return null if we are at the end of the content
		asAtomHandler::setNull(ret);
		th->textLineCreationResult="complete";
		return;
	}
	
	textLine->previousLine = previousLine;
		

	// Set baseline font
	textLine->font = th->baselineZero;

	// Set font from FontDescription in TextBlock if possible
 	if (!th->baselineFontDescription.isNull())
	{
 		textLine->font = th->baselineFontDescription->fontName;
	}

	// Set font from FontDescription in TextElement if possible
	auto elementFormat = th->content->as<TextElement>()->elementFormat;
	if (!elementFormat.isNull() &&
		!elementFormat->fontDescription.isNull()   )
	{
		th->content->as<TextElement>()->elementFormat->fontDescription->fontName;
	}

	textLine->specifiedWidth = width;
	textLine->updateSizes();
	textLine->width = textLine->textWidth;
	
	if (previousLine.isNull())
	{
		th->firstLine = textLine;
		if (th->lastLine.isNull())
			th->lastLine = textLine;
	}
	else
	{
		if (th->lastLine == previousLine)
			th->lastLine = textLine;
		previousLine->nextLine = textLine;
	}
	
	th->textLineCreationResult="success";
	textLine->incRef();
	ret = asAtomHandler::fromObject(textLine.getPtr());
}
ASFUNCTIONBODY_ATOM(TextBlock, recreateTextLine)
{
	TextBlock* th=asAtomHandler::as<TextBlock>(obj);
	_NR<TextLine> previousLine;
	_NR<TextLine> textLine;
	int32_t width;
	number_t lineOffset;
	bool fitSomething;
	ARG_CHECK(ARG_UNPACK (textLine) (previousLine, NullRef) (width, MAX_LINE_WIDTH) (lineOffset, 0.0) (fitSomething, false));

	if (lineOffset != 0.0)
		LOG(LOG_NOT_IMPLEMENTED, "TextBlock::recreateTextLine ignores parameter lineOffset");

	if (th->content.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}
	if (!th->content->is<TextElement>())
	{
		LOG(LOG_NOT_IMPLEMENTED,"TextBlock.recreateTextLine support for content other than TextElement not yet implemented:"<<th->content->toDebugString());
		asAtomHandler::setNull(ret);
		return;
	}
	if (asAtomHandler::isNull(th->content->as<TextElement>()->text))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	if (!fitSomething && ((width < 0) || (width > MAX_LINE_WIDTH)))
	{
		createError<ArgumentError>(wrk,kOutOfRangeError,"Invalid width");
		return;
	}

	if (textLine.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"textLine");
		return;
	}
	if (textLine == previousLine)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"previousLine");
		return;
	}
	if (!previousLine.isNull() && !previousLine->textBlock.isNull() && th != previousLine->textBlock.getPtr())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"previousLine");
		return;
	}
	if (!th->fillTextLine(textLine, fitSomething, previousLine))
	{
		// it's not in the specs but it seems that we have to return null if we are at the end of the content
		asAtomHandler::setNull(ret);
		th->textLineCreationResult="complete";
		return;
	}
	
	textLine->specifiedWidth = width;
	textLine->previousLine = previousLine;
	textLine->updateSizes();
	textLine->width = textLine->textWidth;
	th->incRef();
	textLine->textBlock= _MNR(th);
	if (width < (int)textLine->textWidth)
	{
		th->textLineCreationResult="insufficientWidth";
		asAtomHandler::setNull(ret);
		return;
	}
	if (previousLine.isNull())
	{
		th->firstLine = textLine;
		if (th->lastLine.isNull())
			th->lastLine = textLine;
	}
	else
	{
		if (th->lastLine == previousLine)
			th->lastLine = textLine;
		previousLine->nextLine = textLine;
	}

	textLine->incRef();
	textLine->validity = "valid";
	
	th->textLineCreationResult="success";
	ret = asAtomHandler::fromObject(textLine.getPtr());
}

ASFUNCTIONBODY_ATOM(TextBlock, releaseLines)
{
	TextBlock* th=asAtomHandler::as<TextBlock>(obj);
	_NR<TextLine> firstLine;
	_NR<TextLine> lastLine;
	ARG_CHECK(ARG_UNPACK (firstLine) (lastLine));

	// TODO handle non TextElement Content
	if (th->content.isNull() || !th->content->is<TextElement>())
		return;
	if (firstLine.isNull())
	{
		createError<ArgumentError>(wrk,kNullPointerError,"firstLine");
		return;
	}
	if (!firstLine->textBlock.isNull() && firstLine->textBlock != th)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"firstLine");
		return;
	}
	if (lastLine.isNull())
	{
		createError<ArgumentError>(wrk,kNullPointerError,"lastLine");
		return;
	}
	if (!lastLine->textBlock.isNull() && lastLine->textBlock != th)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"lastLine");
		return;
	}

	bool afterlast = false;
	_NR<TextLine> tmpLine;
	_NR<TextLine> tmpLine2;
	if (th->firstLine == firstLine)
		th->firstLine = lastLine->nextLine;
	if (th->lastLine == lastLine)
		th->lastLine = lastLine->nextLine;
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
			}
			firstLine->textBlock = NullRef;
			firstLine->nextLine = NullRef;
		}
		if (firstLine == lastLine)
			afterlast = true;
		firstLine = tmpLine2;
	}
}

void TextElement::settext_cb(asAtom /*oldValue*/)
{
	rawText = asAtomHandler::isNull(text) ? "" : asAtomHandler::toString(text,getInstanceWorker());
}

void TextElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ContentElement, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("replaceText","",c->getSystemState()->getBuiltinFunction(replaceText),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, text, ASString);
}
void TextElement::finalize()
{
	ContentElement::finalize();
	ASATOM_REMOVESTOREDMEMBER(text);
	text = asAtomHandler::nullAtom;
}

bool TextElement::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(text);
	text = asAtomHandler::nullAtom;
	return ContentElement::destruct();
}

void TextElement::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ContentElement::prepareShutdown();

	ASObject* o = asAtomHandler::getObject(text);
	if (o)
		o->prepareShutdown();
}

bool TextElement::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ContentElement::countCylicMemberReferences(gcstate);
	ASObject* o = asAtomHandler::getObject(text);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_GETTER_SETTER_CB(TextElement, text,settext_cb)

ASFUNCTIONBODY_ATOM(TextElement,_constructor)
{
	TextElement* th=asAtomHandler::as<TextElement>(obj);
	asAtom txtAtom = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	ARG_CHECK(ARG_UNPACK (txtAtom, asAtomHandler::nullAtom)(th->elementFormat, NullRef));
	th->text = txtAtom;
}
ASFUNCTIONBODY_ATOM(TextElement, replaceText)
{
	TextElement* th=asAtomHandler::as<TextElement>(obj);
	int beginIndex;
	int endIndex;
	asAtom newtext = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	ARG_CHECK(ARG_UNPACK (beginIndex)(endIndex)(newtext));
	
	tiny_string oldtext = asAtomHandler::isNull(th->text) ? "" : asAtomHandler::toString(th->text,wrk);
	if ((beginIndex < 0) || (endIndex < 0) || beginIndex > (int32_t)oldtext.numChars())
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	if (beginIndex > endIndex)
	{
		int tmp = endIndex;
		endIndex = beginIndex;
		beginIndex = tmp;
	}
	if (asAtomHandler::isNull(th->text) || asAtomHandler::isNull(newtext) )
	{
		ASATOM_ADDSTOREDMEMBER(newtext);
		ASATOM_REMOVESTOREDMEMBER(th->text);
		th->text = newtext;
		th->rawText = asAtomHandler::isNull(newtext) ? "" : asAtomHandler::toString(newtext,wrk);
	}
	else
	{
		tiny_string s;
		if ( beginIndex > 0)
			s = oldtext.substr(0,(uint32_t)beginIndex);
		s += asAtomHandler::toString(newtext,wrk);
		if (endIndex < (int32_t)oldtext.numChars())
			s += oldtext.substr(endIndex,oldtext.numChars()-endIndex);
		th->text = asAtomHandler::fromString(wrk->getSystemState(),s);
		th->rawText = s;
	}
}

void GroupElement::sinit(Class_base* c)
{
	CLASS_SETUP(c, ContentElement, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->isReusable=true;
}
void GroupElement::finalize()
{
	ContentElement::finalize();
}

bool GroupElement::destruct()
{
	return ContentElement::destruct();
}

void GroupElement::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ContentElement::prepareShutdown();
}

bool GroupElement::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ContentElement::countCylicMemberReferences(gcstate);
	return ret;
}

ASFUNCTIONBODY_ATOM(GroupElement,_constructor)
{
	//GroupElement* th=static_cast<GroupElement*>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "GroupElement constructor not implemented");
}

TextLine::TextLine(ASWorker* wrk, Class_base* c, _NR<TextBlock> owner)
  : DisplayObjectContainer(wrk,c), TextData(), TokenContainer(this),nextLine(nullptr),previousLine(nullptr),userData(nullptr)
  ,hasGraphicElement(false),hasTabs(false),rawTextLength(0),specifiedWidth(0),textBlockBeginIndex(0)
{
	subtype = SUBTYPE_TEXTLINE;
	textBlock = owner;
	fillstyleTextColor.push_back(0xff);
}

void TextLine::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObjectContainer, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("MAX_LINE_WIDTH",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)MAX_LINE_WIDTH),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("getBaselinePosition","",c->getSystemState()->getBuiltinFunction(getBaselinePosition,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("flushAtomData","",c->getSystemState()->getBuiltinFunction(flushAtomData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descent","",c->getSystemState()->getBuiltinFunction(getDescent,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ascent","",c->getSystemState()->getBuiltinFunction(getAscent,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",c->getSystemState()->getBuiltinFunction(getTextWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",c->getSystemState()->getBuiltinFunction(getTextHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("unjustifiedTextWidth","",c->getSystemState()->getBuiltinFunction(getUnjustifiedTextWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c, textBlock,TextBlock);
	REGISTER_GETTER_RESULTTYPE(c, nextLine,TextLine);
	REGISTER_GETTER_RESULTTYPE(c, previousLine,TextLine);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, validity,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, userData,ASObject);
	REGISTER_GETTER_RESULTTYPE(c, hasGraphicElement, Boolean);
	REGISTER_GETTER_RESULTTYPE(c, hasTabs, Boolean);
	REGISTER_GETTER_RESULTTYPE(c, rawTextLength, Integer);
	REGISTER_GETTER_RESULTTYPE(c, specifiedWidth, Number);
	REGISTER_GETTER_RESULTTYPE(c, textBlockBeginIndex, Integer);
}

void TextLine::finalize()
{
	textBlock.reset();
	nextLine.reset();
	previousLine.reset();
	userData.reset();
	DisplayObjectContainer::finalize();
}

ASFUNCTIONBODY_GETTER(TextLine, textBlock)
ASFUNCTIONBODY_GETTER(TextLine, nextLine)
ASFUNCTIONBODY_GETTER(TextLine, previousLine)
ASFUNCTIONBODY_GETTER_SETTER(TextLine, validity)
ASFUNCTIONBODY_GETTER_SETTER(TextLine, userData)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, hasGraphicElement)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, hasTabs)
ASFUNCTIONBODY_GETTER(TextLine, rawTextLength)
ASFUNCTIONBODY_GETTER(TextLine, specifiedWidth)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(TextLine, textBlockBeginIndex)

ASFUNCTIONBODY_ATOM(TextLine, getBaselinePosition)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextLine.getBaselinePosition");
	asAtomHandler::setInt(ret,wrk,0);
}

ASFUNCTIONBODY_ATOM(TextLine, flushAtomData)
{
	// According to specs this method does nothing
}

ASFUNCTIONBODY_ATOM(TextLine, getDescent)
{
	TextLine* th=asAtomHandler::as<TextLine>(obj);
	if (th->embeddedFont)
		asAtomHandler::setInt(ret,wrk,th->embeddedFont->getAscent());
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"TextLine.descent without embedded font");
		asAtomHandler::setInt(ret,wrk,0);
	}
}

ASFUNCTIONBODY_ATOM(TextLine, getAscent)
{
	TextLine* th=asAtomHandler::as<TextLine>(obj);
	if (th->embeddedFont)
		asAtomHandler::setInt(ret,wrk,th->embeddedFont->getAscent());
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"TextLine.ascent without embedded font");
		asAtomHandler::setInt(ret,wrk,th->textHeight);
	}
}

ASFUNCTIONBODY_ATOM(TextLine, getTextWidth)
{
	TextLine* th=asAtomHandler::as<TextLine>(obj);
	asAtomHandler::setInt(ret,wrk,th->textWidth);
}

ASFUNCTIONBODY_ATOM(TextLine, getTextHeight)
{
	TextLine* th=asAtomHandler::as<TextLine>(obj);
	asAtomHandler::setInt(ret,wrk,th->textHeight);
}

ASFUNCTIONBODY_ATOM(TextLine, getUnjustifiedTextWidth)
{
	TextLine* th=asAtomHandler::as<TextLine>(obj);
	asAtomHandler::setInt(ret,wrk,th->width);
}

void TextLine::updateSizes()
{
	number_t w,h;
	w = width;
	h = height;
	//Compute (text)width, (text)height
	getTextSizes(this->getText(), w, h);
	textWidth = w;
	textHeight = h;
}

string TextLine::toDebugString() const
{
	string res = DisplayObjectContainer::toDebugString();
	res += " (";
	res += this->validity;
	res += ")";
	if (textBlock)
	{
		char buf[100];
		sprintf(buf," ow:%p",textBlock.getPtr());
		res += buf;
	}
	res += " \"";
	res += this->getText();
	res += "\"";
	return res;
}

bool TextLine::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

void TextLine::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	DisplayObjectContainer::requestInvalidation(q,forceTextureRefresh);
	if (!tokensEmpty())
		TokenContainer::requestInvalidation(q,forceTextureRefresh);
	else
	{
		requestInvalidationFilterParent(q);
		incRef();
		q->addToInvalidateQueue(_MR(this));
	}
}

IDrawable* TextLine::invalidate(bool smoothing)
{
	number_t x,y;
	number_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax,false)==false)
	{
		//No contents, nothing to do
		return nullptr;
	}

	tokens.clear();
	if (embeddedFont)
	{
		scaling = 1.0f/1024.0f/20.0f;
		fillstyleTextColor.front().FillStyleType=SOLID_FILL;
		fillstyleTextColor.front().Color= RGBA(textColor.Red,textColor.Green,textColor.Blue,255);
		int32_t startposy = TEXTFIELD_PADDING;
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (isPassword)
			{
				tiny_string pwtxt;
				for (uint32_t i = 0; i < (*it).text.numChars(); i++)
					pwtxt+="*";
				embeddedFont->fillTextTokens(tokens,pwtxt,fontSize,fillstyleTextColor,leading,TEXTFIELD_PADDING+(*it).autosizeposition,startposy);
			}
			else
				embeddedFont->fillTextTokens(tokens,(*it).text,fontSize,fillstyleTextColor,leading,TEXTFIELD_PADDING+(*it).autosizeposition,startposy);
			startposy += this->leading+(embeddedFont->getAscent()+embeddedFont->getDescent()+embeddedFont->getLeading())*fontSize/1024;
		}
		if (tokens.empty())
			return nullptr;
		return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,false);
	}
	MATRIX matrix = getMatrix();
	bool isMask=this->isMask();
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);
	if (getLineCount()==0)
		return nullptr;
	if(width==0 || height==0)
		return nullptr;
	if(matrix.getScaleX() != 1 || matrix.getScaleY() != 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextField when scaled is not correctly implemented:"<<x<<"/"<<y<<" "<<width<<"x"<<height<<" "<<matrix.getScaleX()<<" "<<matrix.getScaleY()<<" "<<this->getText());
	float xscale = getConcatenatedMatrix().getScaleX();
	float yscale = getConcatenatedMatrix().getScaleY();
	// use specialized Renderer from EngineData, if available, otherwise fallback to Pango
	IDrawable* res = this->getSystemState()->getEngineData()->getTextRenderDrawable(*this,matrix, x, y, ceil(width), ceil(height),
																					xscale,yscale,isMask,cacheAsBitmap, 1.0f,getConcatenatedAlpha(),
																					ColorTransformBase(),
																					smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode());
	if (res != nullptr)
		return res;
	return new CairoPangoRenderer(*this,matrix,
				x, y, ceil(width), ceil(height),
				xscale,yscale,
				isMask, cacheAsBitmap,
				1.0f,getConcatenatedAlpha(),
				ColorTransformBase(),
				smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),0);
}

_NR<DisplayObject> TextLine::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	number_t xmin,xmax,ymin,ymax;
	// TODO: Add an overload for RECT.
	boundsRect(xmin,xmax,ymin,ymax,false);
	//TODO: Add a point intersect function to RECT, and use that instead.
	if( xmin <= localPoint.x && localPoint.x <= xmax && ymin <= localPoint.y && localPoint.y <= ymax)
	{
		incRef();
		return _MNR(this);
	}
	else
	{
		return DisplayObjectContainer::hitTestImpl(globalPoint, localPoint, type, interactiveObjectsOnly);
	}
}

void TabStop::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(TabStop,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "TabStop constructor not implemented");
}
void BreakOpportunity::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ALL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"all"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"any"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}
void CFFHinting::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("HORIZONTAL_STEM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"horizontalStem"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}
void DigitCase::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"default"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lining"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OLD_STYLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"oldStyle"),CONSTANT_TRAIT);
}
void DigitWidth::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"default"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PROPORTIONAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"proportional"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TABULAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"tabular"),CONSTANT_TRAIT);
}
void JustificationStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("PRIORITIZE_LEAST_ADJUSTMENT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"prioritizeLeastAdjustment"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PUSH_IN_KINSOKU",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pushInKinsoku"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PUSH_OUT_ONLY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pushOutOnly"),CONSTANT_TRAIT);
}
void LigatureLevel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("COMMON",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"common"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EXOTIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exotic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MINIMUM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"minimum"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNCOMMON",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"uncommon"),CONSTANT_TRAIT);
}
void RenderingMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CFF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"cff"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}
void TabAlignment::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CENTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"center"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DECIMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"decimal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("END",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"end"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("START",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"start"),CONSTANT_TRAIT);
}
void TextLineValidity::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("INVALID",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("POSSIBLY_INVALID",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"possiblyInvalid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STATIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"static"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VALID",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"valid"),CONSTANT_TRAIT);
}
void TextRotation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_0",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rotate0"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_180",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rotate180"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_270",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rotate270"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROTATE_90",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rotate90"),CONSTANT_TRAIT);
}
