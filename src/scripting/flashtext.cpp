/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "flashtext.h"
#include "class.h"
#include "compat.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "backends/graphics.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.text");

REGISTER_CLASS_NAME2(lightspark::Font,"Font","flash.text");
REGISTER_CLASS_NAME(TextField);
REGISTER_CLASS_NAME(TextFieldType);
REGISTER_CLASS_NAME(TextFieldAutoSize);
REGISTER_CLASS_NAME(TextFormatAlign);
REGISTER_CLASS_NAME(TextFormat);
REGISTER_CLASS_NAME(StyleSheet);
REGISTER_CLASS_NAME(StaticText);

void lightspark::Font::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->setDeclaredMethodByQName("enumerateFonts","",Class<IFunction>::getFunction(enumerateFonts),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(lightspark::Font,enumerateFonts)
{
	return Class<Array>::getInstanceS();
}

void TextField::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_getText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_setText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendText","",Class<IFunction>::getFunction(TextField:: appendText),NORMAL_METHOD,true);
}

void TextField::buildTraits(ASObject* o)
{
}

_NR<InteractiveObject> TextField::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y)
{
	return NullRef;
}

bool TextField::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
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
	th->width=args[0]->toInt();
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
	th->height=args[0]->toInt();
	return NULL;
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

void TextField::updateText(const tiny_string& new_text)
{
	text = new_text;
	requestInvalidation();
}

void TextField::requestInvalidation()
{
	incRef();
	sys->addToInvalidateQueue(_MR(this));
}

void TextField::invalidate()
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return;
	}

	computeDeviceBoundsForRect(bxmin,bxmax,bymin,bymax,x,y,width,height);
	if(width==0 || height==0)
		return;
	CairoPangoRenderer* r=new CairoPangoRenderer(this, cachedSurface, *this,
				getConcatenatedMatrix(), x, y, width, height, 1.0f,
				getConcatenatedAlpha());
	sys->addJob(r);
}

void TextField::renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
{
	if(!isSimple())
		rt->glAcquireTempBuffer(t1,t2,t3,t4);

	defaultRender(maskEnabled);

	if(!isSimple())
		rt->glBlitTempBuffer(t1,t2,t3,t4);
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
	c->setVariableByQName("JUSTIFY","",Class<ASString>::getInstanceS("justify"),DECLARED_TRAIT);
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"),DECLARED_TRAIT);
}

void TextFormat::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
}

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
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
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
		return new Null;
	return NULL;
}

ASFUNCTIONBODY(StyleSheet,_getStyleNames)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==0);
	Array* ret=Class<Array>::getInstanceS();
	map<tiny_string, _R<ASObject>>::const_iterator it=th->styles.begin();
	for(;it!=th->styles.end();++it)
		ret->push(Class<ASString>::getInstanceS(it->first));
	return ret;
}

void StaticText::sinit(Class_base* c)
{
	//TODO: spec says that constructor should throw ArgumentError
	c->setConstructor(NULL);
	c->super=Class<InteractiveObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(_getText),GETTER_METHOD,true);
}

ASFUNCTIONBODY(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	return Class<ASString>::getInstanceS("");
}
