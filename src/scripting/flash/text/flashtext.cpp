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

#include "platforms/engineutils.h"
#include "scripting/abc.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/ui/keycodes.h"
#include "scripting/class.h"
#include "compat.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "backends/rendering.h"
#include "backends/rendering_context.h"
#include "backends/cachedsurface.h"
#include "scripting/argconv.h"
#include <3rdparty/pugixml/src/pugixml.hpp>
#include "parsing/tags.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"

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
	c->setDeclaredMethodByQName("enumerateFonts","",c->getSystemState()->getBuiltinFunction(enumerateFonts),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("registerFont","",c->getSystemState()->getBuiltinFunction(registerFont),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("hasGlyphs","",c->getSystemState()->getBuiltinFunction(hasGlyphs),NORMAL_METHOD,true);

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
ASFUNCTIONBODY_GETTER(ASFont, fontName)
ASFUNCTIONBODY_GETTER(ASFont, fontStyle)
ASFUNCTIONBODY_GETTER(ASFont, fontType)

ASFUNCTIONBODY_ATOM(ASFont,enumerateFonts)
{
	bool enumerateDeviceFonts=false;
	ARG_CHECK(ARG_UNPACK(enumerateDeviceFonts,false));

	if (enumerateDeviceFonts)
		LOG(LOG_NOT_IMPLEMENTED,"Font::enumerateFonts: flag enumerateDeviceFonts is not handled");
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
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
	ARG_CHECK(ARG_UNPACK(fontclass));
	if (!fontclass->is<Class_inherit>())
	{
		createError<ArgumentError>(wrk,kCheckTypeFailedError,
								  fontclass->getClassName(),
								  Class<ASFont>::getClass(wrk->getSystemState())->getQualifiedClassName());
		return;
	}
	ASFont* font = new (fontclass->as<Class_base>()->memoryAccount) ASFont(wrk,fontclass->as<Class_base>());
	fontclass->as<Class_base>()->setupDeclaredTraits(font);
	font->constructionComplete();
	font->setConstructIndicator();
	getFontList()->push_back(asAtomHandler::fromObject(font));
}
ASFUNCTIONBODY_ATOM(ASFont,hasGlyphs)
{
	ASFont* th = asAtomHandler::as<ASFont>(obj);
	tiny_string text;
	ARG_CHECK(ARG_UNPACK(text));
	if (th->fontType == "embedded")
	{
		FontTag* f = wrk->getSystemState()->mainClip->getEmbeddedFont(th->fontName);
		if (f)
		{
			asAtomHandler::setBool(ret,f->hasGlyphs(text));
			return;
		}
	}
	LOG(LOG_NOT_IMPLEMENTED,"Font.hasGlyphs always returns true for not embedded fonts:"<<text<<" "<<th->fontName<<" "<<th->fontStyle<<" "<<th->fontType);
	asAtomHandler::setBool(ret,true);
}
TextField::TextField(ASWorker* wrk, Class_base* c, const TextData& textData, bool _selectable, bool readOnly, const char *varname, DefineEditTextTag *_tag)
	: InteractiveObject(wrk,c), TextData(textData), TokenContainer(this), type(ET_READ_ONLY),
	  antiAliasType(AA_NORMAL), gridFitType(GF_PIXEL),
	  textInteractionMode(TI_NORMAL),autosizeposition(0),tagvarname(varname,true),tagvartarget(nullptr),tag(_tag),originalXPosition(0),originalWidth(textData.width),
	  fillstyleBackgroundColor(0xff),lineStyleBorder(0xff),lineStyleCaret(0xff),linemutex(new Mutex()),inAVM1syncVar(false),
	  inUpdateVarBinding(false),
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
	c->setDeclaredMethodByQName("appendText","",c->getSystemState()->getBuiltinFunction(TextField:: appendText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTextFormat","",c->getSystemState()->getBuiltinFunction(_getTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextFormat","",c->getSystemState()->getBuiltinFunction(_setTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getCharIndexAtPoint","",c->getSystemState()->getBuiltinFunction(_getCharIndexAtPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineIndexAtPoint","",c->getSystemState()->getBuiltinFunction(_getLineIndexAtPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineIndexOfChar","",c->getSystemState()->getBuiltinFunction(_getLineIndexOfChar),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineLength","",c->getSystemState()->getBuiltinFunction(_getLineLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineMetrics","",c->getSystemState()->getBuiltinFunction(_getLineMetrics),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineOffset","",c->getSystemState()->getBuiltinFunction(_getLineOffset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineText","",c->getSystemState()->getBuiltinFunction(_getLineText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replaceSelectedText","",c->getSystemState()->getBuiltinFunction(_replaceSelectedText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replaceText","",c->getSystemState()->getBuiltinFunction(_replaceText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSelection","",c->getSystemState()->getBuiltinFunction(_setSelection),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getCharBoundaries","",c->getSystemState()->getBuiltinFunction(_getCharBoundaries),NORMAL_METHOD,true);

	// properties
	c->setDeclaredMethodByQName("antiAliasType","",c->getSystemState()->getBuiltinFunction(TextField::_getAntiAliasType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("antiAliasType","",c->getSystemState()->getBuiltinFunction(TextField::_setAntiAliasType),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",c->getSystemState()->getBuiltinFunction(TextField::_setAutoSize),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",c->getSystemState()->getBuiltinFunction(TextField::_getAutoSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_getDefaultTextFormat),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_setDefaultTextFormat),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("gridFitType","",c->getSystemState()->getBuiltinFunction(TextField::_getGridFitType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("gridFitType","",c->getSystemState()->getBuiltinFunction(TextField::_setGridFitType),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(TextField::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(TextField::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",c->getSystemState()->getBuiltinFunction(TextField::_getHtmlText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",c->getSystemState()->getBuiltinFunction(TextField::_setHtmlText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(TextField::_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",c->getSystemState()->getBuiltinFunction(TextField::_getText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",c->getSystemState()->getBuiltinFunction(TextField::_setText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",c->getSystemState()->getBuiltinFunction(TextField::_getTextHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",c->getSystemState()->getBuiltinFunction(TextField::_getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(TextField::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(TextField::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",c->getSystemState()->getBuiltinFunction(TextField::_setWordWrap),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",c->getSystemState()->getBuiltinFunction(TextField::_getWordWrap),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numLines","",c->getSystemState()->getBuiltinFunction(TextField::_getNumLines,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("maxScrollH","",c->getSystemState()->getBuiltinFunction(TextField::_getMaxScrollH,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("maxScrollV","",c->getSystemState()->getBuiltinFunction(TextField::_getMaxScrollV,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomScrollV","",c->getSystemState()->getBuiltinFunction(TextField::_getBottomScrollV,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("restrict","",c->getSystemState()->getBuiltinFunction(TextField::_getRestrict),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("restrict","",c->getSystemState()->getBuiltinFunction(TextField::_setRestrict),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textInteractionMode","",c->getSystemState()->getBuiltinFunction(TextField::_getTextInteractionMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("displayAsPassword","",c->getSystemState()->getBuiltinFunction(TextField::_getdisplayAsPassword),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("displayAsPassword","",c->getSystemState()->getBuiltinFunction(TextField::_setdisplayAsPassword),SETTER_METHOD,true);

	// special handling neccessary when getting/setting x
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(TextField::_getTextFieldX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(TextField::_setTextFieldX),SETTER_METHOD,true);

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

ASFUNCTIONBODY_GETTER_SETTER(TextField, alwaysShowSelection) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, background)
ASFUNCTIONBODY_GETTER_SETTER(TextField, backgroundColor)
ASFUNCTIONBODY_GETTER_SETTER(TextField, border)
ASFUNCTIONBODY_GETTER_SETTER(TextField, borderColor)
ASFUNCTIONBODY_GETTER(TextField, caretIndex)
ASFUNCTIONBODY_GETTER_SETTER(TextField, condenseWhite)
ASFUNCTIONBODY_GETTER_SETTER(TextField, embedFonts) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, maxChars) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, multiline)
ASFUNCTIONBODY_GETTER_SETTER(TextField, mouseWheelEnabled) // stub
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, scrollH, validateScrollH)
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, scrollV, validateScrollV)
ASFUNCTIONBODY_GETTER_SETTER(TextField, selectable)
ASFUNCTIONBODY_GETTER(TextField, selectionBeginIndex)
ASFUNCTIONBODY_GETTER(TextField, selectionEndIndex)
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, sharpness, validateSharpness) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, styleSheet) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, textColor)
ASFUNCTIONBODY_GETTER_SETTER_CB(TextField, thickness, validateThickness) // stub
ASFUNCTIONBODY_GETTER_SETTER(TextField, useRichTextClipboard) // stub

void TextField::finalize()
{
	InteractiveObject::finalize();
	restrictChars.reset();
	styleSheet.reset();
}

void TextField::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	InteractiveObject::prepareShutdown();
	if(restrictChars)
		restrictChars->prepareShutdown();
	if (styleSheet)
		styleSheet->prepareShutdown();
	if (tagvartarget)
		tagvartarget->prepareShutdown();
}
bool TextField::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = InteractiveObject::countCylicMemberReferences(gcstate);
	if (tagvartarget)
		ret = tagvartarget->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

bool TextField::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
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
		if (wordWrap)
			xmax=max(0.0f,float(width))+ (tag ? tag->Bounds.Xmin/20.0f : 0.0f);
		else
			xmax=max(0.0f,float(textWidth+autosizeposition))+2*TEXTFIELD_PADDING+ (tag ? tag->Bounds.Xmin/20.0f : 0.0f);
		ymin=tag ? tag->Bounds.Ymin/20.0f : 0.0f;
		ymax=max(0.0f,float(height)+(tag ? tag->Bounds.Ymin/20.0f :0.0f)+2*TEXTFIELD_PADDING);
		return true;
	}
	xmin=tag->Bounds.Xmin/20.0f;
	if (wordWrap)
		xmax=max(0.0f,float(width)+tag->Bounds.Xmin/20.0f);
	else
		xmax=max(0.0f,float(textWidth)+2*TEXTFIELD_PADDING+tag->Bounds.Xmin/20.0f);
	ymin=tag->Bounds.Ymin/20.0f;
	ymax=max(0.0f,float(height)+tag->Bounds.Ymin/20.0f);
	return true;
}

_NR<DisplayObject> TextField::hitTestImpl(const Vector2f&, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	/* I suppose one does not have to actually hit a character */
	number_t xmin,xmax,ymin,ymax;
	// TODO: Add an overload for RECT.
	boundsRect(xmin,xmax,ymin,ymax,false);
	//TODO: Add a point intersect function to RECT, and use that instead.
	if( xmin <= localPoint.x && localPoint.x <= xmax && ymin <= localPoint.y && localPoint.y <= ymax)
	{
		if (interactiveObjectsOnly && this->tag && this->tag->WasStatic && this->type == ET_READ_ONLY && (type == MOUSE_CLICK_HIT || type == DOUBLE_CLICK_HIT))
		{
			// it seems that TextFields are not set as target to MouseEvents if constructed from a DefineEditTextTag that has the flag WasStatic 
			if (this->getParent())
			{
				this->getParent()->incRef();
				return _MNR(this->getParent());
			}
			return NullRef;
		}
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
	ARG_CHECK(ARG_UNPACK(th->isPassword));
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
	ARG_CHECK(ARG_UNPACK(th->wordWrap));
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
			ret = asAtomHandler::fromString(wrk->getSystemState(),"none");
			return;
		case AS_LEFT:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"left");
			return;
		case AS_RIGHT:
			ret =asAtomHandler::fromString(wrk->getSystemState(),"right");
			return;
		case AS_CENTER:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"center");
			return;
	}
}

ASFUNCTIONBODY_ATOM(TextField,_setAutoSize)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string autoSizeString;
	ARG_CHECK(ARG_UNPACK(autoSizeString));

	ALIGNMENT newAutoSize = AS_NONE;
	if(autoSizeString == "none" || (!th->loadedFrom->usesActionScript3 && autoSizeString=="false"))
		newAutoSize = AS_NONE;
	else if (autoSizeString == "left" || (!th->loadedFrom->usesActionScript3 && autoSizeString=="true"))
		newAutoSize = AS_LEFT;
	else if (autoSizeString == "right")
		newAutoSize = AS_RIGHT;
	else if (autoSizeString == "center")
		newAutoSize = AS_CENTER;
	else
	{
		createError<ArgumentError>(wrk,kInvalidEnumError, "autoSize");
		return;
	}

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
		switch (align)
		{
			case AS_RIGHT:
				linemutex->lock();
				autosizeposition = width-textWidth-TEXTFIELD_PADDING*2;
				for (auto it = textlines.begin(); it != textlines.end(); it++)
				{
					if ((*it).textwidth < textWidth)
						(*it).autosizeposition = (textWidth+TEXTFIELD_PADDING*2-(*it).textwidth);
					else
						(*it).autosizeposition = 0;
				}
				linemutex->unlock();
				break;
			case AS_CENTER:
				linemutex->lock();
				autosizeposition = (width-textWidth-TEXTFIELD_PADDING*2)/2;
				for (auto it = textlines.begin(); it != textlines.end(); it++)
				{
					if ((*it).textwidth < textWidth)
						(*it).autosizeposition = (textWidth+TEXTFIELD_PADDING*2-(*it).textwidth)/2;
					else
						(*it).autosizeposition = 0;
				}
				linemutex->unlock();
				break;
			default:
				autosizeposition = 0;
				break;
		}
		if (updatewidth && !wordWrap)
			width = originalWidth;
		return;
	}
	switch (autoSize)
	{
		case AS_RIGHT:
			linemutex->lock();
			autosizeposition = 0;
			if (!wordWrap) // not in the specs but Adobe changes x position if wordWrap is not set
			{
				this->setX(originalXPosition + (int(originalWidth-TEXTFIELD_PADDING*2 - textWidth))*sx);
				if (updatewidth)
					width = textWidth+TEXTFIELD_PADDING*2;
			}
			else
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
				this->setX(originalXPosition + (int(originalWidth-TEXTFIELD_PADDING*2 - textWidth))/2*sx);
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
	geometryChanged();
}

ASFUNCTIONBODY_ATOM(TextField,_getWidth)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,wrk,th->width);
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
			th->requestInvalidation(wrk->getSystemState());
		th->legacy=false;
	}
}

ASFUNCTIONBODY_ATOM(TextField,_getHeight)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	// it seems that Adobe returns the textHeight if in autoSize mode
	if (th->autoSize != AS_NONE)
		asAtomHandler::setUInt(ret,wrk,th->textHeight);
	else
		asAtomHandler::setUInt(ret,wrk,th->height);
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

ASFUNCTIONBODY_ATOM(TextField,_getTextFieldX)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setNumber(ret,wrk,th->originalXPosition+th->autosizeposition);
}
ASFUNCTIONBODY_ATOM(TextField,_setTextFieldX)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	DisplayObject::_setX(ret,wrk,obj,args,argslen);
	th->originalXPosition=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(TextField,_getTextWidth)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,wrk,th->textWidth);
}

ASFUNCTIONBODY_ATOM(TextField,_getTextHeight)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setUInt(ret,wrk,th->textHeight);
}

ASFUNCTIONBODY_ATOM(TextField,_getHtmlText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toHtmlText()));
}

ASFUNCTIONBODY_ATOM(TextField,_setHtmlText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(value));
	th->setHtmlText(value);
	th->legacy=false;
}

ASFUNCTIONBODY_ATOM(TextField,_getText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	Locker l(*th->linemutex);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->getText()));
}

ASFUNCTIONBODY_ATOM(TextField,_setText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	th->updateText(asAtomHandler::toString(args[0],wrk));
	th->legacy=false;
}

ASFUNCTIONBODY_ATOM(TextField, appendText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	assert_and_throw(argslen==1);
	th->updateText(th->getText() + asAtomHandler::toString(args[0],wrk));
}

ASFUNCTIONBODY_ATOM(TextField,_getTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	TextFormat *format=Class<TextFormat>::getInstanceS(wrk);

	format->color= asAtomHandler::fromUInt(th->textColor.toUInt());
	format->font = asAtomHandler::fromString(wrk->getSystemState(),th->font);
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
	{
		ARG_CHECK(ARG_UNPACK(tf)(beginIndex, -1)(endIndex, -1));
	}
	else
	{
		if (argslen == 2)
		{
			ARG_CHECK(ARG_UNPACK(beginIndex)(tf)(endIndex, -1));
		}
		else
		{
			ARG_CHECK(ARG_UNPACK(beginIndex)(endIndex)(tf));
		}
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

	tiny_string align="left";
	if (!asAtomHandler::isNull(tf->align))
		align = asAtomHandler::toString(tf->align,wrk);
	ALIGNMENT newAlign = th->align;
	if(align == "none")
		newAlign = AS_NONE;
	else if (align == "left")
		newAlign = AS_LEFT;
	else if (align == "right")
		newAlign = AS_RIGHT;
	else if (align == "center")
		newAlign = AS_CENTER;
	if (th->align != newAlign)
	{
		th->align = newAlign;
		updatesizes = true;
	}
	if(!asAtomHandler::isNull(tf->font))
	{
		tiny_string fnt = asAtomHandler::toString(tf->font,wrk);
		if (fnt != th->font)
			updatesizes = true;
		th->font = fnt;
		th->fontID = UINT32_MAX;
	}
	if (!asAtomHandler::isNull(tf->size) && th->fontSize != asAtomHandler::toUInt(tf->size) && asAtomHandler::toUInt(tf->size)>0)
	{
		th->fontSize = asAtomHandler::toUInt(tf->size);
		updatesizes = true;
	}
	if (updatesizes)
	{
		th->linemutex->lock();
		th->checkEmbeddedFont(th);
		th->linemutex->unlock();
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
	
	TextFormat* tf = Class<TextFormat>::getInstanceS(wrk);
	tf->font = asAtomHandler::fromString(wrk->getSystemState(),th->font);
	tf->bold = th->isBold ? asAtomHandler::trueAtom : asAtomHandler::nullAtom;
	tf->italic = th->isItalic ? asAtomHandler::trueAtom : asAtomHandler::nullAtom;
	LOG(LOG_NOT_IMPLEMENTED,"getDefaultTextFormat does not get all fields of TextFormat");
	ret = asAtomHandler::fromObject(tf);
}

ASFUNCTIONBODY_ATOM(TextField,_setDefaultTextFormat)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	_NR<TextFormat> tf;

	ARG_CHECK(ARG_UNPACK(tf));

	if(!asAtomHandler::isNull(tf->color))
		th->textColor = asAtomHandler::toUInt(tf->color);
	if(!asAtomHandler::isNull(tf->font))
	{
		th->font = asAtomHandler::toString(tf->font,wrk);
		th->fontID = UINT32_MAX;
	}
	if (!asAtomHandler::isNull(tf->size) && asAtomHandler::toUInt(tf->size) > 0)
		th->fontSize = asAtomHandler::toUInt(tf->size);
	tiny_string align="left";
	if (!asAtomHandler::isNull(tf->align))
		align = asAtomHandler::toString(tf->align,wrk);
	ALIGNMENT newAlign = th->align;
	if(align == "none")
		newAlign = AS_NONE;
	else if (align == "left")
		newAlign = AS_LEFT;
	else if (align == "right")
		newAlign = AS_RIGHT;
	else if (align == "center")
		newAlign = AS_CENTER;
	th->isBold=asAtomHandler::toInt(tf->bold);
	th->isItalic=asAtomHandler::toInt(tf->italic);
	if (th->align != newAlign)
	{
		th->align = newAlign;
		th->updateSizes();
		th->setSizeAndPositionFromAutoSize();
		th->hasChanged=true;
		th->setNeedsTextureRecalculation();
	}
	LOG(LOG_NOT_IMPLEMENTED,"setDefaultTextFormat does not set all fields of TextFormat");
}

ASFUNCTIONBODY_ATOM(TextField, _getter_type)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->type == ET_READ_ONLY)
		ret = asAtomHandler::fromString(wrk->getSystemState(),"dynamic");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"input");
}

ASFUNCTIONBODY_ATOM(TextField, _setter_type)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	tiny_string value;
	ARG_CHECK(ARG_UNPACK(value));

	if (value == "dynamic")
		th->type = ET_READ_ONLY;
	else if (value == "input")
		th->type = ET_EDITABLE;
	else
		createError<ArgumentError>(wrk,kInvalidEnumError, "type");
}

ASFUNCTIONBODY_ATOM(TextField,_getCharIndexAtPoint)
{
//	TextField* th=asAtomHandler::as<TextField>(obj);
	number_t x;
	number_t y;
	ARG_CHECK(ARG_UNPACK(x) (y));
	LOG(LOG_NOT_IMPLEMENTED,"Textfield.getCharIndexAtPoint");

	asAtomHandler::setInt(ret,wrk,-1);
}


ASFUNCTIONBODY_ATOM(TextField,_getLineIndexAtPoint)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	number_t x;
	number_t y;
	ARG_CHECK(ARG_UNPACK(x) (y));

	Locker l(*th->linemutex);
	int32_t Ymin = 0;
	int32_t Ymax = 0;
	for (uint32_t i=0; i<th->getLineCount(); i++)
	{
		Ymax+=th->textlines[i].height;
		
		if (x > th->textlines[i].autosizeposition && x <= th->textlines[i].autosizeposition+th->textlines[i].textwidth &&
		    y > Ymin && y <= Ymax)
		{
			asAtomHandler::setInt(ret,wrk,i);
			return;
		}
		Ymin+=th->textlines[i].height;
	}
	asAtomHandler::setInt(ret,wrk,-1);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineIndexOfChar)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t charIndex;
	ARG_CHECK(ARG_UNPACK(charIndex));

	if (charIndex < 0)
	{
		asAtomHandler::setInt(ret,wrk,-1);
		return;
	}

	Locker l(*th->linemutex);
	uint32_t firstCharOffset=0;
	for (uint32_t i=0; i < th->textlines.size(); ++i)
	{
		if (uint32_t(charIndex) >= firstCharOffset &&
		    uint32_t(charIndex) < firstCharOffset + th->textlines[i].text.numChars())
		{
			asAtomHandler::setInt(ret,wrk,i);
			return;
		}
		firstCharOffset+=th->textlines[i].text.numChars()+1; // add one for the \n
	}

	// testing shows that returns -1 on invalid index instead of
	// throwing RangeError
	asAtomHandler::setInt(ret,wrk,-1);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineLength)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t lineIndex;
	ARG_CHECK(ARG_UNPACK(lineIndex));

	Locker l(*th->linemutex);
	if ((lineIndex < 0) || (lineIndex >= (int32_t)th->textlines.size()))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}

	asAtomHandler::setInt(ret,wrk,th->textlines[lineIndex].text.numChars());
}

ASFUNCTIONBODY_ATOM(TextField,_getLineMetrics)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_CHECK(ARG_UNPACK(lineIndex));

	Locker l(*th->linemutex);
	std::vector<LineData> lines = CairoPangoRenderer::getLineData(*th);
	if ((lineIndex < 0) || (lineIndex >= (int32_t)lines.size()))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}

	ret = asAtomHandler::fromObject(Class<TextLineMetrics>::getInstanceS(wrk,
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
	ARG_CHECK(ARG_UNPACK(lineIndex));

	Locker l(*th->linemutex);
	if ((lineIndex < 0) || (lineIndex >= (int32_t)th->textlines.size()))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	uint32_t firstCharOffset=0;
	for (int32_t i=0; i < lineIndex; ++i)
	{
		firstCharOffset+=th->textlines[i].text.numChars()+1; // add one for the \n
	}

	asAtomHandler::setInt(ret,wrk,firstCharOffset);
}

ASFUNCTIONBODY_ATOM(TextField,_getLineText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t  lineIndex;
	ARG_CHECK(ARG_UNPACK(lineIndex));

	Locker l(*th->linemutex);
	if (lineIndex < 0 || lineIndex >= (int32_t)th->textlines.size())
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->textlines[lineIndex].text));
}

ASFUNCTIONBODY_ATOM(TextField,_getAntiAliasType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->antiAliasType == AA_NORMAL)
		ret = asAtomHandler::fromString(wrk->getSystemState(),"normal");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"advanced");
}

ASFUNCTIONBODY_ATOM(TextField,_setAntiAliasType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(value));

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
		ret = asAtomHandler::fromString(wrk->getSystemState(),"none");
	else if (th->gridFitType == GF_PIXEL)
		ret = asAtomHandler::fromString(wrk->getSystemState(),"pixel");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"subpixel");
}

ASFUNCTIONBODY_ATOM(TextField,_setGridFitType)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(value));

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
	Locker l(*th->linemutex);
	asAtomHandler::setUInt(ret,wrk,th->getText().numChars());
}

ASFUNCTIONBODY_ATOM(TextField,_getNumLines)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	Locker l(*th->linemutex);
	asAtomHandler::setInt(ret,wrk,(int32_t)th->getLineCount());
}

ASFUNCTIONBODY_ATOM(TextField,_getMaxScrollH)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setInt(ret,wrk,th->getMaxScrollH());
}

ASFUNCTIONBODY_ATOM(TextField,_getMaxScrollV)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setInt(ret,wrk,th->getMaxScrollV());
}

ASFUNCTIONBODY_ATOM(TextField,_getBottomScrollV)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	
	Locker l(*th->linemutex);
	int32_t Ymin = 0;
	for (unsigned int k=1; k<th->getLineCount(); k++)
	{
		if (Ymin >= (int)th->height)
		{
			asAtomHandler::setInt(ret,wrk,(int32_t)k);
			return;
		}
		Ymin+=th->textlines[k-1].height;
	}
	asAtomHandler::setInt(ret,wrk,(int32_t)th->getLineCount() + 1);
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
	ARG_CHECK(ARG_UNPACK(th->restrictChars));
	if (!th->restrictChars.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "TextField restrict property");
}

ASFUNCTIONBODY_ATOM(TextField,_getTextInteractionMode)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	if (th->textInteractionMode == TI_NORMAL)
		ret = asAtomHandler::fromString(wrk->getSystemState(),"normal");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"selection");
}

ASFUNCTIONBODY_ATOM(TextField,_setSelection)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	ARG_CHECK(ARG_UNPACK(th->selectionBeginIndex) (th->selectionEndIndex));

	if (th->selectionBeginIndex < 0)
		th->selectionBeginIndex = 0;

	Locker l(*th->linemutex);
	tiny_string text = th->getText();
	if (th->selectionEndIndex >= (int32_t)text.numChars())
		th->selectionEndIndex = text.numChars()-1;

	if (th->selectionEndIndex < 0)
		th->selectionEndIndex = 0;

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
	ARG_CHECK(ARG_UNPACK(newText));
	th->replaceText(th->selectionBeginIndex, th->selectionEndIndex, newText);
}

ASFUNCTIONBODY_ATOM(TextField,_replaceText)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	int32_t begin;
	int32_t end;
	tiny_string newText;
	ARG_CHECK(ARG_UNPACK(begin) (end) (newText));
	th->replaceText(begin, end, newText);
}

void TextField::replaceText(unsigned int begin, unsigned int end, const tiny_string& newText)
{
	if (!styleSheet.isNull())
	{
		createError<ASError>(getInstanceWorker(),0,"Can not replace text on text field with a style sheet");
		return;
	}

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
	linemutex->lock();
	setText(text.raw_buf());
	linemutex->unlock();
	textUpdated();
}

void TextField::getTextBounds(const tiny_string& txt,number_t &xmin,number_t &xmax,number_t &ymin,number_t &ymax)
{
	if (embeddedFont)
		scaling = 1.0f/1024.0f/20.0f;
	getTextSizes(txt,xmax,ymax);
	xmin = autosizeposition;
	xmax += autosizeposition;
	ymin=0;
}

ASFUNCTIONBODY_ATOM(TextField,_getCharBoundaries)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	int32_t charIndex;
	ARG_CHECK(ARG_UNPACK(charIndex));

	Rectangle* rect = Class<Rectangle>::getInstanceSNoArgs(wrk);
	th->linemutex->lock();
	tiny_string text = th->getText();
	th->linemutex->unlock();
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
	Locker l(*linemutex);
	if (getLineCount() <= 1)
		return 1;
	int32_t Ymax = 0;
	for (uint32_t i = 0; i < getLineCount(); i++)
	{
		Ymax+=textlines[i].height;
	}
	if (Ymax <= (int32_t)height)
		return 1;

	// one full page from the bottom
	int32_t pagesize=0;
	for (int k=(int)getLineCount()-1; k>=0; k--)
	{
		pagesize+=textlines[k].height;
		if (Ymax - pagesize > (int32_t)height)
		{
			return imin(k+1+1, getLineCount());
		}
	}
	return 1;
}

void TextField::updateSizes()
{
	Locker l(invalidatemutex);
	uint32_t tw,th;
	tw = 0;
	th = 0;
	
	scaling = 1.0f/1024.0f/20.0f;
	th=0;
	number_t w=0;
	number_t h=0;
	linemutex->lock();
	auto it = textlines.begin();
	while (it != textlines.end())
	{
		getTextSizes((*it).text,w,h);
		(*it).textwidth=w;
		bool listchanged=false;
		if (wordWrap && width > TEXTFIELD_PADDING*2 && uint32_t(w) > width-TEXTFIELD_PADDING*2)
		{
			// calculate lines for wordwrap
			tiny_string text =(*it).text;
			uint32_t c= text.rfind(" ");// TODO check for other whitespace characters
			while (c != tiny_string::npos && c != 0)
			{
				getTextSizes(text.substr(0,c),w,h);
				if (w <= width-TEXTFIELD_PADDING*2)
				{
					if(w>tw)
						tw = w;
					(*it).textwidth=w;
					(*it).text = text.substr(0,c);
					textline t;
					t.autosizeposition=0;
					t.text=text.substr(c+1,UINT32_MAX);
					getTextSizes(t.text,w,h);
					t.textwidth=w;
					t.height=h;
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
		else if (w>tw)
			tw = w;
		if (!listchanged)
			it++;
		th+=h;
		if (it != textlines.end())
			th+=this->leading;
	}
	linemutex->unlock();
	if(w>tw)
		tw = w;
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

	Locker l(*linemutex);
	//Split text into paragraphs and wraps them into <p> tags
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		FormatText& format = (*it).format;
		pugi::xml_node node = root;
		if (format.bold)
			node = node.append_child("b");
		if (format.italic)
			node = node.append_child("i");
		if (format.underline)
			node = node.append_child("u");
		if (format.bullet)
			node = node.append_child("li");
		node.append_child("p").text().set((*it).text.raw_buf());
	}

	ostringstream buf;
	doc.print(buf);
	tiny_string ret = tiny_string(buf.str());
	return ret;
}

void TextField::setHtmlText(const tiny_string& html)
{
	linemutex->lock();
	vector<tiny_string> oldtext;
	if (this->isConstructed())
	{
		oldtext.reserve(textlines.size());
		for (uint32_t i =0; i < textlines.size(); i++)
		{
			oldtext.push_back(textlines[i].text);
		}
	}
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
	if (getLineCount()>1 && this->textlines.back().text.empty())
	{
		//more than one line and last line is empty => remove last line
		this->textlines.pop_back();
	}
	linemutex->unlock();

	if (this->isConstructed() && !this->TextIsEqual(oldtext))
	{
		hasChanged=true;
		setNeedsTextureRecalculation();
		textUpdated();
	}
}

std::string TextField::toDebugString() const
{
	std::string res = InteractiveObject::toDebugString();
	res += " \"";
	res += this->getText(0);
	res += "\";";
	char buf[100];
	sprintf(buf,"%dx%d %5.2f %d/%d %s",textWidth,textHeight,autosizeposition,autoSize,align,font.raw_buf());
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
	linemutex->lock();
	setText(new_text.raw_buf());
	linemutex->unlock();
	textUpdated();
}

void TextField::avm1SyncTagVar()
{
	if (!tagvarname.empty()
		&& tagvarname != "_url") // "_url" is readonly and always read from root movie, no need to update
	{
		if (tagvartarget && !inAVM1syncVar)
		{
			inAVM1syncVar=true;
			asAtom value=asAtomHandler::invalidAtom;
			number_t n;
			linemutex->lock();
			if (Integer::fromStringFlashCompatible(getText().raw_buf(),n,10,true))
				value = asAtomHandler::fromNumber(getInstanceWorker(),n,false);
			else
				value = asAtomHandler::fromString(getSystemState(),getText());
			linemutex->unlock();
			ASATOM_INCREF(value); // ensure that value is not destructed during AVM1SetVariable
			tagvartarget->as<MovieClip>()->AVM1SetVariable(tagvarname,value);
			ASATOM_DECREF(value);
			inAVM1syncVar=false;
		}
	}
}

void TextField::UpdateVariableBinding(asAtom v)
{
	inUpdateVarBinding = true;
	tiny_string s = asAtomHandler::toString(v,getInstanceWorker());
	if (tag->isHTML())
		setHtmlText(s);
	else
		updateText(s);
	inUpdateVarBinding = false;
}

void TextField::afterLegacyInsert()
{
	if (!tagvarname.empty() && !getConstructIndicator())
	{
		tagvartarget = getParent();
		uint32_t finddot = tagvarname.rfind(".");
		if (finddot != tiny_string::npos)
		{
			tiny_string path = tagvarname.substr(0,finddot);
			tagvartarget = tagvartarget->AVM1GetClipFromPath(path);
			tagvarname = tagvarname.substr(finddot+1,tagvarname.numChars()-(finddot+1));
		}
		while (tagvartarget)
		{
			if (tagvartarget->is<MovieClip>())
			{
				tagvartarget->as<MovieClip>()->setVariableBinding(tagvarname,_MR(this));
				asAtom value = tagvartarget->as<MovieClip>()->getVariableBindingValue(tagvarname);
				if (asAtomHandler::isValid(value) && !asAtomHandler::isUndefined(value))
				{
					linemutex->lock();
					this->setText(asAtomHandler::toString(value,getInstanceWorker()).raw_buf());
					linemutex->unlock();
				}
				ASATOM_DECREF(value);
				break;
			}
			tagvartarget = tagvartarget->getParent();
		}
		if (tagvartarget)
		{
			tagvartarget->incRef();
			tagvartarget->addStoredMember();
		}
	}
	if (!loadedFrom->usesActionScript3 && !getConstructIndicator())
	{
		setConstructIndicator();
		constructionComplete();
		afterConstruction();
	}
	avm1SyncTagVar();
	updateSizes();
	setSizeAndPositionFromAutoSize();
	InteractiveObject::afterLegacyInsert();
}

void TextField::afterLegacyDelete(bool inskipping)
{
	if (!tagvarname.empty() && !inskipping)
	{
		if (tagvartarget)
		{
			tagvartarget->as<MovieClip>()->setVariableBinding(tagvarname,NullRef);
			tagvartarget->removeStoredMember();
			tagvartarget=nullptr;
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
}

void TextField::gotFocus()
{
	if (this->type != ET_EDITABLE)
		return;
	SDL_StartTextInput();
	linemutex->lock();
	selectionBeginIndex = getText().numChars();
	linemutex->unlock();
	selectionEndIndex = selectionBeginIndex;
	caretIndex=selectionBeginIndex;
	getSystemState()->addTick(500,this);
}

void TextField::textInputChanged(const tiny_string &newtext)
{
	if (this->type != ET_EDITABLE)
		return;
	linemutex->lock();
	tiny_string tmptext = getText();
	linemutex->unlock();
	
	if (maxChars == 0 || tmptext.numChars()+newtext.numChars() <= uint32_t(maxChars))
	{
		if (caretIndex < tmptext.numChars())
			tmptext.replace(caretIndex,0,newtext);
		else
			tmptext+=newtext;
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
	// Don't sync the bound variable if we're updating the binding.
	if (!inUpdateVarBinding)
		avm1SyncTagVar();
	scrollH = 0;
	scrollV = 1;
	selectionBeginIndex = 0;
	selectionEndIndex = 0;
	linemutex->lock();
	checkEmbeddedFont(this);
	linemutex->unlock();
	updateSizes();
	setSizeAndPositionFromAutoSize();
	setNeedsTextureRecalculation();
	hasChanged=true;

	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
	else
		requestInvalidationFilterParent(this->getSystemState());
}

void TextField::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (!tokensEmpty())
		TokenContainer::requestInvalidation(q,forceTextureRefresh);
	else
	{
		incRef();
		updateSizes();
		requestInvalidationFilterParent(q);
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
					linemutex->lock();
					if (!this->getText().empty() && caretIndex > 0)
					{
						caretIndex--;
						tiny_string tmptext = getText();
						if (caretIndex < tmptext.numChars())
							tmptext.replace(caretIndex,1,"");
						else
							tmptext = tmptext.substr(0,tmptext.numChars()-1);
						setText(tmptext.raw_buf());
						linemutex->unlock();
						textUpdated();
					}
					else
						linemutex->unlock();
					break;
				case AS3KEYCODE_LEFT:
					if (this->caretIndex > 0)
						this->caretIndex--;
					break;
				case AS3KEYCODE_RIGHT:
					linemutex->lock();
					if (this->caretIndex < this->getText().numChars())
						this->caretIndex++;
					linemutex->unlock();
					break;
				default:
					break;
			}
		}
		else
		{
			bool handled = false;
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_V:
					if (modifiers & KMOD_CTRL)
					{
						textInputChanged(tiny_string(SDL_GetClipboardText()));
						handled = true;
					}
					break;
				default:
					break;
			}
			if (!handled)
				LOG(LOG_NOT_IMPLEMENTED,"TextField keyDown event handling for modifier "<<modifiers<<" and char code "<<hex<<ev->getSDLScanCode());
		}
	}
}
IDrawable* TextField::invalidate(bool smoothing)
{
	Locker l(invalidatemutex);
	number_t x,y;
	number_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax,false)==false)
	{
		//No contents, nothing to do
		return nullptr;
	}
	
	ColorTransformBase ct;
	ct.fillConcatenated(this);
	MATRIX matrix = getMatrix();
	bool isMask=this->isMask();
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);
	tokens.clear();
	if (embeddedFont)
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
				getTextSizes(tmptxt,w,h);
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
		int32_t startposy = TEXTFIELD_PADDING+bymin;
		linemutex->lock();
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (isPassword)
			{
				tiny_string pwtxt;
				for (uint32_t i = 0; i < (*it).text.numChars(); i++)
					pwtxt+="*";
				embeddedFont->fillTextTokens(tokens,pwtxt,fontSize,fillstyleTextColor,leading,TEXTFIELD_PADDING+autosizeposition+(*it).autosizeposition,startposy);
			}
			else
				embeddedFont->fillTextTokens(tokens,(*it).text,fontSize,fillstyleTextColor,leading,TEXTFIELD_PADDING+autosizeposition+(*it).autosizeposition,startposy);
			startposy += this->leading+(embeddedFont->getAscent()+embeddedFont->getDescent()+embeddedFont->getLeading())*fontSize/1024;
		}
		linemutex->unlock();
		if (tokens.empty())
		{
			this->resetNeedsTextureRecalculation();
			return new RefreshableDrawable(x, y, ceil(width), ceil(height)
										   , matrix.getScaleX(), matrix.getScaleY()
										   , isMask, cacheAsBitmap
										   , getScaleFactor(), getConcatenatedAlpha()
										   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
		}
		// it seems that textfields are always rendered with subpixel smoothing when rendering to bitmap
		return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,false);
	}
	if (this->type != ET_EDITABLE)
	{
		Locker l(*linemutex);
		if (getLineCount()==0)
		{
			this->resetNeedsTextureRecalculation();
			return new RefreshableDrawable(x, y, ceil(width), ceil(height)
										   , matrix.getScaleX(), matrix.getScaleY()
										   , isMask, cacheAsBitmap
										   , getScaleFactor(), getConcatenatedAlpha()
										   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
		}
	}
	if(width==0 || height==0)
	{
		this->resetNeedsTextureRecalculation();
		return new RefreshableDrawable(x, y, ceil(width), ceil(height)
									   , matrix.getScaleX(), matrix.getScaleY()
									   , isMask, cacheAsBitmap
									   , getScaleFactor(), getConcatenatedAlpha()
									   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
	}		
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
	/**  TODO: The scaling is done differently for textfields : height changes are applied directly
		on the font size. In some cases, it can change the width (if autosize is on and wordwrap off).
		Width changes do not change the font size, and do nothing when autosize is on and wordwrap off.
		Currently, the TextField is stretched in case of scaling.
	*/
	return new CairoPangoRenderer(*this,matrix,
				x, y, ceil(width), ceil(height),
				xscale,yscale,
				isMask, cacheAsBitmap,
				1.0f, getConcatenatedAlpha(),
				ColorTransformBase(),
				smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),caretIndex);
}

void TextField::HtmlTextParser::parseTextAndFormating(const tiny_string& html,
						      TextData *dest)
{
	textdata = dest;
	if (!textdata)
		return;

	tiny_string rooted = tiny_string("<root>") + html + tiny_string("</root>");
	uint32_t pos=0;
	// ensure <br> tags are properly parsed
	while ((pos = rooted.find("<br>",pos)) != tiny_string::npos)
		rooted.replace_bytes(pos,4,"<br />");
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(rooted.raw_buf(),rooted.numBytes(), pugi::parse_default & ~pugi::parse_validate_closing_tags);
	if (result.status == pugi::status_ok)
	{
		textdata->setText("");
		doc.traverse(*this);
		formatStack.erase(formatStack.begin(), formatStack.end());
	}
	else
	{
		LOG(LOG_ERROR, "TextField HTML parser error:"<<rooted);
		LOG(LOG_ERROR, "Reason: " << result.description());
		LOG(LOG_ERROR, "Offset: " << result.offset);
		LOG(LOG_ERROR, "Text at offset: " << (rooted.raw_buf() + result.offset));
		return;
	}
}

FORCE_INLINE bool skipFormatStackPushPop(const tiny_string& name)
{
	return name.empty() || name == "br" || name == "sbr";
}

bool TextField::HtmlTextParser::for_each(pugi::xml_node &node)
{
	if (!textdata)
		return true;

	int currentDepth = depth();
	tiny_string name = node.name();
	name = name.lowercase();
	tiny_string v = node.value();
	tiny_string newtext;
	FormatText format = !formatStack.empty() ? formatStack.back() : FormatText {};
	prevDepth = currentDepth;
	prevName = name;
	if (currentDepth < prevDepth && !skipFormatStackPushPop(prevName))
	{
		for (int i = currentDepth; i < prevDepth; ++i)
			formatStack.pop_back();
	}
	uint32_t index =v.find("&nbsp;");
	while (index != tiny_string::npos)
	{
		v.replace(index,6," ");
		index =v.find("&nbsp;",index);
	}
	newtext += v;
	if (name == "br" || name == "sbr") // adobe seems to interpret the unknown tag <sbr /> as <br> ?
	{
		if (textdata->multiline)
			newtext += "\n";
			
	}
	if (name == "p")
	{
		if (textdata->multiline)
		{
			if (textdata->getLineCount() &&
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
				{
					textdata->align = TextData::AS_LEFT;
					format.align = FormatText::AS_LEFT;
				}
				if (value == "center")
				{
					textdata->align = TextData::AS_CENTER;
					format.align = FormatText::AS_CENTER;
				}
				if (value == "right")
				{
					textdata->align = TextData::AS_RIGHT;
					format.align = FormatText::AS_RIGHT;
				}
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <"<<name<<">: unsupported attribute:"<<attrname);
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
					format.font = it->value();
					textdata->fontID = UINT32_MAX;
				}
			}
			else if (attrname == "size")
			{
				textdata->fontSize = parseFontSize(it->value(), textdata->fontSize);
				format.fontSize = parseFontSize(it->value(), format.fontSize);
			}
			else if (attrname == "color")
			{
				textdata->textColor = RGB(tiny_string(it->value()));
				format.fontColor = RGB(tiny_string(it->value()));
			}
			else
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <font>: unsupported attribute:"<<attrname<<" "<<it->value());
		}
	}
	else if (name == "a")
	{
		for (auto it : node.attributes())
		{
			tiny_string attrname = it.name();
			attrname = attrname.lowercase();
			if (attrname == "href")
				format.url = it.value();
			else if (attrname == "target")
				format.target = it.value();
		}
	}
	else if (name == "b")
	{
		format.bold = true;
	}
	else if (name == "i")
	{
		format.italic = true;
	}
	else if (name == "u")
	{
		format.underline = true;
	}
	else if (name == "li" && textdata->multiline)
	{
		format.bullet = true;
	}
	else if (name == "img" || name == "span" || name == "textformat" ||
		 name == "tab")
	{
		LOG(LOG_NOT_IMPLEMENTED, "Unsupported tag in TextField: " << name);
	}
	if (!skipFormatStackPushPop(name))
		formatStack.push_back(format);
	if (!newtext.empty())
		textdata->appendFormatText(newtext.raw_buf(), format);
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
	c->isReusable=true;
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

void TextFormat::finalize()
{
	ASATOM_REMOVESTOREDMEMBER(blockIndent);
	ASATOM_REMOVESTOREDMEMBER(bold);
	ASATOM_REMOVESTOREDMEMBER(bullet);
	ASATOM_REMOVESTOREDMEMBER(color);
	ASATOM_REMOVESTOREDMEMBER(indent);
	ASATOM_REMOVESTOREDMEMBER(italic);
	ASATOM_REMOVESTOREDMEMBER(kerning);
	ASATOM_REMOVESTOREDMEMBER(leading);
	ASATOM_REMOVESTOREDMEMBER(leftMargin);
	ASATOM_REMOVESTOREDMEMBER(letterSpacing);
	ASATOM_REMOVESTOREDMEMBER(rightMargin);
	if (tabStops)
		tabStops->removeStoredMember();
	tabStops.reset();
	ASATOM_REMOVESTOREDMEMBER(underline);
	ASATOM_REMOVESTOREDMEMBER(display);
	ASATOM_REMOVESTOREDMEMBER(align);
	ASATOM_REMOVESTOREDMEMBER(url);
	ASATOM_REMOVESTOREDMEMBER(target);
	ASATOM_REMOVESTOREDMEMBER(font);
	ASATOM_REMOVESTOREDMEMBER(size);
}

bool TextFormat::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(blockIndent);
	blockIndent = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(bold);
	bold = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(bullet);
	bullet = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(color);
	color = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(indent);
	indent = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(italic);
	italic = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(kerning);
	kerning = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(leading);
	leading = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(leftMargin);
	leftMargin = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(letterSpacing);
	letterSpacing = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(rightMargin);
	rightMargin = asAtomHandler::nullAtom;
	if (tabStops)
		tabStops->removeStoredMember();
	tabStops.reset();
	ASATOM_REMOVESTOREDMEMBER(underline);
	underline = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(display);
	display = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(align);
	align = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(url);
	url = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(target);
	target = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(font);
	font = asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(size);
	size=asAtomHandler::nullAtom;
	return destructIntern();
}
void TextFormat::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	ASATOM_PREPARESHUTDOWN(blockIndent);
	ASATOM_PREPARESHUTDOWN(bold);
	ASATOM_PREPARESHUTDOWN(bullet);
	ASATOM_PREPARESHUTDOWN(color);
	ASATOM_PREPARESHUTDOWN(indent);
	ASATOM_PREPARESHUTDOWN(italic);
	ASATOM_PREPARESHUTDOWN(kerning);
	ASATOM_PREPARESHUTDOWN(leading);
	ASATOM_PREPARESHUTDOWN(leftMargin);
	ASATOM_PREPARESHUTDOWN(letterSpacing);
	ASATOM_PREPARESHUTDOWN(rightMargin);
	if (tabStops)
		tabStops->prepareShutdown();
	ASATOM_PREPARESHUTDOWN(underline);
	ASATOM_PREPARESHUTDOWN(display);
	ASATOM_PREPARESHUTDOWN(align);
	ASATOM_PREPARESHUTDOWN(url);
	ASATOM_PREPARESHUTDOWN(target);
	ASATOM_PREPARESHUTDOWN(font);
	ASATOM_PREPARESHUTDOWN(size);
}

ASFUNCTIONBODY_ATOM(TextFormat,_constructor)
{
	TextFormat* th=asAtomHandler::as<TextFormat>(obj);
	ARG_CHECK(ARG_UNPACK (th->font, asAtomHandler::nullAtom)
		(th->size,asAtomHandler::nullAtom)
		(th->color,asAtomHandler::nullAtom)
		(th->bold,asAtomHandler::nullAtom)
		(th->italic,asAtomHandler::nullAtom)
		(th->underline,asAtomHandler::nullAtom)
		(th->url,asAtomHandler::nullAtom)
		(th->target,asAtomHandler::nullAtom)
		(th->align,asAtomHandler::nullAtom)
		(th->leftMargin,asAtomHandler::nullAtom)
		(th->rightMargin,asAtomHandler::nullAtom)
		(th->indent,asAtomHandler::nullAtom)
		(th->leading,asAtomHandler::nullAtom));
	ASATOM_ADDSTOREDMEMBER(th->font);
	ASATOM_ADDSTOREDMEMBER(th->size);
	ASATOM_ADDSTOREDMEMBER(th->color);
	ASATOM_ADDSTOREDMEMBER(th->bold);
	ASATOM_ADDSTOREDMEMBER(th->italic);
	ASATOM_ADDSTOREDMEMBER(th->underline);
	ASATOM_ADDSTOREDMEMBER(th->url);
	ASATOM_ADDSTOREDMEMBER(th->target);
	ASATOM_ADDSTOREDMEMBER(th->align);
	ASATOM_ADDSTOREDMEMBER(th->leftMargin);
	ASATOM_ADDSTOREDMEMBER(th->rightMargin);
	ASATOM_ADDSTOREDMEMBER(th->indent);
	ASATOM_ADDSTOREDMEMBER(th->leading);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(TextFormat,align,onAlign)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,blockIndent)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bold)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bullet)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,color)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,font)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,indent)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,italic)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,kerning)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leading)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leftMargin)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,letterSpacing)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,rightMargin)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,size)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(TextFormat,tabStops)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,target)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,underline)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,url)
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,display)

void TextFormat::onAlign(const asAtom& old)
{
	if (asAtomHandler::isNull(align)) // null is also allowed
		return;
	tiny_string a = asAtomHandler::toString(align,getInstanceWorker());
	if (a != "" && a != "center" && a != "end" && a != "justify" && 
	    a != "left" && a != "right" && a != "start")
	{
		align = old;
		createError<ArgumentError>(getInstanceWorker(),kInvalidEnumError, "align");
	}
}

TextFormat::TextFormat(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_TEXTFORMAT)
{
}

TextFormat::~TextFormat()
{
}

void StyleSheet::finalize()
{
	EventDispatcher::finalize();
	styles.clear();
}

void StyleSheet::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("styleNames","",c->getSystemState()->getBuiltinFunction(_getStyleNames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("setStyle","",c->getSystemState()->getBuiltinFunction(setStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getStyle","",c->getSystemState()->getBuiltinFunction(getStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parseCSS","",c->getSystemState()->getBuiltinFunction(parseCSS),NORMAL_METHOD,true);
}

void StyleSheet::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(StyleSheet,setStyle)
{
	StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	assert_and_throw(argslen==2);
	const tiny_string& arg0=asAtomHandler::toString(args[0],wrk);
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
	const tiny_string& arg0=asAtomHandler::toString(args[0],wrk);
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
		ret = asAtomHandler::fromObject(Class<ASObject>::getInstanceS(wrk));
	}
}

ASFUNCTIONBODY_ATOM(StyleSheet,_getStyleNames)
{
	StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	assert_and_throw(argslen==0);
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	map<tiny_string, asAtom>::const_iterator it=th->styles.begin();
	for(;it!=th->styles.end();++it)
		res->push(asAtomHandler::fromObject(abstract_s(wrk,it->first)));
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(StyleSheet,parseCSS)
{
	//StyleSheet* th=asAtomHandler::as<StyleSheet>(obj);
	tiny_string css;
	ARG_CHECK(ARG_UNPACK(css));
	LOG(LOG_NOT_IMPLEMENTED,"StyleSheet.parseCSS does nothing");
}

void StaticText::sinit(Class_base* c)
{
	// FIXME: the constructor should be
	// _constructorNotInstantiatable but that breaks when
	// DisplayObjectContainer::initFrame calls the constructor
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("text","",c->getSystemState()->getBuiltinFunction(_getText),GETTER_METHOD,true);
}

IDrawable* StaticText::invalidate(bool smoothing)
{
	return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,false);
}
bool StaticText::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	xmin=bounds.Xmin/20.0;
	xmax=bounds.Xmax/20.0;
	ymin=bounds.Ymin/20.0;
	ymax=bounds.Ymax/20.0;
	return true;
}

_NR<DisplayObject> StaticText::hitTestImpl(const Vector2f&, const Vector2f& localPoint, DisplayObject::HIT_TYPE type, bool interactiveObjectsOnly)
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
		return NullRef;
}
ASFUNCTIONBODY_ATOM(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	ret = asAtomHandler::fromString(wrk->getSystemState(),"");
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
	ARG_CHECK(ARG_UNPACK (th->x) (th->width) (th->height) (th->ascent) (th->descent) (th->leading));
}

ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, ascent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, descent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, height);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, leading);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, width);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, x);
