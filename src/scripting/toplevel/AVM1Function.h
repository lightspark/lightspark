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

#ifndef SCRIPTING_TOPLEVEL_AVM1FUNCTION_H
#define SCRIPTING_TOPLEVEL_AVM1FUNCTION_H 1

#include "scripting/toplevel/IFunction.h"

#define AVM1_RECURSION_LIMIT_INTERN 65 // according to ruffle the script limit for internal (getter,setter) AVM1 calls is 65

namespace lightspark
{
class AVM1Scope;

class AVM1Function : public IFunction
{
	friend class Class<IFunction>;
	friend class Class_base;
protected:
	DisplayObject* clip;
	ASObject* avm1Class;
	AVM1context context;
	asAtom superobj;
	std::vector<uint8_t> actionlist;
	std::vector<uint32_t> paramnames;
	std::vector<uint8_t> paramregisternumbers;
	std::vector<asAtom> implementedinterfaces;
	_NR<AVM1Scope> scope;
	bool preloadParent;
	bool preloadRoot;
	bool suppressSuper;
	bool preloadSuper;
	bool suppressArguments;
	bool preloadArguments;
	bool suppressThis;
	bool preloadThis;
	bool preloadGlobal;
	AVM1Function(ASWorker* wrk,Class_base* c,DisplayObject* cl,AVM1context* ctx, std::vector<uint32_t>& p, std::vector<uint8_t>& a,const _NR<AVM1Scope>& _scope,std::vector<uint8_t> _registernumbers=std::vector<uint8_t>(), bool _preloadParent=false, bool _preloadRoot=false, bool _suppressSuper=false, bool _preloadSuper=false, bool _suppressArguments=false, bool _preloadArguments=false,bool _suppressThis=false, bool _preloadThis=false, bool _preloadGlobal=false);
	~AVM1Function();
	method_info* getMethodInfo() const override { return nullptr; }
	IFunction* clone(ASWorker* wrk) override
	{
		// no cloning needed in AVM1
		return nullptr;
	}
	asAtom computeSuper();
	void checkInternalException();
	uint32_t getSWFVersion();
public:
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	FORCE_INLINE void call(asAtom* ret, asAtom* obj, asAtom *args, uint32_t num_args, AVM1Function* caller = nullptr, bool isInternalCall=false)
	{
		if (needsSuper() || getSWFVersion() < 7)
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,scope.getPtr(),false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,&newsuper,isInternalCall);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,scope.getPtr(),false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,nullptr,isInternalCall);
		if (isInternalCall)
			checkInternalException();
 	}
	FORCE_INLINE multiname* callGetter(asAtom& ret, asAtom& target, ASWorker* wrk) override
	{
		asAtom obj = target;
		if (needsSuper() || getSWFVersion() < 7)
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,scope.getPtr(),false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,&newsuper,true);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,scope.getPtr(),false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,nullptr,true);
		checkInternalException();
		return nullptr;
	}
	FORCE_INLINE Class_base* getReturnType(bool opportunistic=false) override
	{
		return nullptr;
	}
	FORCE_INLINE void setSuper(asAtom s)
	{
		ASATOM_ADDSTOREDMEMBER(s);
		superobj.uintval = s.uintval;
	}
	FORCE_INLINE DisplayObject* getClip() const
	{
		return clip;
	}
	FORCE_INLINE bool needsSuper() const
	{
		return !suppressSuper;
	}
	FORCE_INLINE asAtom getSuper() const
	{
		return superobj;
	}
	FORCE_INLINE void addInterface(asAtom& iface)
	{
		ASObject* o = asAtomHandler::getObject(iface);
		if (o)
			o->addStoredMember();
		implementedinterfaces.push_back(iface);
	}
	bool implementsInterface(asAtom& iface);
	FORCE_INLINE void setAVM1Class(ASObject* cls)
	{
		avm1Class = cls;
	}
	FORCE_INLINE ASObject* getAVM1Class() const
	{
		return avm1Class;
	}
};

}

#endif /* SCRIPTING_TOPLEVEL_AVM1FUNCTION_H */
