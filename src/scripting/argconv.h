/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Matthias Gehre (M.Gehre@gmx.de)

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

#ifndef SCRIPTING_ARGCONV_H
#define SCRIPTING_ARGCONV_H 1

#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"

/* Usage of ARG_UNPACK:
 * You have to use it within a ASFUNCTIONBODY() { }, because it uses the implicit arguments 'args' and 'argslen'.
 * Standard usage is:
 * int32_t i;
 * bool b;
 * _NR<DisplayObject> o;
 * ARG_UNPACK (i) (b) (o);
 * which coerces the given arguments implicitly (according to ecma) to the types Integer, Boolean, DisplayObject and then puts them
 * into the given variables.
 * ATTENTION: The object 'o' is the same as the argument passed into the function, so changing it will be visible for the caller.
 *
 * A exception will be thrown if a type cannot be coerces implicitly or when not enough arguments are provided by the caller. A LOG_NOT_IMPLEMENTED
 * will be emitted if more than the unpacked arguments are provided by the caller.
 *
 * To specify default values, use the (var, defvalue) operator like
 * ARG_UNPACK (i) (b,true) (o,NullRef);
 * this will work as above if all arguments are supplied. When the second argument is not supplied, no error is thrown and
 * b is set to 'true'. If the third argument is not supplied, not error is thrown and o is set to NullRef. Note that you cannot
 * put a Null into DisplayObject, as Null derives directly from ASObject (i.e. Null is not a subclass DisplayObject).
 * If a 'null' value is provided for an Object type, it's reference is set to a NullRef.
 */

namespace lightspark
{

template<class T>
class ArgumentConversion
{
public:
	static T toConcrete(ASObject* obj);
	static ASObject* toAbstract(const T& val);
};

template<class T>
class ArgumentConversion<Ref<T>>
{
public:
	static Ref<T> toConcrete(ASObject* obj)
	{
		T* o = dynamic_cast<T*>(obj);
		if(!o)
			throw Class<ArgumentError>::getInstanceS("Error #1034: Wrong type");
		o->incRef();
		return _MR(o);
	}
	static ASObject* toAbstract(const Ref<T>& val)
	{
		val->incRef();
		return val.getPtr();
	}
};

/* For objects derived from ASObject, Null is not a subclass. Therefore the Null
 * object is converted to a nullref.
 * ArgumentConversion<NullableRef<T>>::toConcrete(new Null) = nullref
 */
template<class T>
class ArgumentConversion<NullableRef<T>>
{
public:
	static NullableRef<T> toConcrete(ASObject* obj)
	{
		if(obj->getObjectType() == T_NULL)
			return NullRef;

		T* o = dynamic_cast<T*>(obj);
		if(!o)
			throw Class<ArgumentError>::getInstanceS("Error #1034: Wrong type");
		o->incRef();
		return _MNR(o);
	}
	static ASObject* toAbstract(const NullableRef<T>& val)
	{
		if(val.isNull())
			return getSys()->getNullRef();
		val->incRef();
		return val.getPtr();
	}
};

/* For NullableRefs of type ASObject, there is no problem with Null, so it is just passed
 * unconverted.
 * ArgumentConversion<NullableRef<ASObject>>::toConcrete(new Null) = _MNR(new Null)
 */
template<>
class ArgumentConversion<NullableRef<ASObject>>
{
public:
	static NullableRef<ASObject> toConcrete(ASObject* obj)
	{
		obj->incRef();
		return _MNR(obj);
	}
	static ASObject* toAbstract(const NullableRef<ASObject>& val)
	{
		if(val.isNull())
			return getSys()->getNullRef();
		val->incRef();
		return val.getPtr();
	}
};

template<>
inline number_t lightspark::ArgumentConversion<number_t>::toConcrete(ASObject* obj)
{
	return obj->toNumber();
}

template<>
inline bool lightspark::ArgumentConversion<bool>::toConcrete(ASObject* obj)
{
	return Boolean_concrete(obj);
}

template<>
inline uint32_t lightspark::ArgumentConversion<uint32_t>::toConcrete(ASObject* obj)
{
	return obj->toUInt();
}

template<>
inline int32_t lightspark::ArgumentConversion<int32_t>::toConcrete(ASObject* obj)
{
	return obj->toInt();
}

template<>
inline tiny_string lightspark::ArgumentConversion<tiny_string>::toConcrete(ASObject* obj)
{
	return obj->toString();
}

template<>
inline std::string lightspark::ArgumentConversion<std::string>::toConcrete(ASObject* obj)
{
	//TODO: mark as deprecated, this should not be used. use tiny_string
	return std::string(obj->toString());
}

template<>
inline RGB lightspark::ArgumentConversion<RGB>::toConcrete(ASObject* obj)
{
	return RGB(obj->toUInt());
}

template<>
inline ASObject* lightspark::ArgumentConversion<int32_t>::toAbstract(const int32_t& val)
{
	return abstract_i(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<uint32_t>::toAbstract(const uint32_t& val)
{
	return abstract_ui(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<number_t>::toAbstract(const number_t& val)
{
	return abstract_d(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<bool>::toAbstract(const bool& val)
{
	return abstract_b(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<tiny_string>::toAbstract(const tiny_string& val)
{
	return Class<ASString>::getInstanceS(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<std::string>::toAbstract(const std::string& val)
{
	return Class<ASString>::getInstanceS(val);
}

template<>
inline ASObject* lightspark::ArgumentConversion<RGB>::toAbstract(const RGB& val)
{
	return abstract_ui(val.toUInt());
}

#define ARG_UNPACK ArgUnpack(args,argslen)

class ArgUnpack
{
private:
	ASObject* const * args;
	int argslen;
public:
	ArgUnpack(ASObject* const * _args, int _argslen) : args(_args), argslen(_argslen) {}

	template<class T> ArgUnpack& operator()(T& v)
	{
		if(argslen == 0)
			throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");

		v = ArgumentConversion<T>::toConcrete(args[0]);
		args++;
		argslen--;
		return *this;
	}
	template<class T, class TD> ArgUnpack& operator()(T& v,const TD& defvalue)
	{
		if(argslen > 0)
		{
			v = ArgumentConversion<T>::toConcrete(args[0]);
			args++;
			argslen--;
		}
		else
		{
			v = defvalue;
		}
		return *this;
	}
	~ArgUnpack()
	{
		if(argslen > 0)
			LOG(LOG_NOT_IMPLEMENTED,"Not all arguments were unpacked");
	}
};

}
#endif /* SCRIPTING_ARGCONV_H */
