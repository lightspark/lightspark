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

#ifndef SCRIPTING_TOPLEVEL_IFUNCTION_H
#define SCRIPTING_TOPLEVEL_IFUNCTION_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{
class method_info;

/*
 * The base-class for everything that resambles a function or method.
 * It is specialized in Function for C-implemented functions
 * and SyntheticFunction for AS3-implemented functions (from the SWF)
 */
class IFunction: public ASObject
{
public:
	uint32_t length;
	ASFUNCTION_ATOM(_length);
protected:
	IFunction(ASWorker* wrk,Class_base *c, CLASS_SUBTYPE st);
	virtual IFunction* clone(ASWorker* wrk)=0;
public:
	asAtom closure_this;
	static void sinit(Class_base* c);
	/* If this is a method, inClass is the class this is defined in.
	 * If this is a function, inClass == NULL
	 */
	Class_base* inClass;
	// if this is a class method, this indicates if it is a static or instance method
	bool isStatic:1;
	bool isGetter:1;
	bool isSetter:1;
	uint32_t namespaceNameID;


	IFunction* clonedFrom;
	/* returns whether this is this a method of a function */
	bool isMethod() const { return inClass != nullptr; }
	bool isConstructed() const override { return constructIndicator; }
	inline bool destruct() override
	{
		inClass=nullptr;
		isStatic=false;
		isGetter=false;
		isSetter=false;
		namespaceNameID = BUILTIN_STRINGS::EMPTY;

		clonedFrom=nullptr;
		functionname=0;
		length=0;
		ASATOM_REMOVESTOREDMEMBER(closure_this);
		closure_this=asAtomHandler::invalidAtom;
		ASATOM_REMOVESTOREDMEMBER(prototype);
		prototype=asAtomHandler::invalidAtom;
		return destructIntern();
	}
	inline void finalize() override
	{
		inClass=nullptr;
		clonedFrom=nullptr;
		ASATOM_REMOVESTOREDMEMBER(closure_this);
		closure_this=asAtomHandler::invalidAtom;
		ASATOM_REMOVESTOREDMEMBER(prototype);
		prototype=asAtomHandler::invalidAtom;
	}
	
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	IFunction* bind(const asAtom& c, ASWorker* wrk)
	{
		IFunction* ret=nullptr;
		ret=clone(wrk);
		ret->setClass(getClass());
		ret->closure_this=c;
		ASATOM_ADDSTOREDMEMBER(ret->closure_this);
		ret->clonedFrom=this;
		ret->isStatic=isStatic;
		ret->isGetter=isGetter;
		ret->isSetter=isSetter;
		ret->namespaceNameID=namespaceNameID;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		ret->prototype=this->prototype;
		ASATOM_ADDSTOREDMEMBER(ret->prototype);
		return ret;
	}
	IFunction* createFunctionInstance(ASWorker* wrk)
	{
		IFunction* ret=nullptr;
		ret=clone(wrk);
		ret->setClass(getClass());
		ret->isStatic=isStatic;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}
	ASFUNCTION_ATOM(apply);
	ASFUNCTION_ATOM(_call);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER_SETTER(asAtom,prototype);
	virtual method_info* getMethodInfo() const=0;
	ASObject *describeType(ASWorker* wrk) const override;
	uint32_t functionname;
	virtual multiname* callGetter(asAtom& ret, asAtom& target,ASWorker* wrk, uint16_t resultlocalnumberpos) =0;
	virtual Type* getReturnType(bool opportunistic=false) =0;
	virtual Type* getParamType(uint32_t index) =0;
	virtual uint32_t getParamOptionalCount() =0;
	std::string toDebugString() const override;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};
}

#endif /* SCRIPTING_TOPLEVEL_IFUNCTION_H */
