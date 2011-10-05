/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Matthias Gehre (M.Gehre@gmx.de)

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

#ifndef ARGCONV_H_
#define ARGCONV_H_

#include "toplevel/toplevel.h"
#include "class.h"

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
			throw ArgumentError("Wrong type");
		o->incRef();
		return _MR(o);
	}
	static ASObject* toAbstract(const Ref<T>& val)
	{
		val->incRef();
		return val.getPtr();
	}
};

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
			throw ArgumentError("Wrong type");
		o->incRef();
		return _MNR(o);
	}
	static ASObject* toAbstract(const NullableRef<T>& val)
	{
		if(val.isNull())
			return new Null();
		val->incRef();
		return val.getPtr();
	}
};

template<>
inline number_t ArgumentConversion<number_t>::toConcrete(ASObject* obj)
{
	/* TODO: throw ArgumentError if object is not convertible to number */
	return obj->toNumber();
}

template<>
inline bool ArgumentConversion<bool>::toConcrete(ASObject* obj)
{
	/* TODO: throw ArgumentError if object is not convertible to number */
	return Boolean_concrete(obj);
}

template<>
inline uint32_t ArgumentConversion<uint32_t>::toConcrete(ASObject* obj)
{
	/* TODO: throw ArgumentError if object is not convertible to number */
	return obj->toUInt();
}

template<>
inline int32_t ArgumentConversion<int32_t>::toConcrete(ASObject* obj)
{
	/* TODO: throw ArgumentError if object is not convertible to number */
	return obj->toInt();
}

template<>
inline tiny_string ArgumentConversion<tiny_string>::toConcrete(ASObject* obj)
{
	ASString* str = Class<ASString>::cast(obj);
	if(!str)
		throw ArgumentError("Not an ASString");

	return str->toString();
}

template<>
inline RGB ArgumentConversion<RGB>::toConcrete(ASObject* obj)
{
	/* TODO: throw ArgumentError if object is not convertible to number */
	return RGB(obj->toUInt());
}

template<>
inline ASObject* ArgumentConversion<int32_t>::toAbstract(const int32_t& val)
{
	return abstract_i(val);
}

template<>
inline ASObject* ArgumentConversion<uint32_t>::toAbstract(const uint32_t& val)
{
	return abstract_ui(val);
}

template<>
inline ASObject* ArgumentConversion<number_t>::toAbstract(const number_t& val)
{
	return abstract_d(val);
}

template<>
inline ASObject* ArgumentConversion<bool>::toAbstract(const bool& val)
{
	return abstract_b(val);
}

template<>
inline ASObject* ArgumentConversion<tiny_string>::toAbstract(const tiny_string& val)
{
	return Class<ASString>::getInstanceS(val);
}

template<>
inline ASObject* ArgumentConversion<RGB>::toAbstract(const RGB& val)
{
	return abstract_ui(val.toUInt());
}

}
#endif /* ARGCONV_H_ */
