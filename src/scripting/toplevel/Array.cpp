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

#include "scripting/toplevel/Array.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/flash/utils/flashutils.h"

using namespace std;
using namespace lightspark;

Array::Array(Class_base* c):ASObject(c,T_ARRAY),currentsize(0)
{
}

void Array::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;
	c->setVariableByQName("CASEINSENSITIVE","",abstract_di(c->getSystemState(),CASEINSENSITIVE),CONSTANT_TRAIT);
	c->setVariableByQName("DESCENDING","",abstract_di(c->getSystemState(),DESCENDING),CONSTANT_TRAIT);
	c->setVariableByQName("NUMERIC","",abstract_di(c->getSystemState(),NUMERIC),CONSTANT_TRAIT);
	c->setVariableByQName("RETURNINDEXEDARRAY","",abstract_di(c->getSystemState(),RETURNINDEXEDARRAY),CONSTANT_TRAIT);
	c->setVariableByQName("UNIQUESORT","",abstract_di(c->getSystemState(),UNIQUESORT),CONSTANT_TRAIT);

	// properties
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_setLength),SETTER_METHOD,true);

	// public functions
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),_concat,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,Class<IFunction>::getFunction(c->getSystemState(),every,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,Class<IFunction>::getFunction(c->getSystemState(),filter,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,Class<IFunction>::getFunction(c->getSystemState(),forEach,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,Class<IFunction>::getFunction(c->getSystemState(),join,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,Class<IFunction>::getFunction(c->getSystemState(),_map,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(c->getSystemState(),_push_as3,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,Class<IFunction>::getFunction(c->getSystemState(),some,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sortOn",AS3,Class<IFunction>::getFunction(c->getSystemState(),sortOn),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,Class<IFunction>::getFunction(c->getSystemState(),splice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",AS3,Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),insertAt,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),removeAt,1),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),_concat,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("every","",Class<IFunction>::getFunction(c->getSystemState(),every,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("filter","",Class<IFunction>::getFunction(c->getSystemState(),filter,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("forEach","",Class<IFunction>::getFunction(c->getSystemState(),forEach,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("indexOf","",Class<IFunction>::getFunction(c->getSystemState(),indexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf","",Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("join","",Class<IFunction>::getFunction(c->getSystemState(),join,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("map","",Class<IFunction>::getFunction(c->getSystemState(),_map,1),DYNAMIC_TRAIT);
	// workaround, pop was encountered not in the AS3 namespace before, need to investigate it further
	c->setDeclaredMethodByQName("pop","",Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("pop","",Class<IFunction>::getFunction(c->getSystemState(),_pop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("push","",Class<IFunction>::getFunction(c->getSystemState(),_push,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("reverse","",Class<IFunction>::getFunction(c->getSystemState(),_reverse),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("shift","",Class<IFunction>::getFunction(c->getSystemState(),shift),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("slice","",Class<IFunction>::getFunction(c->getSystemState(),slice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("some","",Class<IFunction>::getFunction(c->getSystemState(),some,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sort","",Class<IFunction>::getFunction(c->getSystemState(),_sort),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sortOn","",Class<IFunction>::getFunction(c->getSystemState(),sortOn),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("splice","",Class<IFunction>::getFunction(c->getSystemState(),splice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift","",Class<IFunction>::getFunction(c->getSystemState(),unshift),DYNAMIC_TRAIT);
}

void Array::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Array,_constructor)
{
	Array* th=obj.as<Array>();
	th->constructorImpl(args, argslen);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Array,generator)
{
	Array* th=Class<Array>::getInstanceSNoArgs(getSys());
	th->constructorImpl(args, argslen);
	return asAtom::fromObject(th);
}

void Array::constructorImpl(asAtom* args, const unsigned int argslen)
{
	if(argslen==1 && (args[0].is<Integer>() || args[0].is<UInteger>() || args[0].is<Number>()))
	{
		uint32_t size = args[0].toUInt();
		if ((number_t)size != args[0].toNumber())
			throwError<RangeError>(kArrayIndexNotIntegerError, Number::toString(args[0].toNumber()));
		LOG(LOG_CALLS,_("Creating array of length ") << size);
		resize(size);
	}
	else
	{
		LOG(LOG_CALLS,_("Called Array constructor"));
		resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			ASATOM_INCREF(args[i]);
			set(i,args[i]);
		}
	}
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceSNoArgs(obj->getSystemState());
	
	// copy values into new array
	ret->resize(th->size());
	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		ret->data[it->first]=it->second;
		ASATOM_INCREF(ret->data[it->first]);
	}

	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[i]->is<Array>())
		{
			// Insert the contents of the array argument
			uint64_t oldSize=ret->size();
			Array* otherArray=args[i]->as<Array>();
			auto itother=otherArray->data.begin();
			for(;itother!=otherArray->data.end(); ++itother)
			{
				uint32_t newIndex=ret->size()+itother->first;
				ret->data[newIndex]=itother->second;
				ASATOM_INCREF(ret->data[newIndex]);
			}
			ret->resize(oldSize+otherArray->size());
		}
		else
		{
			//Insert the argument
			args[i]->incRef();
			ret->push(asAtom::fromObject(args[i]));
		}
	}

	return ret;
}

ASFUNCTIONBODY_ATOM(Array,filter)
{
	Array* th=static_cast<Array*>(obj.getObject());
	Array* ret=Class<Array>::getInstanceSNoArgs(th->getSystemState());
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL)
		return asAtom::fromObject(ret);

	asAtom params[3];
	asAtom funcRet;

	uint32_t index = 0;
	while (index < th->currentsize)
	{
		auto it=th->data.find(index);
		index++;
		if (it == th->data.end())
			continue;
		params[0] = it->second;
		params[1] = asAtom(it->first);
		params[2] = asAtom::fromObject(th);

		// ensure that return values are the original values
		asAtom origval = params[0];
		ASATOM_INCREF(origval);
		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3,false);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3,false);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
				ret->push(origval);
			else
			{
				ASATOM_DECREF(origval);
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY_ATOM(Array, some)
{
	Array* th=static_cast<Array*>(obj.getObject());
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL)
		return asAtom::falseAtom;

	asAtom params[3];
	asAtom funcRet;

	uint32_t index = 0;
	while (index < th->currentsize)
	{
		auto it=th->data.find(index);
		index++;
		if (it == th->data.end())
			continue;
		params[0] = it->second;
		params[1] = asAtom(it->first);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3,false);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3,false);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
			{
				return funcRet;
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::falseAtom;
}

ASFUNCTIONBODY_ATOM(Array, every)
{
	Array* th=static_cast<Array*>(obj.getObject());
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL)
		return asAtom::trueAtom;

	asAtom params[3];
	asAtom funcRet;

	uint32_t index = 0;
	while (index < th->currentsize)
	{
		auto it=th->data.find(index);
		index++;
		if (it == th->data.end())
			continue;
		params[0] = it->second;
		params[1] = asAtom(it->first);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3,false);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3,false);
		}
		if(funcRet.type != T_INVALID)
		{
			if(!funcRet.Boolean_concrete())
			{
				return funcRet;
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::trueAtom;
}

ASFUNCTIONBODY_ATOM(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj.getObject());
	return asAtom((uint32_t)th->currentsize);
}

ASFUNCTIONBODY_ATOM(Array,_setLength)
{
	uint32_t newLen;
	ARG_UNPACK_ATOM(newLen);
	Array* th=static_cast<Array*>(obj.getObject());
	if (th->getClass() && th->getClass()->isSealed)
		return asAtom::invalidAtom;
	//If newLen is equal to size do nothing
	if(newLen==th->size())
		return asAtom::invalidAtom;
	th->resize(newLen);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Array,forEach)
{
	Array* th=static_cast<Array*>(obj.getObject());
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL)
		return asAtom::invalidAtom;
	asAtom params[3];

	uint32_t s = th->size();
	for (uint32_t i=0; i < s; i++ )
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			if(it->second.type!=T_INVALID)
				params[0]=it->second;
			else
				params[0]=asAtom::undefinedAtom;
		}
		else
			continue;
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);

		asAtom funcret;
		if( argslen == 1 )
		{
			funcret=f.callFunction(asAtom::nullAtom, params, 3,false);
		}
		else
		{
			funcret=f.callFunction(args[1], params, 3,false);
		}
		ASATOM_DECREF(funcret);
	}

	return asAtom::invalidAtom;
}

ASFUNCTIONBODY(Array, _reverse)
{
	Array* th = static_cast<Array*>(obj);

	std::unordered_map<uint32_t, asAtom> tmp = std::unordered_map<uint32_t, asAtom>(th->data.begin(),th->data.end());
	uint32_t size = th->size();
	th->data.clear();
	auto it=tmp.begin();
	for(;it != tmp.end();++it)
 	{
		th->data[size-(it->first+1)]=it->second;
	}
	th->incRef();
	return th;
}

ASFUNCTIONBODY_ATOM(Array,lastIndexOf)
{
	Array* th=obj.as<Array>();
	number_t index;
	asAtom arg0;
	ARG_UNPACK_ATOM(arg0) (index, 0x7fffffff);
	int32_t ret=-1;

	if(argslen == 1 && th->data.empty())
		return asAtom((int32_t)-1);

	size_t i = th->size()-1;

	if(std::isnan(index))
		return asAtom((int32_t)0);

	int j = index; //Preserve sign
	if(j < 0) //Negative offset, use it as offset from the end of the array
	{
		if((size_t)-j > th->size())
			return asAtom((int32_t)-1);
		else
			i = th->size()+j;
	}
	else //Positive offset, use it directly
	{
		if((size_t)j > th->size()) //If the passed offset is bigger than the array, cap the offset
			i = th->size()-1;
		else
			i = j;
	}
	do
	{
		auto it = th->data.find(i);
		if (it == th->data.end())
		    continue;
		if(it->second.isEqualStrict(th->getSystemState(),arg0))
		{
			ret=i;
			break;
		}
	}
	while(i--);

	return asAtom(ret);
}

ASFUNCTIONBODY_ATOM(Array,shift)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic shift calls
		if (obj.is<Vector>())
			return Vector::shift(sys,obj,args,argslen);
		if (obj.is<ByteArray>())
			return ByteArray::shift(sys,obj,args,argslen);
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=obj.getObject()->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res-1);
		if (res > 0)
			obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		return asAtom::undefinedAtom;
	}
	Array* th=obj.as<Array>();
	if(!th->size())
		return asAtom::undefinedAtom;
	asAtom ret;
	auto it = th->data.find(0);
	if(it == th->data.end())
		ret = asAtom::undefinedAtom;
	else
		ret=it->second;
	std::unordered_map<uint32_t,asAtom> tmp;
	it=th->data.begin();
	for (; it != th->data.end(); ++it )
	{
		if(it->first)
		{
			tmp[it->first-1]=it->second;
		}
	}
	th->data.clear();
	th->data.insert(tmp.begin(),tmp.end());
	th->resize(th->size()-1);
	return ret;
}

int Array::capIndex(int i)
{
	int totalSize=size();

	if(totalSize <= 0)
		return 0;
	else if(i < -totalSize)
		return 0;
	else if(i > totalSize)
		return totalSize;
	else if(i>=0)     // 0 <= i < totalSize
		return i;
	else              // -totalSize <= i < 0
	{
		//A negative i is relative to the end
		return i+totalSize;
	}
}

ASFUNCTIONBODY(Array,slice)
{
	Array* th=static_cast<Array*>(obj);
	int startIndex;
	int endIndex;

	ARG_UNPACK(startIndex, 0) (endIndex, 16777215);
	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);

	Array* ret=Class<Array>::getInstanceSNoArgs(obj->getSystemState());
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			ASATOM_INCREF(it->second);
			ret->data[j]=it->second;
		}
		j++;
	}
	ret->resize(j);
	return ret;
}

ASFUNCTIONBODY(Array,splice)
{
	Array* th=static_cast<Array*>(obj);
	int startIndex;
	int deleteCount;
	//By default, delete all the element up to the end
	//DeleteCount defaults to the array len, it will be capped below
	ARG_UNPACK_MORE_ALLOWED(startIndex) (deleteCount, th->size());

	int totalSize=th->size();
	Array* ret=Class<Array>::getInstanceSNoArgs(obj->getSystemState());

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	ret->resize(deleteCount);
	if(deleteCount)
	{
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			auto it = th->data.find(startIndex+i);
			if (it != th->data.end())
				ret->data[i] = it->second;
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			auto it = th->data.find(startIndex+i);
			if (it != th->data.end())
			{
				th->data.erase(it);
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<asAtom> tmp = vector<asAtom>(totalSize- (startIndex+deleteCount));
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			tmp[i-(startIndex+deleteCount)] = it->second;
			th->data.erase(it);
		}
	}
	th->resize(startIndex);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		args[i]->incRef();
		th->push(asAtom::fromObject(args[i]));
	}
	// move remembered items to new position
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (tmp[i].type != T_INVALID)
			th->data[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
	th->resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0));
	return ret;
}

ASFUNCTIONBODY_ATOM(Array,join)
{
	Array* th=obj.as<Array>();
	string ret;
	tiny_string del;
	ARG_UNPACK_ATOM(del, ",");

	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom o = th->at(i);
		if (!o.is<Undefined>() && !o.is<Null>())
			ret+= o.toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del.raw_buf();
	}
	return asAtom::fromObject(abstract_s(th->getSystemState(),ret));
}

ASFUNCTIONBODY_ATOM(Array,indexOf)
{
	Array* th=obj.as<Array>();
	int32_t ret=-1;
	int32_t index;
	asAtom arg0;
	ARG_UNPACK_ATOM(arg0) (index, 0);
	if (index < 0) index = th->size()+ index;
	if (index < 0) index = 0;

	for (auto it=th->data.begin() ; it != th->data.end(); ++it )
	{
		if (it->first < (uint32_t)index)
			continue;
		if(it->second.isEqualStrict(th->getSystemState(),arg0))
		{
			ret=it->first;
			break;
		}
	}
	return asAtom(ret);
}


ASFUNCTIONBODY_ATOM(Array,_pop)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic pop calls
		if (obj.is<Vector>())
			return Vector::_pop(sys,obj,args,argslen);
		if (obj.is<ByteArray>())
			return ByteArray::pop(sys,obj,args,argslen);
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=obj.getObject()->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res-1);
		if (res > 0)
			obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		return asAtom::undefinedAtom;
	}
	Array* th=obj.as<Array>();
	uint32_t size =th->size();
	if (size == 0)
		return asAtom::undefinedAtom;
	asAtom ret;
	
	auto it = th->data.find(size-1);
	if (it != th->data.end())
	{
		ret=it->second;
		th->data.erase(it);
	}
	else
		ret = asAtom::undefinedAtom;

	th->currentsize--;
	return ret;
}

bool Array::sortComparatorDefault::operator()(asAtom& d1, asAtom& d2)
{
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		a=d1.toNumber();
		b=d2.toNumber();

		if(std::isnan(a) || std::isnan(b))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		if(isDescending)
			return b>a;
		else
			return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1;
		tiny_string s2;
		s1=d1.toString();
		s2=d2.toString();

		if(isDescending)
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)>0;
			else
				return s1>s2;
		}
		else
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)<0;
			else
				return s1<s2;
		}
	}
}

bool Array::sortComparatorWrapper::operator()(asAtom& d1, asAtom& d2)
{
	asAtom objs[2];
	objs[0]=d1;
	objs[1]=d2;

	assert(comparator.type == T_FUNCTION);
	asAtom ret=comparator.callFunction(asAtom::nullAtom, objs, 2,false);
	assert_and_throw(ret.type != T_INVALID);
	return (ret.toNumber()<0); //Less
}

ASFUNCTIONBODY_ATOM(Array,_sort)
{
	Array* th=static_cast<Array*>(obj.getObject());
	asAtom comp;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	bool isDescending=false;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(args[i].type==T_FUNCTION) //Comparison func
		{
			assert_and_throw(comp.type == T_INVALID);
			comp=args[i];
		}
		else
		{
			uint32_t options=args[i].toInt();
			if(options&NUMERIC)
				isNumeric=true;
			if(options&CASEINSENSITIVE)
				isCaseInsensitive=true;
			if(options&DESCENDING)
				isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}
	std::vector<asAtom> tmp;
	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		if (it->second.type==T_INVALID || it->second.type==T_UNDEFINED)
			continue;
		tmp.push_back(it->second);
	}
	
	if(comp.type != T_INVALID)
		sort(tmp.begin(),tmp.end(),sortComparatorWrapper(comp));
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(isNumeric,isCaseInsensitive,isDescending));

	th->data.clear();
	std::vector<asAtom>::iterator ittmp=tmp.begin();
	int i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->data[i++]= *ittmp;
	}
	ASATOM_INCREF(obj);
	return obj;
}

bool Array::sortOnComparator::operator()(asAtom& d1, asAtom& d2)
{
	std::vector<sorton_field>::iterator it=fields.begin();
	for(;it != fields.end();++it)
	{
		asAtom obj1 = d1.toObject(getSys())->getVariableByMultiname(it->fieldname);
		asAtom obj2 = d2.toObject(getSys())->getVariableByMultiname(it->fieldname);
		if(it->isNumeric)
		{
			number_t a=numeric_limits<double>::quiet_NaN();
			number_t b=numeric_limits<double>::quiet_NaN();
			a=obj1.toNumber();
			
			b=obj2.toNumber();
			
			if(std::isnan(a) || std::isnan(b))
				throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
			if (b != a)
			{
				if(it->isDescending)
					return b>a;
				else
					return a<b;
			}
		}
		else
		{
			//Comparison is always in lexicographic order
			tiny_string s1;
			tiny_string s2;
			s1=obj1.toString();
			s2=obj2.toString();
			if (s1 != s2)
			{
				if(it->isDescending)
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)>0;
					else
						return s1>s2;
				}
				else
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)<0;
					else
						return s1<s2;
				}
			}
		}
	}
	return false;
}

ASFUNCTIONBODY_ATOM(Array,sortOn)
{
	if (argslen != 1 && argslen != 2)
		throwError<ArgumentError>(kWrongArgumentCountError, "1",
					  Integer::toString(argslen));
	Array* th=static_cast<Array*>(obj.getObject());
	std::vector<sorton_field> sortfields;
	if(args[0].is<Array>())
	{
		Array* obj=static_cast<Array*>(args[0].getObject());
		int n = 0;
		auto it=obj->data.begin();
		for(;it != obj->data.end();++it)
		{
			multiname sortfieldname(NULL);
			sortfieldname.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
			asAtom atom = it->second;
			sortfieldname.setName(atom,obj->getSystemState());
			sorton_field sf(sortfieldname);
			sortfields.push_back(sf);
		}
		if (argslen == 2 && args[1].is<Array>())
		{
			Array* opts=static_cast<Array*>(args[1].getObject());
			auto itopt=opts->data.begin();
			int nopt = 0;
			for(;itopt != opts->data.end() && nopt < n;++itopt)
			{
				uint32_t options=0;
				options = itopt->second.toInt();
				if(options&NUMERIC)
					sortfields[nopt].isNumeric=true;
				if(options&CASEINSENSITIVE)
					sortfields[nopt].isCaseInsensitive=true;
				if(options&DESCENDING)
					sortfields[nopt].isDescending=true;
				if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
					throw UnsupportedException("Array::sort not completely implemented");
				nopt++;
			}
		}
	}
	else
	{
		multiname sortfieldname(NULL);
		asAtom atom = args[0];
		sortfieldname.setName(atom,th->getSystemState());
		sortfieldname.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		sorton_field sf(sortfieldname);
		if (argslen == 2)
		{
			uint32_t options = args[1].toInt();
			if(options&NUMERIC)
				sf.isNumeric=true;
			if(options&CASEINSENSITIVE)
				sf.isCaseInsensitive=true;
			if(options&DESCENDING)
				sf.isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
		sortfields.push_back(sf);
	}
	
	std::vector<asAtom> tmp;
	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		if (it->second.type==T_UNDEFINED)
			continue;
		tmp.push_back(it->second);
	}
	
	sort(tmp.begin(),tmp.end(),sortOnComparator(sortfields));

	th->data.clear();
	std::vector<asAtom>::iterator ittmp=tmp.begin();
	int i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->data[i++]= *ittmp;
	}
	ASATOM_INCREF(obj);
	return obj;
}

ASFUNCTIONBODY_ATOM(Array,unshift)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic unshift calls
		if (obj.is<Vector>())
			return Vector::unshift(sys,obj,args,argslen);
		if (obj.is<ByteArray>())
			return ByteArray::unshift(sys,obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=obj.getObject()->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		return asAtom::undefinedAtom;
	}
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"unshift",th->getClass()->getQualifiedClassName());
	if (argslen > 0)
	{
		th->resize(th->size()+argslen);
		std::map<uint32_t,asAtom> tmp;
		for (auto it=th->data.begin(); it != th->data.end(); ++it )
		{
			tmp[it->first+argslen]=it->second;
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			tmp[i] = args[i];
			ASATOM_INCREF(args[i]);
		}
		th->data.clear();
		th->data.insert(tmp.begin(),tmp.end());
	}
	return asAtom((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_push)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj.is<Vector>())
			return Vector::push(sys,obj,args,argslen);
		if (obj.is<ByteArray>())
			return ByteArray::push(sys,obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=obj.getObject()->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		return asAtom::undefinedAtom;
	}
	Array* th=static_cast<Array*>(obj.getObject());
	uint64_t s = th->currentsize;
	for(unsigned int i=0;i<argslen;i++)
	{
		ASATOM_INCREF(args[i]);
		th->push(args[i]);
	}
	// currentsize is set even if push fails
	th->currentsize = s+argslen;
	return asAtom((int32_t)th->size());
}
// AS3 handles push on uint.MAX_VALUE differently than ECMA, so we need to push methods
ASFUNCTIONBODY_ATOM(Array,_push_as3)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj.is<Vector>())
			return Vector::push(sys,obj,args,argslen);
		if (obj.is<ByteArray>())
			return ByteArray::push(sys,obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=obj.getObject()->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		return asAtom::undefinedAtom;
	}
	Array* th=static_cast<Array*>(obj.getObject());
	for(unsigned int i=0;i<argslen;i++)
	{
		if (th->size() >= UINT32_MAX)
			break;
		ASATOM_INCREF(args[i]);
		th->push(args[i]);
	}
	return asAtom((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_map)
{
	Array* th=static_cast<Array*>(obj.getObject());

	if(argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Array.map", "1", Integer::toString(argslen));
	asAtom func;
	if (!args[0].is<RegExp>())
	{
		assert_and_throw(args[0].type==T_FUNCTION);
		func=args[0];
	}
	Array* arrayRet=Class<Array>::getInstanceSNoArgs(th->getSystemState());

	uint32_t s = th->size();
	for (uint32_t i=0; i < s; i++ )
	{
		asAtom funcArgs[3];
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			if(it->second.type!=T_INVALID)
			{
				funcArgs[0]=it->second;
			}
			else
				funcArgs[0]=asAtom::undefinedAtom;
		}
		else
			funcArgs[0]=asAtom::undefinedAtom;
		funcArgs[1]=asAtom(i);
		funcArgs[2]=asAtom::fromObject(th);
		asAtom funcRet;
		if (func.type != T_INVALID)
		{
			funcRet = func.callFunction(argslen > 1? args[1] : asAtom::nullAtom, funcArgs, 3,false);
		}
		else
		{
			funcRet = RegExp::exec(sys,args[0],args,1);
		}
		assert_and_throw(funcRet.type != T_INVALID);
		arrayRet->push(funcRet);
	}

	return asAtom::fromObject(arrayRet);
}

ASFUNCTIONBODY(Array,_toString)
{
	if(Class<Number>::getClass(obj->getSystemState())->prototype->getObj() == obj)
		return abstract_s(obj->getSystemState(),"");
	if(!obj->is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toString");
		return abstract_s(obj->getSystemState(),"");
	}
	
	Array* th=obj->as<Array>();
	return abstract_s(obj->getSystemState(),th->toString_priv());
}

ASFUNCTIONBODY(Array,_toLocaleString)
{
	if(Class<Number>::getClass(obj->getSystemState())->prototype->getObj() == obj)
		return abstract_s(obj->getSystemState(),"");
	if(!obj->is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toLocaleString");
		return abstract_s(obj->getSystemState(),"");
	}
	
	Array* th=obj->as<Array>();
	return abstract_s(obj->getSystemState(),th->toString_priv(true));
}

ASFUNCTIONBODY_ATOM(Array,insertAt)
{
	Array* th=obj.as<Array>();
	int32_t index;
	asAtom o;
	ARG_UNPACK_ATOM(index)(o);
	if (index < 0 && th->currentsize >= (uint32_t)(-index))
		index = th->currentsize+(index);
	if (index < 0)
		index = 0;
	if ((uint32_t)index >= th->currentsize)
	{
		th->currentsize++;
		th->set(th->currentsize-1,o);
	}
	else
	{
		std::map<uint32_t,asAtom> tmp;
		auto it=th->data.begin();
		for (; it != th->data.end(); ++it )
		{
			tmp[it->first+(it->first >= (uint32_t)index ? 1 : 0)]=it->second;
		}
		th->data.clear();
		th->data.insert(tmp.begin(),tmp.end());
		
		th->currentsize++;
		th->set(index,o);
	}
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY(Array,removeAt)
{
	Array* th=static_cast<Array*>(obj);
	int32_t index;
	ARG_UNPACK(index);
	if (index < 0)
		index = th->currentsize+index;
	if (index < 0)
		index = 0;
	ASObject* o = NULL;
	auto it = th->data.find(index);
	if(it != th->data.end())
	{
		asAtom sl = it->second;
		o = sl.toObject(obj->getSystemState());
		th->data.erase(it);
	}
	if ((uint32_t)index < th->currentsize)
		th->currentsize--;
	std::map<uint32_t,asAtom> tmp;
	it=th->data.begin();
	for (; it != th->data.end(); ++it )
	{
		tmp[it->first-(it->first > (uint32_t)index ? 1 : 0)]=it->second;
	}
	th->data.clear();
	th->data.insert(tmp.begin(),tmp.end());
	return o;
}
int32_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<size())
	{
		auto it = data.find(index);
		if (it == data.end())
			return 0;
		return it->second.toInt();
	}

	return ASObject::getVariableByMultiname_i(name);
}

asAtom Array::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
		return ASObject::getVariableByMultiname(name,opt);

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname(name,opt);
	auto it = data.find(index);
	if(it != data.end())
	{
		return it->second;
	}
	if (name.hasEmptyNS)
	{
		asAtom ret;
		//Check prototype chain
		Prototype* proto = this->getClass()->prototype.getPtr();
		while(proto)
		{
			ret = proto->getObj()->getVariableByMultiname(name, opt);
			if(ret.type != T_INVALID)
				return ret;
			proto = proto->prevPrototype.getPtr();
		}
	}
	if(index<size())
		return asAtom::undefinedAtom;
	return asAtom::invalidAtom;
}

void Array::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}
	if (index==0xFFFFFFFF)
		return;
	if(index>=size())
		resize(index+1);
	auto it = data.find(index);
	if(it != data.end())
		ASATOM_DECREF(it->second);
	data[index] = asAtom(value);
}


bool Array::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	if (!isConstructed())
		return false;

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	return (data.find(index) != data.end());
}

bool Array::isValidMultiname(SystemState* sys, const multiname& name, uint32_t& index)
{
	switch (name.name_type)
	{
		case multiname::NAME_UINT:
			index = name.name_ui;
			return true;
		case multiname::NAME_INT:
			if (name.name_i >= 0)
			{
				index = name.name_i;
				return true;
			}
			break;
		case multiname::NAME_STRING:
			if (name.name_s_id < BUILTIN_STRINGS::LAST_BUILTIN_STRING ||
					 !isIntegerWithoutLeadingZeros(name.normalizedName(sys)))
				return false;
			break;
		default:
			break;
	}
	if(name.isEmpty())
		return false;
	if(!name.hasEmptyNS)
		return false;
	
	return name.toUInt(sys,index);
}

bool Array::isIntegerWithoutLeadingZeros(const tiny_string& value)
{
	if (value.empty())
		return false;
	else if (value == "0")
		return true;

	bool first = true;
	for (CharIterator it=value.begin(); it!=value.end(); ++it)
	{
		if (!it.isdigit() || (first && *it == '0'))
			return false;

		first = false;
	}
	
	return true;
}

void Array::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		ASObject::setVariableByMultiname(name,o,allowConst);
//		setIsEnumerable(name,false);
		return;
	}
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,
					   name.normalizedNameUnresolved(getSystemState()),
					   getClass()->getQualifiedClassName());
	if (index==0xFFFFFFFF)
		return;
	if(index>=size())
		resize((uint64_t)index+1);

	auto it = data.find(index);
	if(it != data.end())
		ASATOM_DECREF(it->second);
	data[index] = o;
}

bool Array::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::deleteVariableByMultiname(name);
	if (getClass() && getClass()->isSealed)
		return false;

	if(index>=size())
		return true;
	auto it = data.find(index);
	if(it == data.end())
		return true;
	ASATOM_DECREF(it->second);
	data.erase(it);
	return true;
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index)
{
	if(ns!="")
		return false;
	assert_and_throw(!name.empty());
	index=0;
	//First we try to convert the string name to an index, at the first non-digit
	//we bail out
	for(auto i=name.begin(); i!=name.end(); ++i)
	{
		if(!i.isdigit())
			return false;

		index*=10;
		index+=i.digit_value();
	}
	return true;
}

tiny_string Array::toString()
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Array::toString_priv(bool localized)
{
	string ret;
	for(uint32_t i=0;i<size();i++)
	{
		auto it = data.find(i);
		if(it != data.end())
		{
			asAtom sl = it->second;
			if(sl.type != T_UNDEFINED && sl.type != T_NULL && sl.type != T_INVALID)
			{
				if (localized)
					ret += sl.toLocaleString().raw_buf();
				else
					ret += sl.toString().raw_buf();
			}
		}
		if(i!=size()-1)
			ret+=',';
	}
	return ret;
}

asAtom Array::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
	{
		--index;
		data_iterator it;
		it = data.find(index);
		if(it == data.end() || it->first != index)
			return asAtom::undefinedAtom;
		asAtom sl = it->second;
		if(sl.type == T_INVALID)
			return asAtom::undefinedAtom;
		else
		{
			ASATOM_INCREF(sl);
			return sl;
		}
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<size())
	{
		while (!data.count(cur_index) && cur_index<size())
		{
			cur_index++;
		}
		if(cur_index<size())
			return cur_index+1;
	}
	//Fall back on object properties
	uint32_t ret=ASObject::nextNameIndex(cur_index-size());
	if(ret==0)
		return 0;
	else
		return ret+size();
	
}

asAtom Array::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
		return asAtom(index-1);
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-size());
	}
}

asAtom Array::at(unsigned int index)
{
	if(size()<=index)
		outofbounds(index);

	auto it = data.find(index);
	if(it == data.end())
		return asAtom::undefinedAtom;
	asAtom ret = it->second;
	if(ret.type != T_INVALID)
	{
		ASATOM_INCREF(ret);
		return ret;
	}
	return asAtom::undefinedAtom;
}

void Array::outofbounds(unsigned int index) const
{
	throwError<RangeError>(kInvalidArrayLengthError, Number::toString(index));
}

void Array::resize(uint64_t n)
{
	// Bug-for-bug compatible wrapping. See Tamarin test
	// as3/Array/length_mods.swf and Tamarin bug #685323.
	if (n > 0xFFFFFFFF)
		n = (n % 0x100000000);

	if (n < currentsize)
	{
		data_iterator it=data.begin();
		while (it != data.end())
		{
			if (it->first >= n)
			{
				data_iterator it2 = it;
				++it;
				ASATOM_DECREF(it2->second);
				data.erase(it2);
			}
			else
				++it;
		}
	}
	currentsize = n;
}

void Array::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Array in AMF0 not implemented");
		return;
	}
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(array_marker);
	//Check if the array has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the array to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t denseCount = currentsize;
		assert_and_throw(denseCount<0x20000000);
		uint32_t value = (denseCount << 1) | 1;
		out->writeU29(value);
		serializeDynamicProperties(out, stringMap, objMap, traitsMap);
		for(uint32_t i=0;i<denseCount;i++)
		{
			if (data.find(i) == data.end())
				out->writeByte(null_marker);
			else
				data.at(i).toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
		}
	}
}

tiny_string Array::toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string& spaces,const tiny_string& filter)
{
	bool ok;
	tiny_string res = call_toJSON(ok,path,replacer,spaces,filter);
	if (ok)
		return res;
	// check for cylic reference
	if (std::find(path.begin(),path.end(), this) != path.end())
		throwError<TypeError>(kJSONCyclicStructure);
	
	path.push_back(this);
	res += "[";
	bool bfirst = true;
	tiny_string newline = (spaces.empty() ? "" : "\n");
	uint32_t denseCount = currentsize;
	for (uint32_t i=0 ; i < denseCount; i++)
	{
		auto it = data.find(i);
		tiny_string subres;
		if (replacer.type != T_INVALID && it != data.end())
		{
			asAtom params[2];
			
			params[0] = asAtom(it->first);
			params[1] = it->second;
			asAtom funcret=replacer.callFunction(asAtom::nullAtom, params, 2,false);
			if (funcret.type != T_INVALID)
				subres = funcret.toObject(getSystemState())->toJSON(path,asAtom::invalidAtom,spaces,filter);
		}
		else
		{
			ASObject* o = it == data.end() ? getSystemState()->getNullRef() : it->second.toObject(getSystemState());
			if (o)
				subres = o->toJSON(path,replacer,spaces,filter);
			else
				continue;
		}
		if (!subres.empty())
		{
			if (!bfirst)
				res += ",";
			res += newline+spaces;
			
			bfirst = false;
			res += subres;
		}
	}
	if (!bfirst)
		res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);
	res += "]";
	path.pop_back();
	return res;
	
}

Array::~Array()
{
}

void Array::set(unsigned int index, asAtom& o)
{
	if(index<currentsize)
	{
		if(data.find(index) != data.end())
			ASATOM_DECREF(data[index]);
		ASATOM_INCREF(o);
		data[index]=o;
	}
	else
		outofbounds(index);
}

uint64_t Array::size()
{
	if (this->getClass()->is<Class_inherit>())
	{
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = false;
		if (hasPropertyByMultiname(lengthName, true, true))
		{
			asAtom o=getVariableByMultiname(lengthName,SKIP_IMPL);
			return o.toUInt();
		}
	}
	return currentsize;
}

void Array::push(asAtom o)
{
	if (currentsize == UINT32_MAX)
		return;
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"push",getClass()->getQualifiedClassName());
	currentsize++;
	set(currentsize-1,o);
}

