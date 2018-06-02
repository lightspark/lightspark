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
	c->setVariableAtomByQName("CASEINSENSITIVE",nsNameAndKind(),asAtom(CASEINSENSITIVE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESCENDING",nsNameAndKind(),asAtom(DESCENDING),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMERIC",nsNameAndKind(),asAtom(NUMERIC),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RETURNINDEXEDARRAY",nsNameAndKind(),asAtom(RETURNINDEXEDARRAY),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNIQUESORT",nsNameAndKind(),asAtom(UNIQUESORT),CONSTANT_TRAIT);

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
}

ASFUNCTIONBODY_ATOM(Array,generator)
{
	Array* th=Class<Array>::getInstanceSNoArgs(sys);
	th->constructorImpl(args, argslen);
	ret = asAtom::fromObject(th);
}

void Array::constructorImpl(asAtom* args, const unsigned int argslen)
{
	if(argslen==1 && (args[0].is<Integer>() || args[0].is<UInteger>() || args[0].is<Number>()))
	{
		uint32_t size = args[0].toUInt();
		if ((number_t)size != args[0].toNumber())
			throwError<RangeError>(kArrayIndexNotIntegerError, Number::toString(args[0].toNumber()));
		LOG_CALL(_("Creating array of length ") << size);
		resize(size);
	}
	else
	{
		LOG_CALL(_("Called Array constructor"));
		resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			set(i,args[i],false);
		}
	}
}

ASFUNCTIONBODY_ATOM(Array,_concat)
{
	Array* th=obj.as<Array>();
	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	
	// copy values into new array
	res->resize(th->size());
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		res->data_first.push_back(*it1);
		ASATOM_INCREF((*it1));
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		res->data_second[it2->first]=it2->second;
		ASATOM_INCREF(res->data_second[it2->first]);
	}

	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[i].is<Array>())
		{
			// Insert the contents of the array argument
			uint64_t oldSize=res->currentsize;
			uint64_t newSize=oldSize;
			Array* otherArray=args[i].as<Array>();
			auto itother1=otherArray->data_first.begin();
			for(;itother1!=otherArray->data_first.end(); ++itother1)
			{
				res->push(*itother1);
				newSize++;
			}
			auto itother2=otherArray->data_second.begin();
			for(;itother2!=otherArray->data_second.end(); ++itother2)
			{
				asAtom a = itother2->second;
				res->set(newSize+itother2->first, a,false);
			}
			res->resize(oldSize+otherArray->size());
		}
		else
		{
			//Insert the argument
			res->push(args[i]);
		}
	}
	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,filter)
{
	Array* th=obj.as<Array>();
	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL || th->currentsize == 0)
	{
		ret = asAtom::fromObject(res);
		return;
	}
	
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"filter",th->getClass()->getQualifiedClassName());

	asAtom params[3];
	asAtom funcRet;
	ASATOM_INCREF(f);
	uint32_t index = 0;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (a.type == T_INVALID)
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}

		params[1] = asAtom(index-1);
		params[2] = asAtom::fromObject(th);

		// ensure that return values are the original values
		asAtom origval = params[0];
		ASATOM_INCREF(origval);
		if(argslen==1)
		{
			f.callFunction(funcRet,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(funcRet,args[1], params, 3,false);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
				res->push(origval);
			ASATOM_DECREF(funcRet);
		}
		ASATOM_DECREF(origval);
	}
	ASATOM_DECREF(f);

	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array, some)
{
	Array* th=obj.as<Array>();
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL || th->currentsize == 0)
	{
		ret.setBool(false);
		return;
	}
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"some",th->getClass()->getQualifiedClassName());

	asAtom params[3];

	ASATOM_INCREF(f);
	uint32_t index = 0;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (a.type == T_INVALID)
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}
		params[1] = asAtom(index-1);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			f.callFunction(ret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(ret,args[1], params, 3,false);
		}
		if(ret.type != T_INVALID)
		{
			if(ret.Boolean_concrete())
			{
				ASATOM_DECREF(f);
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ASATOM_DECREF(f);
	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(Array, every)
{
	Array* th=obj.as<Array>();
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	if (f.type == T_NULL || th->currentsize == 0)
	{
		ret.setBool(true);
		return;
	}
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"every",th->getClass()->getQualifiedClassName());

	asAtom params[3];
	ASATOM_INCREF(f);

	uint32_t index = 0;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (a.type == T_INVALID)
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}
		params[1] = asAtom(index-1);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			f.callFunction(ret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(ret,args[1], params, 3,false);
		}
		if(ret.type != T_INVALID)
		{
			if(!ret.Boolean_concrete())
			{
				ASATOM_DECREF(f);
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ASATOM_DECREF(f);
	ret.setBool(true);
}

ASFUNCTIONBODY_ATOM(Array,_getLength)
{
	Array* th=obj.as<Array>();
	ret.setUInt(th->currentsize);
}

ASFUNCTIONBODY_ATOM(Array,_setLength)
{
	uint32_t newLen;
	ARG_UNPACK_ATOM(newLen);
	Array* th=obj.as<Array>();
	if (th->getClass() && th->getClass()->isSealed)
		return;
	//If newLen is equal to size do nothing
	if(newLen==th->size())
		return;
	th->resize(newLen);
}

ASFUNCTIONBODY_ATOM(Array,forEach)
{
	Array* th=obj.as<Array>();
	asAtom f(T_FUNCTION);
	ARG_UNPACK_ATOM(f);
	ret.setUndefined();
	if (f.type == T_NULL || th->currentsize == 0)
		return;
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"foreach",th->getClass()->getQualifiedClassName());
	asAtom params[3];

	ASATOM_INCREF(f);
	uint32_t index = 0;
	uint32_t s = th->size(); // remember current size, as it may change inside the called function
	while (index < s)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (a.type == T_INVALID)
				continue;
			else
				params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			else
				params[0]=it->second;
		}
		params[1] = asAtom(index-1);
		params[2] = asAtom::fromObject(th);

		asAtom funcret;
		if( argslen == 1 )
		{
			f.callFunction(funcret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(funcret,args[1], params, 3,false);
		}
		ASATOM_DECREF(funcret);
	}
	ASATOM_DECREF(f);
}

ASFUNCTIONBODY_ATOM(Array, _reverse)
{
	Array* th=obj.as<Array>();

	if (th->data_second.empty())
		std::reverse(th->data_first.begin(),th->data_first.end());
	else
	{
		std::unordered_map<uint32_t, asAtom> tmp = std::unordered_map<uint32_t, asAtom>(th->data_second.begin(),th->data_second.end());
		for (uint32_t i = 0; i < th->data_first.size(); i++)
		{
			tmp[i] = th->data_first[i];
		}
		uint32_t size = th->size();
		th->data_first.clear();
		th->data_second.clear();
		auto it=tmp.begin();
		for(;it != tmp.end();++it)
		{
			th->set(size-(it->first+1),it->second,false);
		}
	}
	th->incRef();
	ret = asAtom::fromObject(th);
}

ASFUNCTIONBODY_ATOM(Array,lastIndexOf)
{
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"lastIndexOf",th->getClass()->getQualifiedClassName());
	number_t index;
	asAtom arg0;
	ARG_UNPACK_ATOM(arg0) (index, 0x7fffffff);
	int32_t res=-1;

	if(argslen == 1 && th->currentsize == 0)
	{
		ret.setInt((int32_t)-1);
		return;
	}

	size_t i = th->size()-1;

	if(std::isnan(index))
	{
		ret.setInt((int32_t)0);
		return;
	}

	int j = index; //Preserve sign
	if(j < 0) //Negative offset, use it as offset from the end of the array
	{
		if((size_t)-j > th->size())
		{
			ret.setInt((int32_t)-1);
			return;
		}
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
		asAtom a;
		if ( i >= ARRAY_SIZE_THRESHOLD)
		{
			auto it = th->data_second.find(i);
			if (it == th->data_second.end())
				continue;
			a = it->second;
		}
		else
		{
			a = th->data_first[i];
			if (a.type == T_INVALID)
				continue;
		}
		if(a.isEqualStrict(th->getSystemState(),arg0))
		{
			res=i;
			break;
		}
	}
	while(i--);

	ret.setInt(res);
}

ASFUNCTIONBODY_ATOM(Array,shift)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic shift calls
		if (obj.is<Vector>())
		{
			Vector::shift(ret,sys,obj,args,argslen);
			return;
		}
		if (obj.is<ByteArray>())
		{
			ByteArray::shift(ret,sys,obj,args,argslen);
			return;
		}
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o;
		obj.getObject()->getVariableByMultiname(o,lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res-1);
		if (res > 0)
			obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		ret.setUndefined();
		return;
	}
	Array* th=obj.as<Array>();
	if(!th->size())
	{
		ret.setUndefined();
		return;
	}
	if (th->data_first.size() > 0)
		ret = th->data_first[0];
	if (ret.type == T_INVALID)
		ret = asAtom::undefinedAtom;
	if (th->data_first.size() > 0)
		th->data_first.erase(th->data_first.begin());

	std::unordered_map<uint32_t,asAtom> tmp;
	auto it=th->data_second.begin();
	for (; it != th->data_second.end(); ++it )
	{
		if(it->first)
		{
			if (it->first == ARRAY_SIZE_THRESHOLD)
				th->data_first[ARRAY_SIZE_THRESHOLD-1] = it->second;
			else
				tmp[it->first-1]=it->second;
		}
	}
	th->data_second.clear();
	th->data_second.insert(tmp.begin(),tmp.end());
	th->resize(th->size()-1);
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

ASFUNCTIONBODY_ATOM(Array,slice)
{
	Array* th=obj.as<Array>();
	uint32_t startIndex;
	uint32_t endIndex;

	ARG_UNPACK_ATOM(startIndex, 0) (endIndex, 16777215);
	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);

	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	uint32_t j = 0;
	for(uint32_t i=startIndex; i<endIndex && i< th->currentsize; i++) 
	{
		asAtom a = th->at((uint32_t)i);
		if (a.type != T_INVALID)
			res->push(a);
		j++;
	}
	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,splice)
{
	Array* th=obj.as<Array>();
	int startIndex;
	int deleteCount;
	//By default, delete all the element up to the end
	//DeleteCount defaults to the array len, it will be capped below
	ARG_UNPACK_ATOM_MORE_ALLOWED(startIndex) (deleteCount, th->size());

	uint32_t totalSize=th->size();
	Array* res=Class<Array>::getInstanceSNoArgs(sys);

	startIndex=th->capIndex(startIndex);

	if((uint32_t)(startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	res->resize(deleteCount);
	if(deleteCount)
	{
		// Derived classes may be sealed!
		if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
			throwError<ReferenceError>(kReadSealedError,"splice",th->getClass()->getQualifiedClassName());
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			asAtom a = th->at((uint32_t)startIndex+i);
			if (a.type != T_INVALID)
				res->set(i,a,false);
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			if (i+startIndex < ARRAY_SIZE_THRESHOLD)
			{
				if ((uint32_t)startIndex <th->data_first.size())
					th->data_first.erase(th->data_first.begin()+startIndex);
			}
			else
			{
				auto it = th->data_second.find(startIndex+i);
				if (it != th->data_second.end())
				{
					th->data_second.erase(it);
				}
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<asAtom> tmp = vector<asAtom>(totalSize- (startIndex+deleteCount));
	for (uint32_t i = (uint32_t)startIndex+deleteCount; i < totalSize ; i++)
	{
		if (i < ARRAY_SIZE_THRESHOLD)
		{
			if ((uint32_t)startIndex < th->data_first.size())
			{
				tmp[i-(startIndex+deleteCount)] = th->data_first[(uint32_t)startIndex];
				th->data_first.erase(th->data_first.begin()+startIndex);
			}
		}
		else
		{
			auto it = th->data_second.find(i);
			if (it != th->data_second.end())
			{
				tmp[i-(startIndex+deleteCount)] = it->second;
				th->data_second.erase(it);
			}
		}
	}
	th->resize(startIndex);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		th->push(args[i]);
	}
	// move remembered items to new position
	th->resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0));
	for(uint32_t i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (tmp[i].type != T_INVALID)
			th->set(startIndex+i+(argslen > 2 ? argslen-2 : 0),tmp[i],false);
	}
	ret =asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,join)
{
	Array* th=obj.as<Array>();
	string res;
	tiny_string del;
	ARG_UNPACK_ATOM(del, ",");

	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"join",th->getClass()->getQualifiedClassName());
	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom o = th->at(i);
		if (!o.is<Undefined>() && !o.is<Null>())
			res+= o.toString(sys).raw_buf();
		if(i!=th->size()-1)
			res+=del.raw_buf();
	}
	ret = asAtom::fromObject(abstract_s(th->getSystemState(),res));
}

ASFUNCTIONBODY_ATOM(Array,indexOf)
{
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"indexOf",th->getClass()->getQualifiedClassName());
	int32_t res=-1;
	int32_t index;
	asAtom arg0;
	ARG_UNPACK_ATOM(arg0) (index, 0);
	if (index < 0) index = th->size()+ index;
	if (index < 0) index = 0;

	if ((uint32_t)index < th->data_first.size())
	{
		for (auto it=th->data_first.begin()+index ; it != th->data_first.end(); ++it )
		{
			if(it->isEqualStrict(sys,arg0))
			{
				res=it - th->data_first.begin();
				break;
			}
		}
	}
	if (res == -1)
	{
		for (auto it=th->data_second.begin() ; it != th->data_second.end(); ++it )
		{
			if (it->first < (uint32_t)index)
				continue;
			if(it->second.isEqualStrict(sys,arg0))
			{
				res=it->first;
				break;
			}
		}
	}
	ret.setInt(res);
}


ASFUNCTIONBODY_ATOM(Array,_pop)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic pop calls
		if (obj.is<Vector>())
		{
			Vector::_pop(ret,sys,obj,args,argslen);
			return;
		}
		if (obj.is<ByteArray>())
		{
			ByteArray::pop(ret, sys,obj,args,argslen);
			return;
		}
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o;
		obj.getObject()->getVariableByMultiname(o,lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res-1);
		if (res > 0)
			obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		ret.setUndefined();
		return;
	}
	Array* th=obj.as<Array>();
	uint32_t size =th->size();
	ret.setUndefined();
	if (size == 0)
		return;
	
	if (size <= ARRAY_SIZE_THRESHOLD)
	{
		if (th->data_first.size() > 0)
		{
			ret = *th->data_first.rbegin();
			th->data_first.pop_back();
			if (ret.type == T_INVALID)
				ret.setUndefined();
		}
	}
	else
	{
		auto it = th->data_second.find(size-1);
		if (it != th->data_second.end())
		{
			ret=it->second;
			th->data_second.erase(it);
		}
		else
			ret.setUndefined();
	}
	th->currentsize--;
}


bool Array::sortComparatorDefault::operator()(const asAtom& d1, const asAtom& d2)
{
	asAtom o1 = d1;
	asAtom o2 = d2;
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		if (useoldversion)
		{
			a=o1.toInt() & 0x1fffffff;
			b=o2.toInt() & 0x1fffffff;
		}
		else
		{
			a=o1.toNumber();
			b=o2.toNumber();
		}
		if((!o1.isNumeric() && std::isnan(a)) || (!o2.isNumeric() && std::isnan(b)))
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
		s1=o1.toString(getSys());
		s2=o2.toString(getSys());

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
// std::sort expects strict weak ordering for the comparison function
// this is not guarranteed by user defined comparison functions, so we need out own sorting method.
// TODO this is just a simple quicksort implementation, not optimized for performance
void simplequicksortArray(std::vector<asAtom>& v, Array::sortComparatorWrapper& comp, int32_t lo, int32_t hi)
{
	if (v.size() == 2)
	{
		if (comp.compare(v[0],v[1]) > 0)
			std::swap(v[0],v[1]);
		return;
	}
	if (lo < hi)
	{
		asAtom pivot = v[lo];
		int32_t i = lo - 1;
		int32_t j = hi + 1;
		while (true)
		{
			do
			{
				i++;
			}
			while (i < hi && (comp.compare(v[i],pivot) < 0));
			
			do
			{
				j--;
			}
			while (j >= 0 && (comp.compare(v[j],pivot) > 0));
	
			if (i >= j)
				break;

			std::swap(v[i],v[j]);
		}
		if (j >= lo)
		{
			simplequicksortArray(v, comp, lo, j);
			simplequicksortArray(v, comp, j + 1, hi);
		}
	}
}

number_t Array::sortComparatorWrapper::compare(const asAtom& d1, const asAtom& d2)
{
	asAtom objs[2];
	objs[0]=d1;
	objs[1]=d2;

	assert(comparator.type == T_FUNCTION);
	asAtom ret;
	// don't coerce the result, as it may be an int that would loose it's sign through coercion
	comparator.callFunction(ret,asAtom::nullAtom, objs, 2,false,false);
	assert_and_throw(ret.type != T_INVALID);
	return ret.toNumber();
}

ASFUNCTIONBODY_ATOM(Array,_sort)
{
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"sort",th->getClass()->getQualifiedClassName());
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
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		if (it1->type==T_INVALID || it1->type==T_UNDEFINED)
			continue;
		tmp.push_back(*it1);
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		if (it2->second.type==T_INVALID || it2->second.type==T_UNDEFINED)
			continue;
		tmp.push_back(it2->second);
	}
	
	if(comp.type != T_INVALID)
	{
		sortComparatorWrapper c(comp);
		simplequicksortArray(tmp,c,0,tmp.size()-1);
	}
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(sys->getSwfVersion() < 11, isNumeric,isCaseInsensitive,isDescending));

	th->data_first.clear();
	th->data_second.clear();
	std::vector<asAtom>::iterator ittmp=tmp.begin();
	int i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->set(i++,*ittmp,false);
	}
	ASATOM_INCREF(obj);
	ret = obj;
}

bool Array::sortOnComparator::operator()(const sorton_value& d1, const sorton_value& d2)
{
	std::vector<sorton_field>::iterator it=fields.begin();
	uint32_t i = 0;
	for(;it != fields.end();++it)
	{
		asAtom obj1 = d1.sortvalues.at(i);
		asAtom obj2 = d2.sortvalues.at(i);
		i++;
		if(it->isNumeric)
		{
			number_t a=numeric_limits<double>::quiet_NaN();
			number_t b=numeric_limits<double>::quiet_NaN();
			a=obj1.toNumber();
			
			b=obj2.toNumber();
			
			if((!obj1.isNumeric() && std::isnan(a)) || (!obj2.isNumeric() && std::isnan(b)))
				throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
			if(it->isDescending)
				return b>a;
			else
				return a<b;
		}
		else
		{
			//Comparison is always in lexicographic order
			tiny_string s1;
			tiny_string s2;
			s1=obj1.toString(sys);
			s2=obj2.toString(sys);
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
	Array* th=obj.as<Array>();
	std::vector<sorton_field> sortfields;
	if(args[0].is<Array>())
	{
		Array* obj=args[0].as<Array>();
		int n = 0;
		for(uint32_t i = 0;i<obj->size();i++)
		{
			multiname sortfieldname(NULL);
			sortfieldname.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
			asAtom atom = obj->at(i);
			sortfieldname.setName(atom,sys);
			sorton_field sf(sortfieldname);
			sortfields.push_back(sf);
		}
		if (argslen == 2 && args[1].is<Array>())
		{
			Array* opts=args[1].as<Array>();
			auto itopt=opts->data_first.begin();
			int nopt = 0;
			for(;itopt != opts->data_first.end() && nopt < n;++itopt)
			{
				uint32_t options=0;
				options = itopt->toInt();
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
	
	std::vector<sorton_value> tmp;
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		if (it1->type==T_INVALID || it1->type==T_UNDEFINED)
			continue;
		// ensure ASObjects are created
		it1->toObject(sys);
		
		sorton_value v(*it1);
		for (auto itsf=sortfields.begin();itsf != sortfields.end(); itsf++)
		{
			asAtom tmpval;
			it1->getObject()->getVariableByMultiname(tmpval,itsf->fieldname);
			v.sortvalues.push_back(tmpval);
		}
		tmp.push_back(v);
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		if (it2->second.type==T_INVALID || it2->second.type==T_UNDEFINED)
			continue;
		// ensure ASObjects are created
		it2->second.toObject(sys);
		
		sorton_value v(it2->second);
		for (auto itsf=sortfields.begin();itsf != sortfields.end(); itsf++)
		{
			asAtom tmpval;
			it2->second.getObject()->getVariableByMultiname(tmpval,itsf->fieldname);
			v.sortvalues.push_back(tmpval);
		}
		tmp.push_back(v);
	}
	
	sort(tmp.begin(),tmp.end(),sortOnComparator(sortfields,sys));

	th->data_first.clear();
	th->data_second.clear();
	std::vector<sorton_value>::iterator ittmp=tmp.begin();
	uint32_t i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->set(i++, ittmp->dataAtom,false);
	}
}

ASFUNCTIONBODY_ATOM(Array,unshift)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic unshift calls
		if (obj.is<Vector>())
		{
			Vector::unshift(ret,sys,obj,args,argslen);
			return;
		}
		if (obj.is<ByteArray>())
		{
			ByteArray::unshift(ret,sys,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o;
		obj.getObject()->getVariableByMultiname(o,lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		ret.setUndefined();
		return;
	}
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() > 12 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"unshift",th->getClass()->getQualifiedClassName());
	if (argslen > 0)
	{
		th->resize(th->size()+argslen);
		std::map<uint32_t,asAtom> tmp;
		for (uint32_t i = 0; i< th->data_first.size(); i++)
		{
			tmp[i+argslen]=th->data_first[i];
		}
		for (auto it=th->data_second.begin(); it != th->data_second.end(); ++it )
		{
			tmp[it->first+argslen]=it->second;
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			tmp[i] = args[i];
			ASATOM_INCREF(args[i]);
		}
		th->data_first.clear();
		th->data_second.clear();
		for (auto it=tmp.begin(); it != tmp.end(); ++it )
		{
			th->set(it->first,it->second,false);
		}
	}
	ret.setUInt((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_push)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj.is<Vector>())
		{
			Vector::push(ret,sys,obj,args,argslen);
			return;
		}
		if (obj.is<ByteArray>())
		{
			ByteArray::push(ret,sys,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o;
		obj.getObject()->getVariableByMultiname(o,lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		ret.setUndefined();
		return;
	}
	Array* th=obj.as<Array>();
	uint64_t s = th->currentsize;
	for(unsigned int i=0;i<argslen;i++)
	{
		th->push(args[i]);
	}
	// currentsize is set even if push fails
	th->currentsize = s+argslen;
	ret.setInt((int32_t)th->size());
}
// AS3 handles push on uint.MAX_VALUE differently than ECMA, so we need to push methods
ASFUNCTIONBODY_ATOM(Array,_push_as3)
{
	if (!obj.is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj.is<Vector>())
		{
			Vector::push(ret,sys,obj,args,argslen);
			return;
		}
		if (obj.is<ByteArray>())
		{
			ByteArray::push(ret,sys,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(sys,AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o;
		obj.getObject()->getVariableByMultiname(o,lengthName,SKIP_IMPL);
		uint32_t res = o.toUInt();
		asAtom v = asAtom(res+argslen);
		obj.getObject()->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED);
		ret.setUndefined();
		return;
	}
	Array* th=obj.as<Array>();
	for(unsigned int i=0;i<argslen;i++)
	{
		if (th->size() >= UINT32_MAX)
			break;
		th->push(args[i]);
	}
	ret.setInt((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_map)
{
	Array* th=obj.as<Array>();

	if(argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Array.map", "1", Integer::toString(argslen));
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"map",th->getClass()->getQualifiedClassName());
	asAtom func;
	if (!args[0].is<RegExp>())
	{
		assert_and_throw(args[0].type==T_FUNCTION);
		func=args[0];
	}
	Array* arrayRet=Class<Array>::getInstanceSNoArgs(th->getSystemState());

	asAtom params[3];
	uint32_t index = 0;
	uint32_t s = th->size(); // remember current size, as it may change inside the called function
	while (index < s)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if(a.type!=T_INVALID)
				params[0] = a;
			else
				params[0]=asAtom::undefinedAtom;
		}
		else
		{
			auto it=th->data_second.find(index);
			if(it->second.type!=T_INVALID)
				params[0]=it->second;
			else
				params[0]=asAtom::undefinedAtom;
		}
		params[1] = asAtom(index-1);
		params[2] = asAtom::fromObject(th);
		asAtom funcRet;
		if (func.type != T_INVALID)
		{
			func.callFunction(funcRet,argslen > 1? args[1] : asAtom::nullAtom, params, 3,false);
		}
		else
		{
			RegExp::exec(funcRet,sys,args[0],args,1);
		}
		assert_and_throw(funcRet.type != T_INVALID);
		arrayRet->push(funcRet);
	}

	ret = asAtom::fromObject(arrayRet);
}

ASFUNCTIONBODY_ATOM(Array,_toString)
{
	if(obj.getObject() == Class<Number>::getClass(sys)->prototype->getObj())
	{
		ret = asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!obj.is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toString");
		ret = asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"toString",th->getClass()->getQualifiedClassName());
	ret = asAtom::fromObject(abstract_s(sys,th->toString_priv()));
}

ASFUNCTIONBODY_ATOM(Array,_toLocaleString)
{
	if(obj.getObject() == Class<Number>::getClass(sys)->prototype->getObj())
	{
		ret = asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!obj.is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toLocaleString");
		ret = asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	
	Array* th=obj.as<Array>();
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,"toLocaleString",th->getClass()->getQualifiedClassName());
	ret = asAtom::fromObject(abstract_s(sys,th->toString_priv(true)));
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
		th->set(th->currentsize-1,o,false);
	}
	else
	{
		std::map<uint32_t,asAtom> tmp;
		if (index < ARRAY_SIZE_THRESHOLD)
			th->data_first.insert(th->data_first.begin()+index,o);
		auto it=th->data_second.begin();
		for (; it != th->data_second.end(); ++it )
		{
			tmp[it->first+(it->first >= (uint32_t)index ? 1 : 0)]=it->second;
		}
		if (th->data_first.size() > ARRAY_SIZE_THRESHOLD)
		{
			tmp[ARRAY_SIZE_THRESHOLD] = th->data_first[ARRAY_SIZE_THRESHOLD];
			th->data_first.pop_back();
		}
		th->data_second.clear();
		auto ittmp = tmp.begin();
		while (ittmp != tmp.end())
		{
			th->set(ittmp->first,ittmp->second,false);
			ittmp++;
		}
		th->currentsize++;
		th->set(index,o,false);
	}
}

ASFUNCTIONBODY_ATOM(Array,removeAt)
{
	Array* th=obj.as<Array>();
	int32_t index;
	ARG_UNPACK_ATOM(index);
	if (index < 0)
		index = th->currentsize+index;
	if (index < 0)
		index = 0;
	ret.setUndefined();
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if ((uint32_t)index < th->data_first.size())
		{
			ret = th->data_first[index];
			if (ret.type == T_INVALID)
				ret.setUndefined();
			th->data_first.erase(th->data_first.begin()+index);
		}
	}
	else
	{
		auto it = th->data_second.find(index);
		if(it != th->data_second.end())
		{
			ret = it->second;
			if (ret.type == T_INVALID)
				ret.setUndefined();
			th->data_second.erase(it);
		}
	}
	if ((uint32_t)index < th->currentsize)
		th->currentsize--;
	std::map<uint32_t,asAtom> tmp;
	auto it=th->data_second.begin();
	for (; it != th->data_second.end(); ++it )
	{
		if (it->first == ARRAY_SIZE_THRESHOLD)
			th->data_first[ARRAY_SIZE_THRESHOLD-1]=it->second;
		else
			tmp[it->first-(it->first > (uint32_t)index ? 1 : 0)]=it->second;
	}
	th->data_second.clear();
	th->data_second.insert(tmp.begin(),tmp.end());
}
int32_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<size())
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			return data_first.size() > index ? data_first.at(index).toInt() : 0;
		}
		auto it = data_second.find(index);
		if (it == data_second.end())
			return 0;
		return it->second.toInt();
	}

	return ASObject::getVariableByMultiname_i(name);
}

GET_VARIABLE_RESULT Array::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}

	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}

	if (getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError,name.normalizedNameUnresolved(getSystemState()),getClass()->getQualifiedClassName());
	
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if (data_first.size() > index)
		{
			ret = data_first.at(index);
			ASATOM_INCREF(ret);
			if (ret.type != T_INVALID)
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
	}
	auto it = data_second.find(index);
	if(it != data_second.end())
	{
		ASATOM_INCREF(it->second);
		ret = it->second;
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	if (name.hasEmptyNS)
	{
		//Check prototype chain
		Prototype* proto = this->getClass()->prototype.getPtr();
		while(proto)
		{
			GET_VARIABLE_RESULT res = proto->getObj()->getVariableByMultiname(ret,name, opt);
			if(ret.type != T_INVALID)
				return res;
			proto = proto->prevPrototype.getPtr();
		}
	}
	if(index<size())
		ret.setUndefined();
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
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
	asAtom v(value);
	set(index,v,false);
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

	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		return false;
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		return data_first.size() > index ? (data_first.at(index).type != T_INVALID) : false;
	}
	
	return (data_second.find(index) != data_second.end());
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
			if (name.name_s_id >= '0' && name.name_s_id <= '9')
			{
				index = name.name_s_id - '0';
				return true;
			}
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

	set(index, o,false);
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
	if (index < data_first.size())
	{
		ASATOM_DECREF(data_first.at(index));
		data_first[index]=asAtom::invalidAtom;
		return true;
	}
	
	auto it = data_second.find(index);
	if(it == data_second.end())
		return true;
	ASATOM_DECREF(it->second);
	data_second.erase(it);
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
		asAtom sl;
		if (i < ARRAY_SIZE_THRESHOLD)
		{
			if (i < data_first.size())
				sl = data_first[i];
		}
		else
		{
			auto it = data_second.find(i);
			if(it != data_second.end())
				sl = it->second;
		}
		if(sl.type != T_UNDEFINED && sl.type != T_NULL && sl.type != T_INVALID)
		{
			if (localized)
				ret += sl.toLocaleString().raw_buf();
			else
				ret += sl.toString(getSystemState()).raw_buf();
		}
		if(i!=size()-1)
			ret+=',';
	}
	return ret;
}

void Array::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
	{
		--index;
		if (index < ARRAY_SIZE_THRESHOLD)
			ret = data_first.at(index);
		else
		{
			auto it = data_second.find(index);
			if(it == data_second.end() || it->first != index)
				ret.setUndefined();
			else
				ret = it->second;
		}
		if(ret.type == T_INVALID)
			ret.setUndefined();
		else
		{
			ASATOM_INCREF(ret);
		}
	}
	else
	{
		//Fall back on object properties
		ASObject::nextValue(ret,index-size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	uint32_t s = size();
	if(cur_index<s)
	{
		uint32_t firstsize = data_first.size();
		while (cur_index < ARRAY_SIZE_THRESHOLD && cur_index<s && cur_index < firstsize && data_first.at(cur_index).type == T_INVALID)
		{
			cur_index++;
		}
		if(cur_index<firstsize)
			return cur_index+1;
		
		while (!data_second.count(cur_index) && cur_index<s)
		{
			cur_index++;
		}
		if(cur_index<s)
			return cur_index+1;
	}
	//Fall back on object properties
	uint32_t ret=ASObject::nextNameIndex(cur_index-s);
	if(ret==0)
		return 0;
	else
		return ret+s;
	
}

void Array::nextName(asAtom& ret, uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
		ret.setUInt(index-1);
	else
	{
		//Fall back on object properties
		ASObject::nextName(ret,index-size());
	}
}

asAtom Array::at(unsigned int index)
{
	if(size()<=index)
		outofbounds(index);
	
	asAtom ret;
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if (index < data_first.size())
			ret = data_first.at(index);
	}
	else
	{
		auto it = data_second.find(index);
		if (it != data_second.end())
			ret = it->second;
	}
	if(ret.type != T_INVALID)
	{
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
		if (n < data_first.size())
		{
			auto it1 = data_first.begin()+n;
			while (it1 != data_first.end())
			{
				ASATOM_DECREF((*it1));
				it1 = data_first.erase(it1);
			}
		}
		auto it2=data_second.begin();
		while (it2 != data_second.end())
		{
			if (it2->first >= n)
			{
				auto it2a = it2;
				++it2;
				ASATOM_DECREF(it2a->second);
				data_second.erase(it2a);
			}
			else
				++it2;
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
			if (i < ARRAY_SIZE_THRESHOLD)
			{
				if (data_first.at(i).type == T_INVALID)
					out->writeByte(null_marker);
				else
					data_first.at(i).toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
			}
			else
			{
				if (data_second.find(i) == data_second.end())
					out->writeByte(null_marker);
				else
					data_second.at(i).toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
			}
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
		asAtom a;
		if ( i < ARRAY_SIZE_THRESHOLD)
			a = data_first.at(i);
		else
		{
			auto it = data_second.find(i);
			if (it != data_second.end())
				a = it->second;
		}
		tiny_string subres;
		if (replacer.type != T_INVALID && a.type != T_INVALID)
		{
			asAtom params[2];
			
			params[0] = asAtom(i);
			params[1] = a;
			asAtom funcret;
			replacer.callFunction(funcret,asAtom::nullAtom, params, 2,false);
			if (funcret.type != T_INVALID)
				subres = funcret.toObject(getSystemState())->toJSON(path,asAtom::invalidAtom,spaces,filter);
		}
		else
		{
			ASObject* o = a.type == T_INVALID ? getSystemState()->getNullRef() : a.toObject(getSystemState());
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

void Array::set(unsigned int index, asAtom& o, bool checkbounds, bool addref)
{
	if(index<currentsize)
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			if (index < data_first.size())
				ASATOM_DECREF(data_first.at(index));
			else
				data_first.resize(index+1);
			if (addref)
				ASATOM_INCREF(o);
			
			data_first[index]=o;
		}
		else
		{
			if(data_second.find(index) != data_second.end())
				ASATOM_DECREF(data_second[index]);
			if (addref)
				ASATOM_INCREF(o);
			data_second[index]=o;
		}
	}
	else if (checkbounds)
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
			asAtom o;
			getVariableByMultiname(o,lengthName,SKIP_IMPL);
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
	if (getSystemState()->getSwfVersion() > 12 && getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"push",getClass()->getQualifiedClassName());
	currentsize++;
	set(currentsize-1,o);
}

