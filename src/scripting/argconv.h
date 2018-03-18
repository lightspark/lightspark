/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Matthias Gehre (M.Gehre@gmx.de)

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

#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"

/* Usage of ARG_UNPACK:
 * You have to use it within a ASFUNCTIONBODY_ATOM() { }, because it uses the implicit arguments 'args' and 'argslen'.
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
class ArgumentConversionAtom
{
public:
	static T toConcrete(SystemState* sys,asAtom obj,const T& v);
	static void toAbstract(asAtom& ret, SystemState* sys,const T& val);
};

template<>
class ArgumentConversionAtom<asAtom>
{
public:
	static asAtom toConcrete(SystemState* sys,asAtom obj,const asAtom& v)
	{
		if(obj.type == T_NULL)
			return asAtom::nullAtom;
		if(obj.type == T_UNDEFINED)
			return asAtom::undefinedAtom;
		if(v.type != T_INVALID && v.type != T_NULL && v.type != T_UNDEFINED && !v.checkArgumentConversion(obj))
                        throwError<ArgumentError>(kCheckTypeFailedError,
                                                  obj.toObject(sys)->getClassName(),
                                                  "?"); // TODO
		return obj;
	}
	static void toAbstract(asAtom& ret, SystemState* sys,asAtom val)
	{
		if(val.type == T_INVALID)
			ret.setNull();
		else
		{
			ASATOM_INCREF(val);
			ret = val;
		}
	}
};

template<class T>
class ArgumentConversionAtom<Ref<T>>
{
public:
	static Ref<T> toConcrete(SystemState* sys,asAtom obj,const Ref<T> v)
	{
		if(!obj.is<T>())
                        throwError<ArgumentError>(kCheckTypeFailedError,
                                                  obj.toObject(sys)->getClassName(),
                                                  Class<T>::getClass(sys)->getQualifiedClassName());
        T* o = obj.as<T>();
		o->incRef();
		return _MR(o);
	}
	static void toAbstract(asAtom& ret, SystemState* /*sys*/,const Ref<T>& val)
	{
		val->incRef();
		ret = asAtom::fromObject(val.getPtr());
	}
};

template<class T>
class ArgumentConversionAtom<NullableRef<T>>
{
public:
	static NullableRef<T> toConcrete(SystemState* sys,asAtom obj,const NullableRef<T>& v)
	{
		if(obj.type == T_NULL)
			return NullRef;
		if(obj.type == T_UNDEFINED)
			return NullRef;

        if(!obj.is<T>())
                        throwError<ArgumentError>(kCheckTypeFailedError,
												  obj.toObject(sys)->getClassName(),
                                                  Class<T>::getClass(sys)->getQualifiedClassName());
		ASATOM_INCREF(obj);
		T* o = obj.as<T>();
		return _MNR(o);
	}
	static void toAbstract(asAtom& ret, SystemState* sys,const NullableRef<T>& val)
	{
		if(val.isNull())
			ret.setNull();
		else
		{
			val->incRef();
			ret = asAtom::fromObject(val.getPtr());
		}
	}
};

template<>
class ArgumentConversionAtom<NullableRef<ASObject>>
{
public:
	static NullableRef<ASObject> toConcrete(SystemState* sys,asAtom obj,const NullableRef<ASObject>& v)
	{
		ASObject* o = obj.toObject(sys);
		o->incRef();
		return _MNR(o);
	}
	static void toAbstract(asAtom& ret, SystemState* sys,const NullableRef<ASObject>& val)
	{
		if(val.isNull())
			ret.setNull();
		else
		{
			val->incRef();
			ret = asAtom::fromObject(val.getPtr());
		}
	}
};

template<>
inline number_t lightspark::ArgumentConversionAtom<number_t>::toConcrete(SystemState* sys,asAtom obj,const number_t& v)
{
	return obj.toNumber();
}

template<>
inline bool lightspark::ArgumentConversionAtom<bool>::toConcrete(SystemState* sys,asAtom obj,const bool& v)
{
	return obj.Boolean_concrete();
}

template<>
inline uint32_t lightspark::ArgumentConversionAtom<uint32_t>::toConcrete(SystemState* sys,asAtom obj,const uint32_t& v)
{
	return obj.toUInt();
}

template<>
inline int32_t lightspark::ArgumentConversionAtom<int32_t>::toConcrete(SystemState* sys,asAtom obj,const int32_t& v)
{
	return obj.toInt();
}

template<>
inline int64_t lightspark::ArgumentConversionAtom<int64_t>::toConcrete(SystemState* sys,asAtom obj,const int64_t& v)
{
	return obj.toInt64();
}

template<>
inline tiny_string lightspark::ArgumentConversionAtom<tiny_string>::toConcrete(SystemState* sys,asAtom obj,const tiny_string& v)
{
	return obj.toString(sys);
}

template<>
inline RGB lightspark::ArgumentConversionAtom<RGB>::toConcrete(SystemState* sys,asAtom obj,const RGB& v)
{
	return RGB(obj.toUInt());
}

template<>
inline void lightspark::ArgumentConversionAtom<int32_t>::toAbstract(asAtom& ret, SystemState* sys,const int32_t& val)
{
	ret.setInt(val);
}

template<>
inline void lightspark::ArgumentConversionAtom<uint32_t>::toAbstract(asAtom& ret, SystemState* sys,const uint32_t& val)
{
	ret.setUInt(val);
}

template<>
inline void lightspark::ArgumentConversionAtom<number_t>::toAbstract(asAtom& ret, SystemState* sys,const number_t& val)
{
	ret.setNumber(val);
}

template<>
inline void lightspark::ArgumentConversionAtom<bool>::toAbstract(asAtom& ret, SystemState* sys,const bool& val)
{
	ret.setBool(val);
}

template<>
inline void lightspark::ArgumentConversionAtom<tiny_string>::toAbstract(asAtom& ret, SystemState* sys,const tiny_string& val)
{
	ret = asAtom::fromObject(abstract_s(sys,val));
}

template<>
inline void lightspark::ArgumentConversionAtom<RGB>::toAbstract(asAtom& ret, SystemState* sys,const RGB& val)
{
	ret.setUInt(val.toUInt());
}

#define ARG_UNPACK_ATOM ArgUnpackAtom(sys,args,argslen,false)
#define ARG_UNPACK_ATOM_MORE_ALLOWED ArgUnpackAtom(sys,args,argslen,true)

class ArgUnpackAtom
{
private:
	SystemState* sys;
	asAtom* args;
	int argslen;
	bool moreAllowed;
public:
	ArgUnpackAtom(SystemState* _sys,asAtom* _args, int _argslen, bool _moreAllowed) : sys(_sys),args(_args), argslen(_argslen), moreAllowed(_moreAllowed) {}

	template<class T> ArgUnpackAtom& operator()(T& v)
	{
		if(argslen == 0)
                        throwError<ArgumentError>(kWrongArgumentCountError, "object", "?", "?");

		v = ArgumentConversionAtom<T>::toConcrete(sys,args[0],v);
		args++;
		argslen--;
		return *this;
	}
	template<class T, class TD> ArgUnpackAtom& operator()(T& v,const TD& defvalue)
	{
		if(argslen > 0)
		{
			v = ArgumentConversionAtom<T>::toConcrete(sys,args[0],v);
			args++;
			argslen--;
		}
		else
		{
			v = defvalue;
		}
		return *this;
	}
#ifndef NDEBUG
	~ArgUnpackAtom()
	{
		if(argslen > 0 && !moreAllowed)
			LOG(LOG_NOT_IMPLEMENTED,"Not all arguments were unpacked");
	}
#endif
};

}
#endif /* SCRIPTING_ARGCONV_H */
