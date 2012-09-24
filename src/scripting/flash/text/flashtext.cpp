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

#include "scripting/flash/text/flashtext.h"
#include "scripting/class.h"
#include "compat.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void lightspark::AntiAliasType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("ADVANCED","",Class<ASString>::getInstanceS("advanced"),DECLARED_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),DECLARED_TRAIT);
}

void ASFont::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	//c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("enumerateFonts","",Class<IFunction>::getFunction(enumerateFonts),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("registerFont","",Class<IFunction>::getFunction(registerFont),NORMAL_METHOD,false);

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

std::vector<ASObject*>* ASFont::getFontList()
{
	static std::vector<ASObject*> fontlist;
	return &fontlist;
}
ASFUNCTIONBODY_GETTER(ASFont, fontName);
ASFUNCTIONBODY_GETTER(ASFont, fontStyle);
ASFUNCTIONBODY_GETTER(ASFont, fontType);

ASFUNCTIONBODY(ASFont,enumerateFonts)
{
	bool enumerateDeviceFonts=false;
	ARG_UNPACK(enumerateDeviceFonts,false);

	LOG(LOG_NOT_IMPLEMENTED,"Font::enumerateFonts: flag enumerateDeviceFonts is not handled");
	Array* ret = Class<Array>::getInstanceS();
	std::vector<ASObject*>* fontlist = getFontList();
	for(auto i = fontlist->begin(); i != fontlist->end(); ++i)
	{
		(*i)->incRef();
		ret->push(_MR(*i));
	}
	return ret;
}
ASFUNCTIONBODY(ASFont,registerFont)
{
	getFontList()->push_back(args[0]);
	return NULL;
}

void TextField::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<InteractiveObject>::getRef());
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",Class<IFunction>::getFunction(TextField::_getTextHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",Class<IFunction>::getFunction(TextField::_getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_getText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_setText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(TextField::_setWordWrap),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(TextField::_getWordWrap),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(TextField::_setAutoSize),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(TextField::_getAutoSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendText","",Class<IFunction>::getFunction(TextField:: appendText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTextFormat","",Class<IFunction>::getFunction(_getTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextFormat","",Class<IFunction>::getFunction(_setTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(TextField::_getDefaultTextFormat),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(TextField::_setDefaultTextFormat),SETTER_METHOD,true);

	REGISTER_GETTER_SETTER(c,textColor);
}

ASFUNCTIONBODY_GETTER_SETTER(TextField,textColor);

void TextField::buildTraits(ASObject* o)
{
}

bool TextField::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

_NR<InteractiveObject> TextField::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	/* I suppose one does not have to actually hit a character */
	number_t xmin,xmax,ymin,ymax;
	boundsRect(xmin,xmax,ymin,ymax);
	if( xmin <= x && x <= xmax && ymin <= y && y <= ymax && isHittable(type))
		return last;
	else
		return NullRef;
}

ASFUNCTIONBODY(TextField,_getWordWrap)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_b(th->wordWrap);
}

ASFUNCTIONBODY(TextField,_setWordWrap)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->wordWrap=Boolean_concrete(args[0]);
	if(th->onStage)
		th->requestInvalidation(getSys());
	return NULL;
}

ASFUNCTIONBODY(TextField,_getAutoSize)
{
	TextField* th=Class<TextField>::cast(obj);
	switch(th->autoSize)
	{
		case AS_NONE:
			return Class<ASString>::getInstanceS("none");
		case AS_LEFT:
			return Class<ASString>::getInstanceS("left");
		case AS_RIGHT:
			return Class<ASString>::getInstanceS("right");
		case AS_CENTER:
			return Class<ASString>::getInstanceS("center");
	}
	return NULL;
}

ASFUNCTIONBODY(TextField,_setAutoSize)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	tiny_string temp = args[0]->toString();
	if(temp == "none")
		th->autoSize = AS_NONE;//TODO: take care of corner cases : what to do with sizes when changing the autoSize
	else if (temp == "left")
		th->autoSize = AS_LEFT;
	else if (temp == "right")
		th->autoSize = AS_RIGHT;
	else if (temp == "center")
		th->autoSize = AS_CENTER;
	else
		throw Class<ArgumentError>::getInstanceS("Wrong argument in TextField.autoSize");
	if(th->onStage)
		th->requestInvalidation(getSys());//TODO:check if there was any change
	return NULL;
}

ASFUNCTIONBODY(TextField,_getWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(TextField,_setWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	//The width needs to be updated only if autoSize is off or wordWrap is on TODO:check this, adobe's behavior is not clear
	if((th->autoSize == AS_NONE)||(th->wordWrap == true))
	{
		th->width=args[0]->toInt();
		if(th->onStage)
			th->requestInvalidation(getSys());
		else
			th->updateSizes();
	}
	return NULL;
}

ASFUNCTIONBODY(TextField,_getHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(TextField,_setHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	if(th->autoSize == AS_NONE)
	{
		th->height=args[0]->toInt();
		if(th->onStage)
			th->requestInvalidation(getSys());
		else
			th->updateSizes();
	}
	//else do nothing as the height is determined by autoSize
	return NULL;
}

ASFUNCTIONBODY(TextField,_getTextWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->textWidth);
}

ASFUNCTIONBODY(TextField,_getTextHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->textHeight);
}

ASFUNCTIONBODY(TextField,_getText)
{
	TextField* th=Class<TextField>::cast(obj);
	return Class<ASString>::getInstanceS(th->text);
}

ASFUNCTIONBODY(TextField,_setText)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->updateText(args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(TextField, appendText)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->updateText(th->text + args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(TextField,_getTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	TextFormat *format=Class<TextFormat>::getInstanceS();

	format->color=_MNR(abstract_ui(th->textColor.toUInt()));
	format->font = th->font;
	format->size = th->fontSize;

	LOG(LOG_NOT_IMPLEMENTED, "getTextFormat is not fully implemeted");

	return format;
}

ASFUNCTIONBODY(TextField,_setTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	_NR<TextFormat> tf;
	int beginIndex;
	int endIndex;

	ARG_UNPACK(tf)(beginIndex, -1)(endIndex, -1);

	if(beginIndex!=-1 || endIndex!=-1)
		LOG(LOG_NOT_IMPLEMENTED,"setTextFormat with beginIndex or endIndex");

	if(tf->color)
		th->textColor = tf->color->toUInt();
	if (tf->font != "")
		th->font = tf->font;
	th->fontSize = tf->size;

	LOG(LOG_NOT_IMPLEMENTED,"setTextFormat does not read all fields of TextFormat");
	return NULL;
}

ASFUNCTIONBODY(TextField,_getDefaultTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	
	TextFormat* tf = Class<TextFormat>::getInstanceS();
	tf->font = th->font;
	LOG(LOG_NOT_IMPLEMENTED,"getDefaultTextFormat does not get all fields of TextFormat");
	return tf;
}

ASFUNCTIONBODY(TextField,_setDefaultTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	_NR<TextFormat> tf;

	ARG_UNPACK(tf);

	if(tf->color)
		th->textColor = tf->color->toUInt();
	if (tf->font != "")
		th->font = tf->font;
	th->fontSize = tf->size;
	LOG(LOG_NOT_IMPLEMENTED,"setDefaultTextFormat does not set all fields of TextFormat");
	return NULL;
}

void TextField::updateSizes()
{
	uint32_t w,h,tw,th;
	w = width;
	h = height;
	//Compute (text)width, (text)height
	CairoPangoRenderer::getBounds(*this, w, h, tw, th);
	width = w; //TODO: check the case when w,h == 0
	textWidth=tw;
	height = h;
	textHeight=th;
}

void TextField::updateText(const tiny_string& new_text)
{
	text = new_text;
	if(onStage)
		requestInvalidation(getSys());
	else
		updateSizes();
}

void TextField::requestInvalidation(InvalidateQueue* q)
{
	incRef();
	updateSizes();
	q->addToInvalidateQueue(_MR(this));
}

IDrawable* TextField::invalidate(DisplayObject* target, const MATRIX& initialMatrix)
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return NULL;
	}

	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;
	computeMasksAndMatrix(target, masks, totalMatrix);
	totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
	if(width==0 || height==0)
		return NULL;
	if(totalMatrix.getScaleX() != 1 || totalMatrix.getScaleY() != 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextField when scaled is not correctly implemented");
	/**  TODO: The scaling is done differently for textfields : height changes are applied directly
		on the font size. In some cases, it can change the width (if autosize is on and wordwrap off).
		Width changes do not change the font size, and do nothing when autosize is on and wordwrap off.
		Currently, the TextField is stretched in case of scaling.
	*/
	return new CairoPangoRenderer(*this,
				totalMatrix, x, y, width, height, 1.0f,
				getConcatenatedAlpha(), masks);
}

void TextField::renderImpl(RenderContext& ctxt) const
{
	defaultRender(ctxt);
}

void TextFieldAutoSize ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"),DECLARED_TRAIT);
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"),DECLARED_TRAIT);
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"),DECLARED_TRAIT);
}

void TextFieldType::sinit(Class_base* c)
{
	c->setVariableByQName("DYNAMIC","",Class<ASString>::getInstanceS("dynamic"),DECLARED_TRAIT);
	c->setVariableByQName("INPUT","",Class<ASString>::getInstanceS("input"),DECLARED_TRAIT);
}

void TextFormatAlign ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"),DECLARED_TRAIT);
	c->setVariableByQName("END","",Class<ASString>::getInstanceS("end"),DECLARED_TRAIT);
	c->setVariableByQName("JUSTIFY","",Class<ASString>::getInstanceS("justify"),DECLARED_TRAIT);
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"),DECLARED_TRAIT);
	c->setVariableByQName("START","",Class<ASString>::getInstanceS("start"),DECLARED_TRAIT);
}

void TextFormat::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,font);
	REGISTER_GETTER_SETTER(c,size);
}
ASFUNCTIONBODY(TextFormat,_constructor)
{
	TextFormat* th=static_cast<TextFormat*>(obj);
	tiny_string font;
	int32_t size;
	_NR<ASObject> color;
	ARG_UNPACK (font, "")(size, 12)(color,_MNR(getSys()->getNullRef()));
	th->font = font;
	th->size = size;
	th->color = color;
	LOG(LOG_NOT_IMPLEMENTED,"TextFormat: not all properties are set in constructor");
	return NULL;
}



ASFUNCTIONBODY_GETTER_SETTER(TextFormat,color);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,font);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,size);

void TextFormat::buildTraits(ASObject* o)
{
}

void StyleSheet::finalize()
{
	EventDispatcher::finalize();
	styles.clear();
}

void StyleSheet::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("styleNames","",Class<IFunction>::getFunction(_getStyleNames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("setStyle","",Class<IFunction>::getFunction(setStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getStyle","",Class<IFunction>::getFunction(getStyle),NORMAL_METHOD,true);
}

void StyleSheet::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(StyleSheet,setStyle)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==2);
	const tiny_string& arg0=args[0]->toString();
	args[1]->incRef(); //TODO: should make a copy, see reference
	map<tiny_string, _R<ASObject>>::iterator it=th->styles.find(arg0);
	//NOTE: we cannot use the [] operator as References cannot be non initialized
	if(it!=th->styles.end()) //Style already exists
		it->second=_MR(args[1]);
	else
		th->styles.insert(make_pair(arg0,_MR(args[1])));
	return NULL;
}

ASFUNCTIONBODY(StyleSheet,getStyle)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	map<tiny_string, _R<ASObject>>::iterator it=th->styles.find(arg0);
	if(it!=th->styles.end()) //Style already exists
	{
		//TODO: should make a copy, see reference
		it->second->incRef();
		return it->second.getPtr();
	}
	else
	{
		// Tested behaviour is to return an empty ASObject
		// instead of Null as is said in the documentation
		return Class<ASObject>::getInstanceS();
	}
	return NULL;
}

ASFUNCTIONBODY(StyleSheet,_getStyleNames)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==0);
	Array* ret=Class<Array>::getInstanceS();
	map<tiny_string, _R<ASObject>>::const_iterator it=th->styles.begin();
	for(;it!=th->styles.end();++it)
		ret->push(_MR(Class<ASString>::getInstanceS(it->first)));
	return ret;
}

void StaticText::sinit(Class_base* c)
{
	//TODO: spec says that constructor should throw ArgumentError
	c->setConstructor(NULL);
	c->setSuper(Class<InteractiveObject>::getRef());
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(_getText),GETTER_METHOD,true);
}

ASFUNCTIONBODY(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	return Class<ASString>::getInstanceS("");
}

void FontStyle::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("BOLD","",Class<ASString>::getInstanceS("bold"),DECLARED_TRAIT);
	c->setVariableByQName("BOLD_ITALIC","",Class<ASString>::getInstanceS("boldItalic"),DECLARED_TRAIT);
	c->setVariableByQName("ITALIC","",Class<ASString>::getInstanceS("italic"),DECLARED_TRAIT);
	c->setVariableByQName("REGULAR","",Class<ASString>::getInstanceS("regular"),DECLARED_TRAIT);
}

void FontType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("DEVICE","",Class<ASString>::getInstanceS("device"),DECLARED_TRAIT);
	c->setVariableByQName("EMBEDDED","",Class<ASString>::getInstanceS("embedded"),DECLARED_TRAIT);
	c->setVariableByQName("EMBEDDED_CFF","",Class<ASString>::getInstanceS("embeddedCFF"),DECLARED_TRAIT);
}

void TextDisplayMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("CRT","",Class<ASString>::getInstanceS("crt"),DECLARED_TRAIT);
	c->setVariableByQName("DEFAULT","",Class<ASString>::getInstanceS("default"),DECLARED_TRAIT);
	c->setVariableByQName("LCD","",Class<ASString>::getInstanceS("lcd"),DECLARED_TRAIT);
}

void TextColorType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("DARK_COLOR","",Class<ASString>::getInstanceS("dark"),DECLARED_TRAIT);
	c->setVariableByQName("LIGHT_COLOR","",Class<ASString>::getInstanceS("light"),DECLARED_TRAIT);
}

void GridFitType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
	c->setVariableByQName("PIXEL","",Class<ASString>::getInstanceS("pixel"),DECLARED_TRAIT);
	c->setVariableByQName("SUBPIXEL","",Class<ASString>::getInstanceS("subpixel"),DECLARED_TRAIT);
}
