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


namespace lightspark
{

class AVM1Function : public IFunction
{
	friend class Class<IFunction>;
	friend class Class_base;
protected:
	DisplayObject* clip;
	Activation_object* activationobject;
	AVM1context context;
	asAtom superobj;
	std::vector<uint8_t> actionlist;
	std::vector<uint32_t> paramnames;
	std::vector<uint8_t> paramregisternumbers;
	std::map<uint32_t, asAtom> scopevariables;
	bool preloadParent;
	bool preloadRoot;
	bool suppressSuper;
	bool preloadSuper;
	bool suppressArguments;
	bool preloadArguments;
	bool suppressThis;
	bool preloadThis;
	bool preloadGlobal;
	AVM1Function(ASWorker* wrk,Class_base* c,DisplayObject* cl,Activation_object* act,AVM1context* ctx, std::vector<uint32_t>& p, std::vector<uint8_t>& a,std::vector<uint8_t> _registernumbers=std::vector<uint8_t>(), bool _preloadParent=false, bool _preloadRoot=false, bool _suppressSuper=false, bool _preloadSuper=false, bool _suppressArguments=false, bool _preloadArguments=false,bool _suppressThis=false, bool _preloadThis=false, bool _preloadGlobal=false);
	~AVM1Function();
	method_info* getMethodInfo() const override { return nullptr; }
	IFunction* clone(ASWorker* wrk) override
	{
		// no cloning needed in AVM1
		return nullptr;
	}
	asAtom computeSuper();
public:
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	FORCE_INLINE void call(asAtom* ret, asAtom* obj, asAtom *args, uint32_t num_args, AVM1Function* caller=nullptr, std::map<uint32_t,asAtom>* locals=nullptr)
	{
		if (locals)
			this->setscopevariables(*locals);
		if (needsSuper())
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,activationobject,&newsuper);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,activationobject);
	}
	FORCE_INLINE multiname* callGetter(asAtom& ret, ASObject* target, ASWorker* wrk) override
	{
		asAtom obj = asAtomHandler::fromObject(target);
		if (needsSuper())
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,activationobject,&newsuper);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,activationobject);
		return nullptr;
	}
	FORCE_INLINE Class_base* getReturnType(bool opportunistic=false) override
	{
		return nullptr;
	}
	FORCE_INLINE Activation_object* getActivationObject() const
	{
		return activationobject;
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
		return preloadSuper && !suppressSuper;
	}
	FORCE_INLINE asAtom getSuper() const
	{
		return superobj;
	}
	void filllocals(std::map<uint32_t,asAtom>& locals);
	void setscopevariables(std::map<uint32_t,asAtom>& locals);
};

}

#endif /* SCRIPTING_TOPLEVEL_AVM1FUNCTION_H */
