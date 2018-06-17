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

#include "scripting/argconv.h"
#include "scripting/toplevel/JSON.h"

using namespace std;
using namespace lightspark;

JSON::JSON(Class_base* c):ASObject(c)
{
}


void JSON::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("parse","",Class<IFunction>::getFunction(c->getSystemState(),_parse,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("stringify","",Class<IFunction>::getFunction(c->getSystemState(),_stringify,3),NORMAL_METHOD,false);
}
void JSON::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(JSON,_constructor)
{
	throwError<ArgumentError>(kCantInstantiateError);
}
ASFUNCTIONBODY_ATOM(JSON,generator)
{
	throwError<ArgumentError>(kCoerceArgumentCountError);
	ret = asAtom::invalidAtom;
}

ASObject *JSON::doParse(const tiny_string &jsonstring, asAtom reviver)
{
	ASObject* res = NULL;
	multiname dummy(NULL);
	
	parseAll(jsonstring,&res,dummy,reviver);
	return res;
}

ASFUNCTIONBODY_ATOM(JSON,_parse)
{
	tiny_string text;
	asAtom reviver;

	if (argslen > 0 && (args[0].is<Null>() ||args[0].is<Undefined>()))
		throwError<SyntaxError>(kJSONInvalidParseInput);
	ARG_UNPACK_ATOM_MORE_ALLOWED(text);
	if (argslen > 1)
	{
		if (!args[1].is<IFunction>())
			throwError<TypeError>(kCheckTypeFailedError);
		reviver = args[1];
	}
	ASObject* res = doParse(text,reviver);
	if (!res)
		throwError<TypeError>(kJSONInvalidParseInput);
	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(JSON,_stringify)
{
	_NR<ASObject> value;
	ARG_UNPACK_ATOM_MORE_ALLOWED(value);
	std::vector<ASObject *> path;
	tiny_string filter;
	asAtom replacer;
	if (argslen > 1 && args[1].type != T_NULL && args[1].type != T_UNDEFINED)
	{
		if (args[1].type == T_FUNCTION)
		{
			replacer = args[1];
		}
		else if (args[1].type == T_ARRAY)
		{
			filter = " ";
			Array* ar = args[1].as<Array>();
			for (uint64_t i = 0; i < ar->size(); i++)
			{
				filter += ar->at(i).toString(sys);
				filter += " ";
			}
		}
		else
			throwError<TypeError>(kJSONInvalidReplacer);
	}

	tiny_string spaces = "";
	if (argslen > 2)
	{
		asAtom space = args[2];
		spaces = "          ";
		if (space.is<Number>() || space.is<Integer>() || space.is<UInteger>())
		{
			int32_t v = space.toInt();
			if (v < 0) v = 0;
			if (v > 10) v = 10;
			spaces = spaces.substr_bytes(0,v);
		}
		else if (space.is<Boolean>() || space.is<Null>())
		{
			spaces = "";
		}
		else
		{
			if(space.getObject() && space.getObject()->has_toString())
			{
				asAtom ret;
				space.getObject()->call_toString(ret);
				spaces = ret.toString(sys);
			}
			else
				spaces = space.toString(sys);
			if (spaces.numBytes() > 10)
				spaces = spaces.substr_bytes(0,10);
		}
	}
	tiny_string res = value->toJSON(path,replacer,spaces,filter);

	ret = asAtom::fromObject(abstract_s(sys,res));
}
void JSON::parseAll(const tiny_string &jsonstring, ASObject** parent , const multiname& key, asAtom reviver)
{
	int len = jsonstring.numChars();
	int pos = 0;
	while (pos < len)
	{
		if (*parent && (*parent)->isPrimitive())
			throwError<SyntaxError>(kJSONInvalidParseInput);
		pos = parse(jsonstring, pos, parent , key, reviver);
		while (jsonstring.charAt(pos) == ' ' ||
			   jsonstring.charAt(pos) == '\t' ||
			   jsonstring.charAt(pos) == '\n' ||
			   jsonstring.charAt(pos) == '\r'
			   )
			pos++;
	}
}
int JSON::parse(const tiny_string &jsonstring, int pos, ASObject** parent , const multiname& key, asAtom reviver)
{
	while (jsonstring.charAt(pos) == ' ' ||
		   jsonstring.charAt(pos) == '\t' ||
		   jsonstring.charAt(pos) == '\n' ||
		   jsonstring.charAt(pos) == '\r'
		   )
		   pos++;
	int len = jsonstring.numChars();
	if (pos < len)
	{
		char c = jsonstring.charAt(pos);
		switch(c)
		{
			case '{':
				pos = parseObject(jsonstring,pos,parent,key, reviver);
				break;
			case '[': 
				pos = parseArray(jsonstring,pos,parent,key, reviver);
				break;
			case '"':
				pos = parseString(jsonstring,pos,parent,key);
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
				pos = parseNumber(jsonstring,pos,parent,key);
				break;
			case 't':
				pos = parseTrue(jsonstring,pos,parent,key);
				break;
			case 'f':
				pos = parseFalse(jsonstring,pos,parent,key);
				break;
			case 'n':
				pos = parseNull(jsonstring,pos,parent,key);
				break;
			default:
				throwError<SyntaxError>(kJSONInvalidParseInput);
		}
	}
	if (reviver.type != T_INVALID)
	{
		bool haskey = key.name_type!= multiname::NAME_OBJECT;
		asAtom params[2];
		
		if (haskey)
		{
			params[0] = asAtom::fromObject(abstract_s(getSys(),key.normalizedName(getSys())));
			if ((*parent)->hasPropertyByMultiname(key,true,false))
			{
				(*parent)->getVariableByMultiname(params[1],key);
				ASATOM_INCREF(params[1]);
			}
			else
				params[1] = asAtom::nullAtom;
		}
		else
		{
			params[0] = asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
			params[1] = asAtom::fromObject(*parent);
			ASATOM_INCREF(params[1]);
		}

		asAtom funcret;
		reviver.callFunction(funcret,asAtom::nullAtom, params, 2,true);
		if(funcret.type != T_INVALID)
		{
			if (haskey)
			{
				if (funcret.type == T_UNDEFINED)
				{
					(*parent)->deleteVariableByMultiname(key);
					ASATOM_DECREF(funcret);
				}
				else
				{
					(*parent)->setVariableByMultiname(key,funcret,ASObject::CONST_NOT_ALLOWED);
				}
			}
			else 
				*parent= funcret.toObject(getSys());
		}
	}
	return pos;
}
int JSON::parseTrue(const tiny_string &jsonstring, int pos,ASObject** parent,const multiname& key)
{
	int len = jsonstring.numChars();
	if (len >= pos+4)
	{
		if (jsonstring.charAt(pos) == 't' && 
				jsonstring.charAt(pos + 1) == 'r' &&
				jsonstring.charAt(pos + 2) == 'u' && 
				jsonstring.charAt(pos + 3) == 'e')
		{
			pos += 4;
			if (*parent == NULL)
				*parent = abstract_b(getSys(),true);
			else
			{
				asAtom v(true);
				(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
			}
		}
		else
			throwError<SyntaxError>(kJSONInvalidParseInput);
	}
	else
		throwError<SyntaxError>(kJSONInvalidParseInput);
	return pos;
}
int JSON::parseFalse(const tiny_string &jsonstring, int pos,ASObject** parent,const multiname& key)
{
	int len = jsonstring.numChars();
	if (len >= pos+5)
	{
		if (jsonstring.charAt(pos) == 'f' && 
				jsonstring.charAt(pos + 1) == 'a' &&
				jsonstring.charAt(pos + 2) == 'l' && 
				jsonstring.charAt(pos + 3) == 's' && 
				jsonstring.charAt(pos + 4) == 'e')
		{
			pos += 5;
			if (*parent == NULL)
				*parent = abstract_b(getSys(),false);
			else 
			{
				asAtom v(false);
				(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
			}
		}
		else
			throwError<SyntaxError>(kJSONInvalidParseInput);
	}
	else
		throwError<SyntaxError>(kJSONInvalidParseInput);
	return pos;
}
int JSON::parseNull(const tiny_string &jsonstring, int pos,ASObject** parent,const multiname& key)
{
	int len = jsonstring.numChars();
	if (len >= pos+4)
	{
		if (jsonstring.charAt(pos) == 'n' && 
				jsonstring.charAt(pos + 1) == 'u' &&
				jsonstring.charAt(pos + 2) == 'l' && 
				jsonstring.charAt(pos + 3) == 'l')
		{
			pos += 4;
			if (*parent == NULL)
				*parent = getSys()->getNullRef();
			else 
				(*parent)->setVariableByMultiname(key,asAtom::nullAtom,ASObject::CONST_NOT_ALLOWED);
		}
		else
			throwError<SyntaxError>(kJSONInvalidParseInput);
	}
	else
		throwError<SyntaxError>(kJSONInvalidParseInput);
	return pos;
}
int JSON::parseString(const tiny_string &jsonstring, int pos,ASObject** parent,const multiname& key, tiny_string* result)
{
	pos++; // ignore starting quotes
	int len = jsonstring.numChars();
	if (pos >= len)
		throwError<SyntaxError>(kJSONInvalidParseInput);

	tiny_string sub = jsonstring.substr(pos,len-pos);
	
	tiny_string res;
	bool done = false;
	for (CharIterator it=sub.begin(); it!=sub.end(); it++)
	{
		pos++;
		if (*it == '\"')
		{
			done = true;
			break;
		}
		else if(*it == '\\')
		{
			it++;
			pos++;
			if(it == sub.end())
				break;
			if(*it == '\"')
				res += '\"';
			else if(*it == '\\')
				res += '\\';
			else if(*it == '/')
				res += '/';
			else if(*it == 'b')
				res += '\b';
			else if(*it == 'f')
				res += '\f';
			else if(*it == 'n')
				res += '\n';
			else if(*it == 'r')
				res += '\r';
			else if(*it == 't')
				res += '\t';
			else if(*it == 'u')
			{
				tiny_string strhex;
				for (int i = 0; i < 4; i++)
				{
					it++; pos++; 
					if (it==sub.end()) 
						throwError<SyntaxError>(kJSONInvalidParseInput);
					switch(*it)
					{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							strhex += *it;
							break;
						default:
							throwError<SyntaxError>(kJSONInvalidParseInput);
					}
					if (it==sub.end()) 
						throwError<SyntaxError>(kJSONInvalidParseInput);
				}
				number_t hexnum;
				if (Integer::fromStringFlashCompatible(strhex.raw_buf(),hexnum,16))
				{
					if (hexnum < 0x20 && hexnum != 0xf)
						throwError<SyntaxError>(kJSONInvalidParseInput);
					res += tiny_string::fromChar(hexnum);
				}
				else
					break;
			}
			else
				throwError<SyntaxError>(kJSONInvalidParseInput);
		}
		else if (*it < 0x20)
		{
			throwError<SyntaxError>(kJSONInvalidParseInput);
		}
		else
		{
			res += *it;
		}
	}
	if (!done)
		throwError<SyntaxError>(kJSONInvalidParseInput);
	
	if (parent != NULL)
	{
		if (*parent == NULL)
			*parent = abstract_s(getSys(),res);
		else 
		{
			asAtom v = asAtom::fromObject(abstract_s(getSys(),res));
			(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
		}
	}
	if (result)
		*result =res;
	return pos;
}
int JSON::parseNumber(const tiny_string &jsonstring, int pos, ASObject** parent, const multiname& key)
{
	int len = jsonstring.numChars();
	tiny_string res;
	bool done = false;
	while (!done && pos < len)
	{
		char c = jsonstring.charAt(pos);
		switch(c)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
			case '+':
			case '.':
			case 'E':
			case 'e':
				res += c;
				pos++;
				break;
			default:
				done = true;
				break;
		}
	}
	ASObject* numstr = abstract_s(getSys(),res);
	number_t num = numstr->toNumber();

	if (std::isnan(num))
		throwError<SyntaxError>(kJSONInvalidParseInput);

	if (*parent == NULL)
		*parent = abstract_d(getSys(),num);
	else 
	{
		asAtom v(num);
		(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
	}
	return pos;
}
int JSON::parseObject(const tiny_string &jsonstring, int pos, ASObject** parent, const multiname& key, asAtom reviver)
{
	int len = jsonstring.numChars();
	pos++; // ignore '{' or ','
	ASObject* subobj = Class<ASObject>::getInstanceS(getSys());
	if (*parent == NULL)
		*parent = subobj;
	else 
	{
		asAtom v = asAtom::fromObject(subobj);
		(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
	}
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind(getSys(),"",NAMESPACE));
	name.isAttribute = false;
	bool done = false;
	bool bfirst = true;
	bool needkey = true;
	bool needvalue = false;

	while (!done && pos < len)
	{
		while (jsonstring.charAt(pos) == ' ' ||
			   jsonstring.charAt(pos) == '\t' ||
			   jsonstring.charAt(pos) == '\n' ||
			   jsonstring.charAt(pos) == '\r'
			   )
			pos++;
		char c = jsonstring.charAt(pos);
		switch(c)
		{
			case '}':
				if (!bfirst && (needkey || needvalue))
					throwError<SyntaxError>(kJSONInvalidParseInput);
				done = true;
				pos++;
				break;
			case '\"':
				{
					tiny_string keyname;
					pos = parseString(jsonstring,pos,NULL,name,&keyname);
					name.name_s_id=getSys()->getUniqueStringId(keyname);
					needkey = false;
					needvalue = true;
				}
				break;
			case ',':
				if (needkey || needvalue)
					throwError<SyntaxError>(kJSONInvalidParseInput);
				pos++;
				name.name_s_id=0;
				needkey = true;
				break;
			case ':':
				pos++;
				pos = parse(jsonstring,pos,&subobj,name,reviver);
				needvalue = false;
				break;
			default:
				throwError<SyntaxError>(kJSONInvalidParseInput);
		}
		bfirst=false;
	}
	if (!done)
		throwError<SyntaxError>(kJSONInvalidParseInput);

	return pos;
}

int JSON::parseArray(const tiny_string &jsonstring, int pos, ASObject** parent, const multiname& key, asAtom reviver)
{
	int len = jsonstring.numChars();
	pos++; // ignore '['
	ASObject* subobj = Class<Array>::getInstanceSNoArgs(getSys());
	if (*parent == NULL)
		*parent = subobj;
	else 
	{
		asAtom v = asAtom::fromObject(subobj);
		(*parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED);
	}
	multiname name(NULL);
	name.name_type=multiname::NAME_UINT;
	name.name_ui = 0;
	name.ns.push_back(nsNameAndKind(getSys(),"",NAMESPACE));
	name.isAttribute = false;
	bool done = false;
	bool needdata = false;
	while (!done && pos < len)
	{
		while (jsonstring.charAt(pos) == ' ' ||
			   jsonstring.charAt(pos) == '\t' ||
			   jsonstring.charAt(pos) == '\n' ||
			   jsonstring.charAt(pos) == '\r'
			   )
			pos++;
		char c = jsonstring.charAt(pos);
		switch(c)
		{
			case ']':
				done = true;
				pos++;
				break;
			case ',':
				name.name_ui++;
				needdata = true;
				pos++;
				break;
			default:
				pos = parse(jsonstring,pos,&subobj,name, reviver);
				needdata = false;
				break;
		}
	}
	if (!done || needdata)
		throwError<SyntaxError>(kJSONInvalidParseInput);

	return pos;
}




/***** 

static QString sanitizeString(QString str)
{
        str.replace(QLatin1String("\\"), QLatin1String("\\\\"));
        str.replace(QLatin1String("\""), QLatin1String("\\\""));
        str.replace(QLatin1String("\b"), QLatin1String("\\b"));
        str.replace(QLatin1String("\f"), QLatin1String("\\f"));
        str.replace(QLatin1String("\n"), QLatin1String("\\n"));
        str.replace(QLatin1String("\r"), QLatin1String("\\r"));
        str.replace(QLatin1String("\t"), QLatin1String("\\t"));
        return QString(QLatin1String("\"%1\"")).arg(str);
}

static QByteArray join(const QList<QByteArray> &list, const QByteArray &sep)
{
        QByteArray res;
        Q_FOREACH(const QByteArray &i, list)
        {
                if(!res.isEmpty())
                {
                        res += sep;
                }
                res += i;
        }
        return res;
}



static DSVariantList parseArray(const QString &json, int &index, bool &success)
{
        DSVariantList list;

        nextToken(json, index);

        bool done = false;
        while(!done)
        {
                int token = lookAhead(json, index);

                if(token == JsonTokenNone)
                {
                        success = false;
                        return DSVariantList();
                }
                else if(token == JsonTokenComma)
                {
                        nextToken(json, index);
                }
                else if(token == JsonTokenSquaredClose)
                {
                        nextToken(json, index);
                        break;
                }
                else
                {
                        QVariant value = parseValue(json, index, success);

                        if(!success)
                        {
                            return DSVariantList();
                        }

                        list.push_back(value);
                }
        }

        return list;
}


static QVariant parseNumber(const QString &json, int &index)
{
        eatWhitespace(json, index);

        int lastIndex = lastIndexOfNumber(json, index);
        int charLength = (lastIndex - index) + 1;
        QString numberStr;

        numberStr = json.mid(index, charLength);

        index = lastIndex + 1;

        if (numberStr.contains('.')) {
                return QVariant(numberStr.toDouble(NULL));
        } else if (numberStr.startsWith('-')) {
                return QVariant(numberStr.toLongLong(NULL));
        } else {
                return QVariant(numberStr.toULongLong(NULL));
        }
}

static int lastIndexOfNumber(const QString &json, int index)
{
        int lastIndex;

        for(lastIndex = index; lastIndex < json.size(); lastIndex++)
        {
                if(QString("0123456789+-.eE").indexOf(json[lastIndex]) == -1)
                {
                        break;
                }
        }

        return lastIndex -1;
}



static int lookAhead(const QString &json, int index)
{
        int saveIndex = index;
        return nextToken(json, saveIndex);
}

static int nextToken(const QString &json, int &index)
{
        eatWhitespace(json, index);

        if(index == json.size())
        {
                return JsonTokenNone;
        }

        QChar c = json[index];
        index++;
        switch(c.toLatin1())
        {
                case '{': return JsonTokenCurlyOpen;
                case '}': return JsonTokenCurlyClose;
                case '[': return JsonTokenSquaredOpen;
                case ']': return JsonTokenSquaredClose;
                case ',': return JsonTokenComma;
                case '"': return JsonTokenString;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                case '-': return JsonTokenNumber;
                case ':': return JsonTokenColon;
        }

        index--;

        int remainingLength = json.size() - index;

        //True
        if(remainingLength >= 4)
        {
                if (json[index] == 't' && json[index + 1] == 'r' &&
                        json[index + 2] == 'u' && json[index + 3] == 'e')
                {
                        index += 4;
                        return JsonTokenTrue;
                }
        }

        //False
        if (remainingLength >= 5)
        {
                if (json[index] == 'f' && json[index + 1] == 'a' &&
                        json[index + 2] == 'l' && json[index + 3] == 's' &&
                        json[index + 4] == 'e')
                {
                        index += 5;
                        return JsonTokenFalse;
                }
        }

        //Null
        if (remainingLength >= 4)
        {
                if (json[index] == 'n' && json[index + 1] == 'u' &&
                        json[index + 2] == 'l' && json[index + 3] == 'l')
                {
                        index += 4;
                        return JsonTokenNull;
                }
        }

        return JsonTokenNone;
}
*****/
