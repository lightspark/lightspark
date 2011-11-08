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

#include "Array.h"
#include "abc.h"
#include "class.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(Array);

Array::Array()
{
	type=T_ARRAY;
}

void Array::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	// public constants
	c->setSuper(Class<ASObject>::getRef());

	c->setVariableByQName("CASEINSENSITIVE","",abstract_d(CASEINSENSITIVE),DECLARED_TRAIT);
	c->setVariableByQName("DESCENDING","",abstract_d(DESCENDING),DECLARED_TRAIT);
	c->setVariableByQName("NUMERIC","",abstract_d(NUMERIC),DECLARED_TRAIT);
	c->setVariableByQName("RETURNINDEXEDARRAY","",abstract_d(RETURNINDEXEDARRAY),DECLARED_TRAIT);
	c->setVariableByQName("UNIQUESORT","",abstract_d(UNIQUESORT),DECLARED_TRAIT);

	// properties
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),GETTER_METHOD,true);

	// public functions
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,Class<IFunction>::getFunction(every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,Class<IFunction>::getFunction(filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,Class<IFunction>::getFunction(forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,Class<IFunction>::getFunction(join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,Class<IFunction>::getFunction(_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,Class<IFunction>::getFunction(_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(_push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,Class<IFunction>::getFunction(_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,Class<IFunction>::getFunction(shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,Class<IFunction>::getFunction(some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,Class<IFunction>::getFunction(_sort),NORMAL_METHOD,true);
	//c->setDeclaredMethodByQName("sortOn",AS3,Class<IFunction>::getFunction(sortOn),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,Class<IFunction>::getFunction(splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("unshift",AS3,Class<IFunction>::getFunction(unshift),NORMAL_METHOD,true);

	// workaround, pop was encountered not in the AS3 namespace before, need to investigate it further
	c->setDeclaredMethodByQName("pop","",Class<IFunction>::getFunction(_pop),NORMAL_METHOD,true);
}

void Array::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Array,_constructor)
{
	Array* th=static_cast<Array*>(obj);

	if(argslen==1 && args[0]->getObjectType()==T_INTEGER)
	{
		int size=args[0]->toInt();
		LOG(LOG_CALLS,_("Creating array of length ") << size);
		th->resize(size);
	}
	else
	{
		LOG(LOG_CALLS,_("Called Array constructor"));
		th->resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			th->set(i,args[i]);
			args[i]->incRef();
		}
	}
	return NULL;
}

ASFUNCTIONBODY(Array,generator)
{
	Array* arr=Class<Array>::getInstanceS();
	arr->resize(argslen);
	for(unsigned int i=0;i<argslen;i++)
	{
		args[i]->incRef();
		arr->set(i,args[i]);
	}
	return arr;
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ret->data=th->data;
	if(argslen==1 && args[0]->getObjectType()==T_ARRAY)
	{
		Array* tmp=Class<Array>::cast(args[0]);
		ret->data.insert(ret->data.end(),tmp->data.begin(),tmp->data.end());
	}
	else
	{
		//Insert the arguments in the array
		ret->data.reserve(ret->data.size()+argslen);
		for(unsigned int i=0;i<argslen;i++)
			ret->push(args[i]);
	}

	//All the elements in the new array should be increffed, as args will be deleted and
	//this array could die too
	for(unsigned int i=0;i<ret->data.size();i++)
	{
		if(ret->data[i].type==DATA_OBJECT)
			ret->data[i].data->incRef();
	}

	return ret;
}

ASFUNCTIONBODY(Array,filter)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	Array* ret=Class<Array>::getInstanceS();
	ASObject *funcRet;

	for(unsigned int i=0;i<th->data.size();i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		th->data[i].data->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(new Null, params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(Boolean_concrete(funcRet))
			{
				th->data[i].data->incRef();
				ret->push(th->data[i].data);
			}
			funcRet->decRef();
		}
	}
	return ret;
}

ASFUNCTIONBODY(Array, some)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	ASObject *funcRet;

	for(unsigned int i=0; i < th->data.size(); i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		th->data[i].data->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(new Null, params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(Boolean_concrete(funcRet))
			{
				return funcRet;
			}
			funcRet->decRef();
		}
	}
	return abstract_b(false);
}

ASFUNCTIONBODY(Array, every)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	ASObject *funcRet;

	for(unsigned int i=0; i < th->data.size(); i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		th->data[i].data->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(new Null, params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(!Boolean_concrete(funcRet))
			{
				return funcRet;
			}
			funcRet->decRef();
		}
	}
	return abstract_b(true);
}

ASFUNCTIONBODY(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj);
	return abstract_i(th->data.size());
}

ASFUNCTIONBODY(Array,forEach)
{
	assert_and_throw(argslen == 1 || argslen == 2);
	Array* th=static_cast<Array*>(obj);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];

	for(unsigned int i=0; i < th->data.size(); i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		th->data[i].data->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if( argslen == 1 )
		{
			f->call(new Null, params, 3);
		}
		else
		{
			args[1]->incRef();
			f->call(args[1], params, 3);
		}
	}

	return NULL;
}

ASFUNCTIONBODY(Array, _reverse)
{
	Array* th = static_cast<Array*>(obj);

	reverse(th->data.begin(), th->data.end());

	th->incRef();
	return th;
}

ASFUNCTIONBODY(Array,lastIndexOf)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	if(th->data.empty())
		return abstract_d(0);

	size_t i = th->data.size()-1;

	if(argslen == 2 && std::isnan(args[1]->toNumber()))
		return abstract_i(0);

	if(argslen == 2 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
	{
		int j = args[1]->toInt(); //Preserve sign
		if(j < 0) //Negative offset, use it as offset from the end of the array
		{
			if((size_t)-j > th->data.size())
				i = 0;
			else
				i = th->data.size()+j;
		}
		else //Positive offset, use it directly
		{
			if((size_t)j > th->data.size()) //If the passed offset is bigger than the array, cap the offset
				i = th->data.size()-1;
			else
				i = j;
		}
	}

	DATA_TYPE dtype = th->data[i].type;
	do
	{
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		dtype = th->data[i].type;
		if((dtype == DATA_OBJECT && ABCVm::strictEqualImpl(th->data[i].data,arg0)) ||
			(dtype == DATA_INT && arg0->toInt() == th->data[i].data_i))
		{
			ret=i;
			break;
		}
	}
	while(i--);

	return abstract_i(ret);
}

ASFUNCTIONBODY(Array,shift)
{
	Array* th=static_cast<Array*>(obj);
	if(th->data.empty())
		return new Undefined;
	ASObject* ret;
	if(th->data[0].type==DATA_OBJECT)
		ret=th->data[0].data;
	else
		throw UnsupportedException("Array::shift not completely implemented");
	th->data.erase(th->data.begin());
	return ret;
}

int Array::capIndex(int i) const
{
	int totalSize=data.size();

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

	int startIndex=0;
	int endIndex=16777215;
	if(argslen>0)
		startIndex=args[0]->toInt();
	if(argslen>1)
		endIndex=args[1]->toInt();

	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);

	Array* ret=Class<Array>::getInstanceS();
	for(int i=startIndex; i<endIndex; i++) {
		th->data[i].data->incRef();
		ret->data.push_back(th->data[i]);
	}
	return ret;
}

ASFUNCTIONBODY(Array,splice)
{
	Array* th=static_cast<Array*>(obj);

	int startIndex=args[0]->toInt();
	int deleteCount=args[1]->toUInt();
	int totalSize=th->data.size();
	Array* ret=Class<Array>::getInstanceS();

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	if(deleteCount)
	{
		ret->data.reserve(deleteCount);

		for(int i=0;i<deleteCount;i++)
			ret->data.push_back(th->data[startIndex+i]);

		th->data.erase(th->data.begin()+startIndex,th->data.begin()+startIndex+deleteCount);
	}

	//Insert requested values starting at startIndex
	for(unsigned int i=2,n=0;i<argslen;i++,n++)
	{
		args[i]->incRef();
		th->data.insert(th->data.begin()+startIndex+n,data_slot(args[i]));
	}

	return ret;
}

ASFUNCTIONBODY(Array,join)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* del=args[0];
	string ret;
	for(int i=0;i<th->size();i++)
	{
		ret+=th->at(i)->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del->toString().raw_buf();
	}
	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(Array,indexOf)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = args[1]->toInt();
	}

	DATA_TYPE dtype;
	for(;i<th->data.size();i++)
	{
		dtype = th->data[i].type;
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		if((dtype == DATA_OBJECT && ABCVm::strictEqualImpl(th->data[i].data,arg0)) ||
			(dtype == DATA_INT && arg0->toInt() == th->data[i].data_i))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(ret);
}


ASFUNCTIONBODY(Array,_pop)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* ret;
	if(!th->data.empty() && th->data.back().type==DATA_OBJECT)
		ret=th->data.back().data;
	else
		throw UnsupportedException("Array::pop not completely implemented");
	th->data.pop_back();
	return ret;
}

bool Array::sortComparatorDefault::operator()(const data_slot& d1, const data_slot& d2)
{
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		if(d1.type==DATA_INT)
			a=d1.data_i;
		else if(d1.type==DATA_OBJECT && d1.data)
			a=d1.data->toNumber();

		if(d2.type==DATA_INT)
			b=d2.data_i;
		else if(d2.type==DATA_OBJECT && d2.data)
			b=d2.data->toNumber();

		if(std::isnan(a) || std::isnan(b))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1;
		tiny_string s2;
		if(d1.type==DATA_INT)
			s1=Integer::toString(d1.data_i);
		else if(d1.type==DATA_OBJECT && d1.data)
			s1=d1.data->toString();
		else
			s1="undefined";
		if(d2.type==DATA_INT)
			s2=Integer::toString(d2.data_i);
		else if(d2.type==DATA_OBJECT && d2.data)
			s2=d2.data->toString();
		else
			s2="undefined";

		//TODO: unicode support
		if(isCaseInsensitive)
			return strcasecmp(s1.raw_buf(),s2.raw_buf())<0;
		else
			return s1<s2;
	}
}

bool Array::sortComparatorWrapper::operator()(const data_slot& d1, const data_slot& d2)
{
	ASObject* objs[2];
	if(d1.type==DATA_INT)
		objs[0]=abstract_i(d1.data_i);
	else if(d1.type==DATA_OBJECT && d1.data)
	{
		objs[0]=d1.data;
		objs[0]->incRef();
	}
	else
		objs[0]=new Undefined;

	if(d2.type==DATA_INT)
		objs[1]=abstract_i(d2.data_i);
	else if(d2.type==DATA_OBJECT && d2.data)
	{
		objs[1]=d2.data;
		objs[1]->incRef();
	}
	else
		objs[1]=new Undefined;

	assert(comparator);
	ASObject* ret=comparator->call(new Null, objs, 2);
	assert_and_throw(ret);
	return (ret->toInt()<0); //Less
}

ASFUNCTIONBODY(Array,_sort)
{
	Array* th=static_cast<Array*>(obj);
	IFunction* comp=NULL;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(args[i]->getObjectType()==T_FUNCTION) //Comparison func
		{
			assert_and_throw(comp==NULL);
			comp=static_cast<IFunction*>(args[i]);
		}
		else
		{
			uint32_t options=args[i]->toInt();
			if(options&NUMERIC)
				isNumeric=true;
			if(options&CASEINSENSITIVE)
				isCaseInsensitive=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}

	if(comp)
		sort(th->data.begin(),th->data.end(),sortComparatorWrapper(comp));
	else
		sort(th->data.begin(),th->data.end(),sortComparatorDefault(isNumeric,isCaseInsensitive));

	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,sortOn)
{
//	Array* th=static_cast<Array*>(obj);
/*	if(th->data.size()>1)
		throw UnsupportedException("Array::sort not completely implemented");
	LOG(LOG_NOT_IMPLEMENTED,_("Array::sort not really implemented"));*/
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,unshift)
{
	Array* th=static_cast<Array*>(obj);
	for(uint32_t i=0;i<argslen;i++)
	{
		th->data.insert(th->data.begin(),data_slot(args[i],DATA_OBJECT));
		args[i]->incRef();
	}
	return abstract_i(th->size());;
}

ASFUNCTIONBODY(Array,_push)
{
	Array* th=static_cast<Array*>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		th->push(args[i]);
		args[i]->incRef();
	}
	return abstract_i(th->size());
}

ASFUNCTIONBODY(Array,_map)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_FUNCTION);
	IFunction* func=static_cast<IFunction*>(args[0]);
	Array* arrayRet=Class<Array>::getInstanceS();

	for(uint32_t i=0;i<th->data.size();i++)
	{
		ASObject* funcArgs[3];
		const data_slot& slot=th->data[i];
		if(slot.type==DATA_INT)
			funcArgs[0]=abstract_i(slot.data_i);
		else if(slot.type==DATA_OBJECT && slot.data)
		{
			funcArgs[0]=slot.data;
			funcArgs[0]->incRef();
		}
		else
			funcArgs[0]=new Undefined;

		funcArgs[1]=abstract_i(i);
		funcArgs[2]=th;
		funcArgs[2]->incRef();
		ASObject* funcRet=func->call(new Null, funcArgs, 3);
		assert_and_throw(funcRet);
		arrayRet->push(funcRet);
	}

	return arrayRet;
}

ASFUNCTIONBODY(Array,_toString)
{
	Array* th=static_cast<Array*>(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

int32_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<data.size())
	{
		switch(data[index].type)
		{
			case DATA_OBJECT:
			{
				assert(data[index].data!=NULL);
				if(data[index].data->getObjectType()==T_INTEGER)
				{
					Integer* i=static_cast<Integer*>(data[index].data);
					return i->toInt();
				}
				else if(data[index].data->getObjectType()==T_NUMBER)
				{
					Number* i=static_cast<Number*>(data[index].data);
					return i->toInt();
				}
				else
					throw UnsupportedException("Array::getVariableByMultiname_i not completely implemented");
			}
			case DATA_INT:
				return data[index].data_i;
		}
	}

	return ASObject::getVariableByMultiname_i(name);
}

_NR<ASObject> Array::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,opt);

	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,opt);

	if(index<data.size())
	{
		ASObject* ret=NULL;
		switch(data[index].type)
		{
			case DATA_OBJECT:
				ret=data[index].data;
				if(ret==NULL)
				{
					ret=new Undefined;
					data[index].data=ret;
				}
				ret->incRef();
				break;
			case DATA_INT:
				ret=abstract_i(data[index].data_i);
				break;
		}
		return _MNR(ret);
	}
	else
		return NullRef;
}

void Array::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();
	data[index].data_i=value;
	data[index].type=DATA_INT;
}


bool Array::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	return index<data.size();
}

bool Array::isValidMultiname(const multiname& name, unsigned int& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(name.ns[0].name!="")
		return false;

	index=0;
	switch(name.name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
			for(auto i=name.name_s.begin(); i!=name.name_s.end(); ++i)
			{
				if(!i.isdigit())
					return false;

				index*=10;
				index+=i.digit_value();
			}
			break;
		//This is already an int, so its good enough
		case multiname::NAME_INT:
			index=name.name_i;
			break;
		case multiname::NAME_NUMBER:
			//TODO: check that this is really an integer
			index=name.name_d;
			break;
		case multiname::NAME_OBJECT:
			//TODO: should be use toPrimitive here?
			return false;
		default:
			throw UnsupportedException("Array::isValidMultiname not completely implemented");
	}
	return true;
}

void Array::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o);

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();

	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		data[index].data_i=i->val;
		data[index].type=DATA_INT;
		o->decRef();
	}
	else
	{
		data[index].data=o;
		data[index].type=DATA_OBJECT;
	}
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

tiny_string Array::toString_priv() const
{
	string ret;
	for(unsigned int i=0;i<data.size();i++)
	{
		if(data[i].type==DATA_OBJECT)
		{
			if(data[i].data)
				ret+=data[i].data->toString().raw_buf();
		}
		else if(data[i].type==DATA_INT)
		{
			char buf[20];
			snprintf(buf,20,"%i",data[i].data_i);
			ret+=buf;
		}
		else
			throw UnsupportedException("Array::toString not completely implemented");

		if(i!=data.size()-1)
			ret+=',';
	}
	return ret;
}

_R<ASObject> Array::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		index--;
		if(data[index].type==DATA_OBJECT)
		{
			if(data[index].data==NULL)
				return _MR(new Undefined);
			else
			{
				data[index].data->incRef();
				return _MR(data[index].data);
			}
		}
		else if(data[index].type==DATA_INT)
			return _MR(abstract_i(data[index].data_i));
		else
			throw UnsupportedException("Unexpected data type");
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-data.size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

_R<ASObject> Array::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
		return _MR(abstract_i(index-1));
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-data.size());
	}
}

ASObject* Array::at(unsigned int index) const
{
	if(data.size()<=index)
		outofbounds();

	switch(data[index].type)
	{
		case DATA_OBJECT:
		{
			if(data[index].data)
				return data[index].data;
		}
		case DATA_INT:
			return abstract_i(data[index].data_i);
	}

	//We should be here only if data is an object and is NULL
	return new Undefined;
}

void Array::outofbounds() const
{
	throw ParseException("Array access out of bounds");
}

void Array::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(amf3::array_marker);
	uint32_t denseCount = data.size();
	assert_and_throw(denseCount<0x20000000);
	uint32_t value = (denseCount << 1) | 1;
	out->writeU29(value);
	serializeDynamicProperties(out, stringMap, objMap);
	for(uint32_t i=0;i<denseCount;i++)
	{
		switch(data[i].type)
		{
			case DATA_INT:
				throw UnsupportedException("int not supported in Array::serialize");
			case DATA_OBJECT:
				data[i].data->serialize(out, stringMap, objMap);
		}
	}
}

void Array::finalize()
{
	ASObject::finalize();
	for(unsigned int i=0;i<data.size();i++)
	{
		if(data[i].type==DATA_OBJECT && data[i].data)
			data[i].data->decRef();
	}
	data.clear();
}


