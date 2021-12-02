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

#include "scripting/abc.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/ui/keycodes.h"
#include "scripting/class.h"
#include "compat.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "backends/rendering.h"
#include "backends/rendering_context.h"
#include "scripting/argconv.h"
#include <3rdparty/pugixml/src/pugixml.hpp>

using namespace std;
using namespace lightspark;

void AntiAliasType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ADVANCED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"advanced"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

void ASFont::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("enumerateFonts","",Class<IFunction>::getFunction(c->getSystemState(),enumerateFonts),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("registerFont","",Class<IFunction>::getFunction(c->getSystemState(),registerFont),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("hasGlyphs","",Class<IFunction>::getFunction(c->getSystemState(),hasGlyphs),NORMAL_METHOD,true);

	REGISTER_GETTER(c,fontName);
	REGISTER_GETTER(c,fontStyle);
	REGISTER_GETTER(c,fontType);
}
void ASFont::SetFont(tiny_string& fontname,bool is_bold,bool is_italic, bool is_Embedded, bool is_EmbeddedCFF)
{
	fontName = fontname;
	fontStyle = (is_bold ? 
					 (is_italic ? "boldItalic" : "bold") :
					 (is_italic ? "italic" : "regular")
					 );
	fontType = (is_Embedded ?
					(is_EmbeddedCFF ? "embeddedCFF" : "embedded") :
					"device");
}

std::vector<asAtom> *ASFont::getFontList()
{
	static std::vector<asAtom> fontlist;
	return &fontlist;
}
ASFUNCTIONBODY_GETTER(ASFont, fontName);
ASFUNCTIONBODY_GETTER(ASFont, fontStyle);
ASFUNCTIONBODY_GETTER(ASFont, fontType);

ASFUNCTIONBODY_ATOM(ASFont,enumerateFonts)
{
	bool enumerateDeviceFonts=false;
	ARG_UNPACK_ATOM(enumerateDeviceFonts,false);

	if (enumerateDeviceFonts)
		LOG(LOG_NOT_IMPLEMENTED,"Font::enumerateFonts: flag enumerateDeviceFonts is not handled");
	Array* res = Class<Array>::getInstanceSNoArgs(sys);
	std::vector<asAtom>* fontlist = getFontList();
	for(auto i = fontlist->begin(); i != fontlist->end(); ++i)
	{
		ASATOM_INCREF((*i));
		res->push(*i);
	}
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(ASFont,registerFont)
{
	_NR<ASObject> fontclass;
	ARG_UNPACK_ATOM(fontclass);
	if (!fontclass->is<Class_inherit>())
		throwError<ArgumentError>(kCheckTypeFailedError,
								  fontclass->getClassName(),
								  Class<ASFont>::getClass(sys)->getQualifiedClassName());
	ASFont* font = new (fontclass->as<Class_base>()->memoryAccount) ASFont(fontclass->as<Class_base>());
	fontclass->as<Class_base>()->setupDeclaredTraits(font);
	font->constructionComplete();
	font->setConstructIndicator();
	getFontList()->push_back(asAtomHandler::fromObject(font));
}
ASFUNCTIONBODY_ATOM(ASFont,hasGlyphs)
{
	ASFont* th = asAtomHandler::as<ASFont>(obj);
	tiny_string text;
	ARG_UNPACK_ATOM(text);
	if (th->fontType == "embedded")
	{
		FontTag* f = sys->mainClip->getEmbeddedFont(th->fontName);
		if (f)
		{
			asAtomHandler::setBool(ret,f->hasGlyphs(text));
			return;
		}
	}
	LOG(LOG_NOT_IMPLEMENTED,"Font.hasGlyphs always returns true for not embedded fonts:"<<text<<" "<<th->fontName<<" "<<th->fontStyle<<" "<<th->fontType);
	asAtomHandler::setBool(ret,true);
}
TextField::TextField(Class_base* c, const TextData& textData, bool _selectable, bool readOnly, const char *varname, DefineEditTextTag *_tag)
	: InteractiveObject(c), TextData(textData), TokenContainer(this), type(ET_READ_ONLY),
	  antiAliasType(AA_NORMAL), gridFitType(GF_PIXEL),
	  textInteractionMode(TI_NORMAL),autosizeposition(0),tagvarname(varname),tag(_tag),originalXPosition(0),originalWidth(textData.width),
	  fillstyleBackgroundColor(0xff),lineStyleBorder(0xff),lineStyleCaret(0xff),linemutex(new Mutex()),
	  alwaysShowSelection(false),
	  condenseWhite(false),
	  embedFonts(false), maxChars(_tag ? int32_t(_tag->MaxLength) : 0), mouseWheelEnabled(true),
	  selectable(_selectable), selectionBeginIndex(0), selectionEndIndex(0),
	  sharpness(0), thickness(0), useRichTextClipboard(false)
{
	subtype=SUBTYPE_TEXTFIELD;
	if (!readOnly)
	{
		type = ET_EDITABLE;
		tabEnabled = true;
	}
	fillstyleTextColor.push_back(0xff);
}

TextField::~TextField()
{
	delete linemutex;
	linemutex=nullptr;
}

void TextField::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, InteractiveObject, CLASS_SEALED);

	// methods
	c->setDeclaredMethodByQName("appendText","",Class<IFunction>::getFunction(c->getSystemState(),TextField:: appendText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTextFormat","",Class<IFunction>::getFunction(c->getSystemState(),_getTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextFormat","",Class<IFunction>::getFunction(c->getSystemState(),_setTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineIndexAtPoint","",Class<IFunction>::getFunction(c->getSystemState(),_getLineIndexAtPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineIndexOfChar","",Class<IFunction>::getFunction(c->getSystemState(),_getLineIndexOfChar),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineLength","",Class<IFunction>::getFunction(c->getSystemState(),_getLineLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineMetrics","",Class<IFunction>::getFunction(c->getSystemState(),_getLineMetrics),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineOffset","",Class<IFunction>::getFunction(c->getSystemState(),_getLineOffset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineText","",Class<IFunction>::getFunction(c->getSystemState(),_getLineText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replaceSelectedText","",Class<IFunction>::getFunction(c->getSystemState(),_replaceSelectedText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replaceText","",Class<IFunction>::getFunction(c->getSystemState(),_replaceText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSelection","",Class<IFunction>::getFunction(c->getSystemState(),_setSelection),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getCharBoundaries","",Class<IFunction>::getFunction(c->getSystemState(),_getCharBoundaries),NORMAL_METHOD,true);

	// properties
	c->setDeclaredMethodByQName("antiAliasType","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getAntiAliasType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("antiAliasType","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setAntiAliasType),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setAutoSize),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getAutoSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getDefaultTextFormat),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setDefaultTextFormat),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("gridFitType","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getGridFitType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("gridFitType","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setGridFitType),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getHtmlText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setHtmlText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getTextHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setWordWrap),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getWordWrap),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numLines","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getNumLines),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("maxScrollH","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getMaxScrollH),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("maxScrollV","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getMaxScrollV),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomScrollV","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getBottomScrollV),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("restrict","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getRestrict),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("restrict","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setRestrict),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textInteractionMode","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getTextInteractionMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("displayAsPassword","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_getdisplayAsPassword),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("displayAsPassword","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setdisplayAsPassword),SETTER_METHOD,true);

	// special handling neccessary when setting x
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),TextField::_setTextFieldX),SETTER_METHOD,true);

	REGISTER_GETTER_SETTER(c, alwaysShowSelection);
	REGISTER_GETTER_SETTER(c, background);
	REGISTER_GETTER_SETTER(c, backgroundColor);
	REGISTER_GETTER_SETTER(c, border);
	REGISTER_GETTER_SETTER(c, borderColor);
	REGISTER_GETTER(c, caretIndex);
	REGISTER_GETTER_SETTER(c, condenseWhite);
	REGISTER_GETTER_SETTER(c, embedFonts);
	REGISTER_GETTER_SETTER(c, maxChars);
	REGISTER_GETTER_SETTER(c, multiline);
	REGISTER_GETTER_SETTER(c, mouseWheelEnabled);
	REGISTER_GETTER_SETTER(c, scrollH);
	REGISTER_GETTER_SETTER(c, scrollV);
	REGISTER_GETTER_SETTER(c, selectable);
	REGISTER_GETTER(c, selectionBeginIndex);
	REGISTER_GETTER(c, selectionEndIndex);
	REGISTER_GETTER_SETTER(c, sharpness);
	REGISTER_GETTER_SETTER(c, styleSheet);
	REGISTER_GETTER_SETTER(c, textColor);
	REGISTER_GETTER_SETTER(c, thickness);
	REGISTER_GETTER_SETTER(c, type);
	REGISTER_GETTER_SETTER(c, useRichTextClipboard);
}

ASFUNCTIONBODY_GETTER_SETTER(TextField, alwaysShowSelection); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, background);
ASFUNCTIONBODY_GETTER_SETTER(TextField, backgroundColor);
ASFUNCTIONBODY_GETTER_SETTER(TextField, border);
ASFUNCTIONBODY_GETTER_SETTER(TextField, borderColor);
ASFUNCTIONBODY_GETTER(TextField, caretIndex);
ASFUNCTIONBODY_GETTER_SETTER(TextField, condenseWhite);
ASFUNCTIONBODY_GETTER_SETTER(TextField, embedFonts); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, maxChars); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, multiline);
ASFUNCTIONBODY_GETTER_SETTER(TextField, mouseWheelEnabled); // stub
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, scrollH, validateScrollH);
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, scrollV, validateScrollV);
ASFUNCTIONBODY_GETTER_SETTER(TextField, selectable);
ASFUNCTIONBODY_GETTER(TextField, selectionBeginIndex);
ASFUNCTIONBODY_GETTER(TextField, selectionEndIndex);
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, sharpness, validateSharpness); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, styleSheet); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, textColor);
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, thickness, validateThickness); // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, useRichTextClipboard); // stub

void TextField::finalize()
{
	InteractiveObject::finalize();
	restrictChars.reset();
	styleSheet.reset();
}

void TextField::buildTraits(ASObject* o)
{
}

bool TextField::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if (this->type == ET_EDITABLE && tag)
	{
		xmin=tag->Bounds.Xmin/20.0f;
		xmax=tag->Bounds.Xmax/20.0f;
		ymin=tag->Bounds.Ymin/20.0f;
		ymax=tag->Bounds.Ymax/20.0f;
		return true;
	}
	if ((!this->legacy || (tag==nullptr) || autoSize!=AS_NONE))
	{
		xmin=tag ? tag->Bounds.Xmin/20.0f : 0.0f;
		xmax=max(0.0f,float(textWidth+autosizeposition))+2*TEXTFIELD_PADDING+ (tag ? tag->Bounds.Xmin/20.0f : 0.0f);
		ymin=tag ? tag->Bounds.Ymin/20.0f : 0.0f;
		ymax=max(0.0f,float(height)+(tag ? tag->Bounds.Ymin/20.0f :0.0f));
		return true;
	}
	xmin=tag->Bounds.Xmin/20.0f;
	xmax=max(0.0f,float(textWidth)+tag->Bounds.Xmin/20.0f);
	ymin=tag->Bounds.Ymin/20.0f;
	ymax=max(0.0f,float(height)+tag->Bounds.Ymin/20.0f);
	return true;
}

_NR<DisplayObject> TextField::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	/* I suppose one does not have to actually hit a character */
	number_t xmin,xmax,ymin,ymax;
	boundsRect(xmin,xmax,ymin,ymax);
	if( xmin <= x && x <= xmax && ymin <= y && y <= ymax && isHittable(type))
	{
		if (!selectable)
			return last;
		incRef();
		return _MNR(this);
	}
	else
		return NullRef;
}

ASFUNCTIONBODY_ATOM(TextField,_getdisplayAsPassword)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setBool(ret,th->isPassword);
}

ASFUNCTIONBODY_ATOM(TextField,_setdisplayAsPassword)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ARG_UNPACK_ATOM(th->isPassword);
	th->setSizeAndPositionFromAutoSize();
	th->hasChanged=true;
	th->setNeedsTextureRecalculation();
	if(th->onStage && th->isVisible())
		th->requestInvalidation(th->getSystemState());
}

ASFUNCTIONBODY_ATOM(TextField,_getWordWrap)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setBool(ret,th->wordWrap);
}

ASFUNCTIONBODY_ATOM(TextField,_setWordWrap)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ARG_UNPACK_ATOM(th->wordWrap);
	th->setSizeAndPositionFromAutoSize();
	th->hasChanged=true;
	th->setNeedsTextureRecalculation();
	if(th->onStage && th->isVisible())
		th->requestInvalidation(th->getSystemState());
}

ASFUNCTIONBODY_ATOM(TextField,_getAutoSize)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	switch(th->autoSize)
	{
		case AS_NONE:
			ret = asAtomHandler::fromString(sys,"none");
			return;
		case AS_LEFT:
			ret = asAtomHandler::fromString(sys,"left");
			return;
		case AS_RIGHT:
			ret =asAtomHandler::fromString(sys,"right");
			return;
		case AS_CENTER:
			ret = asAtomHandler::fromString(sys,"center");
			return;
	}
}

ASFUNCTIONBODY_ATOM(TextField,_setAutoSize)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string autoSizeString;
	ARG_UNPACK_ATOM(autoSizeString);

	AUTO_SIZE newAutoSize = AS_NONE;
	if(autoSizeString == "none" || (!th->loadedFrom->usesActionScript3 && autoSizeString=="false"))
		newAutoSize = AS_NONE;
	else if (autoSizeString == "left" || (!th->loadedFrom->usesActionScript3 && autoSizeString=="true"))
		newAutoSize = AS_LEFT;
	else if (autoSizeString == "right")
		newAutoSize = AS_RIGHT;
	else if (autoSizeString == "center")
		newAutoSize = AS_CENTER;
	else
		throwError<ArgumentError>(kInvalidEnumError, "autoSize");

	if (th->autoSize != newAutoSize)
	{
		th->autoSize = newAutoSize;
		th->setSizeAndPositionFromAutoSize();
		th->hasChanged=true;
		th->setNeedsTextureRecalculation();
		if(th->onStage && th->isVisible())
			th->requestInvalidation(th->getSystemState());
	}
}
void TextField::setSizeAndPositionFromAutoSize(bool updatewidth)
{
	if (autoSize == AS_NONE)
	{
		if (updatewidth && !wordWrap)
			width = originalWidth;
		return;
	}

	switch (autoSize)
	{
		case AS_RIGHT:
			linemutex->lock();
			autosizeposition = max(0.0f,float(width-TEXTFIELD_PADDING*2)-textWidth);
			for (auto it = textlines.begin(); it != textlines.end(); it++)
			{
				if ((*it).textwidth< textWidth)
					(*it).autosizeposition = (textWidth-(*it).textwidth);
				else
					(*it).autosizeposition = 0;
			}
			linemutex->unlock();
			break;
		case AS_CENTER:
			autosizeposition = 0;
			if (!wordWrap) // not in the specs but Adobe changes x position if wordWrap is not set
			{
				this->setX(originalXPosition + (int(originalWidth - textWidth))/2*sx);
				if (updatewidth)
					width = textWidth+TEXTFIELD_PADDING*2;
			}
			else
				autosizeposition = (int32_t((width-TEXTFIELD_PADDING*2) - textWidth)/2);
			linemutex->lock();
			for (auto it = textlines.begin(); it != textlines.end(); it++)
			{
				if ((*it).textwidth< textWidth)
					(*it).autosizeposition = (textWidth-(*it).textwidth)/2;
				else
					(*it).autosizeposition = 0;
			}
			linemutex->unlock();
			break;
		default:
			autosizeposition = 0;
			if (updatewidth && !wordWrap)
				width = textWidth+TEXTFIELD_PADDING*2;
			break;
	}
	height = textHeight+TEXTFIELD_PADDING*2;
}

ASFUNCTIONBODY_ATOM(TextField,_getWidth)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,sys,th->width);
}

ASFUNCTIONBODY_ATOM(TextField,_setWidth)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	if((th->width != asAtomHandler::toUInt(args[0]))
			&& (th->width != asAtomHandler::toUInt(args[0]))
			&&  (asAtomHandler::toInt(args[0]) >= 0))
	{
		th->width=asAtomHandler::toUInt(args[0]);
		th->originalWidth=th->width;
		th->hasChanged=true;
		th->setNeedsTextureRecalculation();
		th->updateSizes();
		th->setSizeAndPositionFromAutoSize(false);
		th->width -= TEXTFIELD_PADDING*2;
		if(th->onStage && th->isVisible())
			th->requestInvalidation(sys);
		th->legacy=false;
	}
}

ASFUNCTIONBODY_ATOM(TextField,_getHeight)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	// it seems that Adobe returns the textHeight if in autoSize mode
	if (th->autoSize != AS_NONE || th->wordWrap)
		asAtomHandler::setUInt(ret,sys,th->textHeight);
	else
		asAtomHandler::setUInt(ret,sys,th->height);
}

ASFUNCTIONBODY_ATOM(TextField,_setHeight)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	if((th->autoSize == AS_NONE)
		&& (th->height != asAtomHandler::toUInt(args[0]))
			&&  (asAtomHandler::toInt(args[0]) >= 0))
	{
		th->height=asAtomHandler::toUInt(args[0]);
		th->hasChanged=true;
		th->setNeedsTextureRecalculation();
		th->updateSizes();
		th->setSizeAndPositionFromAutoSize();
		if(th->onStage && th->isVisible())
			th->requestInvalidation(th->getSystemState());
		th->legacy=false;
	}
	//else do nothing as the height is determined by autoSize
}

ASFUNCTIONBODY_ATOM(TextField,_setTextFieldX)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	DisplayObject::_setX(ret,sys,obj,args,argslen);
	th->originalXPosition=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(TextField,_getTextWidth)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,sys,th->textWidth);
}

ASFUNCTIONBODY_ATOM(TextField,_getTextHeight)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,sys,th->textHeight);
}

ASFUNCTIONBODY_ATOM(TextField,_getHtmlText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->toHtmlText()));
}

ASFUNCTIONBODY_ATOM(TextField,_setHtmlText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_UNPACK_ATOM(value);
	th->setHtmlText(value);
	th->legacy=false;
}

ASFUNCTIONBODY_ATOM(TextField,_getText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->getText()));
}

ASFUNCTIONBODY_ATOM(TextField,_setText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	th->updateText(asAtomHandler::toString(args[0],sys));
	th->legacy=false;
}

ASFUNCTIONBODY_ATOM(TextField, appendText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	th->updateText(th->getText() + asAtomHandler::toString(args[0],sys));
}

ASFUNCTIONBODY_ATOM(TextField,_getTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	TextFormat *format=Class<TextFormat>::getInstanceS(sys);

	format->color= asAtomHandler::fromUInt(th->textColor.toUInt());
	format->font = th->font;
	format->size = asAtomHandler::fromUInt(th->fontSize);

	LOG(LOG_NOT_IMPLEMENTED, "getTextFormat is not fully implemeted");

	ret = asAtomHandler::fromObject(format);
}

ASFUNCTIONBODY_ATOM(TextField,_setTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	_NR<TextFormat> tf;
	int beginIndex;
	int endIndex;

	if (th->loadedFrom->usesActionScript3 || argslen == 1)
		ARG_UNPACK_ATOM(tf)(beginIndex, -1)(endIndex, -1);
	else
	{
		if (argslen == 2)
			ARG_UNPACK_ATOM(beginIndex)(tf)(endIndex, -1);
		else
			ARG_UNPACK_ATOM(beginIndex)(endIndex)(tf);
	}

	if(beginIndex!=-1 || endIndex!=-1)
		LOG(LOG_NOT_IMPLEMENTED,"setTextFormat with beginIndex or endIndex");
	if (tf.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"setTextFormat with textformat null");
		return;
	}
	if(!asAtomHandler::isNull(tf->color))
		th->textColor = asAtomHandler::toUInt(tf->color);
	bool updatesizes = false;
	if (tf->font != "")
	{
		if (tf->font != th->font)
			updatesizes = true;
		th->font = tf->font;
		th->fontID = UINT32_MAX;
	}
	if (!asAtomHandler::isNull(tf->size) && th->fontSize != asAtomHandler::toUInt(tf->size))
	{
		th->fontSize = asAtomHandler::toUInt(tf->size);
		updatesizes = true;
	}
	if (updatesizes)
	{
		th->updateSizes();
		th->setSizeAndPositionFromAutoSize();
		th->hasChanged=true;
		th->setNeedsTextureRecalculation();
	}

	LOG(LOG_NOT_IMPLEMENTED,"setTextFormat does not read all fields of TextFormat");
}

ASFUNCTIONBODY_ATOM(TextField,_getDefaultTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	
	TextFormat* tf = Class<TextFormat>::getInstanceS(sys);
	tf->font = th->font;
	tf->bold = th->isBold ? asAtomHandler::trueAtom : asAtomHandler::nullAtom;
	tf->italic = th->isItalic ? asAtomHandler::trueAtom : asAtomHandler::nullAtom;
	LOG(LOG_NOT_IMPLEMENTED,"getDefaultTextFormat does not get all fields of TextFormat");
	ret = asAtomHandler::fromObject(tf);
}

ASFUNCTIONBODY_ATOM(TextField,_setDefaultTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	_NR<TextFormat> tf;

	ARG_UNPACK_ATOM(tf);

	if(!asAtomHandler::isNull(tf->color))
		th->textColor = asAtomHandler::toUInt(tf->color);
	if (tf->font != "")
	{
		th->font = tf->font;
		th->fontID = UINT32_MAX;
	}
	if (!asAtomHandler::isNull(tf->size))
		th->fontSize = asAtomHandler::toUInt(tf->size);
	AUTO_SIZE newAutoSize = th->autoSize;
	if(tf->align == "none")
		newAutoSize = AS_NONE;
	else if (tf->align == "left")
		newAutoSize = AS_LEFT;
	else if (tf->align == "right")
		newAutoSize = AS_RIGHT;
	else if (tf->align == "center")
		newAutoSize = AS_CENTER;
	th->isBold=asAtomHandler::toInt(tf->bold);
	th->isItalic=asAtomHandler::toInt(tf->italic);
	if (th->autoSize != newAutoSize)
	{
		th->autoSize = newAutoSize;
		th->setSizeAndPositionFromAutoSize();
	}
	LOG(LOG_NOT_IMPLEMENTED,"setDefaultTextFormat does not set all fields of TextFormat");
}

ASFUNCTIONBODY_ATOM(TextField, _getter_type)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->type == ET_READ_ONLY)
		ret = asAtomHandler::fromString(sys,"dynamic");
	else
		ret = asAtomHandler::fromString(sys,"input");
}

ASFUNCTIONBODY_ATOM(TextField, _setter_type)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	tiny_string value;
	ARG_UNPACK_ATOM(value);

	if (value == "dynamic")
		th->type = ET_READ_ONLY;
	else if (value == "input")
		th->type = ET_EDITABLE;
	else
		throwError<ArgumentError>(kInvalidEnumError, "type");
}

ASFUNCTIONBODY_ATOM(TextField,_getLineIndexAtPoint)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	number_t x;
	number_t y;
	ARG_UNPACK_ATOM(x) (y);

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	std::vector<LineData>::const_iterator it;
	int i;
	for (i=0, it=lines.begin(); it!=lines.end(); ++i, ++it)
	{
		if (x > it->extents.Xmin && x <= it->extents.Xmax &&
		    y > it->extents.Ymin && y <= it->extents.Ymax)
		{
			asAtomHandler::setInt(ret,sys,i);
			return;
		}
	}

	asAtomHandler::setInt(ret,sys,-1);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineIndexOfChar)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t charIndex;
	ARG_UNPACK_ATOM(charIndex);

	if (charIndex < 0)
	{
		asAtomHandler::setInt(ret,sys,-1);
		return;
	}

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	std::vector<LineData>::const_iterator it;
	int i;
	for (i=0, it=lines.begin(); it!=lines.end(); ++i, ++it)
	{
		if (charIndex >= it->firstCharOffset &&
		    charIndex < it->firstCharOffset + it->length)
		{
			asAtomHandler::setInt(ret,sys,i);
			return;
		}
	}

	// testing shows that returns -1 on invalid index instead of
	// throwing RangeError
	asAtomHandler::setInt(ret,sys,-1);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineLength)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_UNPACK_ATOM(lineIndex);

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	if (lineIndex < 0 || lineIndex >= (int32_t)lines.size())
		throwError<RangeError>(kParamRangeError);

	asAtomHandler::setInt(ret,sys,lines[lineIndex].length);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineMetrics)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_UNPACK_ATOM(lineIndex);

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	if (lineIndex < 0 || lineIndex >= (int32_t)lines.size())
		throwError<RangeError>(kParamRangeError);

	ret = asAtomHandler::fromObject(Class<TextLineMetrics>::getInstanceS(sys,
		lines[lineIndex].indent,
		lines[lineIndex].extents.Xmax - lines[lineIndex].extents.Xmin,
		lines[lineIndex].extents.Ymax - lines[lineIndex].extents.Ymin,
		lines[lineIndex].ascent,
		lines[lineIndex].descent,
		lines[lineIndex].leading));
}

ASFUNCTIONBODY_ATOM(TextField,_getLineOffset)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_UNPACK_ATOM(lineIndex);

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	if (lineIndex < 0 || lineIndex >= (int32_t)lines.size())
		throwError<RangeError>(kParamRangeError);

	asAtomHandler::setInt(ret,sys,lines[lineIndex].firstCharOffset);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_UNPACK_ATOM(lineIndex);

	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	if (lineIndex < 0 || lineIndex >= (int32_t)lines.size())
		throwError<RangeError>(kParamRangeError);

	tiny_string substr = th->getText().substr(lines[lineIndex].firstCharOffset,
					     lines[lineIndex].length);
	ret = asAtomHandler::fromObject(abstract_s(sys,substr));
}

ASFUNCTIONBODY_ATOM(TextField,_getAntiAliasType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->antiAliasType == AA_NORMAL)
		ret = asAtomHandler::fromString(sys,"normal");
	else
		ret = asAtomHandler::fromString(sys,"advanced");
}

ASFUNCTIONBODY_ATOM(TextField,_setAntiAliasType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_UNPACK_ATOM(value);

	if (value == "advanced")
	{
		th->antiAliasType = AA_ADVANCED;
		LOG(LOG_NOT_IMPLEMENTED, "TextField advanced antiAliasType not implemented");
	}
	else
		th->antiAliasType = AA_NORMAL;
}

ASFUNCTIONBODY_ATOM(TextField,_getGridFitType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->gridFitType == GF_NONE)
		ret = asAtomHandler::fromString(sys,"none");
	else if (th->gridFitType == GF_PIXEL)
		ret = asAtomHandler::fromString(sys,"pixel");
	else
		ret = asAtomHandler::fromString(sys,"subpixel");
}

ASFUNCTIONBODY_ATOM(TextField,_setGridFitType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_UNPACK_ATOM(value);

	if (value == "none")
		th->gridFitType = GF_NONE;
	else if (value == "pixel")
		th->gridFitType = GF_PIXEL;
	else
		th->gridFitType = GF_SUBPIXEL;

	LOG(LOG_NOT_IMPLEMENTED, "TextField gridFitType not implemented");

}

ASFUNCTIONBODY_ATOM(TextField,_getLength)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,sys,th->getText().numChars());
}

ASFUNCTIONBODY_ATOM(TextField,_getNumLines)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setInt(ret,sys,(int32_t)CairoPangoRenderer::getLineData(*th).size());
}

ASFUNCTIONBODY_ATOM(TextField,_getMaxScrollH)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setInt(ret,sys,th->getMaxScrollH());
}

ASFUNCTIONBODY_ATOM(TextField,_getMaxScrollV)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setInt(ret,sys,th->getMaxScrollV());
}

ASFUNCTIONBODY_ATOM(TextField,_getBottomScrollV)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	for (unsigned int k=0; k<lines.size()-1; k++)
	{
		if (lines[k+1].extents.Ymin >= (int)th->height)
		{
			asAtomHandler::setInt(ret,sys,(int32_t)k + 1);
			return;
		}
	}

	asAtomHandler::setInt(ret,sys,(int32_t)lines.size() + 1);
}

ASFUNCTIONBODY_ATOM(TextField,_getRestrict)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->restrictChars.isNull())
		return;
	else
	{
		th->restrictChars->incRef();
		ret = asAtomHandler::fromObject(th->restrictChars.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(TextField,_setRestrict)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ARG_UNPACK_ATOM(th->restrictChars);
	if (!th->restrictChars.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "TextField restrict property");
}

ASFUNCTIONBODY_ATOM(TextField,_getTextInteractionMode)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->textInteractionMode == TI_NORMAL)
		ret = asAtomHandler::fromString(sys,"normal");
	else
		ret = asAtomHandler::fromString(sys,"selection");
}

ASFUNCTIONBODY_ATOM(TextField,_setSelection)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ARG_UNPACK_ATOM(th->selectionBeginIndex) (th->selectionEndIndex);

	if (th->selectionBeginIndex < 0)
		th->selectionBeginIndex = 0;

	tiny_string text = th->getText();
	if (th->selectionEndIndex >= (int32_t)text.numChars())
		th->selectionEndIndex = text.numChars()-1;

	if (th->selectionBeginIndex > th->selectionEndIndex)
		th->selectionBeginIndex = th->selectionEndIndex;

	if (th->selectionBeginIndex == th->selectionEndIndex)
		th->caretIndex = th->selectionBeginIndex;

	LOG(LOG_NOT_IMPLEMENTED, "TextField selection will not be rendered");
}

ASFUNCTIONBODY_ATOM(TextField,_replaceSelectedText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string newText;
	ARG_UNPACK_ATOM(newText);
	th->replaceText(th->selectionBeginIndex, th->selectionEndIndex, newText);
}

ASFUNCTIONBODY_ATOM(TextField,_replaceText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t begin;
	int32_t end;
	tiny_string newText;
	ARG_UNPACK_ATOM(begin) (end) (newText);
	th->replaceText(begin, end, newText);
}

void TextField::replaceText(unsigned int begin, unsigned int end, const tiny_string& newText)
{
	if (!styleSheet.isNull())
		throw Class<ASError>::getInstanceS(getSystemState(),"Can not replace text on text field with a style sheet");

	tiny_string text = getText();
	if (begin >= text.numChars())
	{
		text = text + newText;
	}
	else if (begin > end)
	{
		return;
	}
	else if (end >= text.numChars())
	{
		text = text.substr(0, begin) + newText;
	}
	else
	{
		text = text.substr(0, begin) + newText + text.substr(end, text.end());
	}
	setText(text.raw_buf());
	textUpdated();
}

void TextField::getTextBounds(const tiny_string& txt,number_t &xmin,number_t &xmax,number_t &ymin,number_t &ymax)
{
	FontTag* embeddedfont = (fontID != UINT32_MAX ? loadedFrom->getEmbeddedFontByID(fontID) : loadedFrom->getEmbeddedFont(font));
	if (embeddedfont && embeddedfont->hasGlyphs(getText()))
	{
		scaling = 1.0f/1024.0f/20.0f;
		embeddedfont->getTextBounds(txt,fontSize,xmax,ymax);
		xmin = autosizeposition;
		xmax += autosizeposition;
		ymin=0;
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"TextFields: computing of textbounds not implemented for non-embedded fonts");
}

ASFUNCTIONBODY_ATOM(TextField,_getCharBoundaries)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	int32_t charIndex;
	ARG_UNPACK_ATOM(charIndex);

	Rectangle* rect = Class<Rectangle>::getInstanceSNoArgs(sys);
	tiny_string text = th->getText();
	if (charIndex >= 0 && charIndex < (int32_t)text.numChars())
	{
		number_t xmin=0,xmax=0,ymin=0,ymax=0;
		if (charIndex > 0)
			th->getTextBounds(text.substr(0,charIndex-1),xmin,xmax,ymin,ymax);
		number_t xmin2=0,xmax2=0,ymin2=0,ymax2=0;
		th->getTextBounds(text.substr(0,charIndex),xmin2,xmax2,ymin2,ymax2);
		rect->x = xmin;
		rect->y = ymin2;
		rect->width = xmax2-xmax;
		rect->height = ymax2-ymin2;
	}
	ret = asAtomHandler::fromObjectNoPrimitive(rect);
}

void TextField::afterSetLegacyMatrix()
{
	originalXPosition = getXY().x;
	originalWidth = width;
	textUpdated();
}

void TextField::validateThickness(number_t /*oldValue*/)
{
	thickness = dmin(dmax(thickness, -200.), 200.);
}

void TextField::validateSharpness(number_t /*oldValue*/)
{
	sharpness = dmin(dmax(sharpness, -400.), 400.);
}

void TextField::validateScrollH(int32_t oldValue)
{
	int32_t maxScrollH = getMaxScrollH();
	if (scrollH > maxScrollH)
		scrollH = maxScrollH;
	hasChanged=true;
	setNeedsTextureRecalculation();

	if (onStage && (scrollH != oldValue) && isVisible())
		requestInvalidation(this->getSystemState());
}

void TextField::validateScrollV(int32_t oldValue)
{
	int32_t maxScrollV = getMaxScrollV();
	if (scrollV < 1)
		scrollV = 1;
	else if (scrollV > maxScrollV)
		scrollV = maxScrollV;
	hasChanged=true;
	setNeedsTextureRecalculation();

	if (onStage && (scrollV != oldValue) && isVisible())
		requestInvalidation(this->getSystemState());
}

int32_t TextField::getMaxScrollH()
{
	if (wordWrap || (textWidth <= width))
		return 0;
	else
		return textWidth;
}

int32_t TextField::getMaxScrollV()
{
	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*this);
	if (lines.size() <= 1)
		return 1;

	int32_t Ymax = lines[lines.size()-1].extents.Ymax;
	int32_t measuredTextHeight = Ymax - lines[0].extents.Ymin;
	if (measuredTextHeight <= (int32_t)height)
		return 1;

	// one full page from the bottom
	for (int k=(int)lines.size()-1; k>=0; k--)
	{
		if (Ymax - lines[k].extents.Ymin > (int32_t)height)
		{
			return imin(k+1+1, lines.size());
		}
	}

	return 1;
}

void TextField::updateSizes()
{
	Locker l(invalidatemutex);
	uint32_t tw,th;
	tw = textWidth;
	th = textHeight;
	
	RootMovieClip* currentRoot=this->loadedFrom;
	if (!currentRoot) currentRoot=this->getRoot().getPtr();
	if (!currentRoot) currentRoot = getSystemState()->mainClip;
	FontTag* embeddedfont = (fontID != UINT32_MAX ? currentRoot->getEmbeddedFontByID(fontID) : currentRoot->getEmbeddedFont(font));
	if (embeddedfont && embeddedfont->hasGlyphs(getText()))
	{
		scaling = 1.0f/1024.0f/20.0f;
		th=0;
		number_t w,h;
		linemutex->lock();
		auto it = textlines.begin();
		while (it != textlines.end())
		{
			embeddedfont->getTextBounds((*it).text,fontSize,w,h);
			(*it).textwidth=w;
			if (!wordWrap && w>tw)
				tw = w;
			bool listchanged=false;
			if (wordWrap && width > TEXTFIELD_PADDING*2 && uint32_t(w) > width-TEXTFIELD_PADDING*2)
			{
				// calculate lines for wordwrap
				tiny_string text =(*it).text;
				uint32_t c= text.rfind(" ");// TODO check for other whitespace characters
				while (c != tiny_string::npos && c != 0)
				{
					embeddedfont->getTextBounds(text.substr(0,c),fontSize,w,h);
					if (w <= width-TEXTFIELD_PADDING*2)
					{
						if(w>tw)
							tw = w;
						(*it).textwidth=w;
						(*it).text = text.substr(0,c);
						textline t;
						t.autosizeposition=0;
						t.text=text.substr(c+1,UINT32_MAX);
						embeddedfont->getTextBounds(t.text,fontSize,w,h);
						t.textwidth=w;
						it = textlines.insert(++it,t);
						listchanged=true;
						text =t.text;
						if (uint32_t(w) <= width-TEXTFIELD_PADDING*2)
						{
							if(w>tw)
								tw = w;
							break;
						}
						c=text.numChars();
					}
					c= text.rfind(" ",c-1);// TODO check for other whitespace characters
				}
			}
			if (!listchanged)
				it++;
			th+=h;
			if (it != textlines.end())
				th+=this->leading;
		}
		linemutex->unlock();
		if(w>tw)
			tw = w;
	}
	else
	{
		uint32_t w,h;
		CairoPangoRenderer::getBounds(*this, w, h, tw, th);
	}
	textWidth=tw;
	textHeight=th;
}

tiny_string TextField::toHtmlText()
{
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("font");

	ostringstream ss;
	ss << fontSize;
	root.append_attribute("size").set_value(ss.str().c_str());
	root.append_attribute("color").set_value(textColor.toString().raw_buf());
	root.append_attribute("face").set_value(font.raw_buf());

	//Split text into paragraphs and wraps them into <p> tags
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		root.append_child("p").set_value((*it).text.raw_buf());
	}

	ostringstream buf;
	doc.print(buf);
	tiny_string ret = tiny_string(buf.str());
	return ret;
}

void TextField::setHtmlText(const tiny_string& html)
{
	HtmlTextParser parser;
	if (condenseWhite)
	{
		tiny_string processedHtml = compactHTMLWhiteSpace(html);
		parser.parseTextAndFormating(processedHtml, this);
	}
	else
	{
		parser.parseTextAndFormating(html, this);
	}
	if (this->isConstructed())
	{
		hasChanged=true;
		setNeedsTextureRecalculation();
		textUpdated();
	}
}

std::string TextField::toDebugString()
{
	std::string res = InteractiveObject::toDebugString();
	res += " \"";
	res += this->getText(0);
	res += "\";";
	char buf[100];
	sprintf(buf,"%dx%d %5.2f",textWidth,textHeight,autosizeposition);
	res += buf;
	return res;
}

tiny_string TextField::compactHTMLWhiteSpace(const tiny_string& html)
{
	tiny_string compacted;
	bool previousWasSpace = false;
	for (CharIterator ch=html.begin(); ch!=html.end(); ++ch)
	{
		if (g_unichar_isspace(*ch))
		{
			if (!previousWasSpace)
				compacted += ' ';
			previousWasSpace = true;
		}
		else
		{
			compacted += *ch;
			previousWasSpace = false;
		}
	}

	return compacted;
}

void TextField::updateText(const tiny_string& new_text)
{
	if (getText() == new_text)
		return;
	setText(new_text.raw_buf());
	textUpdated();
}

void TextField::avm1SyncTagVar()
{
	if (!tagvarname.empty()
		&& tagvarname != "_url") // "_url" is readonly and always read from root movie, no need to update
	{
		DisplayObject* par = getParent();
		while (par)
		{
			if (par->is<MovieClip>())
			{
				asAtom value=asAtomHandler::invalidAtom;
				number_t n;
				if (Integer::fromStringFlashCompatible(getText().raw_buf(),n,10,true))
					value = asAtomHandler::fromNumber(getSystemState(),n,false);
				else
					value = asAtomHandler::fromString(getSystemState(),getText());
				par->as<MovieClip>()->AVM1SetVariable(tagvarname,value);
				break;
			}
			par = par->getParent();
		}
	}
}

void TextField::UpdateVariableBinding(asAtom v)
{
	updateText(asAtomHandler::toString(v,getSystemState()));
}

void TextField::afterLegacyInsert()
{
	if (!tagvarname.empty())
	{
		DisplayObject* par = getParent();
		while (par)
		{
			if (par->is<MovieClip>())
			{
				par->as<MovieClip>()->setVariableBinding(tagvarname,_MR(this));
				asAtom value = par->as<MovieClip>()->getVariableBindingValue(tagvarname);
				if (asAtomHandler::isValid(value) && !asAtomHandler::isUndefined(value))
					this->setText(asAtomHandler::toString(value,getSystemState()).raw_buf());
				break;
			}
			par = par->getParent();
		}
	}
	avm1SyncTagVar();
	updateSizes();
	setSizeAndPositionFromAutoSize();
}

void TextField::afterLegacyDelete(DisplayObjectContainer *par)
{
	if (!tagvarname.empty())
	{
		while (par)
		{
			if (par->is<MovieClip>())
			{
				par->as<MovieClip>()->AVM1SetVariable(tagvarname,asAtomHandler::undefinedAtom);
				par->as<MovieClip>()->setVariableBinding(tagvarname,NullRef);
				break;
			}
			par = par->getParent();
		}
	}
}

void TextField::lostFocus()
{
	SDL_StopTextInput();
	getSystemState()->removeJob(this);
	caretblinkstate = false;
	hasChanged=true;
	setNeedsTextureRecalculation();
	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
	invalidateCachedAsBitmapOf();
}

void TextField::gotFocus()
{
	if (this->type != ET_EDITABLE)
		return;
	SDL_StartTextInput();
	selectionBeginIndex = getText().numChars();
	selectionEndIndex = selectionBeginIndex;
	caretIndex=selectionBeginIndex;
	getSystemState()->addTick(500,this);
}

void TextField::textInputChanged(const tiny_string &newtext)
{
	if (this->type != ET_EDITABLE)
		return;
	tiny_string tmptext = getText();
	if (maxChars == 0 || tmptext.numChars()+newtext.numChars() <= uint32_t(maxChars))
	{
		tmptext.replace(caretIndex,0,newtext);
		caretIndex+= newtext.numChars();
	}
	this->updateText(tmptext);
}

void TextField::tick()
{
	if (this->type != ET_EDITABLE)
		return;
	if (this == getSystemState()->stage->getFocusTarget().getPtr())
		caretblinkstate = !caretblinkstate;
	else
		caretblinkstate = false;
	hasChanged=true;
	setNeedsTextureRecalculation();
	
	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
	invalidateCachedAsBitmapOf();
}

void TextField::tickFence()
{
}

uint32_t TextField::getTagID() const
{
	return tag ? tag->getId() : UINT32_MAX;
}

void TextField::textUpdated()
{
	avm1SyncTagVar();
	scrollH = 0;
	scrollV = 1;
	selectionBeginIndex = 0;
	selectionEndIndex = 0;
	updateSizes();
	setSizeAndPositionFromAutoSize();
	FontTag* embeddedfont = (fontID != UINT32_MAX ? this->loadedFrom->getEmbeddedFontByID(fontID) : this->loadedFrom->getEmbeddedFont(font));
	// TODO implement fast rendering path for not embedded fonts
	if (computeCacheAsBitmap() || !embeddedfont || !embeddedfont->hasGlyphs(getText()))
	{
		hasChanged=true;
		setNeedsTextureRecalculation();

		if(onStage && isVisible())
			requestInvalidation(this->getSystemState());
	}
	invalidateCachedAsBitmapOf();
}

void TextField::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (requestInvalidationForCacheAsBitmap(q))
		return;
	if (!tokensEmpty())
		TokenContainer::requestInvalidation(q,forceTextureRefresh);
	else
	{
		incRef();
		updateSizes();
		q->addToInvalidateQueue(_MR(this));
	}
}

void TextField::defaultEventBehavior(_R<Event> e)
{
	if (this->type != ET_EDITABLE)
		return;
	if (e->type == "keyDown")
	{
		KeyboardEvent* ev = e->as<KeyboardEvent>();
		uint32_t modifiers = ev->getModifiers() & (KMOD_LSHIFT | KMOD_RSHIFT |KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT);
		if (modifiers == KMOD_NONE)
		{
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_BACKSPACE:
					if (!this->getText().empty() && caretIndex > 0)
					{
						caretIndex--;
						tiny_string tmptext = getText();
						tmptext.replace(caretIndex,1,"");
						setText(tmptext.raw_buf());
						textUpdated();
					}
					break;
				case AS3KEYCODE_LEFT:
					if (this->caretIndex > 0)
						this->caretIndex--;
					break;
				case AS3KEYCODE_RIGHT:
					if (this->caretIndex < this->getText().numChars())
						this->caretIndex++;
					break;
				default:
					break;
			}
		}
		else
			LOG(LOG_NOT_IMPLEMENTED,"TextField keyDown event handling for modifier "<<modifiers<<" and char code "<<hex<<ev->getCharCode());
	}
}

IDrawable* TextField::invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	Locker l(invalidatemutex);
	if (cachedBitmap && computeCacheAsBitmap() && (!q || !q->getCacheAsBitmapObject() || q->getCacheAsBitmapObject().getPtr()!=this))
	{
		return getCachedBitmapDrawable(target, initialMatrix, cachedBitmap);
	}
	number_t x,y,rx,ry;
	number_t width,height,rwidth,rheight;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return nullptr;
	}

	RootMovieClip* currentRoot=this->loadedFrom;
	if (!currentRoot) currentRoot=this->getRoot().getPtr();
	if (!currentRoot) currentRoot = getSystemState()->mainClip;
	FontTag* embeddedfont = (fontID != UINT32_MAX ? currentRoot->getEmbeddedFontByID(fontID) : currentRoot->getEmbeddedFont(font));
	tokens.clear();
	MATRIX totalMatrix;
	if (embeddedfont && embeddedfont->hasGlyphs(getText()))
	{
		scaling = 1.0f/1024.0f/20.0f;
		if (this->border || this->background)
		{
			fillstyleBackgroundColor.FillStyleType=SOLID_FILL;
			fillstyleBackgroundColor.Color=this->backgroundColor;
			tokens.filltokens.push_back(GeomToken(SET_FILL).uval);
			tokens.filltokens.push_back(GeomToken(fillstyleBackgroundColor).uval);
			tokens.filltokens.push_back(GeomToken(MOVE).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, (bymax-bymin)/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, (bymax-bymin)/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(CLEAR_FILL).uval);
		}
		if (this->border)
		{
			lineStyleBorder.Color=this->borderColor;
			lineStyleBorder.Width=20;
			tokens.filltokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.filltokens.push_back(GeomToken(lineStyleBorder).uval);
			tokens.filltokens.push_back(GeomToken(MOVE).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, (bymax-bymin)/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, (bymax-bymin)/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
			tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
		}
		if (this->caretblinkstate)
		{
			uint32_t tw=0;
			if (!getText().empty())
			{
				tiny_string tmptxt = getText().substr(0,caretIndex);
				number_t w,h;
				embeddedfont->getTextBounds(tmptxt,fontSize,w,h);
				tw = w;
				tw += autosizeposition/scaling;
			}
			else
			{
				tw += autosizeposition;
				tw /=scaling;
			}
			lineStyleCaret.Color=RGB(0,0,0);
			lineStyleCaret.Width=40;
			int ypadding = (bymax-bymin-2)/scaling;
			tokens.filltokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.filltokens.push_back(GeomToken(lineStyleCaret).uval);
			tokens.filltokens.push_back(GeomToken(MOVE).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(tw, bymin/scaling+ypadding)).uval);
			tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
			tokens.filltokens.push_back(GeomToken(Vector2(tw, (bymax-bymin)/scaling-ypadding)).uval);
			tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
		}
		fillstyleTextColor.front().FillStyleType=SOLID_FILL;
		fillstyleTextColor.front().Color= RGBA(textColor.Red,textColor.Green,textColor.Blue,255);
		int32_t startposy = 0;
		linemutex->lock();
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (isPassword)
			{
				tiny_string pwtxt;
				for (uint32_t i = 0; i < (*it).text.numChars(); i++)
					pwtxt+="*";
				embeddedfont->fillTextTokens(tokens,pwtxt,fontSize,fillstyleTextColor,leading,autosizeposition+(*it).autosizeposition,startposy);
			}
			else
				embeddedfont->fillTextTokens(tokens,(*it).text,fontSize,fillstyleTextColor,leading,autosizeposition+(*it).autosizeposition,startposy);
			startposy += this->leading+(embeddedfont->getAscent()+embeddedfont->getDescent()+embeddedfont->getLeading())*fontSize/1024;
		}
		linemutex->unlock();
		if (tokens.empty())
			return nullptr;
		return TokenContainer::invalidate(target, initialMatrix,smoothing,q,cachedBitmap,false);
	}
	if (computeCacheAsBitmap() && (!q || !q->getCacheAsBitmapObject() || q->getCacheAsBitmapObject().getPtr()!=this))
	{
		return getCachedBitmapDrawable(target, initialMatrix, cachedBitmap);
	}
	std::vector<IDrawable::MaskData> masks;
	bool isMask;
	_NR<DisplayObject> mask;
	computeMasksAndMatrix(target, masks, totalMatrix,false,isMask,mask);
	totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
	MATRIX totalMatrix2;
	owner->computeMasksAndMatrix(target,masks,totalMatrix2,true,isMask,mask);
	totalMatrix2=initialMatrix.multiplyMatrix(totalMatrix2);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
	if (this->type != ET_EDITABLE)
	{
		if (getText().empty())
			return nullptr;
	}
	if(width==0 || height==0)
		return nullptr;
	if(totalMatrix.getScaleX() != 1 || totalMatrix.getScaleY() != 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextField when scaled is not correctly implemented:"<<x<<"/"<<y<<" "<<width<<"x"<<height<<" "<<totalMatrix.getScaleX()<<" "<<totalMatrix.getScaleY()<<" "<<this->getText());
	float rotation = getConcatenatedMatrix().getRotation();
	float xscale = getConcatenatedMatrix().getScaleX();
	float yscale = getConcatenatedMatrix().getScaleY();
	// use specialized Renderer from EngineData, if available, otherwise fallback to Pango
	IDrawable* res = this->getSystemState()->getEngineData()->getTextRenderDrawable(*this,totalMatrix, x, y, ceil(width), ceil(height),
																					rx, ry, ceil(rwidth), ceil(rheight), rotation,xscale,yscale,isMask,mask, 1.0f,getConcatenatedAlpha(), masks,
																					1.0f,1.0f,1.0f,1.0f,
																					0.0f,0.0f,0.0f,0.0f,
																					smoothing);
	if (res != nullptr)
		return res;
	/**  TODO: The scaling is done differently for textfields : height changes are applied directly
		on the font size. In some cases, it can change the width (if autosize is on and wordwrap off).
		Width changes do not change the font size, and do nothing when autosize is on and wordwrap off.
		Currently, the TextField is stretched in case of scaling.
	*/
	cachedSurface.isValid=true;
	return new CairoPangoRenderer(*this,totalMatrix,
				x, y, ceil(width), ceil(height),
				rx, ry, ceil(rwidth), ceil(rheight), rotation,
				xscale,yscale,
				isMask,mask,
				1.0f, getConcatenatedAlpha(), masks,
				1.0f,1.0f,1.0f,1.0f,
				0.0f,0.0f,0.0f,0.0f,
				smoothing,caretIndex);
}

bool TextField::renderImpl(RenderContext& ctxt) const
{
	if (computeCacheAsBitmap() && ctxt.contextType == RenderContext::GL)
	{
		_NR<DisplayObject> d=getCachedBitmap(); // this ensures bitmap is not destructed during rendering
		if (d)
			d->Render(ctxt);
		return false;
	}
	if (getText().empty() && !this->border && !this->background)
		return false;
	FontTag* embeddedfont = (fontID != UINT32_MAX ? this->loadedFrom->getEmbeddedFontByID(fontID) : this->loadedFrom->getEmbeddedFont(font));
	if (!computeCacheAsBitmap() && (ctxt.contextType == RenderContext::GL) && embeddedfont && embeddedfont->hasGlyphs(getText()))
	{
		// fast rendering path using pre-generated textures for every glyph
		float rotation = getConcatenatedMatrix().getRotation();
		float xscale = abs(getConcatenatedMatrix().getScaleX());
		float yscale = abs(getConcatenatedMatrix().getScaleY());
		float redMultiplier=1.0;
		float greenMultiplier=1.0;
		float blueMultiplier=1.0;
		float alphaMultiplier=1.0;
		float redOffset=0.0;
		float greenOffset=0.0;
		float blueOffset=0.0;
		float alphaOffset=0.0;
		RGB tcolor = this->textColor;
		ColorTransform* ct = colorTransform.getPtr();
		DisplayObjectContainer* p = getParent();
		while (!ct && p)
		{
			ct = p->colorTransform.getPtr();
			p = p->getParent();
		}
		if (ct)
		{
			redMultiplier=ct->redMultiplier;
			greenMultiplier=ct->greenMultiplier;
			blueMultiplier=ct->blueMultiplier;
			alphaMultiplier=ct->alphaMultiplier;
			redOffset=ct->redOffset;
			greenOffset=ct->greenOffset;
			blueOffset=ct->blueOffset;
			alphaOffset=ct->alphaOffset;
			float r,g,b,a;
			RGBA tmp;
			tmp = tcolor;
			ct->applyTransformation(tmp,r,g,b,a);
			tcolor.Red=r*255.0;
			tcolor.Green=g*255.0;
			tcolor.Blue=b*255.0;
		}
		AS_BLENDMODE bl = this->blendMode;
		if (bl == BLENDMODE_NORMAL)
		{
			DisplayObject* obj = this->getParent();
			while (obj && bl == BLENDMODE_NORMAL)
			{
				bl = obj->getBlendMode();
				obj = obj->getParent();
			}
		}
		uint32_t codetableindex;
		if (this->border || this->background || this->caretblinkstate)
		{
			number_t bxmin,bxmax,bymin,bymax;
			boundsRect(bxmin,bxmax,bymin,bymax);
			TextureChunk tex=getSystemState()->getRenderThread()->allocateTexture(bxmax-bxmin, bymax-bymin, true);
			number_t x,y,rx,ry;
			number_t width,height;
			number_t rwidth,rheight;
			MATRIX totalMatrix;
			std::vector<IDrawable::MaskData> masks;
			float scalex;
			float scaley;
			int offx,offy;
			getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth,getSystemState()->getRenderThread()->windowHeight,offx,offy, scalex,scaley);

			bool isMask;
			_NR<DisplayObject> mask;
			totalMatrix=getConcatenatedMatrix();
			computeMasksAndMatrix(this,masks,totalMatrix,false,isMask,mask);
			computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
			MATRIX totalMatrix2;
			std::vector<IDrawable::MaskData> masks2;
			totalMatrix2=getConcatenatedMatrix();
			computeMasksAndMatrix(this,masks2,totalMatrix2,true,isMask,mask);
			computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
			ctxt.setProperties(bl);
			ctxt.lsglLoadIdentity();
			if (this->border)
			{
				ctxt.renderTextured(tex, x*scalex, y*scaley,
						tex.width*scalex, tex.height*scaley,
						getConcatenatedAlpha(), RenderContext::RGB_MODE,
						rotation, rx*scalex, ry*scaley, ceil(rwidth*scalex), ceil(rheight*scaley), xscale, yscale,
						redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
						redOffset, greenOffset, blueOffset, alphaOffset,
						isMask, mask,3.0, this->borderColor,false,totalMatrix2);
				ctxt.renderTextured(tex, (x+1)*scalex, (y+1)*scaley,
						(tex.width-2)*scalex, (tex.height-2)*scaley,
						getConcatenatedAlpha(), RenderContext::RGB_MODE,
						rotation, (rx+1)*scalex, (ry+1)*scaley, ceil((rwidth-2)*scalex), ceil((rheight-2)*scaley), xscale, yscale,
						redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
						redOffset, greenOffset, blueOffset, alphaOffset,
						isMask, mask,3.0, this->backgroundColor,false,totalMatrix2);
			}
			else if (this->background)
			{
				ctxt.renderTextured(tex, x*scalex, y*scaley,
						tex.width*scalex, tex.height*scaley,
						getConcatenatedAlpha(), RenderContext::RGB_MODE,
						rotation, rx*scalex, ry*scaley, ceil(rwidth*scalex), ceil(rheight*scaley), xscale, yscale,
						redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
						redOffset, greenOffset, blueOffset, alphaOffset,
						isMask, mask,3.0, this->backgroundColor,false,totalMatrix2);
			}

			if (this->caretblinkstate)
			{
				int ypadding = (bymax-bymin-4);
				uint32_t tw=autosizeposition;
				if (!getText().empty())
				{
					tiny_string tmptxt = getText().substr(0,caretIndex);
					number_t w,h;
					embeddedfont->getTextBounds(tmptxt,fontSize,w,h);
					tw += w;
				}
				MATRIX totalMatrix;
				std::vector<IDrawable::MaskData> masks;
				totalMatrix=getConcatenatedMatrix();
				computeMasksAndMatrix(this,masks,totalMatrix,false,isMask,mask);
				computeBoundsForTransformedRect(tw,tw+2,bymin+ypadding,bymax-ypadding,x,y,width,height,totalMatrix);
				MATRIX totalMatrix2;
				std::vector<IDrawable::MaskData> masks2;
				totalMatrix2=getConcatenatedMatrix();
				computeMasksAndMatrix(this,masks2,totalMatrix2,true,isMask,mask);
				computeBoundsForTransformedRect(tw,tw+2,bymin+ypadding,bymax-ypadding,rx,ry,rwidth,rheight,totalMatrix2);
				ctxt.renderTextured(tex, x*scalex, y*scaley,
						ceil(width*scalex), ceil(height*scaley),
						getConcatenatedAlpha(), RenderContext::RGB_MODE,
						rotation, rx*scalex, ry*scaley, ceil(rwidth*scalex), ceil(rheight*scaley), xscale, yscale,
						redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
						redOffset, greenOffset, blueOffset, alphaOffset,
						isMask, mask,3.0, tcolor,true,totalMatrix2);
			}
		}
		number_t ypos=-TEXTFIELD_PADDING/yscale;
		float scalex;
		float scaley;
		int offx,offy;
		getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth,getSystemState()->getRenderThread()->windowHeight,offx,offy, scalex,scaley);
		linemutex->lock();
		for (auto itl = textlines.begin(); itl != textlines.end(); itl++)
		{
			number_t xpos = (autoSize==AS_NONE && tag ? tag->Bounds.Xmin/20.0f : 0.0f)+autosizeposition+(*itl).autosizeposition;
			for (auto it = (*itl).text.begin(); it!= (*itl).text.end(); it++)
			{
				const TextureChunk* tex = embeddedfont->getCharTexture(it,this->fontSize*yscale,codetableindex);
				if (tex)
				{
					number_t x,y,rx,ry;
					number_t width,height;
					number_t rwidth,rheight;
					number_t bxmin=xpos;
					number_t bxmax=xpos+tex->width/xscale;
					number_t bymin=ypos;
					number_t bymax=ypos+tex->height/yscale;
					//Compute the matrix and the masks that are relevant
					MATRIX totalMatrix;
					std::vector<IDrawable::MaskData> masks;
					
					bool isMask;
					_NR<DisplayObject> mask;
					totalMatrix=getConcatenatedMatrix();
					computeMasksAndMatrix(this,masks,totalMatrix,false,isMask,mask);
					computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
					MATRIX totalMatrix2;
					std::vector<IDrawable::MaskData> masks2;
					totalMatrix2=getConcatenatedMatrix();
					computeMasksAndMatrix(this,masks2,totalMatrix2,true,isMask,mask);
					computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
					ctxt.setProperties(bl);
					ctxt.lsglLoadIdentity();
					ctxt.renderTextured(*tex, x*scalex, y*scaley,
										tex->width*scalex, tex->height*scaley,
										getConcatenatedAlpha(), RenderContext::RGB_MODE,
										rotation, rx*scalex, ry*scaley, ceil(rwidth*scalex), ceil(rheight*scaley), 1.0, 1.0,
										redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
										redOffset, greenOffset, blueOffset, alphaOffset,
										isMask, mask,2.0, tcolor,true,totalMatrix2);
				}
				xpos += embeddedfont->getRenderCharAdvance(codetableindex)*fontSize;
			}
			ypos += this->leading+(embeddedfont->getAscent()+embeddedfont->getDescent()+embeddedfont->getLeading())*fontSize/1024;
		}
		linemutex->unlock();
		return false;
	}
	else
		return defaultRender(ctxt);
}

void TextField::HtmlTextParser::parseTextAndFormating(const tiny_string& html,
						      TextData *dest)
{
	textdata = dest;
	if (!textdata)
		return;

	textdata->setText("");

	tiny_string rooted = tiny_string("<root>") + html + tiny_string("</root>");
	uint32_t pos=0;
	// ensure <br> tags are properly parsed
	while ((pos = rooted.find("<br>",pos)) != tiny_string::npos)
		rooted.replace_bytes(pos,4,"<br />");
	pugi::xml_document doc;
	if (doc.load_buffer(rooted.raw_buf(),rooted.numBytes()).status == pugi::status_ok)
	{
		doc.traverse(*this);
	}
	else
	{
		LOG(LOG_ERROR, "TextField HTML parser error:"<<rooted);
		return;
	}
}

bool TextField::HtmlTextParser::for_each(pugi::xml_node &node)
{

	if (!textdata)
		return true;
	tiny_string name = node.name();
	name = name.lowercase();
	tiny_string v = node.value();
	tiny_string newtext=textdata->getText();
	uint32_t index =v.find("&nbsp;");
	while (index != tiny_string::npos)
	{
		v = v.replace(index,6," ");
		index =v.find("&nbsp;",index);
	}
	newtext += v;
	if (name == "br" || name == "sbr") // adobe seems to interpret the unknown tag <sbr /> as <br> ?
	{
		if (textdata->multiline)
			newtext += "\n";
			
	}
	else if (name == "p")
	{
		if (textdata->multiline)
		{
			if (!newtext.empty() &&
				!newtext.endsWith("\n"))
				newtext += "\n";
			if (node.children().begin() ==node.children().end()) // empty paragraph
				newtext += "\n";
		}
		for (auto it=node.attributes_begin(); it!=node.attributes_end(); ++it)
		{
			tiny_string attrname = it->name();
			attrname = attrname.lowercase();
			tiny_string value = it->value();
			if (attrname == "align")
			{
				if (value == "left")
					textdata->autoSize = TextData::AS_LEFT;
				if (value == "center")
					textdata->autoSize = TextData::AS_CENTER;
				if (value == "right")
					textdata->autoSize = TextData::AS_RIGHT;
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <p>: unsupported attribute:"<<attrname);
			}
		}

	}
	else if (name == "font")
	{
		for (auto it=node.attributes_begin(); it!=node.attributes_end(); ++it)
		{
			tiny_string attrname = it->name();
			attrname = attrname.lowercase();
			if (attrname == "face")
			{
				if (textdata->font != it->value())
				{
					textdata->font = it->value();
					textdata->fontID = UINT32_MAX;
				}
			}
			else if (attrname == "size")
			{
				textdata->fontSize = parseFontSize(it->value(), textdata->fontSize);
			}
			else if (attrname == "color")
			{
				textdata->textColor = RGB(tiny_string(it->value()));
			}
			else
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <font>: unsupported attribute:"<<attrname<<" "<<it->value());
		}
	}
	else if (name == "" || name == "root" || name == "body")
	{
		// normal entry
	}
	else if (name == "a" || name == "img" || name == "u" ||
		 name == "li" || name == "b" || name == "i" ||
		 name == "span" || name == "textformat" || name == "tab")
	{
		LOG(LOG_NOT_IMPLEMENTED, _("Unsupported tag in TextField: ") << name);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED, _("Unknown tag in TextField: ") << name);
	}
	textdata->setText(newtext.raw_buf());
	return true;
}

uint32_t TextField::HtmlTextParser::parseFontSize(const char* s,
						  uint32_t currentFontSize)
{
	if (!s)
		return currentFontSize;

	uint32_t basesize = 0;
	int multiplier = 1;
	if (s[0] == '+' || s[0] == '-')
	{
		// relative size
		basesize = currentFontSize;
		if (s[0] == '-')
			multiplier = -1;
	}

	int64_t size = basesize + multiplier*g_ascii_strtoll(s, NULL, 10);
	if (size < 1)
		size = 1;
	if (size > G_MAXUINT32)
		size = G_MAXUINT32;
	
	return (uint32_t)size;
}

void TextFieldAutoSize::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CENTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"center"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"left"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"right"),CONSTANT_TRAIT);
}

void TextFieldType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DYNAMIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"dynamic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INPUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"input"),CONSTANT_TRAIT);
}

void TextFormatAlign::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->isReusable = true;
	c->setVariableAtomByQName("CENTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"center"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("END",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"end"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JUSTIFY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"justify"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"left"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"right"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("START",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"start"),CONSTANT_TRAIT);
}

void TextFormat::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,align);
	REGISTER_GETTER_SETTER(c,blockIndent);
	REGISTER_GETTER_SETTER(c,bold);
	REGISTER_GETTER_SETTER(c,bullet);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,font);
	REGISTER_GETTER_SETTER(c,indent);
	REGISTER_GETTER_SETTER(c,italic);
	REGISTER_GETTER_SETTER(c,kerning);
	REGISTER_GETTER_SETTER(c,leading);
	REGISTER_GETTER_SETTER(c,leftMargin);
	REGISTER_GETTER_SETTER(c,letterSpacing);
	REGISTER_GETTER_SETTER(c,rightMargin);
	REGISTER_GETTER_SETTER(c,size);
	REGISTER_GETTER_SETTER(c,tabStops);
	REGISTER_GETTER_SETTER(c,target);
	REGISTER_GETTER_SETTER(c,underline);
	REGISTER_GETTER_SETTER(c,url);
	// not in spec, but available in adobe player
	REGISTER_GETTER_SETTER(c,display);
}

bool TextFormat::destruct()
{
	blockIndent = asAtomHandler::nullAtom;
	bold = asAtomHandler::nullAtom;
	bullet = asAtomHandler::nullAtom;
	color = asAtomHandler::nullAtom;
	indent = asAtomHandler::nullAtom;
	italic = asAtomHandler::nullAtom;
	kerning = asAtomHandler::nullAtom;
	leading = asAtomHandler::nullAtom;
	leftMargin = asAtomHandler::nullAtom;
	letterSpacing = asAtomHandler::nullAtom;
	rightMargin = asAtomHandler::nullAtom;
	tabStops.reset();
	underline = asAtomHandler::nullAtom;
	display = "";
	align="";
	url="";
	target="";
	font="";
	size=asAtomHandler::nullAtom;
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(TextFormat,_constructor)
{
	TextFormat* th=asAtomHandler::as<TextFormat>(obj);
	ARG_UNPACK_ATOM (th->font, "")
		(th->size,asAtomHandler::nullAtom)
		(th->color,asAtomHandler::nullAtom)
		(th->bold,asAtomHandler::nullAtom)
		(th->italic,asAtomHandler::nullAtom)
		(th->underline,asAtomHandler::nullAtom)
		(th->url,"")
		(th->target,"")
		(th->align,"left")
		(th->leftMargin,asAtomHandler::nullAtom)
		(th->rightMargin,asAtomHandler::nullAtom)
		(th->indent,asAtomHandler::nullAtom)
		(th->leading,asAtomHandler::nullAtom);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(TextFormat,align,onAlign);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,blockIndent);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bold);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bullet);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,color);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,font);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,indent);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,italic);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,kerning);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leading);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leftMargin);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,letterSpacing);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,rightMargin);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,size);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,tabStops);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,target);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,underline);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,url);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,display);

void TextFormat::buildTraits(ASObject* o)
{
}

void TextFormat::onAlign(const tiny_string& old)
{
	if (align != "" && align != "center" && align != "end" && align != "justify" && 
	    align != "left" && align != "right" && align != "start")
	{
		align = old;
		throwError<ArgumentError>(kInvalidEnumError, "align");
	}
}

void StyleSheet::finalize()
{
	EventDispatcher::finalize();
	styles.clear();
}

void StyleSheet::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("styleNames","",Class<IFunction>::getFunction(c->getSystemState(),_getStyleNames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("setStyle","",Class<IFunction>::getFunction(c->getSystemState(),setStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getStyle","",Class<IFunction>::getFunction(c->getSystemState(),getStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parseCSS","",Class<IFunction>::getFunction(c->getSystemState(),parseCSS),NORMAL_METHOD,true);
}

void StyleSheet::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(StyleSheet,setStyle)
{
	StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	assert_and_throw(argslen==2);
	const tiny_string& arg0=asAtomHandler::toString(args[0],sys);
	ASATOM_INCREF(args[1]); //TODO: should make a copy, see reference
	map<tiny_string, asAtom>::iterator it=th->styles.find(arg0);
	//NOTE: we cannot use the [] operator as References cannot be non initialized
	if(it!=th->styles.end()) //Style already exists
		it->second=args[1];
	else
		th->styles.insert(make_pair(arg0,args[1]));
}

ASFUNCTIONBODY_ATOM(StyleSheet,getStyle)
{
	StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=asAtomHandler::toString(args[0],sys);
	map<tiny_string, asAtom>::iterator it=th->styles.find(arg0);
	if(it!=th->styles.end()) //Style already exists
	{
		//TODO: should make a copy, see reference
		ASATOM_INCREF(it->second);
		ret = it->second;
	}
	else
	{
		// Tested behaviour is to return an empty ASObject
		// instead of Null as is said in the documentation
		ret = asAtomHandler::fromObject(Class<ASObject>::getInstanceS(sys));
	}
}

ASFUNCTIONBODY_ATOM(StyleSheet,_getStyleNames)
{
	StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	assert_and_throw(argslen==0);
	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	map<tiny_string, asAtom>::const_iterator it=th->styles.begin();
	for(;it!=th->styles.end();++it)
		res->push(asAtomHandler::fromObject(abstract_s(sys,it->first)));
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(StyleSheet,parseCSS)
{
	//StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	tiny_string css;
	ARG_UNPACK_ATOM(css);
	LOG(LOG_NOT_IMPLEMENTED,"StyleSheet.parseCSS does nothing");
}

void StaticText::sinit(Class_base* c)
{
	// FIXME: the constructor should be
	// _constructorNotInstantiatable but that breaks when
	// DisplayObjectContainer::initFrame calls the constructor
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(c->getSystemState(),_getText),GETTER_METHOD,true);
}

IDrawable* StaticText::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	return TokenContainer::invalidate(target, initialMatrix,smoothing,q,cachedBitmap,false);
}
bool StaticText::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=bounds.Xmin/20.0;
	xmax=bounds.Xmax/20.0;
	ymin=bounds.Ymin/20.0;
	ymax=bounds.Ymax/20.0;
	return true;
}

bool StaticText::renderImpl(RenderContext& ctxt) const
{
	if (computeCacheAsBitmap() && ctxt.contextType == RenderContext::GL)
	{
		_NR<DisplayObject> d=getCachedBitmap(); // this ensures bitmap is not destructed during rendering
		if (d)
			d->Render(ctxt);
		return false;
	}
	return TokenContainer::renderImpl(ctxt);
}

_NR<DisplayObject> StaticText::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type, bool interactiveObjectsOnly)
{
	number_t xmin,xmax,ymin,ymax;
	boundsRect(xmin,xmax,ymin,ymax);
	if( xmin <= x && x <= xmax && ymin <= y && y <= ymax)
	{
		if (interactiveObjectsOnly)
			return last;
		incRef();
		return _MNR(this);
	}
	else
		return NullRef;
}
ASFUNCTIONBODY_ATOM(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	ret = asAtomHandler::fromString(sys,"");
}

void FontStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("BOLD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bold"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOLD_ITALIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"boldItalic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ITALIC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"italic"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REGULAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"regular"),CONSTANT_TRAIT);
}

void FontType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DEVICE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"device"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EMBEDDED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"embedded"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EMBEDDED_CFF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"embeddedCFF"),CONSTANT_TRAIT);
}

void TextDisplayMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("CRT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"crt"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"default"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LCD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lcd"),CONSTANT_TRAIT);
}

void TextColorType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("DARK_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"dark"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIGHT_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"light"),CONSTANT_TRAIT);
}

void GridFitType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PIXEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pixel"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUBPIXEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"subpixel"),CONSTANT_TRAIT);
}

void TextInteractionMode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SELECTION",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"selection"),CONSTANT_TRAIT);
}

void TextLineMetrics::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, ascent);
	REGISTER_GETTER_SETTER(c, descent);
	REGISTER_GETTER_SETTER(c, height);
	REGISTER_GETTER_SETTER(c, leading);
	REGISTER_GETTER_SETTER(c, width);
	REGISTER_GETTER_SETTER(c, x);
}

ASFUNCTIONBODY_ATOM(TextLineMetrics,_constructor)
{
	if (argslen == 0)
	{
		//Assume that the values were initialized by the C++
		//constructor
		return;
	}

	TextLineMetrics* th=asAtomHandler::as<TextLineMetrics>(obj);
	ARG_UNPACK_ATOM (th->x) (th->width) (th->height) (th->ascent) (th->descent) (th->leading);
}

ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, ascent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, descent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, height);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, leading);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, width);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, x);
