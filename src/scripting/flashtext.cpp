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
	c->setMethodByQName("enumerateFonts","",Class<IFunction>::getFunction(enumerateFonts),true);
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
	c->setGetterByQName("width","",Class<IFunction>::getFunction(TextField::_getWidth),true);
	c->setSetterByQName("width","",Class<IFunction>::getFunction(TextField::_setWidth),true);
	c->setGetterByQName("height","",Class<IFunction>::getFunction(TextField::_getHeight),true);
	c->setSetterByQName("height","",Class<IFunction>::getFunction(TextField::_setHeight),true);
	c->setGetterByQName("text","",Class<IFunction>::getFunction(TextField::_getText),true);
	c->setSetterByQName("text","",Class<IFunction>::getFunction(TextField::_setText),true);
	c->setMethodByQName("appendText","",Class<IFunction>::getFunction(TextField:: appendText),true);
}

void TextField::buildTraits(ASObject* o)
{
}

bool TextField::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
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
	th->text=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(TextField, appendText)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->text += args[0]->toString();
	return NULL;
}

void TextField::Render(bool maskEnabled)
{
	//TODO: implement
	LOG(LOG_NOT_IMPLEMENTED,_("TextField::Render ") << text);
}

void TextFieldAutoSize ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"));
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"));
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"));
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"));
}

void TextFieldType ::sinit(Class_base* c)
{
	c->setVariableByQName("DYNAMIC","",Class<ASString>::getInstanceS("dynamic"));
	c->setVariableByQName("INPUT","",Class<ASString>::getInstanceS("input"));
}

void TextFormatAlign ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"));
	c->setVariableByQName("JUSTIFY","",Class<ASString>::getInstanceS("justify"));
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"));
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"));
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
	c->setGetterByQName("styleNames","",Class<IFunction>::getFunction(_getStyleNames),true);
	c->setMethodByQName("setStyle","",Class<IFunction>::getFunction(setStyle),true);
	c->setMethodByQName("getStyle","",Class<IFunction>::getFunction(getStyle),true);
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
	c->setGetterByQName("text","",Class<IFunction>::getFunction(_getText),true);
}

ASFUNCTIONBODY(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	return Class<ASString>::getInstanceS("");
}
