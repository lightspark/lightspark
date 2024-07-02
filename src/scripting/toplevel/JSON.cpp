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
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;

JSON::JSON(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
}


void JSON::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("parse","",c->getSystemState()->getBuiltinFunction(_parse,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("stringify","",c->getSystemState()->getBuiltinFunction(_stringify,3),NORMAL_METHOD,false);
}
void JSON::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(JSON,_constructor)
{
	createError<ArgumentError>(wrk,kCantInstantiateError);
}
ASFUNCTIONBODY_ATOM(JSON,generator)
{
	createError<ArgumentError>(wrk,kCoerceArgumentCountError);
	ret = asAtomHandler::invalidAtom;
}

bool JSON::doParse(asAtom& res, const tiny_string &jsonstring, asAtom reviver, ASWorker* wrk)
{
	multiname dummy(nullptr);
	res = asAtomHandler::invalidAtom;
	return parseAll(jsonstring,res,dummy,reviver,wrk);
}

ASFUNCTIONBODY_ATOM(JSON,_parse)
{
	tiny_string text;
	asAtom reviver=asAtomHandler::invalidAtom;

	if (argslen > 0 && (asAtomHandler::is<Null>(args[0]) ||asAtomHandler::is<Undefined>(args[0])))
	{
		createError<SyntaxError>(wrk,kJSONInvalidParseInput);
		return;
	}
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(text));
	if (argslen > 1)
	{
		if (!asAtomHandler::is<IFunction>(args[1]))
		{
			createError<TypeError>(wrk,kCheckTypeFailedError);
			return;
		}
		reviver = args[1];
	}
	if (!doParse(ret,text,reviver,wrk))
	{
		if (wrk->currentCallContext && !wrk->currentCallContext->exceptionthrown)
			createError<SyntaxError>(wrk,kJSONInvalidParseInput);
		return;
	}
}

ASFUNCTIONBODY_ATOM(JSON,_stringify)
{
	asAtom value= asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(value));
	std::vector<ASObject *> path;
	tiny_string filter;
	asAtom replacer=asAtomHandler::invalidAtom;
	if (argslen > 1 && !asAtomHandler::isNull(args[1]) && !asAtomHandler::isUndefined(args[1]))
	{
		if (asAtomHandler::isFunction(args[1]))
		{
			replacer = args[1];
		}
		else if (asAtomHandler::isArray(args[1]))
		{
			filter = " ";
			Array* ar = asAtomHandler::as<Array>(args[1]);
			for (uint64_t i = 0; i < ar->size(); i++)
			{
				asAtom a = ar->at(i);
				filter += asAtomHandler::toString(a,wrk);
				filter += " ";
			}
		}
		else
		{
			createError<TypeError>(wrk,kJSONInvalidReplacer);
			return;
		}
	}

	tiny_string spaces = "";
	if (argslen > 2)
	{
		asAtom space = args[2];
		spaces = "          ";
		if (asAtomHandler::is<Number>(space) || asAtomHandler::is<Integer>(space) || asAtomHandler::is<UInteger>(space))
		{
			int32_t v = asAtomHandler::toInt(space);
			if (v < 0) v = 0;
			if (v > 10) v = 10;
			spaces = spaces.substr_bytes(0,v);
		}
		else if (asAtomHandler::is<Boolean>(space) || asAtomHandler::is<Null>(space))
		{
			spaces = "";
		}
		else
		{
			if(asAtomHandler::getObject(space) && asAtomHandler::getObject(space)->has_toString())
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtomHandler::getObject(space)->call_toString(ret);
				spaces = asAtomHandler::toString(ret,wrk);
				ASATOM_DECREF(ret);
			}
			else
				spaces = asAtomHandler::toString(space,wrk);
			if (spaces.numBytes() > 10)
				spaces = spaces.substr_bytes(0,10);
		}
	}
	tiny_string res;
	if (asAtomHandler::isObject(value))
		res = asAtomHandler::getObjectNoCheck(value)->toJSON(path,replacer,spaces,filter);
	else if (asAtomHandler::isUndefined(value))
		res ="null";
	else if(asAtomHandler::isString(value))
		res = asAtomHandler::toString(value,wrk).toQuotedString();
	else
		res = asAtomHandler::toString(value,wrk);
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

bool JSON::parseAll(const tiny_string &jsonstring, asAtom& parent , multiname& key, asAtom reviver, ASWorker* wrk)
{
	CharIterator it = jsonstring.begin();
	while (it != jsonstring.end())
	{
		if (asAtomHandler::isPrimitive(parent))
			return false;
		if (!parse(jsonstring, it, parent , key, reviver,wrk))
			return false;
		while (*it == ' ' ||
			   *it == '\t' ||
			   *it == '\n' ||
			   *it == '\r'
			   )
			it++;
	}
	return true;
}
bool JSON::parse(const tiny_string &jsonstring, CharIterator& it, asAtom& parent , multiname& key, asAtom reviver, ASWorker* wrk)
{
	while (*it == ' ' ||
		   *it == '\t' ||
		   *it == '\n' ||
		   *it == '\r'
		   )
		   it++;
	if (it != jsonstring.end())
	{
		char c = *it;
		switch(c)
		{
			case '{':
				if (!parseObject(jsonstring,it,parent,key, reviver,wrk))
					return false;
				break;
			case '[': 
				if (!parseArray(jsonstring,it,parent,key, reviver,wrk))
					return false;
				break;
			case '"':
				if (!parseString(jsonstring,it,parent,key,wrk))
					return false;
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
				if (!parseNumber(jsonstring,it,parent,key,wrk))
					return false;
				break;
			case 't':
				if (!parseTrue(it,parent,key,wrk))
					return false;
				break;
			case 'f':
				if (!parseFalse(it,parent,key,wrk))
					return false;
				break;
			case 'n':
				if (!parseNull(it,parent,key,wrk))
					return false;
				break;
			default:
				return false;
		}
	}
	if (asAtomHandler::isValid(reviver))
	{
		bool haskey = key.name_type!= multiname::NAME_OBJECT;
		asAtom params[2];
		
		if (haskey)
		{
			params[0] = asAtomHandler::fromObject(abstract_s(wrk,key.normalizedName(wrk->getSystemState())));
			if (asAtomHandler::isObject(parent) && asAtomHandler::getObjectNoCheck(parent)->hasPropertyByMultiname(key,true,false,wrk))
			{
				asAtomHandler::getObjectNoCheck(parent)->getVariableByMultiname(params[1],key,GET_VARIABLE_OPTION::NONE,wrk);
			}
			else
				params[1] = asAtomHandler::nullAtom;
		}
		else
		{
			params[0] = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			params[1] = parent;
			ASATOM_INCREF(params[1]);
		}

		asAtom funcret=asAtomHandler::invalidAtom;
		asAtom closure = asAtomHandler::getClosure(reviver) ? asAtomHandler::fromObject(asAtomHandler::getClosure(reviver)) : asAtomHandler::nullAtom;
		ASATOM_INCREF(closure);
		asAtomHandler::callFunction(reviver,wrk,funcret,closure, params, 2,true);
		if(asAtomHandler::isValid(funcret))
		{
			if (haskey)
			{
				if (asAtomHandler::isUndefined(funcret))
				{
					if (asAtomHandler::isObject(parent))
						asAtomHandler::getObjectNoCheck(parent)->deleteVariableByMultiname(key,wrk);
				}
				else
				{
					if (asAtomHandler::isObject(parent))
					{
						bool alreadyset=false;
						asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,funcret,ASObject::CONST_NOT_ALLOWED,&alreadyset,wrk);
						if (alreadyset)
							ASATOM_DECREF(funcret)
					}
					else
						return false;
				}
			}
			else 
			{
				if (parent.uintval == funcret.uintval)
				{
					ASATOM_DECREF(funcret)
				}
				else
				{
					ASATOM_DECREF(parent);
					parent= funcret;
				}
			}
		}
		else
			return false;
	}
	return true;
}
bool JSON::parseTrue(CharIterator& it, asAtom& parent, multiname &key, ASWorker* wrk)
{
	if (*it++ != 't')
		return false;
	if (*it++ != 'r')
		return false;
	if (*it++ != 'u')
		return false;
	if (*it++ != 'e')
		return false;
	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::trueAtom;
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::trueAtom;
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	return true;
}
bool JSON::parseFalse(CharIterator& it, asAtom& parent, multiname &key, ASWorker* wrk)
{
	if (*it++ != 'f')
		return false;
	if (*it++ != 'a')
		return false;
	if (*it++ != 'l')
		return false;
	if (*it++ != 's')
		return false;
	if (*it++ != 'e')
		return false;
	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::falseAtom;
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::falseAtom;
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	return true;
}
bool JSON::parseNull(CharIterator& it, asAtom& parent, multiname &key, ASWorker* wrk)
{
	if (*it++ != 'n')
		return false;
	if (*it++ != 'u')
		return false;
	if (*it++ != 'l')
		return false;
	if (*it++ != 'l')
		return false;
	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::nullAtom;
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::nullAtom;
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	return true;
}
bool JSON::parseString(const tiny_string& jsonstring, CharIterator& it, asAtom& parent, multiname &key, ASWorker* wrk, tiny_string* result)
{
	it++; // ignore starting quotes
	if (it == jsonstring.end())
		return false;

	tiny_string res;
	bool done = false;
	while (it!=jsonstring.end())
	{
		if (*it == '\"')
		{
			it++;
			done = true;
			break;
		}
		else if(*it == '\\')
		{
			it++;
			if (it == jsonstring.end())
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
					it++;
					if (it == jsonstring.end())
						return false;
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
							return false;
					}
					if (it == jsonstring.end())
						return false;
				}
				number_t hexnum;
				if (Integer::fromStringFlashCompatible(strhex.raw_buf(),hexnum,16))
				{
					if (hexnum < 0x20 && hexnum != 0xf)
						return false;
					res += tiny_string::fromChar(hexnum);
				}
				else
					break;
			}
			else
				return false;
		}
		else if (*it < 0x20)
			return false;
		else
		{
			res += *it;
		}
		it++;
	}
	if (!done)
		return false;
	
	if (result)
		*result =res;
	else
	{
		if (asAtomHandler::isInvalid(parent))
			parent = asAtomHandler::fromObject(abstract_s(wrk,res));
		else if (asAtomHandler::isObject(parent))
		{
			asAtom v = asAtomHandler::fromObject(abstract_s(wrk,res));
			asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
		}
		else
			return false;
	}
	return true;
}
bool JSON::parseNumber(const tiny_string &jsonstring, CharIterator& it, asAtom& parent, multiname &key, ASWorker* wrk)
{
	tiny_string res;
	bool done = false;
	while (!done && it != jsonstring.end())
	{
		char c = *it;
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
				it++;
				break;
			default:
				done = true;
				break;
		}
	}
	ASObject* numstr = abstract_s(wrk,res);
	number_t num = numstr->toNumber();
	numstr->decRef();

	if (std::isnan(num))
		return false;

	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::fromNumber(wrk,num,false);
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::fromNumber(wrk,num,false);
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	return true;
}
bool JSON::parseObject(const tiny_string &jsonstring, CharIterator& it, asAtom& parent, multiname &key, asAtom reviver, ASWorker* wrk)
{
	it++; // ignore '{' or ','
	ASObject* subobj = Class<ASObject>::getInstanceS(wrk);
	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::fromObject(subobj);
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::fromObject(subobj);
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	name.isAttribute = false;
	bool done = false;
	bool bfirst = true;
	bool needkey = true;
	bool needvalue = false;

	while (!done && it != jsonstring.end())
	{
		while (*it == ' ' ||
			   *it == '\t' ||
			   *it == '\n' ||
			   *it == '\r'
			   )
			it++;
		char c = *it;
		switch(c)
		{
			case '}':
				if (!bfirst && (needkey || needvalue))
					return false;
				done = true;
				it++;
				break;
			case '\"':
				{
					tiny_string keyname;
					asAtom p = asAtomHandler::invalidAtom;
					if (!parseString(jsonstring,it,p,name,wrk,&keyname))
						return false;
					name.name_s_id=wrk->getSystemState()->getUniqueStringId(keyname);
					needkey = false;
					needvalue = true;
				}
				break;
			case ',':
				if (needkey || needvalue)
					return false;
				it++;
				name.name_s_id=0;
				needkey = true;
				break;
			case ':':
			{
				it++;
				asAtom p = asAtomHandler::fromObjectNoPrimitive(subobj);
				if (!parse(jsonstring,it,p,name,reviver,wrk))
					return false;
				needvalue = false;
				break;
			}
			default:
				return false;
		}
		bfirst=false;
	}
	return done;
}

bool JSON::parseArray(const tiny_string &jsonstring, CharIterator& it, asAtom& parent, multiname &key, asAtom reviver, ASWorker* wrk)
{
	it++; // ignore '['
	ASObject* subobj = Class<Array>::getInstanceSNoArgs(wrk);
	if (asAtomHandler::isInvalid(parent))
		parent = asAtomHandler::fromObject(subobj);
	else if (asAtomHandler::isObject(parent))
	{
		asAtom v = asAtomHandler::fromObject(subobj);
		asAtomHandler::getObjectNoCheck(parent)->setVariableByMultiname(key,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	else
		return false;
	multiname name(nullptr);
	name.name_type=multiname::NAME_UINT;
	name.name_ui = 0;
	name.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	name.isAttribute = false;
	bool done = false;
	bool needdata = false;
	while (!done && it != jsonstring.end())
	{
		while (*it == ' ' ||
			   *it == '\t' ||
			   *it == '\n' ||
			   *it == '\r'
			   )
			it++;
		char c = *it;
		switch(c)
		{
			case ']':
				done = true;
				it++;
				break;
			case ',':
				name.name_ui++;
				needdata = true;
				it++;
				break;
			default:
			{
				asAtom p = asAtomHandler::fromObjectNoPrimitive(subobj);
				if (!parse(jsonstring,it,p,name, reviver,wrk))
					return false;
				needdata = false;
				break;
			}
		}
	}
	if (!done || needdata)
		return false;
	return true;
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
