/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/Vector.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void Vector::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(getFixed),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(setFixed),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,Class<IFunction>::getFunction(every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,Class<IFunction>::getFunction(filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,Class<IFunction>::getFunction(forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,Class<IFunction>::getFunction(join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,Class<IFunction>::getFunction(_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,Class<IFunction>::getFunction(_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,Class<IFunction>::getFunction(_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,Class<IFunction>::getFunction(shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,Class<IFunction>::getFunction(some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,Class<IFunction>::getFunction(_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,Class<IFunction>::getFunction(splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",AS3,Class<IFunction>::getFunction(unshift),NORMAL_METHOD,true);
	

	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("concat",AS3,Class<IFunction>::getFunction(_concat),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("every",AS3,Class<IFunction>::getFunction(every),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("filter",AS3,Class<IFunction>::getFunction(filter),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("forEach",AS3,Class<IFunction>::getFunction(forEach),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("join",AS3,Class<IFunction>::getFunction(join),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("map",AS3,Class<IFunction>::getFunction(_map),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("pop",AS3,Class<IFunction>::getFunction(_pop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("push",AS3,Class<IFunction>::getFunction(push),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("reverse",AS3,Class<IFunction>::getFunction(_reverse),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("shift",AS3,Class<IFunction>::getFunction(shift),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("slice",AS3,Class<IFunction>::getFunction(slice),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("some",AS3,Class<IFunction>::getFunction(some),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sort",AS3,Class<IFunction>::getFunction(_sort),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("splice",AS3,Class<IFunction>::getFunction(splice),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift",AS3,Class<IFunction>::getFunction(unshift),DYNAMIC_TRAIT);
}

Vector::Vector(Class_base* c, Type *vtype):ASObject(c),vec_type(vtype),fixed(false),vec(reporter_allocator<ASObject*>(c->memoryAccount))
{
}

Vector::~Vector()
{
	finalize();
}

void Vector::finalize()
{
	for(unsigned int i=0;i<size();i++)
	{
		if(vec[i])
			vec[i]->decRef();
	}
	vec.clear();
	ASObject::finalize();
}

void Vector::setTypes(const std::vector<Type*>& types)
{
	assert(vec_type == NULL);
	assert_and_throw(types.size() == 1);
	vec_type = types[0];
}

ASObject* Vector::generator(TemplatedClass<Vector>* o_class, ASObject* const* args, const unsigned int argslen)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(args[0]->getClass());
	assert_and_throw(o_class->getTypes().size() == 1);

	Type* type = o_class->getTypes()[0];

	if(args[0]->getClass() == Class<Array>::getClass())
	{
		//create object without calling _constructor
		Vector* ret = o_class->getInstance(false,NULL,0);

		Array* a = static_cast<Array*>(args[0]);
		for(unsigned int i=0;i<a->size();++i)
		{
			ASObject* obj = a->at(i).getPtr();
			obj->incRef();
			//Convert the elements of the array to the type of this vector
			ret->vec.push_back( type->coerce(obj) );
		}
		return ret;
	}
	else if(args[0]->getClass()->getTemplate() == Template<Vector>::getTemplate())
	{
		Vector* arg = static_cast<Vector*>(args[0]);

		//create object without calling _constructor
		Vector* ret = o_class->getInstance(false,NULL,0);
		for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
		{
			(*i)->incRef();
			ret->vec.push_back( type->coerce(*i) );
		}
		return ret;
	}
	else
	{
		throw Class<ArgumentError>::getInstanceS("global Vector() function takes Array or Vector");
	}
}

ASFUNCTIONBODY(Vector,_constructor)
{
	uint32_t len;
	bool fixed;
	ARG_UNPACK (len, 0) (fixed, false);
	assert_and_throw(argslen <= 2);
	ASObject::_constructor(obj,NULL,0);

	Vector* th=static_cast< Vector *>(obj);
	assert(th->vec_type);
	th->fixed = fixed;
	th->vec.resize(len, NULL);

	return NULL;
}

ASFUNCTIONBODY(Vector,_concat)
{
	Vector* th=static_cast<Vector*>(obj);
	Vector* ret= (Vector*)obj->getClass()->getInstance(true,NULL,0);
	// copy values into new Vector
	ret->vec.resize(th->size(), NULL);
	auto it=th->vec.begin();
	uint32_t index = 0;
	for(;it != th->vec.end();++it)
	{
		if (*it)
		{
			ret->vec[index]=*it;
			ret->vec[index]->incRef();
		}
		index++;
	}
	//Insert the arguments in the vector
	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[i]->is<Vector>())
		{
			Vector* arg=static_cast<Vector*>(args[i]);
			ret->vec.resize(index+arg->size(), NULL);
			auto it=arg->vec.begin();
			for(;it != arg->vec.end();++it)
			{
				if (*it)
				{
					// force Class_base to ensure that a TypeError is thrown 
					// if the object type does not match the base vector type
					ret->vec[index]=((Class_base*)th->vec_type)->Class_base::coerce(*it);
					ret->vec[index]->incRef();
				}
				index++;
			}
		}
		else
		{
			ret->vec[index] = th->vec_type->coerce(args[i]);
			ret->vec[index]->incRef();
			index++;
		}
	}	
	return ret;
}

ASFUNCTIONBODY(Vector,filter)
{
	if (argslen < 1 || argslen > 2)
		throw Class<ArgumentError>::getInstanceS("Error #1063"); 
	if (!args[0]->is<IFunction>())
		throw Class<TypeError>::getInstanceS("Error #1034"); 
	Vector* th=static_cast<Vector*>(obj);
	  
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	Vector* ret= (Vector*)obj->getClass()->getInstance(true,NULL,0);
	ASObject *funcRet;

	for(unsigned int i=0;i<th->size();i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = th->vec[i];
		th->vec[i]->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(getSys()->getNullRef(), params, 3);
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
				th->vec[i]->incRef();
				ret->vec.push_back(th->vec[i]);
			}
			funcRet->decRef();
		}
	}
	return ret;
}

ASFUNCTIONBODY(Vector, some)
{
	if (argslen < 1)
		throw Class<ArgumentError>::getInstanceS("Error #1063");
	if (!args[0]->is<IFunction>())
		throw Class<TypeError>::getInstanceS("Error #1034"); 
	Vector* th=static_cast<Vector*>(obj);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	ASObject *funcRet;

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = th->vec[i];
		th->vec[i]->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(getSys()->getNullRef(), params, 3);
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

ASFUNCTIONBODY(Vector, every)
{
	Vector* th=static_cast<Vector*>(obj);
	if (argslen < 1)
		throw Class<ArgumentError>::getInstanceS("Error #1063");
	if (!args[0]->is<IFunction>())
		throw Class<TypeError>::getInstanceS("Error #1034"); 
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];
	ASObject *funcRet;

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (th->vec[i])
		{
			params[0] = th->vec[i];
			th->vec[i]->incRef();
		}
		else
			params[0] = getSys()->getNullRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(getSys()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if (funcRet->is<Undefined>() || funcRet->is<Null>())
				throw Class<TypeError>::getInstanceS("Error #1006");
			if(!Boolean_concrete(funcRet))
			{
				return funcRet;
			}
			funcRet->decRef();
		}
	}
	return abstract_b(true);
}

ASFUNCTIONBODY(Vector,push)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	for(size_t i = 0; i < argslen; ++i)
	{
		args[i]->incRef();
		//The proprietary player violates the specification and allows elements of any type to be pushed;
		//they are converted to the vec_type
		th->vec.push_back( th->vec_type->coerce(args[i]) );
	}
	return abstract_ui(th->vec.size());
}

ASFUNCTIONBODY(Vector,_pop)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	uint32_t size =th->size();
	if (size == 0)
        return th->vec_type->coerce(getSys()->getNullRef());
	ASObject* ret = th->vec[size-1];
	if (!ret)
		ret = th->vec_type->coerce(getSys()->getNullRef());
	th->vec.pop_back();
	return ret;
}

ASFUNCTIONBODY(Vector,getLength)
{
	return abstract_ui(obj->as<Vector>()->vec.size());
}

ASFUNCTIONBODY(Vector,setLength)
{
	Vector* th = obj->as<Vector>();
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	uint32_t len;
	ARG_UNPACK (len);
	if(len <= th->vec.size())
	{
		for(size_t i=len; i< th->vec.size(); ++i)
			if(th->vec[i])
				th->vec[i]->decRef();
	}
	th->vec.resize(len, NULL);
	return NULL;
}

ASFUNCTIONBODY(Vector,getFixed)
{
	return abstract_b(obj->as<Vector>()->fixed);
}

ASFUNCTIONBODY(Vector,setFixed)
{
	Vector* th = obj->as<Vector>();
	bool fixed;
	ARG_UNPACK (fixed);
	th->fixed = fixed;
	return NULL;
}

ASFUNCTIONBODY(Vector,forEach)
{
	if (argslen < 1)
		throw Class<ArgumentError>::getInstanceS("Error #1063");
	if (!args[0]->is<IFunction>())
		throw Class<TypeError>::getInstanceS("Error #1034"); 
	Vector* th=static_cast<Vector*>(obj);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = th->vec[i];
		th->vec[i]->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		ASObject *funcret;
		if( argslen == 1 )
		{
			funcret=f->call(getSys()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcret=f->call(args[1], params, 3);
		}
		if(funcret)
			funcret->decRef();
	}

	return NULL;
}

ASFUNCTIONBODY(Vector, _reverse)
{
	Vector* th = static_cast<Vector*>(obj);

	std::vector<ASObject*> tmp = std::vector<ASObject*>(th->vec.begin(),th->vec.end());
	uint32_t size = th->size();
	th->vec.clear();
	th->vec.resize(size, NULL);
	std::vector<ASObject*>::iterator it=tmp.begin();
	uint32_t index = size-1;
	for(;it != tmp.end();++it)
 	{
		th->vec[index]=*it;
		index--;
	}
	th->incRef();
	return th;
}

ASFUNCTIONBODY(Vector,lastIndexOf)
{
	Vector* th=static_cast<Vector*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	if(th->vec.size() == 0)
		return abstract_d(-1);

	size_t i = th->size()-1;

	if(argslen == 2 && std::isnan(args[1]->toNumber()))
		return abstract_i(0);

	if(argslen == 2 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
	{
		int j = args[1]->toInt(); //Preserve sign
		if(j < 0) //Negative offset, use it as offset from the end of the array
		{
			if((size_t)-j > th->size())
				i = 0;
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
	}
	do
	{
		if (!th->vec[i])
		    continue;
		if (th->vec[i]->isEqualStrict(arg0))
		{
			ret=i;
			break;
		}
	}
	while(i--);

	return abstract_i(ret);
}

ASFUNCTIONBODY(Vector,shift)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	if(!th->size())
		return th->vec_type->coerce(getSys()->getNullRef());
	ASObject* ret;
	if(th->vec[0])
		ret=th->vec[0];
	else
		ret=th->vec_type->coerce(getSys()->getNullRef());
	for(uint32_t i= 1;i< th->size();i++)
	{
		if (th->vec[i])
		{
			th->vec[i-1]=th->vec[i];
		}
		else if (th->vec[i-1])
		{
			th->vec[i-1] = NULL;
		}
	}
	th->vec.resize(th->size()-1, NULL);
	return ret;
}

int Vector::capIndex(int i) const
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

ASFUNCTIONBODY(Vector,slice)
{
	Vector* th=static_cast<Vector*>(obj);

	int startIndex=0;
	int endIndex=16777215;
	if(argslen>0)
		startIndex=args[0]->toInt();
	if(argslen>1)
		endIndex=args[1]->toInt();

	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);
	Vector* ret= (Vector*)obj->getClass()->getInstance(true,NULL,0);
	ret->vec.resize(endIndex-startIndex, NULL);
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		if (th->vec[i])
		{
			th->vec[i]->incRef();
			ret->vec[j] =th->vec_type->coerce(th->vec[i]);
		}
		j++;
	}
	return ret;
}

ASFUNCTIONBODY(Vector,splice)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	int startIndex=args[0]->toInt();
	//By default, delete all the element up to the end
	//Use the array len, it will be capped below
	int deleteCount=th->size();
	if(argslen > 1)
		deleteCount=args[1]->toUInt();
	int totalSize=th->size();
	Vector* ret= (Vector*)obj->getClass()->getInstance(true,NULL,0);

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	ret->vec.resize(deleteCount, NULL);
	if(deleteCount)
	{
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			if (th->vec[startIndex+i])
				ret->vec[i] = th->vec[startIndex+i];
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			if(th->vec[startIndex+i])
			{
				th->vec[startIndex+i] = NULL;
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<ASObject*> tmp = vector<ASObject*>(totalSize- (startIndex+deleteCount));
	tmp.resize(totalSize- (startIndex+deleteCount), NULL);
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		if (th->vec[i])
		{
			tmp[i-(startIndex+deleteCount)] = th->vec[i];
			th->vec[i] = NULL;
		}
	}
	th->vec.resize(startIndex, NULL);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		args[i]->incRef();
		th->vec.push_back(args[i]);
	}
	// move remembered items to new position
	th->vec.resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0), NULL);
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (tmp[i])
			th->vec[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
	return ret;
}

ASFUNCTIONBODY(Vector,join)
{
	Vector* th=static_cast<Vector*>(obj);
	
	tiny_string del = ",";
	if (argslen == 1)
	      del=args[0]->toString();
	string ret;
	for(uint32_t i=0;i<th->size();i++)
	{
		if (th->vec[i])
			ret+=th->vec[i]->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del.raw_buf();
	}
	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(Vector,indexOf)
{
	Vector* th=static_cast<Vector*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = args[1]->toInt();
	}

	for(;i<th->size();i++)
	{
		if (!th->vec[i])
			continue;
		if(th->vec[i]->isEqualStrict(arg0))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(ret);
}
bool Vector::sortComparatorWrapper::operator()(ASObject* d1, ASObject* d2)
{
	ASObject* objs[2];
	if (d1)
	{
		objs[0] = d1;
		objs[0]->incRef();
	}
	else
		objs[0] = vec_type->coerce(getSys()->getNullRef());
	if (d2)
	{
		objs[1] = d2;
		objs[1]->incRef();
	}
	else
		objs[1] = vec_type->coerce(getSys()->getNullRef());

	assert(comparator);
	_NR<ASObject> ret=_MNR(comparator->call(getSys()->getNullRef(), objs, 2));
	assert_and_throw(ret);
	return (ret->toNumber()<0); //Less
}

ASFUNCTIONBODY(Vector,_sort)
{
	if (argslen != 1)
		throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");
	Vector* th=static_cast<Vector*>(obj);
	
	IFunction* comp=static_cast<IFunction*>(args[0]);
	

	sort(th->vec.begin(),th->vec.end(),sortComparatorWrapper(comp,th->vec_type));
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Vector,unshift)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throw Class<RangeError>::getInstanceS("Error #1126");
	th->vec.resize(th->size()+argslen, NULL);
	for(uint32_t i=th->size();i> 0;i--)
	{
		if (th->vec[i-1])
		{
			th->vec[(i-1)+argslen]=th->vec[i-1];
			th->vec[i-1] = NULL;
		}
		
	}

	for(uint32_t i=0;i<argslen;i++)
	{
		args[i]->incRef();
		th->vec[i] = th->vec_type->coerce(args[i]);
	}
	return abstract_i(th->size());
}

ASFUNCTIONBODY(Vector,_map)
{
	Vector* th=static_cast<Vector*>(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_FUNCTION);
	IFunction* func=static_cast<IFunction*>(args[0]);
	Vector* ret= (Vector*)obj->getClass()->getInstance(true,NULL,0);

	for(uint32_t i=0;i<th->size();i++)
	{
		ASObject* funcArgs[3];
		if (!th->vec[i])
			funcArgs[0]=getSys()->getNullRef();
		else
		{
			if(th->vec[i])
			{
				funcArgs[0]=th->vec[i];
				funcArgs[0]->incRef();
			}
			else
				funcArgs[0]=getSys()->getUndefinedRef();
		}
		funcArgs[1]=abstract_i(i);
		funcArgs[2]=th;
		funcArgs[2]->incRef();
		ASObject* funcRet=func->call(getSys()->getNullRef(), funcArgs, 3);
		assert_and_throw(funcRet);
		ret->vec.push_back(funcRet);
	}

	return ret;
}

ASFUNCTIONBODY(Vector,_toString)
{
	tiny_string ret;
	Vector* th = obj->as<Vector>();
	for(size_t i=0; i < th->vec.size(); ++i)
	{
		if (th->vec[i])
			ret += th->vec[i]->toString();
		else
			// use the type's default value
			ret += th->vec_type->coerce( getSys()->getNullRef() )->toString();

		if(i!=th->vec.size()-1)
			ret += ',';
	}
	return Class<ASString>::getInstanceS(ret);
}
bool Vector::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(!considerDynamic)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	if(!name.ns[0].hasEmptyName())
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	unsigned int index=0;
	if(!Vector::isValidMultiname(name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	if(index < vec.size())
		return true;
	else
		return false;
}

/* this handles the [] operator, because vec[12] becomes vec.12 in bytecode */
_NR<ASObject> Vector::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(!name.ns[0].hasEmptyName())
		return ASObject::getVariableByMultiname(name,opt);

	unsigned int index=0;
	if(!Vector::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,opt);

	if(index < vec.size())
	{
		if (vec[index])
		{
			vec[index]->incRef();
			return _MNR(vec[index]);
		}
		else
			return _MNR(vec_type->coerce( getSys()->getNullRef() ));
	}
	else
	{
		throw Class<RangeError>::getInstanceS("Error #1125");
	}
}

void Vector::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(name.ns.size()>0);
	if(!name.ns[0].hasEmptyName())
		return ASObject::setVariableByMultiname(name, o, allowConst);

	unsigned int index=0;
	if(!Vector::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name, o, allowConst);
	ASObject* o2 = this->vec_type->coerce(o);
	  
	if(index < vec.size())
	{
		if (vec[index])
			vec[index]->decRef();
		vec[index] = o2;
	}
	else if(!fixed && index == vec.size())
	{
		vec.push_back( o2 );
	}
	else
	{
		/* Spec says: one may not set a value with an index more than
		 * one beyond the current final index. */
		throw Class<RangeError>::getInstanceS("Error #1125");
	}
}

tiny_string Vector::toString(bool debugMsg)
{
	//TODO: test
	tiny_string t;
	for(size_t i = 0; i < vec.size(); ++i)
	{
		if( i )
			t += ",";
		if (vec[i]) 
			t += vec[i]->toString();
		else
			t += vec_type->coerce( getSys()->getNullRef() )->toString();
	}
	return t;
}

uint32_t Vector::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < vec.size())
		return cur_index+1;
	else
		return 0;
}

_R<ASObject> Vector::nextName(uint32_t index)
{
	if(index<=vec.size())
		return _MR(abstract_i(index-1));
	else
		throw RunTimeException("Vector::nextName out of bounds");
}

_R<ASObject> Vector::nextValue(uint32_t index)
{
	if(index<=vec.size())
	{
		if (vec[index-1])
		{
			vec[index-1]->incRef();
			return _MR(vec[index-1]);
		}
		return _MR(vec_type->coerce( getSys()->getNullRef() ));
	}
	else
		throw RunTimeException("Vector::nextValue out of bounds");
}

bool Vector::isValidMultiname(const multiname& name, uint32_t& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(!name.ns[0].hasEmptyName())
		return false;

	bool validIndex=name.toUInt(index);
	// Don't throw for non-numeric NAME_STRING or NAME_OBJECT
	// because they can still be valid built-in property names.
	if(!validIndex && (name.name_type==multiname::NAME_INT || name.name_type==multiname::NAME_NUMBER))
		throw Class<RangeError>::getInstanceS("Error #1125");

	return validIndex;
}
