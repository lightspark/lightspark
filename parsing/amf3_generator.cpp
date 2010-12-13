/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "amf3_parser.h"
#include "amf3_generator.h"
#include "scripting/toplevel.h"
#include "scripting/class.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace lightspark;

static tiny_string getString(Amf3Deserializer* th, const amf3::Utf8String& s)
{
	if(s.contents==amf3::Utf8String::REFERENCE)
		return "TODO_support_references";
	return s.value;
}

static ASObject* createArray(Amf3Deserializer* th, const amf3::ArrayType&);

static ASObject* createObject(Amf3Deserializer* th, const amf3::ValueType& v)
{
	switch(v.contents)
	{
		case amf3::ValueType::ARRAYTYPE:
			return createArray(th, v.arrayVal);
			break;
		default:
			cout << "Fail " << v.contents << endl;
			exit(-1);
	}
	return NULL;
}

static ASObject* createArray(Amf3Deserializer* th, const amf3::ArrayType& t)
{
	Array* ret=Class<Array>::getInstanceS();
	if(t.contents==amf3::ArrayType::REFERENCE)
	{
		LOG(LOG_NOT_IMPLEMENTED,"References in AMF3 not supported");
		return ret;
	}
	const amf3::Array& a=t.value.get();
	for(uint32_t i=0;i<a.m_associativeSection.size();i++)
	{
		const tiny_string& varName=getString(th, boost::fusion::at_c<0>(a.m_associativeSection[i]));
		ASObject* obj=createObject(th, boost::fusion::at_c<1>(a.m_associativeSection[i]));
		ret->setVariableByQName(varName,"",obj);
	}
	assert(a.m_denseSection.empty());
/*	o << "Dense array " << std::endl;
	for(uint32_t i=0;i<r.m_denseSection.size();i++)
	{
		o << r.m_denseSection[i] << endl;
	}
	o << "=========" << std::endl;*/
	return ret;
}

bool Amf3Deserializer::generateObjects(std::vector<ASObject*>& objs)
{
	vector<amf3::ValueType> ans;
	amf3::Amf3ParserGrammar theParser;

	bool parserResult = boost::spirit::qi::parse(start, end, theParser, ans);
	if(!parserResult || start!=end) //The whole thing must be consumed
		return false;

	//Now create an object for each parsed value
	for (uint32_t i=0;i < ans.size();++i)
		objs.push_back(createObject(this, ans[i]));
	return true;
}

/*int main(int argc, char **argv) {
	std::ifstream input ("../amf3.bin");
	input.unsetf(std::ios::skipws);
	assert(input);
	char buf[4096];
	input.read(buf,4096);
	int available=input.gcount();
	assert(available<4096);
	input.close();
	char* start=buf;
	char* end=buf+available;
	Amf3Deserializer d(start,end);
	std::vector<ASObject*> puppa;
	d.generateObjects(puppa);
	
    return 0;
}*/
