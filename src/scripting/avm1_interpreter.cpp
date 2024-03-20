/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "swftypes.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Number.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1array.h"
#include "parsing/tags.h"
#include "backends/audio.h"

using namespace std;
using namespace lightspark;

void ACTIONRECORD::PushStack(std::stack<asAtom> &stack, const asAtom &a)
{
	stack.push(a);
}

asAtom ACTIONRECORD::PopStack(std::stack<asAtom>& stack)
{
	if (stack.empty())
		return asAtomHandler::undefinedAtom;
	asAtom ret = stack.top();
	stack.pop();
	return ret;
}
asAtom ACTIONRECORD::PeekStack(std::stack<asAtom>& stack)
{
	if (stack.empty())
		throw RunTimeException("AVM1: empty stack");
	return stack.top();
}
Mutex executeactionmutex;
void ACTIONRECORD::executeActions(DisplayObject *clip, AVM1context* context, const std::vector<uint8_t> &actionlist, uint32_t startactionpos, std::map<uint32_t, asAtom> &scopevariables, bool fromInitAction, asAtom* result, asAtom* obj, asAtom *args, uint32_t num_args, const std::vector<uint32_t>& paramnames, const std::vector<uint8_t>& paramregisternumbers,
								  bool preloadParent, bool preloadRoot, bool suppressSuper, bool preloadSuper, bool suppressArguments, bool preloadArguments, bool suppressThis, bool preloadThis, bool preloadGlobal, AVM1Function *caller, AVM1Function *callee, Activation_object *actobj, asAtom *superobj)
{
	Locker l(executeactionmutex);
	bool clip_isTarget=false;
	assert(!clip->needsActionScript3());
	ASWorker* wrk = clip->getSystemState()->worker;
	Log::calls_indent++;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions "<<preloadParent<<preloadRoot<<suppressSuper<<preloadSuper<<suppressArguments<<preloadArguments<<suppressThis<<preloadThis<<preloadGlobal<<" "<<startactionpos<<" "<<num_args);
	if (result)
		asAtomHandler::setUndefined(*result);
	std::stack<asAtom> stack;
	asAtom registers[256];
	std::fill_n(registers,256,asAtomHandler::undefinedAtom);
	std::map<uint32_t,asAtom> locals;
	if (caller)
		caller->filllocals(locals);
	int curdepth = 0;
	int maxdepth= clip->loadedFrom->version < 6 ? 8 : 16;
	asAtom* scopestack = g_newa(asAtom, maxdepth);
	scopestack[0] = obj ? *obj : asAtomHandler::fromObject(clip);
	ASATOM_INCREF(scopestack[0]);
	std::vector<uint8_t>::const_iterator* scopestackstop = g_newa(std::vector<uint8_t>::const_iterator, maxdepth);
	scopestackstop[0] = actionlist.end();
	uint32_t currRegister = 1; // spec is not clear, but gnash starts at register 1
	if (!suppressThis || preloadThis)
	{
		ASATOM_INCREF(scopestack[0]);
		if (preloadThis)
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" preload this:"<<asAtomHandler::toDebugString(scopestack[0]));
			registers[currRegister++] = scopestack[0];
		}
		else
		{
			ASATOM_DECREF(locals[paramnames[currRegister]]);
			locals[paramnames[currRegister++]] = scopestack[0];
		}
	}
	if (!suppressArguments)
	{
		if (preloadArguments)
		{
			Array* regargs = Class<Array>::getInstanceS(wrk);
			for (uint32_t i = 0; i < num_args; i++)
			{
				ASATOM_INCREF(args[i]);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
				regargs->push(args[i]);
			}
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=clip->getSystemState()->getUniqueStringId("caller");
			asAtom c = caller ? asAtomHandler::fromObject(caller) : asAtomHandler::nullAtom;
			if (caller)
				caller->incRef();
			regargs->setVariableByMultiname(m,c,ASObject::CONST_ALLOWED,nullptr,wrk);
			if (callee)
			{
				callee->incRef();
				m.name_s_id=clip->getSystemState()->getUniqueStringId("callee");
				asAtom c = asAtomHandler::fromObject(callee);
				regargs->setVariableByMultiname(m,c,ASObject::CONST_ALLOWED,nullptr,wrk);
			}
			registers[currRegister++] = asAtomHandler::fromObject(regargs);
		}
		else
		{
			for (uint32_t i = 0; i < paramnames.size() && i < num_args; i++)
			{
				ASATOM_INCREF(args[i]);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
				ASATOM_DECREF(locals[paramnames[i]]);
				locals[paramnames[i]] = args[i];
			}
		}
	}
	asAtom super = asAtomHandler::invalidAtom;
	if (!suppressSuper || preloadSuper)
	{
		if (superobj && asAtomHandler::isObject(*superobj))
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" super for "<<asAtomHandler::toDebugString(scopestack[0])<<" "<<asAtomHandler::toDebugString(*superobj));
			super.uintval=superobj->uintval;
		}
		if (asAtomHandler::isInvalid(super))
			LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" no super class found for "<<asAtomHandler::toDebugString(scopestack[0]));
		ASATOM_INCREF(super);
		if (preloadSuper)
			registers[currRegister++] = super;
		else
		{
			ASATOM_DECREF(locals[paramnames[currRegister]]);
			locals[paramnames[currRegister++]] = super;
		}
	}
	if (preloadRoot)
	{
		clip->loadedFrom->incRef();
		registers[currRegister++] = asAtomHandler::fromObject(clip->loadedFrom);
	}
	if (preloadParent)
	{
		if (clip->getParent())
			clip->getParent()->incRef();
		registers[currRegister++] = asAtomHandler::fromObject(clip->getParent());
	}
	if (preloadGlobal)
	{
		clip->getSystemState()->avm1global->incRef();
		registers[currRegister++] = asAtomHandler::fromObject(clip->getSystemState()->avm1global);
	}
	for (uint32_t i = 0; i < paramregisternumbers.size() && i < num_args; i++)
	{
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" set argument "<<i<<" "<<(int)paramregisternumbers[i]<<" "<<clip->getSystemState()->getStringFromUniqueId(paramnames[i])<<" "<<asAtomHandler::toDebugString(args[i]));
		ASATOM_INCREF(args[i]);
		ASATOM_DECREF(locals[paramnames[i]]);
		locals[paramnames[i]] = args[i];
		if (context->keepLocals)
		{
			ASATOM_INCREF(args[i]);
			tiny_string s = clip->getSystemState()->getStringFromUniqueId(paramnames[i]).lowercase();
			clip->AVM1SetVariable(s,args[i],false);
		}
		if (paramregisternumbers[i] != 0)
		{
			ASATOM_INCREF(args[i]);
			ASATOM_DECREF(registers[paramregisternumbers[i]]);
			registers[paramregisternumbers[i]] = args[i];
		}
	}

	Array* argarray = nullptr;
	DisplayObject *originalclip = clip;
	auto it = actionlist.begin()+startactionpos;
	while (it != actionlist.end())
	{
		if (curdepth > 0 && it == scopestackstop[curdepth])
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" end with "<<asAtomHandler::toDebugString(scopestack[curdepth]));
			if (asAtomHandler::is<DisplayObject>(scopestack[curdepth]))
				clip = originalclip;
			ASATOM_DECREF(scopestack[curdepth]);
			curdepth--;
			Log::calls_indent--;
		}
		if (!clip
				&& *it != 0x20 // ActionSetTarget2
				&& *it != 0x8b // ActionSetTarget
				)
		{
			// we are in a target that was not found during ActionSetTarget(2), so these actions are ignored
			continue;
		}
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<(it-actionlist.begin())<< " action code:"<<hex<<(int)*it<<dec<<" "<<clip->toDebugString());
		uint8_t opcode = *it++;
		if (opcode > 0x80)
			it+=2; // skip length
		switch (opcode)
		{
			case 0x00:
				it = actionlist.end(); // force quit loop;
				break;
			case 0x04: // ActionNextFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionNextFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = clip->as<MovieClip>()->state.FP+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNextFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x05: // ActionPreviousFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionPreviousFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = clip->as<MovieClip>()->state.FP-1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPreviousFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x06: // ActionPlay
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionPlay "<<clip->toDebugString());
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPlay");
				clip->as<MovieClip>()->setPlaying();
				break;
			}
			case 0x07: // ActionStop
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionStop "<<clip->toDebugString());
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStop");
				clip->as<MovieClip>()->setStopped();
				break;
			}
			case 0x09: // ActionStopSounds
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStopSounds");
				clip->getSystemState()->audioManager->stopAllSounds();
				break;
			}
			case 0x0a: // ActionAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAdd "<<b<<"+"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,a+b,false));
				break;
			}
			case 0x0b: // ActionSubtract
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSubtract "<<b<<"-"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,b-a,false));
				break;
			}
			case 0x0c: // ActionMultiply
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionMultiply "<<b<<"*"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,b*a,false));
				break;
			}
			case 0x0d: // ActionDivide
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDivide "<<b<<"/"<<a);
				asAtom ret=asAtomHandler::invalidAtom;
				if (a == 0)
				{
					if (clip->loadedFrom->version == 4)
						ret = asAtomHandler::fromString(clip->getSystemState(),"#ERROR#");
					else
						ret = asAtomHandler::fromNumber(wrk,Number::NaN,false);
				}
				else
					ret = asAtomHandler::fromNumber(wrk,b/a,false);
				PushStack(stack,ret);
				break;
			}
			case 0x0e: // ActionEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEquals "<<b<<"=="<<a<<" "<<(b==a));
				PushStack(stack,asAtomHandler::fromBool(b==a));
				break;
			}
			case 0x0f: // ActionLess
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionLess "<<b<<"<"<<a);
				PushStack(stack,asAtomHandler::fromBool(b<a));
				break;
			}
			case 0x10: // ActionAnd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAnd "<<b<<" && "<<a);
				PushStack(stack,asAtomHandler::fromBool(b && a));
				break;
			}
			case 0x11: // ActionOr
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionOr "<<b<<" || "<<a);
				PushStack(stack,asAtomHandler::fromBool(b || a));
				break;
			}
			case 0x12: // ActionNot
			{
				asAtom a = PopStack(stack);
				bool value = asAtomHandler::AVM1toBool(a);
				ASATOM_DECREF(a);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNot "<<value);
				PushStack(stack,asAtomHandler::fromBool(!value));
				break;
			}
			case 0x13: // ActionStringEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::toString(aa,wrk);
				tiny_string b = asAtomHandler::toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringEquals "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromBool(a == b));
				break;
			}
			case 0x15: // ActionStringExtract
			{
				asAtom count = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom str = PopStack(stack);
				tiny_string a = asAtomHandler::toString(str,wrk);
				// start parameter seems to be 1-based
				int startindex = asAtomHandler::toInt(index)-1;
				tiny_string b;
				if (uint32_t(startindex) <= a.numChars())
					b = a.substr(startindex,asAtomHandler::toInt(count));
				ASATOM_DECREF(count);
				ASATOM_DECREF(index);
				ASATOM_DECREF(str);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringExtract "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),b));
				break;
			}
			case 0x17: // ActionPop
			{
				asAtom a= PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPop");
				ASATOM_DECREF(a);
				break;
			}
			case 0x18: // ActionToInteger
			{
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionToInteger "<<asAtomHandler::toDebugString(a));
				PushStack(stack,asAtomHandler::fromInt(asAtomHandler::toInt(a)));
				ASATOM_DECREF(a);
				break;
			}
			case 0x1c: // ActionGetVariable
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetVariable "<<asAtomHandler::toDebugString(name));
				tiny_string s = asAtomHandler::toString(name,wrk);
				asAtom res=asAtomHandler::invalidAtom;
				if (s=="this")
				{
					if(asAtomHandler::isValid(scopestack[curdepth]))
						res = scopestack[curdepth];
					else
						res = asAtomHandler::fromObject(clip);
					ASATOM_INCREF(res);
				}
				else if (s=="arguments")
				{
					if (argarray == nullptr)
					{
						argarray = Class<Array>::getInstanceS(wrk);
						for (uint32_t i = 0; i < num_args; i++)
						{
							ASATOM_INCREF(args[i]);
							argarray->push(args[i]);
						}
					}
					argarray->incRef();
					res = asAtomHandler::fromObjectNoPrimitive(argarray);
				}
				else if (s == "_global")
				{
					res = asAtomHandler::fromObjectNoPrimitive(clip->getSystemState()->avm1global);
					ASATOM_INCREF(res);
				}
				else if (s.numBytes()>6 && s.startsWith("_level"))
				{
					if (s.numBytes() == 7 && s.charAt(6)=='0')
						res = asAtomHandler::fromObjectNoPrimitive(clip->loadedFrom);
					else
						LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetVariable for "<<s);
					ASATOM_INCREF(res);
				}
				else if (context->keepLocals && s.find(".") == tiny_string::npos)
				{
					uint32_t nameidlower=clip->getSystemState()->getUniqueStringId(s.lowercase());
					auto it = locals.find(nameidlower);
					if (it != locals.end()) // local variable
					{
						res = it->second;
						ASATOM_INCREF(res);
					}
					else if (!caller)
					{
						auto it = clip->avm1locals.find(nameidlower);
						if (it != clip->avm1locals.end())
						{
							res = it->second;
							ASATOM_INCREF(res);
						}
					}
				}
				if (asAtomHandler::isInvalid(res) && !clip_isTarget)
				{
					if (actobj)
					{
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						m.name_s_id=asAtomHandler::toStringId(name,wrk);
						actobj->getVariableByMultiname(res,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
				}
				if (asAtomHandler::isInvalid(res) && !clip_isTarget)
				{
					if (asAtomHandler::isObject(scopestack[0]))
					{
						ASObject* o =asAtomHandler::getObjectNoCheck(scopestack[0]);
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						if (s.startsWith("_")) // internal variable names are always lowercase (e.g. "_x","_y"...)
							m.name_s_id=wrk->getSystemState()->getUniqueStringId(s.lowercase());
						else
							m.name_s_id=asAtomHandler::toStringId(name,wrk);
						o->getVariableByMultiname(res,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
				}
				if (asAtomHandler::isInvalid(res) && !scopevariables.empty())
				{
					// scopevariables are locals of the caller, so we have to test case insensitive
					uint32_t nameID = clip->getSystemState()->getUniqueStringId(s.lowercase());
					auto it = scopevariables.find(nameID);
					if (it != scopevariables.end())
					{
						res = it->second;
						ASATOM_INCREF(res);
					}
				}
				if (asAtomHandler::isInvalid(res))
				{
					if (!curdepth && !s.startsWith("/") && !s.startsWith(":"))
					{
						// first look for member of clip
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.name_s_id=asAtomHandler::toStringId(name,wrk);
						m.isAttribute = false;
						if (clip_isTarget)
						{
							clip->getVariableByMultiname(res,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (asAtomHandler::isInvalid(res))
								res = clip->AVM1GetVariable(s,false);
						}
						else
						{
							originalclip->getVariableByMultiname(res,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (asAtomHandler::isInvalid(res))
								res = originalclip->AVM1GetVariable(s,false);
						}
					}
					else
						res = clip->AVM1GetVariable(s,false);
				}
				if (asAtomHandler::isInvalid(res))
				{
					// it seems adobe allows paths in form of "x.y.z" that point to objects in global object
					tiny_string s1 = s;
					ASObject* o = clip->getSystemState()->avm1global;
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.isAttribute = false;
					while (true)
					{
						uint32_t pos = s1.find(".");
						if (pos == tiny_string::npos)
						{
							m.name_s_id=clip->getSystemState()->getUniqueStringId(s1);
							o->getVariableByMultiname(res,m,GET_VARIABLE_OPTION::NONE,wrk);
							break;
						}
						else
						{
							tiny_string s2 = s1.substr_bytes(0,pos);
							s1 = s1.substr_bytes(pos+1,s1.numBytes()-(pos+1));
							asAtom r = asAtomHandler::invalidAtom;
							m.name_s_id=clip->getSystemState()->getUniqueStringId(s2);
							o->getVariableByMultiname(r,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (asAtomHandler::isInvalid(r))
								break;
							o = asAtomHandler::toObject(r,wrk);
						}
					}
				}
				if (asAtomHandler::isInvalid(res))
					asAtomHandler::setUndefined(res);
				ASATOM_DECREF(name);
				PushStack(stack,res);
				break;
			}
			case 0x1d: // ActionSetVariable
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetVariable "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				tiny_string s = asAtomHandler::toString(name,wrk);
				ASATOM_DECREF(name);
				if (context->keepLocals && s.find(".") == tiny_string::npos)
				{
					// variable names are case insensitive
					uint32_t nameidlower = clip->getSystemState()->getUniqueStringId(s.lowercase());
					auto it = locals.find(nameidlower);
					if (it != locals.end()) // local variable
					{
						ASATOM_INCREF(value);
						ASATOM_DECREF(it->second);
						it->second = value;
					}
					else if (!caller)
					{
						auto it = clip->avm1locals.find(nameidlower);
						if (it != clip->avm1locals.end())
						{
							ASATOM_ADDSTOREDMEMBER(value);
							ASATOM_REMOVESTOREDMEMBER(it->second);
							it->second = value;
						}
					}
				}
				if (!curdepth && !s.startsWith("/") && !s.startsWith(":") && !clip_isTarget)
					originalclip->AVM1SetVariable(s,value);
				else
					clip->AVM1SetVariable(s,value);
				break;
			}
			case 0x20: // ActionSetTarget2
			{
				asAtom a = PopStack(stack);
				if (asAtomHandler::is<DisplayObject>(a))
				{
					if (clip_isTarget)
						clip->decRef();
					clip = asAtomHandler::as<MovieClip>(a);
					clip_isTarget=true;
					LOG_CALL("AVM1: ActionSetTarget2: setting target to "<<clip->toDebugString());
				}
				else
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					ASATOM_DECREF(a);
					if (!clip)
					{
						LOG_CALL("AVM1: ActionSetTarget2: setting target from undefined value to "<<s);
					}
					else
					{
						LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget2 "<<s);
					}
					if (clip_isTarget)
						clip->decRef();
					clip_isTarget=false;
					if (s.empty())
					{
						clip = originalclip;
					}
					else
					{
						DisplayObject* c = clip->AVM1GetClipFromPath(s);
						if (!c)
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget2 clip not found:"<<s);
						else
							clip = c;
					}
				}
				break;
			}
			case 0x21: // ActionStringAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::toString(aa,wrk);
				tiny_string b = asAtomHandler::toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringAdd "<<b<<"+"<<a);
				asAtom ret = asAtomHandler::fromString(clip->getSystemState(),b+a);
				PushStack(stack,ret);
				break;
			}
			case 0x22: // ActionGetProperty
			{
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index));
				DisplayObject* o = clip;
				if (asAtomHandler::toStringId(target,wrk) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					o = clip->AVM1GetClipFromPath(s);
				}
				asAtom ret = asAtomHandler::undefinedAtom;
				if (o)
				{
					asAtom obj = asAtomHandler::fromObject(o);
					switch (asAtomHandler::toInt(index))
					{
						case 0:// x
							DisplayObject::_getX(ret,wrk,obj,nullptr,0);
							break;
						case 1:// y
							DisplayObject::_getY(ret,wrk,obj,nullptr,0);
							break;
						case 2:// xscale
							DisplayObject::AVM1_getScaleX(ret,wrk,obj,nullptr,0);
							break;
						case 3:// xscale
							DisplayObject::AVM1_getScaleY(ret,wrk,obj,nullptr,0);
							break;
						case 4:// currentframe
							if (o->is<MovieClip>())
								MovieClip::_getCurrentFrame(ret,wrk,obj,nullptr,0);
							break;
						case 5:// totalframes
							if (o->is<MovieClip>())
								MovieClip::_getTotalFrames(ret,wrk,obj,nullptr,0);
							break;
						case 6:// alpha
							DisplayObject::AVM1_getAlpha(ret,wrk,obj,nullptr,0);
							break;
						case 7:// visible
							DisplayObject::_getVisible(ret,wrk,obj,nullptr,0);
							break;
						case 8:// width
							DisplayObject::_getWidth(ret,wrk,obj,nullptr,0);
							break;
						case 9:// height
							DisplayObject::_getHeight(ret,wrk,obj,nullptr,0);
							break;
						case 10:// rotation
							DisplayObject::_getRotation(ret,wrk,obj,nullptr,0);
							break;
						case 12:// framesloaded
							if (o->is<MovieClip>())
								MovieClip::_getFramesLoaded(ret,wrk,obj,nullptr,0);
							break;
						case 13:// name
							DisplayObject::_getter_name(ret,wrk,obj,nullptr,0);
							break;
						case 15:// url
							ret = asAtomHandler::fromString(clip->getSystemState(),clip->getSystemState()->mainClip->getOrigin().getURL());
							break;
						case 19:// quality
							DisplayObject::AVM1_getQuality(ret,wrk,obj,nullptr,0);
							break;
						case 20:// xmouse
							DisplayObject::_getMouseX(ret,wrk,obj,nullptr,0);
							break;
						case 21:// ymouse
							DisplayObject::_getMouseY(ret,wrk,obj,nullptr,0);
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" GetProperty type:"<<asAtomHandler::toInt(index));
							break;
					}
				}
				ASATOM_DECREF(index);
				ASATOM_DECREF(target);
				PushStack(stack,ret);
				break;
			}
			case 0x23: // ActionSetProperty
			{
				asAtom value = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				DisplayObject* o = clip;
				if (asAtomHandler::is<MovieClip>(target))
				{
					o = asAtomHandler::as<MovieClip>(target);
				}
				else if (asAtomHandler::toStringId(target,wrk) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					o = clip->AVM1GetClipFromPath(s);
					if (!o) // it seems that Adobe falls back to the current clip if the path is invalid
						o = clip;
				}
				if (o)
				{
					asAtom obj = asAtomHandler::fromObject(o);
					asAtom ret=asAtomHandler::invalidAtom;
					switch (asAtomHandler::toInt(index))
					{
						case 0:// x
							DisplayObject::_setX(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 1:// y
							DisplayObject::_setY(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 2:// xscale
							DisplayObject::AVM1_setScaleX(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 3:// xscale
							DisplayObject::AVM1_setScaleY(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 6:// alpha
							DisplayObject::AVM1_setAlpha(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 7:// visible
							DisplayObject::_setVisible(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 8:// width
							DisplayObject::_setWidth(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 9:// height
							DisplayObject::_setHeight(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 10:// rotation
							DisplayObject::_setRotation(ret,wrk,obj,&value,1);
							ASATOM_DECREF(value);
							break;
						case 13:// name
							DisplayObject::_setter_name(ret,wrk,obj,&value,1);
							break;
						case 17:// focusrect
							if (asAtomHandler::is<InteractiveObject>(obj))
								InteractiveObject::_setter_focusRect(ret,wrk,obj,&value,1);
							break;
						case 19:// quality
							DisplayObject::AVM1_setQuality(ret,wrk,obj,&value,1);
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1: SetProperty type:"<<asAtomHandler::toInt(index));
							break;
					}
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetProperty target clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				ASATOM_DECREF(index);
				ASATOM_DECREF(target);
				break;
			}
			case 0x24: // ActionCloneSprite
			{
				asAtom ad = PopStack(stack);
				uint32_t depth = asAtomHandler::toUInt(ad);
				asAtom target = PopStack(stack);
				asAtom sp = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(sp)<<" "<<depth);
				DisplayObject* source = asAtomHandler::is<DisplayObject>(sp) ? asAtomHandler::as<DisplayObject>(sp) : nullptr;
				if (!source)
				{
					tiny_string sourcepath = asAtomHandler::toString(sp,wrk);
					source = clip->AVM1GetClipFromPath(sourcepath);
				}
				if (source)
				{
					DisplayObjectContainer* parent = source->getParent();
					if (!parent || !parent->is<MovieClip>())
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite: the clip has no parent:"<<asAtomHandler::toDebugString(target)<<" "<<source->toDebugString()<<" "<<depth);
					else if (!source->is<MovieClip>())
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite: the source is not a MovieClip:"<<asAtomHandler::toDebugString(target)<<" "<<source->toDebugString()<<" "<<depth);
					else
						source->as<MovieClip>()->AVM1CloneSprite(target,depth,nullptr);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite source clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(sp)<<" "<<depth);
				ASATOM_DECREF(ad);
				ASATOM_DECREF(target);
				ASATOM_DECREF(sp);
				break;
			}
			case 0x25: // ActionRemoveSprite
			{
				asAtom target = PopStack(stack);
				DisplayObject* o = nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionRemoveSprite "<<asAtomHandler::toDebugString(target));
				if (asAtomHandler::is<DisplayObject>(target))
				{
					o = asAtomHandler::as<DisplayObject>(target);
				}
				else
				{
					uint32_t targetID = asAtomHandler::toStringId(target,wrk);
					if (targetID != BUILTIN_STRINGS::EMPTY)
					{
						tiny_string s = asAtomHandler::toString(target,wrk);
						o = clip->AVM1GetClipFromPath(s);
					}
					else
						o = clip;
				}
				if (o && o->getParent() && !o->is<RootMovieClip>() && !o->legacy)
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(o);
					asAtom r = asAtomHandler::invalidAtom;
					MovieClip::AVM1RemoveMovieClip(r,wrk,obj,nullptr,0);
				}
				ASATOM_DECREF(target);
				break;
			}
			case 0x26: // ActionTrace
			{
				asAtom value = PopStack(stack);
				Log::print(asAtomHandler::toString(value,wrk));
				ASATOM_DECREF(value);
				break;
			}
			case 0x27: // ActionStartDrag
			{
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag "<<asAtomHandler::toDebugString(target));
				asAtom lockcenter = PopStack(stack);
				asAtom constrain = PopStack(stack);
				Rectangle* rect = nullptr;
				if (asAtomHandler::toInt(constrain))
				{
					asAtom y2 = PopStack(stack);
					asAtom x2 = PopStack(stack);
					asAtom y1 = PopStack(stack);
					asAtom x1 = PopStack(stack);
					rect = Class<Rectangle>::getInstanceS(wrk);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(rect);
					Rectangle::_setTop(ret,wrk,obj,&y1,1);
					Rectangle::_setLeft(ret,wrk,obj,&x1,1);
					Rectangle::_setBottom(ret,wrk,obj,&y2,1);
					Rectangle::_setRight(ret,wrk,obj,&x2,1);
				}
				asAtom obj=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(target))
				{
					obj = target;
				}
				else
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					ASATOM_DECREF(target);
					DisplayObject* targetclip = clip->AVM1GetClipFromPath(s);
					if (targetclip)
					{
						obj = asAtomHandler::fromObject(targetclip);
					}
					else
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag target clip not found:"<<asAtomHandler::toDebugString(target));
						break;
					}
				}
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom args[2];
				args[0] = lockcenter;
				if (rect)
					args[1] = asAtomHandler::fromObject(rect);
				Sprite::_startDrag(ret,wrk,obj,args,rect ? 2 : 1);
				ASATOM_DECREF(lockcenter);
				ASATOM_DECREF(constrain);
				break;
			}
			case 0x28: // ActionEndDrag
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(clip);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEndDrag "<<asAtomHandler::toDebugString(obj));
				Sprite::_stopDrag(ret,wrk,obj,nullptr,0);
				break;
			}
			case 0x2b: // ActionCastOp
			{
				asAtom obj = PopStack(stack);
				asAtom constr = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCastOp "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(constr));
				ASObject* value = asAtomHandler::toObject(obj,wrk);
				ASObject* type = asAtomHandler::toObject(constr,wrk);
				if (type == Class<ASObject>::getRef(clip->getSystemState()).getPtr() || ABCVm::instanceOf(value,type))
				{
					PushStack(stack,obj);
				}
				else
				{
					ASATOM_DECREF(obj);
					PushStack(stack,asAtomHandler::nullAtom);
				}
				ASATOM_DECREF(constr);
				break;
			}
			case 0x69: // ActionExtends
			{
				asAtom superconstr = PopStack(stack);
				asAtom subconstr = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
				if (!asAtomHandler::isFunction(subconstr))
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends  wrong constructor type "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
					break;
				}
				IFunction* c = asAtomHandler::as<IFunction>(subconstr);
				c->setRefConstant();
				c->prototype = _MNR(Class<ASObject>::getInstanceS(wrk));
				c->prototype->addStoredMember();
				c->prototype->incRef();
				_NR<ASObject> probj = _MR(c->prototype);
				c->setprop_prototype(probj);
				c->setprop_prototype(probj,BUILTIN_STRINGS::STRING_PROTO);
				ASObject* superobj = asAtomHandler::toObject(superconstr,wrk);
				ASObject* proto = superobj->getprop_prototype();
				if (!proto)
				{
					if (superobj->is<Class_base>())
						proto = superobj->as<Class_base>()->getPrototype(wrk)->getObj();
					else if (superobj->is<IFunction>())
						proto = superobj->as<IFunction>()->prototype.getPtr();
				}
				if (proto)
				{
					proto->incRef();
					_NR<ASObject> o = _MR(proto);
					c->prototype->setprop_prototype(o);
					c->prototype->setprop_prototype(o,BUILTIN_STRINGS::STRING_PROTO);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends  super prototype not found "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
				c->prototype->setVariableAtomByQName("constructor",nsNameAndKind(),superconstr, DECLARED_TRAIT);
				if (c->is<AVM1Function>())
					c->as<AVM1Function>()->setSuper(superconstr);
				break;
			}
			case 0x30: // ActionRandomNumber
			{
				asAtom am = PopStack(stack);
				int maximum = asAtomHandler::toInt(am);
				int ret = (((number_t)rand())/(number_t)RAND_MAX)*maximum;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionRandomNumber "<<ret);
				ASATOM_DECREF(am);
				PushStack(stack,asAtomHandler::fromInt(ret));
				break;
			}
			case 0x34: // ActionGetTime
			{
				uint64_t runtime = clip->getSystemState()->getCurrentTime_ms()-clip->getSystemState()->startTime;
				asAtom ret=asAtomHandler::fromNumber(wrk,(number_t)runtime,false);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetTime "<<asAtomHandler::toDebugString(ret));
				PushStack(stack,asAtom(ret));
				break;
			}
			case 0x3a: // ActionDelete
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				asAtom ret = asAtomHandler::falseAtom;
				if (asAtomHandler::isObject(scriptobject))
				{
					ASObject* o = asAtomHandler::getObject(scriptobject);
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=asAtomHandler::toStringId(name,wrk);
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m,wrk))
						ret = asAtomHandler::trueAtom;
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3b: // ActionDelete2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete2 "<<asAtomHandler::toDebugString(name));
				DisplayObject* o = clip;
				asAtom ret = asAtomHandler::falseAtom;
				while (o)
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=asAtomHandler::toStringId(name,wrk);
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m,wrk))
					{
						ret = asAtomHandler::trueAtom;
						break;
					}
					o = o->getParent();
				}
				ASATOM_DECREF(name);
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3c: // ActionDefineLocal
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineLocal "<<asAtomHandler::toDebugString(name)<<" " <<asAtomHandler::toDebugString(value));
				if (context->keepLocals)
				{
					ASATOM_INCREF(value);
					tiny_string s =asAtomHandler::toString(name,wrk).lowercase();
					clip->AVM1SetVariable(s,value,false);
				}
				uint32_t nameID = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,wrk).lowercase());
				if (caller)
				{
					ASATOM_DECREF(locals[nameID]);
					locals[nameID] = value;
				}
				else
				{
					ASObject* v = asAtomHandler::getObject(value);
					if (v)
						v->addStoredMember();
					auto it = clip->avm1locals.find(nameID);
					if (it == clip->avm1locals.end())
						clip->avm1locals.insert(make_pair(nameID,value));
					else
					{
						ASATOM_REMOVESTOREDMEMBER(it->second);
						it->second = value;
					}
				}
				ASATOM_DECREF(name);
				break;
			}
			case 0x3d: // ActionCallFunction
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = numargs ? g_newa(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				uint32_t nameID = asAtomHandler::toStringId(name,wrk);
				tiny_string s = clip->getSystemState()->getStringFromUniqueId(nameID);
				asAtom ret=asAtomHandler::invalidAtom;
				AVM1Function* f =nullptr;
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				else
				{
					tiny_string s = asAtomHandler::toString(name,wrk).lowercase();
					asAtom func = clip->AVM1GetVariable(s);
					if (asAtomHandler::isInvalid(func) && clip != originalclip)
						func = originalclip->AVM1GetVariable(s);
					if (asAtomHandler::is<AVM1Function>(func))
					{
						asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
						asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,numargs,caller,&locals);
						asAtomHandler::as<AVM1Function>(func)->decRef();
					}
					else if (asAtomHandler::is<Function>(func))
					{
						asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
						asAtomHandler::as<Function>(func)->call(ret,wrk,obj,args,numargs);
						asAtomHandler::as<Function>(func)->decRef();
					}
					else if (asAtomHandler::is<Class_base>(func))
					{
						asAtomHandler::as<Class_base>(func)->generator(wrk,ret,args,numargs);
					}
					else
					{
						if (clip->loadedFrom->version > 4)
						{
							func = asAtomHandler::invalidAtom;
							multiname m(nullptr);
							m.name_type=multiname::NAME_STRING;
							m.name_s_id=nameID;
							m.isAttribute = false;
							clip->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (asAtomHandler::isInvalid(func))// get Variable from root movie
								clip->loadedFrom->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (asAtomHandler::isInvalid(func))// get Variable from global object
								clip->getSystemState()->avm1global->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
						}
						if (asAtomHandler::isInvalid(func))
						{
							uint32_t nameIDlower = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,wrk).lowercase());
							f =clip->AVM1GetFunction(nameIDlower);
							if (f)
								f->call(&ret,nullptr, args,numargs,caller,&locals);
							else
							{
								LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction function not found "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func)<<" "<<numargs);
								ret = asAtomHandler::undefinedAtom;
							}
						}
						else if (asAtomHandler::is<AVM1Function>(func))
						{
							asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
							asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,numargs,caller,&locals);
							asAtomHandler::as<AVM1Function>(func)->decRef();
						}
						else if (asAtomHandler::is<Function>(func))
						{
							asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
							asAtomHandler::as<Function>(func)->call(ret,wrk,obj,args,numargs);
							asAtomHandler::as<Function>(func)->decRef();
						}
						else if (asAtomHandler::is<Class_base>(func))
						{
							asAtomHandler::as<Class_base>(func)->generator(wrk,ret,args,numargs);
						}
						else
						{
							LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction function not found "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func)<<" "<<numargs);
							ret = asAtomHandler::undefinedAtom;
						}
					}
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction done "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_DECREF(args[i]);
				ASATOM_DECREF(name);
				ASATOM_DECREF(na);
				PushStack(stack,ret);
				break;
			}
			case 0x3e: // ActionReturn
			{
				if (result)
					*result = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionReturn ");
				it = actionlist.end();
				break;
			}
			case 0x3f: // ActionModulo
			{
				// spec is wrong, y is popped of stack first
				asAtom y = PopStack(stack);
				asAtom x = PopStack(stack);
				asAtom tmp = x;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionModulo "<<asAtomHandler::toDebugString(x)<<" % "<<asAtomHandler::toDebugString(y));
				asAtomHandler::modulo(x,wrk,y,false);
				ASATOM_DECREF(y);
				ASATOM_DECREF(tmp);
				PushStack(stack,x);
				break;
			}
			case 0x40: // ActionNewObject
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				AVM1Function* f =nullptr;
				uint32_t nameID = asAtomHandler::toStringId(name,wrk);
				asAtom ret=asAtomHandler::invalidAtom;
				if (asAtomHandler::isUndefined(name) || asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom cls=asAtomHandler::invalidAtom;
					clip->getSystemState()->avm1global->getVariableByMultiname(cls,m,GET_VARIABLE_OPTION::NONE,wrk);
					if (asAtomHandler::is<Class_base>(cls))
					{
						asAtomHandler::as<Class_base>(cls)->getInstance(wrk,ret,true,args,numargs);
					}
					else if (asAtomHandler::is<AVM1Function>(cls))
					{
						ASObject* o = new_functionObject(asAtomHandler::as<AVM1Function>(cls)->prototype);
						o->setprop_prototype(asAtomHandler::as<AVM1Function>(cls)->prototype);
						o->setprop_prototype(asAtomHandler::as<AVM1Function>(cls)->prototype,BUILTIN_STRINGS::STRING_PROTO);
						ret = asAtomHandler::fromObject(o);
						asAtomHandler::as<AVM1Function>(cls)->call(nullptr,&ret, args,numargs,caller,&locals);
						asAtomHandler::as<AVM1Function>(cls)->decRef();
					}
					else
					{
						uint32_t nameIDlower = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,wrk).lowercase());
						f =clip->AVM1GetFunction(nameIDlower);
					}
				}
				if (f)
				{
					ASObject* pr = f->getprop_prototype();
					ASObject* o = nullptr;
					if (pr)
					{
						pr->incRef();
						_NR<ASObject> proto = _MR(pr);
						o = new_functionObject(proto);
						o->setprop_prototype(proto);
						o->setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
					}
					else
					{
						o = new_functionObject(f->prototype);
						o->setprop_prototype(f->prototype);
						o->setprop_prototype(f->prototype,BUILTIN_STRINGS::STRING_PROTO);
					}
					ret = asAtomHandler::fromObject(o);
					f->call(nullptr,&ret, args,numargs,caller,&locals);
				}
				else if (asAtomHandler::isInvalid(ret))
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject class not found "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				ASATOM_DECREF(name);
				ASATOM_DECREF(na);
				PushStack(stack,ret);
				break;
			}

			case 0x41: // ActionDefineLocal2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineLocal2 "<<asAtomHandler::toDebugString(name));
				tiny_string namelower = asAtomHandler::toString(name,wrk).lowercase();
				uint32_t nameID = clip->getSystemState()->getUniqueStringId(namelower);
				if (caller)
				{
					if (locals.find(nameID) == locals.end())
						locals[nameID] = asAtomHandler::undefinedAtom;
				}
				else
				{
					if (clip->avm1locals.find(nameID) == clip->avm1locals.end())
						clip->avm1locals[nameID] = asAtomHandler::undefinedAtom;
				}
				ASATOM_DECREF(name);
				break;
			}
			case 0x42: // ActionInitArray
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInitArray "<<numargs);
				Array* ret=Class<AVM1Array>::getInstanceSNoArgs(wrk);
				ret->resize(numargs);
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					ret->set(i,value,false,false);
				}
				ASATOM_DECREF(na);
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x43: // ActionInitObject
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInitObject "<<numargs);
				ASObject* ret=Class<ASObject>::getInstanceS(wrk);
				//Duplicated keys overwrite the previous value
				multiname propertyName(nullptr);
				propertyName.name_type=multiname::NAME_STRING;
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					asAtom name = PopStack(stack);
					uint32_t nameid=asAtomHandler::toStringId(name,wrk);
					ret->setDynamicVariableNoCheck(nameid,value);
				}
				ASATOM_DECREF(na);
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x44: // ActionTypeOf
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionTypeOf "<<asAtomHandler::toDebugString(obj));
				asAtom res=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(obj))
					res = asAtomHandler::fromString(clip->getSystemState(),"movieclip");
				else
					res = asAtomHandler::typeOf(obj);
				ASATOM_DECREF(obj);
				PushStack(stack,res);
				break;
			}
			case 0x47: // ActionAdd2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				ASObject* o = asAtomHandler::getObject(arg2);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAdd2 "<<asAtomHandler::toDebugString(arg2)<<" + "<<asAtomHandler::toDebugString(arg1));
				if (asAtomHandler::add(arg2,arg1,wrk,false))
				{
					if (o)
						o->decRef();
				}
				ASATOM_DECREF(arg1);
				PushStack(stack,arg2);
				break;
			}
			case 0x48: // ActionLess2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionLess2 "<<asAtomHandler::toDebugString(arg2)<<" < "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg2,wrk,arg1) == TTRUE));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x49: // ActionEquals2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEquals2 "<<asAtomHandler::toDebugString(arg2)<<" == "<<asAtomHandler::toDebugString(arg1));
				if (clip->loadedFrom->version <= 5)
				{
					// equals works different on SWF5: Objects without valueOf property are treated as undefined
					if (asAtomHandler::getObject(arg1) && (asAtomHandler::isFunction(arg1) || !asAtomHandler::getObject(arg1)->has_valueOf()))
						asAtomHandler::setUndefined(arg1);
					if (asAtomHandler::getObject(arg2) && (asAtomHandler::isFunction(arg2) || !asAtomHandler::getObject(arg2)->has_valueOf()))
						asAtomHandler::setUndefined(arg2);
				}
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isEqual(arg2,wrk,arg1)));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x4a: // ActionToNumber
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(arg,clip->loadedFrom->version),false));
				ASATOM_DECREF(arg);
				break;
			}
			case 0x4b: // ActionToString
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),asAtomHandler::toString(arg,wrk)));
				ASATOM_DECREF(arg);
				break;
			}
			case 0x4c: // ActionPushDuplicate
			{
				asAtom a = PeekStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPushDuplicate "<<asAtomHandler::toDebugString(a));
				ASATOM_INCREF(a);
				PushStack(stack,a);
				break;
			}
			case 0x4d: // ActionStackSwap
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStackSwap "<<asAtomHandler::toDebugString(arg1)<<" "<<asAtomHandler::toDebugString(arg2));
				PushStack(stack,arg1);
				PushStack(stack,arg2);
				break;
			}
			case 0x4e: // ActionGetMember
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom ret=asAtomHandler::invalidAtom;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				if (asAtomHandler::isNull(scriptobject) || asAtomHandler::isUndefined(scriptobject) || asAtomHandler::isInvalid(scriptobject))
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember called on Null/Undefined, ignored "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
					asAtomHandler::setUndefined(ret);
				}
				else
				{
					uint32_t nameID = asAtomHandler::toStringId(name,wrk);
					ASObject* o = asAtomHandler::toObject(scriptobject,wrk);
					int step = 2; // we search in two steps, first with the normal name, then with name in lowercase (TODO find some faster method for case insensitive check for members)
					while (step)
					{
						multiname m(nullptr);
						m.isAttribute = false;
						switch (asAtomHandler::getObjectType(name))
						{
							case T_INTEGER:
								m.name_type=multiname::NAME_INT;
								m.name_i=asAtomHandler::getInt(name);
								break;
							case T_UINTEGER:
								m.name_type=multiname::NAME_UINT;
								m.name_ui=asAtomHandler::getUInt(name);
								break;
							case T_NUMBER:
								m.name_type=multiname::NAME_NUMBER;
								m.name_d=asAtomHandler::toNumber(name);
								break;
							default:
								m.name_type=multiname::NAME_STRING;
								m.name_s_id=nameID;
								break;
						}
						if(o)
						{
							o->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::DONT_CHECK_CLASS,wrk);
							if (!asAtomHandler::isValid(ret))
							{
								ASObject* pr = o->is<Class_base>() && o->as<Class_base>()->getPrototype(wrk) ? o->as<Class_base>()->getPrototype(wrk)->getObj() : o->getprop_prototype();
								// search the prototype before searching the AS3 class
								while (pr)
								{
									bool isGetter = pr->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::DONT_CALL_GETTER,wrk) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
									if (isGetter) // getter from prototype has to be called with o as target
									{
										IFunction* f = asAtomHandler::as<IFunction>(ret);
										ret = asAtom();
										f->callGetter(ret,o,wrk);
										break;
									}
									else if (!asAtomHandler::isInvalid(ret))
										break;
									pr = pr->getprop_prototype();
								}
							}
							if (!asAtomHandler::isValid(ret))
								o->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
						}
						if (asAtomHandler::isInvalid(ret))
						{
							step--;
							if (step)
							{
								tiny_string namelower = asAtomHandler::toString(name,wrk).lowercase();
								nameID = clip->getSystemState()->getUniqueStringId(namelower);
							}
						}
						else
							break;
					}
					if (asAtomHandler::isInvalid(ret))
					{
						switch (asAtomHandler::getObjectType(scriptobject))
						{
							case T_FUNCTION:
								if (nameID == BUILTIN_STRINGS::PROTOTYPE)
									ret = asAtomHandler::fromObject(o->as<IFunction>()->prototype.getPtr());
								break;
							case T_OBJECT:
							case T_ARRAY:
							case T_CLASS:
								switch (nameID)
								{
									case BUILTIN_STRINGS::PROTOTYPE:
									{
										if (o->is<Class_base>())
											ret = asAtomHandler::fromObject(o->as<Class_base>()->getPrototype(wrk)->getObj());
										else if (o->getClass())
											ret = asAtomHandler::fromObject(o->getClass()->getPrototype(wrk)->getObj());
										else
											LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember for scriptobject without class "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
										break;
									}
									case BUILTIN_STRINGS::STRING_AVM1_TARGET:
										if (o->is<DisplayObject>())
											ret = asAtomHandler::fromString(clip->getSystemState(),o->as<DisplayObject>()->AVM1GetPath());
										else
											asAtomHandler::setUndefined(ret);
										break;
									default:
									{
										multiname m(nullptr);
										m.isAttribute = false;
										switch (asAtomHandler::getObjectType(name))
										{
											case T_INTEGER:
												m.name_type=multiname::NAME_INT;
												m.name_i=asAtomHandler::getInt(name);
												break;
											case T_UINTEGER:
												m.name_type=multiname::NAME_UINT;
												m.name_ui=asAtomHandler::getUInt(name);
												break;
											case T_NUMBER:
												m.name_type=multiname::NAME_NUMBER;
												m.name_d=asAtomHandler::toNumber(name);
												break;
											default:
												m.name_type=multiname::NAME_STRING;
												m.name_s_id=nameID;
												break;
										}
										o->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
										break;
									}
								}
								break;
							default:
								LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
								break;
						}
					}
					if (asAtomHandler::isInvalid(ret))
					{
						if (o && o->is<DisplayObject>())
							ret = o->as<DisplayObject>()->AVM1GetVariable(asAtomHandler::toString(name,wrk),false);
					}
					if (asAtomHandler::isInvalid(ret))
						asAtomHandler::setUndefined(ret);
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember done "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name)<<" " <<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				PushStack(stack,ret);
				break;
			}
			case 0x4f: // ActionSetMember
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				if (asAtomHandler::isObject(scriptobject) || asAtomHandler::isFunction(scriptobject) || asAtomHandler::isArray(scriptobject))
				{
					ASObject* o = asAtomHandler::getObject(scriptobject);
					if (o->is<DisplayObject>())
						ASATOM_INCREF(value);// incref here to make sure value is not destructed when setting value, reference is consumed in AVM1SetVariableDirect
					multiname m(nullptr);
					switch (asAtomHandler::getObjectType(name))
					{
						case T_INTEGER:
							m.name_type=multiname::NAME_INT;
							m.name_i=asAtomHandler::getInt(name);
							break;
						case T_UINTEGER:
							m.name_type=multiname::NAME_UINT;
							m.name_ui=asAtomHandler::getUInt(name);
							break;
						case T_NUMBER:
							m.name_type=multiname::NAME_NUMBER;
							m.name_d=asAtomHandler::toNumber(name);
							break;
						default:
							m.name_type=multiname::NAME_STRING;
							m.name_s_id=asAtomHandler::toStringId(name,wrk);
							break;
					}
					m.isAttribute = false;
					if (m.name_type==multiname::NAME_STRING && m.name_s_id == BUILTIN_STRINGS::PROTOTYPE)
					{
						ASObject* protobj = asAtomHandler::toObject(value,wrk);
						if (protobj)
						{
							protobj->incRef();
							_NR<ASObject> p = _MR(protobj);
							o->setprop_prototype(p);
							o->setprop_prototype(p,BUILTIN_STRINGS::STRING_PROTO);
						}
						else
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember no prototype found for "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(value));
					}
					else
					{
						if (asAtomHandler::is<AVM1Function>(value))
						{
							AVM1Function* f = asAtomHandler::as<AVM1Function>(value);
							if (f->needsSuper())
							{
								asAtom a =asAtomHandler::as<AVM1Function>(value)->getSuper();
								if (asAtomHandler::isInvalid(a))
								{
									ASObject* sup = o->getprop_prototype();
									if (!sup && o->getClass() != Class<ASObject>::getRef(wrk->getSystemState()).getPtr())
										sup = o->getClass();
									if (sup)
										asAtomHandler::as<AVM1Function>(value)->setSuper(asAtomHandler::fromObjectNoPrimitive(sup));
								}
							}
						}
						bool hassetter=false;
						if (!fromInitAction)
						{
							ASObject* pr = o->getprop_prototype();
							while (pr)
							{
								variable* var = pr->findVariableByMultiname(m,nullptr,nullptr,nullptr,false,wrk);
								if (var && asAtomHandler::is<AVM1Function>(var->setter))
								{
									ASATOM_INCREF(value);
									asAtomHandler::as<AVM1Function>(var->setter)->call(nullptr,&scriptobject,&value,1,caller,&locals);
									hassetter=true;
									break;
								}
								pr = pr->getprop_prototype();
							}
						}
						if (!hassetter)
						{
							o->setVariableByMultiname(m,value,ASObject::CONST_ALLOWED,nullptr,wrk);
						}
					}
					if (o->is<DisplayObject>())
					{
						o->as<DisplayObject>()->AVM1UpdateVariableBindings(m.name_s_id,value);
						uint32_t nameIDlower = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,wrk).lowercase());
						o->as<DisplayObject>()->AVM1SetVariableDirect(nameIDlower,value);
					}
				}
				else
				{
					if (!asAtomHandler::isUndefined(scriptobject))
						LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" "<<(int)asAtomHandler::getObjectType(scriptobject)<<" "<<asAtomHandler::toDebugString(name));
					ASATOM_DECREF(value);
				}
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				break;
			}
			case 0x50: // ActionIncrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionIncrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(num,clip->loadedFrom->version)+1,false));
				ASATOM_DECREF(num);
				break;
			}
			case 0x51: // ActionDecrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDecrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(num,clip->loadedFrom->version)-1,false));
				ASATOM_DECREF(num);
				break;
			}
			case 0x52: // ActionCallMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = numargs ? g_newa(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				asAtom ret=asAtomHandler::invalidAtom;
				bool done=false;
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
				{
					if (asAtomHandler::is<Function>(scriptobject))
					{
						asAtomHandler::as<Function>(scriptobject)->call(ret,wrk,scopestack[0],args,numargs);
					}
					else if (asAtomHandler::is<AVM1Function>(scriptobject))
					{
						asAtomHandler::as<AVM1Function>(scriptobject)->call(&ret,&scopestack[0],args,numargs,caller,&locals);
					}
					else if (asAtomHandler::is<Class_base>(scriptobject))
					{
						Class_base* cls = asAtomHandler::as<Class_base>(scriptobject);
						if (!asAtomHandler::getObjectNoCheck(scopestack[0])->isConstructed() && !fromInitAction)
						{
							cls->handleConstruction(scopestack[0],args,numargs,true);
						}
						LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from class done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					}
					else if (asAtomHandler::isObject(scriptobject))
					{
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
						asAtom constr = asAtomHandler::invalidAtom;
						asAtomHandler::getObjectNoCheck(scriptobject)->getVariableByMultiname(constr,m,GET_VARIABLE_OPTION::NONE,wrk);
						if (asAtomHandler::isValid(constr))
						{
							if (asAtomHandler::is<Function>(constr))
							{
								asAtomHandler::as<Function>(constr)->call(ret,wrk,scopestack[0],args,numargs);
								asAtomHandler::as<Function>(constr)->decRef();
							}
							else if (asAtomHandler::is<AVM1Function>(constr))
							{
								asAtomHandler::as<AVM1Function>(constr)->call(&ret,&scopestack[0],args,numargs,caller,&locals);
								asAtomHandler::as<AVM1Function>(constr)->decRef();
							}
							else if (asAtomHandler::is<Class_base>(constr))
							{
								Class_base* cls = asAtomHandler::as<Class_base>(constr);
								if (!asAtomHandler::getObjectNoCheck(scopestack[0])->isConstructed() && !fromInitAction)
									cls->handleConstruction(scopestack[0],args,numargs,true);
								LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod constructor from class done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
							}
						}
						else
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod without name and srciptobject has no constructor:"<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					}
					else
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod without name and srciptobject is not a function:"<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					done=true;
				}
				if (!done)
				{
					if (asAtomHandler::is<DisplayObject>(scriptobject))
					{
						uint32_t nameIDlower = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,wrk).lowercase());
						AVM1Function* f = asAtomHandler::as<DisplayObject>(scriptobject)->AVM1GetFunction(nameIDlower);
						if (f)
						{
							f->call(&ret,&scriptobject,args,numargs,caller,&locals);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from displayobject done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
							done=true;
						}
					}
				}
				if (asAtomHandler::isNull(scriptobject) || asAtomHandler::isUndefined(scriptobject))
				{
					ret=asAtomHandler::undefinedAtom;
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from undefined/null ignored "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					done=true;
				}
				if (!done)
				{
					uint32_t nameID = asAtomHandler::toStringId(name,wrk);
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom func=asAtomHandler::invalidAtom;
					if (asAtomHandler::isValid(scriptobject))
					{
						ASObject* scrobj = asAtomHandler::toObject(scriptobject,wrk);
						scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::DONT_CHECK_CLASS,wrk);
						if (!asAtomHandler::isValid(func))
						{
							// search the prototype before searching the class
							ASObject* pr =scrobj->getprop_prototype();
							while (pr)
							{
								pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
								if (asAtomHandler::isValid(func))
									break;
								pr = pr->getprop_prototype();
							}
						}
						if (!asAtomHandler::isValid(func))
						{
							scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (!asAtomHandler::isValid(func) && scrobj->is<Class_base>())
								scrobj->as<Class_base>()->getClassVariableByMultiname(func,m,wrk);
						}
						if (!asAtomHandler::is<IFunction>(func))
						{
							if (scrobj->is<IFunction>())
							{
								ASObject* pr =scrobj->as<IFunction>()->prototype.getPtr();
								while (pr)
								{
									pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
									if (asAtomHandler::isValid(func))
										break;
									pr = pr->getprop_prototype();
								}
							}
						}
						if (asAtomHandler::is<Function>(func))
						{
							asAtomHandler::as<Function>(func)->call(ret,wrk,scriptobject.uintval == super.uintval ? scopestack[0] : scriptobject,args,numargs);
							asAtomHandler::as<Function>(func)->decRef();
						}
						else if (asAtomHandler::is<AVM1Function>(func))
						{
							if (scriptobject.uintval == super.uintval)
								asAtomHandler::as<AVM1Function>(func)->call(&ret,&scopestack[0],args,numargs,caller,&locals);
							else
								asAtomHandler::as<AVM1Function>(func)->call(&ret,&scriptobject,args,numargs,caller,&locals);
							asAtomHandler::as<AVM1Function>(func)->decRef();
						}
						else
						{
							LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
							ret = asAtomHandler::undefinedAtom;
						}
					}
					else
						LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod invalid scriptobject "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject)<<" result:"<<asAtomHandler::toDebugString(ret));
				PushStack(stack,ret);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_DECREF(args[i]);
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				ASATOM_DECREF(na);
				break;
			}
			case 0x53: // ActionNewMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				uint32_t nameID = asAtomHandler::toStringId(name,wrk);
				asAtom ret=asAtomHandler::invalidAtom;
				if (nameID == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				asAtom func=asAtomHandler::invalidAtom;
				ASObject* scrobj = asAtomHandler::toObject(scriptobject,wrk);
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
				{
					func = scriptobject;
				}
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					if (!asAtomHandler::is<IFunction>(func))
					{
						ASObject* pr =scrobj->getprop_prototype();
						if (pr)
							pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
				}
				asAtom tmp=asAtomHandler::invalidAtom;
				_NR<ASObject> proto;
				if (asAtomHandler::is<IFunction>(func))
				{
					proto = _MNR(asAtomHandler::as<IFunction>(func)->getprop_prototype());
					if (proto.isNull())
						proto = asAtomHandler::as<IFunction>(func)->prototype;
					else
						proto->incRef();
					if (!proto.isNull())
					{
						multiname mconstr(nullptr);
						mconstr.name_type=multiname::NAME_STRING;
						mconstr.name_s_id=BUILTIN_STRINGS::STRING_CONSTRUCTOR;
						mconstr.isAttribute = false;
						asAtom constr = asAtomHandler::invalidAtom;
						proto->getVariableByMultiname(constr,mconstr,GET_VARIABLE_OPTION::NONE,wrk);
						if (asAtomHandler::is<Class_base>(constr))
							asAtomHandler::as<Class_base>(constr)->getInstance(wrk,ret,true,nullptr,0);
						else if (proto->getClass())
							proto->getClass()->getInstance(wrk,ret,true,nullptr,0);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
					}
				}
				if (asAtomHandler::is<Function>(func))
				{
					asAtomHandler::as<Function>(func)->call(tmp,wrk,ret,args,numargs);
				}
				else if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtomHandler::as<AVM1Function>(func)->call(nullptr,&ret,args,numargs,caller,&locals);
				}
				else if (asAtomHandler::is<Class_base>(func))
				{
					asAtomHandler::as<Class_base>(func)->getInstance(wrk,ret,true,args,numargs);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));

				if (asAtomHandler::isObject(ret))
					asAtomHandler::getObjectNoCheck(ret)->setVariableAtomByQName(BUILTIN_STRINGS::STRING_CONSTRUCTOR,nsNameAndKind(),func,DYNAMIC_TRAIT,true,false);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject)<<" result:"<<asAtomHandler::toDebugString(ret));
				PushStack(stack,ret);
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				ASATOM_DECREF(na);
				break;
			}
			case 0x54: // ActionInstanceOf
			{
				asAtom constr = PopStack(stack);
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInstanceOf "<<asAtomHandler::toDebugString(constr)<<" "<<asAtomHandler::toDebugString(obj));
				ASObject* value = asAtomHandler::toObject(obj,wrk);
				ASObject* type = asAtomHandler::toObject(constr,wrk);
				PushStack(stack, asAtomHandler::fromBool(ABCVm::instanceOf(value,type)));
				value->decRef();
				type->decRef();
				break;
			}
			case 0x55: // ActionEnumerate2
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEnumerate2 "<<asAtomHandler::toDebugString(obj));
				PushStack(stack,asAtomHandler::nullAtom);
				uint32_t index=0;
				if (asAtomHandler::isObject(obj))
				{
					ASObject* o = asAtomHandler::toObject(obj,wrk);
					while ((index = o->nextNameIndex(index)))
					{
						asAtom name=asAtomHandler::invalidAtom;
						o->nextName(name,index);
						PushStack(stack, name);
					}
				}
				ASATOM_DECREF(obj);
				break;
			}
			case 0x60: // ActionBitAnd
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitAnd "<<asAtomHandler::toDebugString(arg1)<<" & "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_and(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x61: // ActionBitOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitOr "<<asAtomHandler::toDebugString(arg1)<<" | "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_or(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x62: // ActionBitXOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitXOr "<<asAtomHandler::toDebugString(arg1)<<" ^ "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_xor(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x63: // ActionBitLShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitLShift "<<asAtomHandler::toDebugString(value)<<" << "<<asAtomHandler::toDebugString(count));
				asAtomHandler::lshift(value,wrk,count);
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x64: // ActionBitRShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitRShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::rshift(value,wrk,count);
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x65: // ActionBitURShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitURShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::urshift(value,wrk,count);
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x66: // ActionStrictEquals
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStrictEquals "<<asAtomHandler::toDebugString(arg2)<<" === "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isEqualStrict(arg1,wrk,arg2) == TTRUE));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x67: // ActionGreater
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGreater "<<asAtomHandler::toDebugString(arg2)<<" > "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg1,wrk,arg2) == TTRUE));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x81: // ActionGotoFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = uint32_t(*it++) | ((*it++)<<8);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,!clip->as<MovieClip>()->state.stop_FP,false);
				break;
			}
			case 0x83: // ActionGetURL
			{
				tiny_string s1((const char*)&(*it));
				it += s1.numBytes()+1;
				tiny_string s2((const char*)&(*it));
				it += s2.numBytes()+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetURL "<<s1<<" "<<s2);
				clip->getSystemState()->openPageInBrowser(s1,s2);
				break;
			}
			case 0x87: // ActionStoreRegister
			{
				asAtom a = PeekStack(stack);
				ASATOM_INCREF(a);
				uint8_t num = *it++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStoreRegister "<<(int)num<<" "<<asAtomHandler::toDebugString(a));
				ASATOM_DECREF(registers[num]);
				registers[num] = a;
				break;
			}
			case 0x88: // ActionConstantPool
			{
				uint32_t c = uint32_t(*it++) | ((*it++)<<8);
				context->AVM1ClearConstants();
				for (uint32_t i = 0; i < c; i++)
				{
					tiny_string s((const char*)&(*it),true);
					it += s.numBytes()+1;
					context->AVM1AddConstant(clip->getSystemState()->getUniqueStringId(s));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionConstantPool "<<c);
				break;
			}
			case 0x8a: // ActionWaitForFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionWaitForFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = uint32_t(*it++) | ((*it++)<<8);
				uint32_t skipcount = (uint32_t)(*it++);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWaitForFrame "<<frame<<"/"<<clip->as<MovieClip>()->getFramesLoaded()<<" skip "<<skipcount);
				if (clip->as<MovieClip>()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
				{
					// frame not yet loaded, skip actions
					while (skipcount && it != actionlist.end())
					{
						if (*it++ > 0x80)
						{
							uint32_t c = uint32_t(*it++) | ((*it++)<<8);
							it+=c;
						}
						skipcount--;
					}
				}
				break;
			}
			case 0x08: // ActionToggleQuality
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF3 DoActionTag ActionToggleQuality "<<hex<<(int)*it);
				break;
			case 0x8b: // ActionSetTarget
			{
				tiny_string s((const char*)&(*it));
				it += s.numBytes()+1;
				if (!clip)
				{
					LOG_CALL("AVM1: ActionSetTarget: setting target from undefined value to "<<s);
				}
				else
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget "<<s);
				}
				if (clip_isTarget)
					clip->decRef();
				clip_isTarget=false;
				if (s.empty())
					clip = originalclip;
				else
				{
					clip = clip->AVM1GetClipFromPath(s);
					if (!clip)
						LOG(LOG_ERROR,"AVM1: ActionSetTarget clip not found:"<<s);
				}
				break;
			}
			case 0x8c: // ActionGotoLabel
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoLabel "<<clip->toDebugString());
					break;
				}
				tiny_string s((const char*)&(*it));
				it += s.numBytes()+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoLabel "<<s);
				clip->as<MovieClip>()->AVM1gotoFrameLabel(s,true,true);
				break;
			}
			case 0x8e: // ActionDefineFunction2
			{
				tiny_string name((const char*)&(*it),true);
				it += name.numBytes()+1;
				uint32_t paramcount = uint32_t(*it++) | ((*it++)<<8);
				*it++; //register count not used
				uint8_t flags = *it++;
				bool flag1 = flags&0x80;//PreloadParent
				bool flag2 = flags&0x40;//PreloadRoot
				bool flag3 = flags&0x20;//SuppressSuper
				bool flag4 = flags&0x10;//PreloadSuper
				bool flag5 = flags&0x08;//SuppressArguments
				bool flag6 = flags&0x04;//PreloadArguments
				bool flag7 = flags&0x02;//SuppressThis
				bool flag8 = flags&0x01;//PreloadThis
				bool flag9 = (*it++)&0x01;//PreloadGlobal
				std::vector<uint32_t> funcparamnames;
				std::vector<uint8_t> registernumber;
				for (uint16_t i=0; i < paramcount; i++)
				{
					uint8_t regnum = *it++;
					tiny_string n((const char*)&(*it),true);
					it += n.numBytes()+1;
					funcparamnames.push_back(clip->getSystemState()->getUniqueStringId(n.lowercase()));
					registernumber.push_back(regnum);
				}
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				vector<uint8_t> code;
				code.assign(it,it+codesize);
				it += codesize;
				Activation_object* act = name == "" ? new_activationObject(wrk) : nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction2 "<<name<<" "<<paramcount<<" "<<flag1<<flag2<<flag3<<flag4<<flag5<<flag6<<flag7<<flag8<<flag9<<" "<<codesize<<" "<<act);
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,act,context,funcparamnames,code,registernumber,flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8, flag9);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()->worker));
				f->prototype->addStoredMember();
				if (name == "")
				{
					for (uint32_t i = 0; i < paramnames.size(); i++)
					{
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						m.name_s_id=paramnames[i];
						asAtom v = locals[paramnames[i]];
						ASATOM_INCREF(v);
						act->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
					}
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
					clip->AVM1SetFunction(name,_MR(f));
				break;
			}
			case 0x94: // ActionWith
			{
				asAtom obj = PopStack(stack);
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				auto itend = it;
				itend+=codesize;
				if (curdepth >= maxdepth)
				{
					it = itend;
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWith depth exceeds maxdepth");
					break;
				}
				Log::calls_indent++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWith "<<codesize<<" "<<asAtomHandler::toDebugString(obj));
				++curdepth;
				if (clip_isTarget)
					clip->decRef();
				clip_isTarget=true;
				if (asAtomHandler::is<DisplayObject>(obj))
					clip = asAtomHandler::as<DisplayObject>(obj);
				ASATOM_INCREF(obj);
				scopestack[curdepth] = obj;
				scopestackstop[curdepth] = itend;
				break;
			}
			case 0x96: // ActionPush
			{
				uint32_t len = ((*(it-1))<<8) | (*(it-2));
				while (len > 0)
				{
					uint8_t type = *it++;
					len--;
					switch (type)
					{
						case 0:
						{
							tiny_string val((const char*)&(*it),true);
							len -= val.numBytes()+1;
							it += val.numBytes()+1;
							asAtom a = asAtomHandler::fromStringID(clip->getSystemState()->getUniqueStringId(val));
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 0 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 1:
						{
							FLOAT f;
							f.read((const uint8_t*)&(*it));
							it+=4;
							len-=4;
							asAtom a = asAtomHandler::fromNumber(wrk,f,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 1 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 2:
							PushStack(stack,asAtomHandler::nullAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 2 "<<asAtomHandler::toDebugString(asAtomHandler::nullAtom));
							break;
						case 3:
							PushStack(stack,asAtomHandler::undefinedAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 3 "<<asAtomHandler::toDebugString(asAtomHandler::undefinedAtom));
							break;
						case 4:
						{
							uint32_t reg = *it++;
							len--;
							asAtom a = registers[reg];
							ASATOM_INCREF(a);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 4 register "<<reg<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 5:
						{
							asAtom a = asAtomHandler::fromBool((bool)*it++);
							len--;
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 5 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 6:
						{
							DOUBLE d;
							d.read((const uint8_t*)&(*it));
							it+=8;
							len-=8;
							asAtom a = asAtomHandler::fromNumber(wrk,d,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 6 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 7:
						{
							uint32_t d=GUINT32_FROM_LE(*(uint32_t*)&(*it));
							it+=4;
							len-=4;
							asAtom a = asAtomHandler::fromInt((int32_t)d);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 7 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 8:
						{
							uint32_t index = (*it++);
							len--;
							asAtom a = context->AVM1GetConstant(index);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 8 "<<index<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 9:
						{
							uint32_t index = uint32_t(*it++) | ((*it++)<<8);
							len-=2;
							asAtom a = context->AVM1GetConstant(index);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 9 "<<index<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF4 DoActionTag push type "<<(int)type);
							break;
					}
				}
				break;
			}
			case 0x99: // ActionJump
			{
				int32_t skip = int16_t((*it++) | ((*it++)<<8));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionJump "<<skip<<" "<< (it-actionlist.begin()));
				if (skip < 0 && it-actionlist.begin() < -skip)
				{
					LOG(LOG_ERROR,"ActionJump: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
					it = actionlist.begin();
				}
				if (skip >=0 && skip + (it-actionlist.begin()) > (int)actionlist.size())
				{
					LOG(LOG_ERROR,"ActionJump: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
					it = actionlist.end();
				}
				else
					it+= skip;
				break;
			}
			case 0x9a: // ActionGetURL2
			{
				asAtom at=PopStack(stack);
				asAtom au=PopStack(stack);
				uint8_t b = *it++;
				uint8_t method = b&0xc0>>6;
				bool loadtarget = b&0x02;
				bool loadvars = b&0x01;

				if (loadtarget) //LoadTargetFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadTargetFlag");
				if (loadvars) //LoadVariablesFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadVariablesFlag");
				if (method) //SendVarsMethod
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with SendVarsMethod");
				tiny_string url = asAtomHandler::toString(au,wrk);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetURL2 "<<url<<" "<<asAtomHandler::toDebugString(at));
				if (asAtomHandler::is<MovieClip>(at))
				{
					asAtom ret = asAtomHandler::invalidAtom;
					if (url != "")
						MovieClip::AVM1LoadMovie(ret,wrk,at,&au,1);
					else
					{
						// it seems that calling ActionGetURL2 with a clip as target and no url unloads the clip
						MovieClip::AVM1UnloadMovie(ret,wrk,at,nullptr,0);
					}
				}
				else
				{
					tiny_string target = asAtomHandler::toString(at,wrk);
					clip->getSystemState()->openPageInBrowser(url,target);
				}
				ASATOM_DECREF(at);
				ASATOM_DECREF(au);
				break;
			}
			case 0x9b: // ActionDefineFunction
			{
				tiny_string name((const char*)&(*it),true);
				it += name.numBytes()+1;
				uint32_t paramcount = uint32_t(*it++) | ((*it++)<<8);
				std::vector<uint32_t> paramnames;
				for (uint16_t i=0; i < paramcount; i++)
				{
					tiny_string n((const char*)&(*it),true);
					it += n.numBytes()+1;
					paramnames.push_back(clip->getSystemState()->getUniqueStringId(n.lowercase()));
				}
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				vector<uint8_t> code;
				code.assign(it,it+codesize);
				it += codesize;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction "<<name<<" "<<paramcount);
				Activation_object* act = name == "" ? new_activationObject(wrk) : nullptr;
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,act,context,paramnames,code);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()->worker));
				f->prototype->addStoredMember();
				if (name == "")
				{
					for (uint32_t i = 0; i < paramnames.size(); i++)
					{
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						m.name_s_id=paramnames[i];
						asAtom v = locals[paramnames[i]];
						ASATOM_INCREF(v);
						act->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
					}
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
					clip->AVM1SetFunction(name,_MR(f));
				break;
			}
			case 0x9d: // ActionIf
			{
				int32_t skip = int16_t((*it++) | ((*it++)<<8));
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionIf "<<asAtomHandler::toDebugString(a)<<" "<<skip);
				if (asAtomHandler::AVM1toBool(a))
				{
					if (skip < 0 && it-actionlist.begin() < -skip)
					{
						LOG(LOG_ERROR,"ActionIf: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
						it = actionlist.begin();
					}
					if (skip >=0 && skip + (it-actionlist.begin()) > (int)actionlist.size())
					{
						LOG(LOG_ERROR,"ActionIf: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
						it = actionlist.end();
					}
					else
						it += skip;
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x9e: // ActionCall
			{
				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionCall "<<clip->toDebugString());
					break;
				}
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCall label "<<s);
					clip->as<MovieClip>()->AVM1ExecuteFrameActionsFromLabel(s);
				}
				else
				{
					uint32_t frame = asAtomHandler::toUInt(a);
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCall frame "<<frame);
					clip->as<MovieClip>()->AVM1ExecuteFrameActions(frame);
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x9f: // ActionGotoFrame2
			{
				bool biasflag = (*it)&0x02;
				bool playflag = (*it)&0x01;
				it++;
				uint32_t biasframe = 0;
				if (biasflag)
					biasframe = uint32_t(*it++) | ((*it++)<<8);
				
				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoFrame2 "<<clip->toDebugString());
					ASATOM_DECREF(a);
					break;
				}
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					if (biasframe)
						LOG(LOG_NOT_IMPLEMENTED,"AVM1: GotFrame2 with bias and label:"<<asAtomHandler::toDebugString(a));
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame2 label "<<s);
					clip->as<MovieClip>()->AVM1gotoFrameLabel(s,!playflag,clip->as<MovieClip>()->state.stop_FP == playflag);
					
				}
				else
				{
					uint32_t frame = asAtomHandler::toUInt(a)+biasframe;
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame2 "<<frame);
					if (frame>0) // variable frame is 1-based
						frame--;
					clip->as<MovieClip>()->AVM1gotoFrame(frame,!playflag,clip->as<MovieClip>()->state.stop_FP == playflag);
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x8d: // ActionWaitForFrame2
			{
				uint32_t skipcount= (*it);
				it++;
				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionWaitForFrame2 "<<clip->toDebugString());
					ASATOM_DECREF(a);
					break;
				}
				uint32_t frame=0;
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					frame = clip->as<MovieClip>()->getFrameIdByLabel(s,"");
				}
				else
				{
					frame = asAtomHandler::toUInt(a);
				}
				if (clip->as<MovieClip>()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
				{
					// frame not yet loaded, skip actions
					while (skipcount && it != actionlist.end())
					{
						it++;
						if (*it > 0x80)
						{
							uint32_t c = uint32_t(*it++) | ((*it++)<<8);
							it+=c;
						}
						skipcount--;
					}
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x33: // ActionAsciiToChar
			{
				asAtom a = PopStack(stack);
				asAtom ret = asAtomHandler::fromStringID(asAtomHandler::toInt(a));
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x14: // ActionStringLength
			{
				asAtom a = PopStack(stack);
				tiny_string s= asAtomHandler::toString(a,wrk);
				asAtom ret = asAtomHandler::fromInt(s.numChars());
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x29: // ActionStringLess
			case 0x31: // ActionMBStringLength
			case 0x32: // ActionCharToAscii
			case 0x35: // ActionMBStringExtract
			case 0x36: // ActionMBCharToAscii
			case 0x37: // ActionMBAsciiToChar
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF4 DoActionTag "<<hex<<(int)opcode);
				if(opcode >= 0x80)
				{
					uint32_t len = ((*(it-1))<<8) | (*(it-2));
					it+=len;
				}
				break;
			case 0x45: // ActionTargetPath
			case 0x46: // ActionEnumerate
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF5 DoActionTag "<<hex<<(int)opcode);
				break;
			case 0x68: // ActionStringGreater
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF6 DoActionTag "<<hex<<(int)opcode);
				break;
			case 0x2a: // ActionThrow
			case 0x2c: // ActionImplementsOp
			case 0x8f: // ActionTry
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF7 DoActionTag "<<hex<<(int)opcode);
				if(opcode >= 0x80)
				{
					uint32_t len = ((*(it-1))<<8) | (*(it-2));
					it+=len;
				}
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" invalid DoActionTag "<<hex<<(int)opcode);
				throw RunTimeException("invalid AVM1 tag");
				break;
		}
	}
	for (int i = 0; i <= curdepth; i++)
	{
		ASATOM_DECREF(scopestack[i]);
	}
	for (auto it = locals.begin(); it != locals.end(); it++)
	{
		ASATOM_DECREF(it->second);
	}
	for (uint32_t i = 0; i < 256; i++)
	{
		ASATOM_DECREF(registers[i]);
	}
	while (!stack.empty())
	{
		asAtom a = PopStack(stack);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" stack not empty:"<<asAtomHandler::toDebugString(a));
		ASATOM_DECREF(a);
	}
	if (clip_isTarget)
		clip->decRef();
	if (argarray)
		argarray->decRef();
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions done");
	Log::calls_indent--;
}
