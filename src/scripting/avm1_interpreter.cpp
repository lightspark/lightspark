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
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/avm1/avm1display.h"

using namespace std;
using namespace lightspark;

void ACTIONRECORD::PushStack(std::stack<asAtom> &stack, const asAtom &a)
{
	ASATOM_INCREF(a);
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
int ACTIONRECORD::getFullLength()
{
	int res = actionCode < 0x80 ? 1 : Length+3;
	if (actionCode != 0x96) // ActionPush fills data_actionlist, but not from stream
	{
		for (auto it = data_actionlist.begin(); it != data_actionlist.end(); it++)
			res += it->getFullLength();
	}
	return res;
}

void ACTIONRECORD::executeActions(MovieClip *clip, AVM1context* context, std::vector<ACTIONRECORD> &actionlist, std::map<uint32_t, asAtom> &scopevariables, asAtom* result, asAtom* obj, asAtom *args, uint32_t num_args, const std::vector<uint32_t>& paramnames, const std::vector<uint8_t>& paramregisternumbers,
								  bool preloadParent, bool preloadRoot, bool suppressSuper, bool preloadSuper, bool suppressArguments, bool preloadArguments, bool suppressThis, bool preloadThis, bool preloadGlobal)
{
	Log::calls_indent++;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" executeActions "<<preloadParent<<preloadRoot<<suppressSuper<<preloadSuper<<suppressArguments<<preloadArguments<<suppressThis<<preloadThis<<preloadGlobal);
	if (result)
		asAtomHandler::setUndefined(*result);
	std::stack<asAtom> stack;
	asAtom registers[256];
	std::fill_n(registers,256,asAtomHandler::undefinedAtom);
	std::map<uint32_t,asAtom> locals;
	int curdepth = 0;
	int maxdepth= clip->getSystemState()->mainClip->version < 6 ? 8 : 16;
	asAtom* scopestack = g_newa(asAtom, maxdepth);
	scopestack[0] = obj ? *obj : asAtomHandler::fromObject(clip);
	ASATOM_INCREF(scopestack[0]);
	std::vector<ACTIONRECORD>::iterator* scopestackstop = g_newa(std::vector<ACTIONRECORD>::iterator, maxdepth);
	scopestackstop[0] = actionlist.end();
	uint32_t currRegister = 1; // spec is not clear, but gnash starts at register 1
	if (!suppressThis)
	{
		ASATOM_INCREF(scopestack[0]);
		if (preloadThis)
			registers[currRegister++] = scopestack[0];
		else
			locals[paramnames[currRegister++]] = scopestack[0];
	}
	if (!suppressArguments)
	{
		if (preloadArguments)
		{
			LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" placing arguments in register "<<num_args);
			Array* regargs = Class<Array>::getInstanceS(clip->getSystemState());
			for (uint32_t i = 0; i < num_args; i++)
			{
				ASATOM_INCREF(args[i]);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
				regargs->push(args[i]);
			}
			registers[currRegister++] = asAtomHandler::fromObject(regargs);
		}
		else
		{
			for (uint32_t i = 0; i < paramnames.size() && i < num_args; i++)
			{
				ASATOM_INCREF(args[i]);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
				locals[paramnames[i]] = args[i];
			}
		}
	}
	if (!suppressSuper)
	{
		if (clip->getClass()->super)
			clip->getClass()->super->incRef();
		if (preloadSuper)
			registers[currRegister++] = asAtomHandler::fromObject(clip->getClass()->super.getPtr());
		else
			locals[paramnames[currRegister++]] = asAtomHandler::fromObject(clip->getClass()->super.getPtr());
	}
	if (preloadRoot)
	{
		clip->getSystemState()->mainClip->incRef();
		registers[currRegister++] = asAtomHandler::fromObject(clip->getSystemState()->mainClip);
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
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" set argument "<<i<<" "<<(int)paramregisternumbers[i]<<" "<<asAtomHandler::toDebugString(args[i]));
		ASATOM_INCREF(args[i]);
		if (paramregisternumbers[i] == 0)
			locals[paramnames[i]] = args[i];
		else
			registers[paramregisternumbers[i]] = args[i];
	}

	MovieClip *originalclip = clip;
	for (auto it = actionlist.begin(); it != actionlist.end();it++)
	{
		if (curdepth > 0 && it == scopestackstop[curdepth])
		{
			curdepth--;
		}
		if (!clip
				&& it->actionCode != 0x20 // ActionSetTarget2
				&& it->actionCode != 0x8b // ActionSetTarget
				)
		{
			// we are in a target that was not found during ActionSetTarget(2), so these actions are ignored
			continue;
		}
		//LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" action code:"<<hex<<(int)it->actionCode);
		
		switch (it->actionCode)
		{
			case 0x04: // ActionNextFrame
			{
				uint32_t frame = clip->state.FP+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNextFrame "<<frame);
				clip->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x05: // ActionPreviousFrame
			{
				uint32_t frame = clip->state.FP > 0 ? clip->state.FP-1 : 0;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPreviousFrame "<<frame);
				clip->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x06: // ActionPlay
			{
				uint32_t frame = clip->state.next_FP;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPlay ");
				clip->AVM1gotoFrame(frame,false,true);
				break;
			}
			case 0x07: // ActionStop
			{
				uint32_t frame = clip->state.FP;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStop ");
				clip->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x09: // ActionStopSounds
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStopSounds");
				// TODO for now we just mute all sounds instead of stopping them
				clip->getSystemState()->audioManager->muteAll();
				break;
			}
			case 0x0a: // ActionAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAdd "<<b<<"+"<<a);
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),a+b,false));
				break;
			}
			case 0x0b: // ActionSubtract
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSubtract "<<b<<"-"<<a);
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),b-a,false));
				break;
			}
			case 0x0c: // ActionMultiply
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionMultiply "<<b<<"*"<<a);
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),b*a,false));
				break;
			}
			case 0x0d: // ActionDivide
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDivide "<<b<<"/"<<a);
				asAtom ret=asAtomHandler::invalidAtom;
				if (a == 0)
				{
					if (clip->getSystemState()->mainClip->version == 4)
						ret = asAtomHandler::fromString(clip->getSystemState(),"#ERROR#");
					else
						ret = asAtomHandler::fromNumber(clip->getSystemState(),Number::NaN,false);
				}
				else
					ret = asAtomHandler::fromNumber(clip->getSystemState(),b/a,false);
				PushStack(stack,ret);
				break;
			}
			case 0x0e: // ActionEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEquals "<<b<<"=="<<a);
				PushStack(stack,asAtomHandler::fromBool(b==a));
				break;
			}
			case 0x0f: // ActionLess
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionLess "<<b<<"<"<<a);
				PushStack(stack,asAtomHandler::fromBool(b<a));
				break;
			}
			case 0x10: // ActionAnd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAnd "<<b<<" && "<<a);
				PushStack(stack,asAtomHandler::fromBool(b && a));
				break;
			}
			case 0x11: // ActionOr
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->getSystemState()->mainClip->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionOr "<<b<<" || "<<a);
				PushStack(stack,asAtomHandler::fromBool(b || a));
				break;
			}
			case 0x12: // ActionNot
			{
				asAtom a = PopStack(stack);
				bool value = asAtomHandler::AVM1toBool(a);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNot "<<value);
				PushStack(stack,asAtomHandler::fromBool(!value));
				break;
			}
			case 0x13: // ActionStringEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::toString(aa,clip->getSystemState());
				tiny_string b = asAtomHandler::toString(ab,clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStringEquals "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromBool(a == b));
				break;
			}
			case 0x17: // ActionPop
			{
				PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPop");
				break;
			}
			case 0x18: // ActionToInteger
			{
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionToInteger "<<asAtomHandler::toDebugString(a));
				PushStack(stack,asAtomHandler::fromInt(asAtomHandler::toInt(a)));
				break;
			}
			case 0x1c: // ActionGetVariable
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetVariable "<<asAtomHandler::toDebugString(name));
				tiny_string s = asAtomHandler::toString(name,clip->getSystemState());
				asAtom res=asAtomHandler::invalidAtom;
				if (s=="this")
				{
					if(asAtomHandler::isValid(scopestack[curdepth]))
						res = scopestack[curdepth];
					else
						res = asAtomHandler::fromObject(clip);
				}
				else if (s == "_global")
				{
					res = asAtomHandler::fromObject(clip->getSystemState()->avm1global);
				}
				else if (s.find(".") == tiny_string::npos)
				{
					auto it = locals.find(clip->getSystemState()->getUniqueStringId(s.lowercase()));
					if (it != locals.end()) // local variable
						res = it->second;
				}
				if (asAtomHandler::isInvalid(res))
				{
					if (!s.startsWith("/") && !s.startsWith(":"))
						res = originalclip->AVM1GetVariable(s);
					else
						res = clip->AVM1GetVariable(s);
				}
				if (asAtomHandler::isInvalid(res) && !scopevariables.empty())
				{
					uint32_t nameID = clip->getSystemState()->getUniqueStringId(s);
					auto it = scopevariables.find(nameID);
					if (it != scopevariables.end())
						res = it->second;
				}
				if (asAtomHandler::isInvalid(res))
					asAtomHandler::setUndefined(res);
				PushStack(stack,res);
				break;
			}
			case 0x1d: // ActionSetVariable
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetVariable "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				tiny_string s = asAtomHandler::toString(name,clip->getSystemState());
				bool found = false;
				if (!context->keepLocals && s.find(".") == tiny_string::npos)
				{
					// variable names are case insensitive
					auto it = locals.find(clip->getSystemState()->getUniqueStringId(s.lowercase()));
					if (it != locals.end()) // local variable
					{
						it->second = value;
						found = true;
					}
				}
				if (!found)
				{
					if (!s.startsWith("/") && !s.startsWith(":"))
						originalclip->AVM1SetVariable(s,value);
					else
						clip->AVM1SetVariable(s,value);
				}
				break;
			}
			case 0x20: // ActionSetTarget2
			{
				asAtom a = PopStack(stack);
				tiny_string s = asAtomHandler::toString(a,clip->getSystemState());
				if (!clip)
				{
					LOG_CALL("AVM1: ActionSetTarget2: setting target from undefined value to "<<s);
				}
				else
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetTarget2 "<<s);
				}
				if (s.empty())
					clip = originalclip;
				else
				{
					clip = clip->AVM1GetClipFromPath(s);
					if (!clip)
					{
						LOG(LOG_ERROR,"AVM1: ActionSetTarget2 clip not found:"<<s);
					}
				}
				break;
			}
			case 0x21: // ActionStringAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::toString(aa,clip->getSystemState());
				tiny_string b = asAtomHandler::toString(ab,clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStringAdd "<<b<<"+"<<a);
				asAtom ret = asAtomHandler::fromString(clip->getSystemState(),b+a);
				PushStack(stack,ret);
				break;
			}
			case 0x22: // ActionGetProperty
			{
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index));
				DisplayObject* o = clip;
				if (asAtomHandler::toStringId(target,clip->getSystemState()) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,clip->getSystemState());
					o = clip->AVM1GetClipFromPath(s);
				}
				asAtom ret = asAtomHandler::undefinedAtom;
				if (o)
				{
					asAtom obj = asAtomHandler::fromObject(o);
					switch (asAtomHandler::toInt(index))
					{
						case 0:// x
							DisplayObject::_getX(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 1:// y
							DisplayObject::_getY(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 2:// xscale
							DisplayObject::_getScaleX(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 3:// xscale
							DisplayObject::_getScaleY(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 7:// visible
							DisplayObject::_getVisible(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 8:// width
							DisplayObject::_getWidth(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 9:// height
							DisplayObject::_getHeight(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 10:// rotation
							DisplayObject::_getRotation(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 13:// name
							DisplayObject::_getter_name(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 16:// quality
							DisplayObject::AVM1_getQuality(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 20:// xmouse
							DisplayObject::_getMouseX(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 21:// ymouse
							DisplayObject::_getMouseY(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1: GetProperty type:"<<asAtomHandler::toInt(index));
							break;
					}
				}
				PushStack(stack,ret);
				break;
			}
			case 0x23: // ActionSetProperty
			{
				asAtom value = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				DisplayObject* o = clip;
				if (asAtomHandler::is<MovieClip>(target))
				{
					o = asAtomHandler::as<MovieClip>(target);
				}
				else if (asAtomHandler::toStringId(target,clip->getSystemState()) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,clip->getSystemState());
					o = clip->AVM1GetClipFromPath(s);
					if (!o) // it seems that Adobe falls back to the current clip if the path is invalid
						o = clip;
				}
				if (o)
				{
					asAtom obj = asAtomHandler::fromObject(o);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom valueInt = asAtomHandler::fromInt(asAtomHandler::toInt(value));
					switch (asAtomHandler::toInt(index))
					{
						case 0:// x
							DisplayObject::_setX(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 1:// y
							DisplayObject::_setY(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 2:// xscale
							DisplayObject::_setScaleX(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 3:// xscale
							DisplayObject::_setScaleY(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 7:// visible
							DisplayObject::_setVisible(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 8:// width
							DisplayObject::_setWidth(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 9:// height
							DisplayObject::_setHeight(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 10:// rotation
							DisplayObject::_setRotation(ret,clip->getSystemState(),obj,&valueInt,1);
							break;
						case 16:// quality
							DisplayObject::AVM1_setQuality(ret,clip->getSystemState(),obj,&value,1);
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1: SetProperty type:"<<asAtomHandler::toInt(index));
							break;
					}
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetProperty target clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				break;
			}
			case 0x24: // ActionCloneSprite
			{
				asAtom ad = PopStack(stack);
				uint32_t depth = asAtomHandler::toUInt(ad);
				asAtom target = PopStack(stack);
				asAtom sp = PopStack(stack);
				tiny_string sourcepath = asAtomHandler::toString(sp,clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCloneSprite "<<asAtomHandler::toDebugString(target)<<" "<<sourcepath<<" "<<depth);
				MovieClip* source = clip->AVM1GetClipFromPath(sourcepath);
				if (source)
				{
					DisplayObjectContainer* parent = source->getParent();
					if (!parent || !parent->is<MovieClip>())
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCloneSprite: the clip has no parent:"<<asAtomHandler::toDebugString(target)<<" "<<sourcepath<<" "<<depth);
						break;
					}
					DefineSpriteTag* tag = (DefineSpriteTag*)clip->getSystemState()->mainClip->dictionaryLookup(source->getTagID());
					AVM1MovieClip* ret=Class<AVM1MovieClip>::getInstanceS(clip->getSystemState(),*tag,source->getTagID(),asAtomHandler::toStringId(target,clip->getSystemState()));
					ret->setLegacyMatrix(source->getMatrix());
					parent->as<MovieClip>()->insertLegacyChildAt(depth,ret);
					ret->incRef();
					clip->getSystemState()->currentVm->addEvent(NullRef, _MR(new (clip->getSystemState()->unaccountedMemory) ExecuteFrameScriptEvent(_MR(ret))));
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCloneSprite source clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<sourcepath<<" "<<depth);
				break;
			}
			case 0x25: // ActionRemoveSprite
			{
				asAtom target = PopStack(stack);
				DisplayObject* o = nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionRemoveSprite "<<asAtomHandler::toDebugString(target));
				if (asAtomHandler::is<DisplayObject>(target))
				{
					o = asAtomHandler::as<DisplayObject>(target);
				}
				else
				{
					uint32_t targetID = asAtomHandler::toStringId(target,clip->getSystemState());
					if (targetID != BUILTIN_STRINGS::EMPTY)
					{
						tiny_string s = asAtomHandler::toString(target,clip->getSystemState());
						o = clip->AVM1GetClipFromPath(s);
					}
					else
						o = clip;
				}
				if (o && o->getParent())
				{
					o->incRef();
					o->getParent()->_removeChild(o);
				}
				break;
			}
			case 0x26: // ActionTrace
			{
				asAtom value = PopStack(stack);
				Log::print(asAtomHandler::toString(value,clip->getSystemState()));
				break;
			}
			case 0x27: // ActionStartDrag
			{
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStartDrag "<<asAtomHandler::toDebugString(target));
				asAtom lockcenter = PopStack(stack);
				asAtom constrain = PopStack(stack);
				Rectangle* rect = nullptr;
				if (asAtomHandler::toInt(constrain))
				{
					asAtom y2 = PopStack(stack);
					asAtom x2 = PopStack(stack);
					asAtom y1 = PopStack(stack);
					asAtom x1 = PopStack(stack);
					rect = Class<Rectangle>::getInstanceS(clip->getSystemState());
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(rect);
					Rectangle::_setTop(ret,clip->getSystemState(),obj,&y1,1);
					Rectangle::_setLeft(ret,clip->getSystemState(),obj,&x1,1);
					Rectangle::_setBottom(ret,clip->getSystemState(),obj,&y2,1);
					Rectangle::_setRight(ret,clip->getSystemState(),obj,&x2,1);
				}
				asAtom obj=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(target))
				{
					obj = target;
				}
				else
				{
					tiny_string s = asAtomHandler::toString(target,clip->getSystemState());
					MovieClip* targetclip = clip->AVM1GetClipFromPath(s);
					if (targetclip)
					{
						obj = asAtomHandler::fromObject(targetclip);
					}
					else
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStartDrag target clip not found:"<<asAtomHandler::toDebugString(target));
						break;
					}
				}
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom args[2];
				args[0] = lockcenter;
				if (rect)
					args[1] = asAtomHandler::fromObject(rect);
				Sprite::_startDrag(ret,clip->getSystemState(),obj,args,rect ? 2 : 1);
				break;
			}
			case 0x28: // ActionEndDrag
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(clip);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEndDrag "<<asAtomHandler::toDebugString(obj));
				Sprite::_stopDrag(ret,clip->getSystemState(),obj,nullptr,0);
				break;
			}
			case 0x2b: // ActionCastOp
			{
				asAtom obj = PopStack(stack);
				asAtom constr = PopStack(stack);
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCastOp "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(constr));
				PushStack(stack,asAtomHandler::nullAtom);
				break;
			}
			case 0x30: // ActionRandomNumber
			{
				asAtom am = PopStack(stack);
				int maximum = asAtomHandler::toInt(am);
				int ret = (((number_t)rand())/(number_t)RAND_MAX)*maximum;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionRandomNumber "<<ret);
				PushStack(stack,asAtomHandler::fromInt(ret));
				break;
			}
			case 0x34: // ActionGetTime
			{
				gint64 runtime = compat_msectiming()-clip->getSystemState()->startTime;
				asAtom ret=asAtomHandler::fromNumber(clip->getSystemState(),(number_t)runtime,false);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetTime "<<asAtomHandler::toDebugString(ret));
				PushStack(stack,asAtom(ret));
				break;
			}
			case 0x3a: // ActionDelete
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				asAtom ret = asAtomHandler::falseAtom;
				if (asAtomHandler::isObject(scriptobject))
				{
					ASObject* o = asAtomHandler::getObject(scriptobject);
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=asAtomHandler::toStringId(name,clip->getSystemState());
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m))
						ret = asAtomHandler::trueAtom;
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3b: // ActionDelete2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete2 "<<asAtomHandler::toDebugString(name));
				DisplayObject* o = clip;
				asAtom ret = asAtomHandler::falseAtom;
				while (o)
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=asAtomHandler::toStringId(name,clip->getSystemState());
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m))
					{
						ret = asAtomHandler::trueAtom;
						break;
					}
					o = o->getParent();
				}
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3c: // ActionDefineLocal
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineLocal "<<asAtomHandler::toDebugString(name)<<" " <<asAtomHandler::toDebugString(value));
				if (context->keepLocals)
				{
					tiny_string s =asAtomHandler::toString(name,clip->getSystemState()).lowercase();
					clip->AVM1SetVariable(s,value);
				}
				else
				{
					uint32_t nameID = clip->getSystemState()->getUniqueStringId(asAtomHandler::toString(name,clip->getSystemState()).lowercase());
					locals[nameID] = value;
				}
				break;
			}
			case 0x3d: // ActionCallFunction
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				AVM1Function* f =nullptr;
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,clip->getSystemState()) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				else
					f =clip->AVM1GetFunction(nameID);
				asAtom ret=asAtomHandler::invalidAtom;
				if (f)
					f->call(&ret,nullptr, args,numargs);
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom builtinfunc=asAtomHandler::invalidAtom;
					clip->getVariableByMultiname(builtinfunc,m);
					if (asAtomHandler::is<Function>(builtinfunc))
					{
						asAtom obj = asAtomHandler::fromObject(clip);
						asAtomHandler::as<Function>(builtinfunc)->call(ret,obj,args,numargs);
					}
					else
					{
						clip->getSystemState()->avm1global->getVariableByMultiname(builtinfunc,m);
						if (asAtomHandler::is<Function>(builtinfunc))
							asAtomHandler::as<Function>(builtinfunc)->call(ret,asAtomHandler::undefinedAtom,args,numargs);
						else
							LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction function not found "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
					}
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction done "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				PushStack(stack,ret);
				break;
			}
			case 0x3e: // ActionReturn
			{
				if (result)
					*result = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionReturn ");
				Log::calls_indent--;
				return;
			}
			case 0x3f: // ActionModulo
			{
				// spec is wrong, y is popped of stack first
				asAtom y = PopStack(stack);
				asAtom x = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionModulo "<<asAtomHandler::toDebugString(x)<<" % "<<asAtomHandler::toDebugString(y));
				asAtomHandler::modulo(x,clip->getSystemState(),y);
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				AVM1Function* f =nullptr;
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				if (asAtomHandler::isUndefined(name) || asAtomHandler::toStringId(name,clip->getSystemState()) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				else
					f =clip->AVM1GetFunction(nameID);
				asAtom ret=asAtomHandler::invalidAtom;
				if (f)
				{
					ret = asAtomHandler::fromObject(new_functionObject(f->prototype));
					f->call(nullptr,&ret, args,numargs);
				}
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom cls=asAtomHandler::invalidAtom;
					clip->getSystemState()->avm1global->getVariableByMultiname(cls,m);
					if (asAtomHandler::is<Class_base>(cls))
					{
						asAtomHandler::as<Class_base>(cls)->getInstance(ret,true,args,numargs);
					}
					else
						LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject class not found "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(cls));
				}
				PushStack(stack,ret);
				break;
			}

			case 0x41: // ActionDefineLocal2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineLocal2 "<<asAtomHandler::toDebugString(name));
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				if (locals.find(nameID) == locals.end())
					locals[nameID] = asAtomHandler::undefinedAtom;
				break;
			}
			case 0x42: // ActionInitArray
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionInitArray "<<numargs);
				Array* ret=Class<Array>::getInstanceSNoArgs(clip->getSystemState());
				ret->resize(numargs);
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					ret->set(i,value,false,false);
				}
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x43: // ActionInitObject
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionInitObject "<<numargs);
				ASObject* ret=Class<ASObject>::getInstanceS(clip->getSystemState());
				//Duplicated keys overwrite the previous value
				multiname propertyName(NULL);
				propertyName.name_type=multiname::NAME_STRING;
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					asAtom name = PopStack(stack);
					uint32_t nameid=asAtomHandler::toStringId(name,clip->getSystemState());
					ret->setDynamicVariableNoCheck(nameid,value);
				}
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x44: // ActionTypeOf
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionTypeOf "<<asAtomHandler::toDebugString(obj));
				asAtom res=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(obj))
					res = asAtomHandler::fromString(clip->getSystemState(),"movieclip");
				else
					res = asAtomHandler::typeOf(obj,clip->getSystemState());
				PushStack(stack,res);
				break;
			}
			case 0x47: // ActionAdd2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAdd2 "<<asAtomHandler::toDebugString(arg2)<<" + "<<asAtomHandler::toDebugString(arg1));
				asAtomHandler::add(arg2,arg1,clip->getSystemState());
				PushStack(stack,arg2);
				break;
			}
			case 0x48: // ActionLess2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionLess2 "<<asAtomHandler::toDebugString(arg2)<<" < "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg2,clip->getSystemState(),arg1) == TTRUE));
				break;
			}
			case 0x49: // ActionEquals2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEquals2 "<<asAtomHandler::toDebugString(arg2)<<" == "<<asAtomHandler::toDebugString(arg1));
				if (clip->getSystemState()->mainClip->version <= 5)
				{
					// equals works different on SWF5: Objects without valueOf property are treated as undefined
					if (asAtomHandler::getObject(arg1) && (asAtomHandler::isFunction(arg1) || !asAtomHandler::getObject(arg1)->has_valueOf()))
						asAtomHandler::setUndefined(arg1);
					if (asAtomHandler::getObject(arg2) && (asAtomHandler::isFunction(arg2) || !asAtomHandler::getObject(arg2)->has_valueOf()))
						asAtomHandler::setUndefined(arg2);
				}
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isEqual(arg2,clip->getSystemState(),arg1)));
				break;
			}
			case 0x4a: // ActionToNumber
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),asAtomHandler::toNumber(arg),false));
				break;
			}
			case 0x4b: // ActionToString
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),asAtomHandler::toString(arg,clip->getSystemState())));
				break;
			}
			case 0x4c: // ActionPushDuplicate
			{
				asAtom a = PeekStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPushDuplicate "<<asAtomHandler::toDebugString(a));
				PushStack(stack,a);
				break;
			}
			case 0x4d: // ActionStackSwap
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStackSwap "<<asAtomHandler::toDebugString(arg1)<<" "<<asAtomHandler::toDebugString(arg2));
				PushStack(stack,arg1);
				PushStack(stack,arg2);
				break;
			}
			case 0x4e: // ActionGetMember
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom ret=asAtomHandler::invalidAtom;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				ASObject* o = asAtomHandler::toObject(scriptobject,clip->getSystemState());
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
					o->getVariableByMultiname(ret,m);
					if (asAtomHandler::isInvalid(ret))
					{
						ASObject* pr =o->getprop_prototype();
						while (pr)
						{
							bool isGetter = pr->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::DONT_CALL_GETTER) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
							if (isGetter) // getter from prototype has to be called with o as target
							{
								IFunction* f = asAtomHandler::as<IFunction>(ret);
								ret = asAtom();
								f->callGetter(ret,o);
								break;
							}
							pr = pr->getprop_prototype();
						}
					}
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
									if (o->getClass())
										ret = asAtomHandler::fromObject(o->getClass()->prototype.getPtr()->getObj());
									else
										LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember for scriptobject without class "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
									break;
								}
								case BUILTIN_STRINGS::STRING_AVM1_TARGET:
									if (o->is<DisplayObject>())
										ret = asAtomHandler::fromString(clip->getSystemState(),o->as<DisplayObject>()->AVM1GetPath());
									else
										asAtomHandler::setUndefined(ret);
									break;
								default:
									o->getVariableByMultiname(ret,m);
									break;
							}
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
							break;
					}
				}
				if (asAtomHandler::isInvalid(ret))
					asAtomHandler::setUndefined(ret);
				PushStack(stack,ret);
				break;
			}
			case 0x4f: // ActionSetMember
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				if (asAtomHandler::isObject(scriptobject) || asAtomHandler::isFunction(scriptobject) || asAtomHandler::isArray(scriptobject))
				{
					ASObject* o = asAtomHandler::getObject(scriptobject);
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
							m.name_s_id=asAtomHandler::toStringId(name,clip->getSystemState());
							break;
					}
					m.isAttribute = false;
					ASATOM_INCREF(value);
					ASObject* pr = o->getprop_prototype();
					while (pr)
					{
						variable* var = pr->findVariableByMultiname(m,nullptr);
						if (var && asAtomHandler::is<AVM1Function>(var->setter))
						{
							asAtomHandler::as<AVM1Function>(var->setter)->call(nullptr,&scriptobject,&value,1);
							break;
						}
						pr = pr->getprop_prototype();
					}
					o->setVariableByMultiname(m,value,ASObject::CONST_ALLOWED);
					if (o->is<MovieClip>())
						o->as<MovieClip>()->AVM1UpdateVariableBindings(m.name_s_id,value);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" "<<(int)asAtomHandler::getObjectType(scriptobject)<<" "<<asAtomHandler::toDebugString(name));
				break;
			}
			case 0x50: // ActionIncrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionIncrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),asAtomHandler::AVM1toNumber(num,clip->getSystemState()->mainClip->version)+1,false));
				break;
			}
			case 0x51: // ActionDecrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDecrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(clip->getSystemState(),asAtomHandler::AVM1toNumber(num,clip->getSystemState()->mainClip->version)-1,false));
				break;
			}
			case 0x52: // ActionCallMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				asAtom ret=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(scriptobject))
				{
					AVM1Function* f = asAtomHandler::as<MovieClip>(scriptobject)->AVM1GetFunction(nameID);
					if (f)
					{
						f->call(&ret,&scriptobject,args,numargs);
						LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
						PushStack(stack,ret);
						break;
					}
				}
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=nameID;
				m.isAttribute = false;
				asAtom func=asAtomHandler::invalidAtom;
				if (asAtomHandler::isValid(scriptobject))
				{
					if (asAtomHandler::isNull(scriptobject) || asAtomHandler::isUndefined(scriptobject))
					{
						PushStack(stack,asAtomHandler::undefinedAtom);
						break;
					}
					ASObject* scrobj = asAtomHandler::toObject(scriptobject,clip->getSystemState());
					scrobj->getVariableByMultiname(func,m);
					if (!asAtomHandler::is<IFunction>(func))
					{
						ASObject* pr =scrobj->getprop_prototype();
						while (pr)
						{
							pr->getVariableByMultiname(func,m);
							if (asAtomHandler::isValid(func))
								break;
							pr = pr->getprop_prototype();
						}
						
					}
					if (asAtomHandler::is<Function>(func))
						asAtomHandler::as<Function>(func)->call(ret,scriptobject,args,numargs);
					else if (asAtomHandler::is<AVM1Function>(func))
						asAtomHandler::as<AVM1Function>(func)->call(&ret,&scriptobject,args,numargs);
					else
						LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				PushStack(stack,ret);
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				uint32_t nameID = asAtomHandler::toStringId(name,clip->getSystemState());
				asAtom ret=asAtomHandler::invalidAtom;
				if (nameID == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewMethod without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				ASObject* scrobj = asAtomHandler::toObject(scriptobject,clip->getSystemState());
				if (scrobj->getClass())
					scrobj->getClass()->getInstance(ret,true,nullptr,0);
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=nameID;
				m.isAttribute = false;
				asAtom tmp=asAtomHandler::invalidAtom;
				asAtom func=asAtomHandler::invalidAtom;
				scrobj->getVariableByMultiname(func,m);
				if (!asAtomHandler::is<IFunction>(func))
				{
					ASObject* pr =scrobj->getprop_prototype();
					if (pr)
						pr->getVariableByMultiname(func,m);
				}
				_NR<ASObject> proto;
				if (asAtomHandler::is<Function>(func))
				{
					proto = asAtomHandler::as<Function>(func)->prototype;
					if (!proto.isNull())
						asAtomHandler::toObject(ret,clip->getSystemState())->setprop_prototype(proto);
					asAtomHandler::as<Function>(func)->call(tmp,ret,args,numargs);
				}
				else if (asAtomHandler::is<AVM1Function>(func))
				{
					proto = asAtomHandler::as<AVM1Function>(func)->prototype;
					if (!proto.isNull())
						asAtomHandler::toObject(ret,clip->getSystemState())->setprop_prototype(proto);
					asAtomHandler::as<AVM1Function>(func)->call(nullptr,&ret,args,numargs);
				}
				else if (asAtomHandler::is<Class_base>(func))
				{
					asAtomHandler::as<Class_base>(func)->getInstance(ret,true,args,numargs);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
				PushStack(stack,ret);
				break;
			}
			case 0x55: // ActionEnumerate2
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEnumerate2 "<<asAtomHandler::toDebugString(obj));
				PushStack(stack,asAtomHandler::nullAtom);
				uint32_t index=0;
				ASObject* o = asAtomHandler::toObject(obj,clip->getSystemState());
				while ((index = o->nextNameIndex(index)))
				{
					asAtom name=asAtomHandler::invalidAtom;
					o->nextName(name,index);
					PushStack(stack, name);
				}
				break;
			}
			case 0x60: // ActionBitAnd
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitAnd "<<asAtomHandler::toDebugString(arg1)<<" & "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_and(arg1,clip->getSystemState(),arg2);
				PushStack(stack,arg1);
				break;
			}
			case 0x61: // ActionBitOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitOr "<<asAtomHandler::toDebugString(arg1)<<" | "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_or(arg1,clip->getSystemState(),arg2);
				PushStack(stack,arg1);
				break;
			}
			case 0x62: // ActionBitXOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitXOr "<<asAtomHandler::toDebugString(arg1)<<" ^ "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_xor(arg1,clip->getSystemState(),arg2);
				PushStack(stack,arg1);
				break;
			}
			case 0x63: // ActionBitLShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitLShift "<<asAtomHandler::toDebugString(value)<<" << "<<asAtomHandler::toDebugString(count));
				asAtomHandler::lshift(value,clip->getSystemState(),count);
				PushStack(stack,value);
				break;
			}
			case 0x64: // ActionBitRShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitRShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::rshift(value,clip->getSystemState(),count);
				PushStack(stack,value);
				break;
			}
			case 0x65: // ActionBitURShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionBitURShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::urshift(value,clip->getSystemState(),count);
				PushStack(stack,value);
				break;
			}
			case 0x66: // ActionStrictEquals
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStrictEquals "<<asAtomHandler::toDebugString(arg2)<<" === "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isEqualStrict(arg1,clip->getSystemState(),arg2) == TTRUE));
				break;
			}
			case 0x67: // ActionGreater
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGreater "<<asAtomHandler::toDebugString(arg2)<<" > "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg1,clip->getSystemState(),arg2) == TTRUE));
				break;
			}
			case 0x81: // ActionGotoFrame
			{
				uint32_t frame = it->data_uint16;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoFrame "<<frame);
				clip->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x83: // ActionGetURL
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetURL "<<it->data_string[0]<<" "<<it->data_string[1]);
				clip->getSystemState()->openPageInBrowser(it->data_string[0],it->data_string[1]);
				break;
			}
			case 0x87: // ActionStoreRegister
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStoreRegister "<<(int)it->data_byte);
				asAtom a = PeekStack(stack);
				ASATOM_INCREF(a);
				registers[it->data_byte] = a;
				break;
			}
			case 0x88: // ActionConstantPool
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionConstantPool "<<it->data_uint16);
				context->AVM1SetConstants(clip->getSystemState(),it->data_string);
				break;
			}
			case 0x8a: // ActionWaitForFrame
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionWaitForFrame "<<it->data_uint16<<"/"<<clip->getFramesLoaded()<<" skip "<<(uint32_t)it->data_byte);
				if (clip->getFramesLoaded() <= it->data_uint16 && !clip->hasFinishedLoading())
				{
					// frame not yet loaded, skip actions
					uint32_t skipcount = (uint32_t)it->data_byte;
					while (skipcount && it != actionlist.end())
					{
						it++;
						skipcount--;
					}
				}
				break;
			}
			case 0x08: // ActionToggleQuality
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->state.FP<<" SWF3 DoActionTag ActionToggleQuality "<<hex<<(int)it->actionCode);
				break;
			case 0x8b: // ActionSetTarget
			{
				tiny_string s(it->data_string[0].raw_buf(),true);
				if (!clip)
				{
					LOG_CALL("AVM1: ActionSetTarget: setting target from undefined value to "<<s);
				}
				else
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetTarget "<<s);
				}
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
				tiny_string s(it->data_string[0].raw_buf(),true);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoLabel "<<s);
				clip->AVM1gotoFrameLabel(s);
				break;
			}
			case 0x8e: // ActionDefineFunction2
			{
				std::vector<uint32_t> paramnames;
				for (uint32_t i = 1; i < it->data_string.size(); i++)
				{
					paramnames.push_back(clip->getSystemState()->getUniqueStringId(it->data_string[i].lowercase()));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineFunction2 "<<it->data_string.front()<<" "<<it->data_string.size()-1<<" "<<it->data_flag1<<it->data_flag2<<it->data_flag3<<it->data_flag4<<it->data_flag5<<it->data_flag6<<it->data_flag7<<it->data_flag8<<it->data_flag9);
				AVM1Function* f = Class<IFunction>::getAVM1Function(clip->getSystemState(),clip,context,paramnames,it->data_actionlist,locals,it->data_registernumber,it->data_flag1, it->data_flag2, it->data_flag3, it->data_flag4, it->data_flag5, it->data_flag6, it->data_flag7, it->data_flag8, it->data_flag9);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()));
				if (it->data_string.front() == "")
				{
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
				{
					uint32_t nameID = clip->getSystemState()->getUniqueStringId(it->data_string.front());
					clip->AVM1SetFunction(nameID,_MR(f));
				}
				break;
			}
			case 0x94: // ActionWith
			{
				asAtom obj = PopStack(stack);
				auto itend = it;
				uint16_t skip = it->data_uint16;
				while (skip > 0 && itend != actionlist.end())
				{
					itend++;
					skip -= itend->getFullLength();
				}
				if (curdepth >= maxdepth)
				{
					it = itend;
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionWith depth exceeds maxdepth");
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionWith "<<it->data_uint16);
				++curdepth;
				scopestack[curdepth] = obj;
				scopestackstop[curdepth] = itend;
				break;
			}
			case 0x96: // ActionPush
			{
				auto it2 = it->data_actionlist.begin();
				while (it2 != it->data_actionlist.end())
				{
					switch (it2->data_byte) // type
					{
						case 0:
						{
							asAtom a = asAtomHandler::fromStringID(clip->getSystemState()->getUniqueStringId(it2->data_string[0]));
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 0 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 1:
						{
							asAtom a = asAtomHandler::fromNumber(clip->getSystemState(),it2->data_float,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 1 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 2:
							PushStack(stack,asAtomHandler::nullAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 2 "<<asAtomHandler::toDebugString(asAtomHandler::nullAtom));
							break;
						case 3:
							PushStack(stack,asAtomHandler::undefinedAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 3 "<<asAtomHandler::toDebugString(asAtomHandler::undefinedAtom));
							break;
						case 4:
						{
							if (it->data_uint16 > 255)
								throw RunTimeException("AVM1: invalid register number");
							asAtom a = registers[it2->data_uint16];
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 4 register "<<it2->data_uint16<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 5:
						{
							asAtom a = asAtomHandler::fromBool((bool)it2->data_uint16);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 5 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 6:
						{
							asAtom a = asAtomHandler::fromNumber(clip->getSystemState(),it2->data_double,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 6 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 7:
						{
							asAtom a = asAtomHandler::fromInt((int32_t)it2->data_integer);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 7 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 8:
						case 9:
						{
							asAtom a = context->AVM1GetConstant(it2->data_uint16);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 8/9 "<<it2->data_uint16<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->state.FP<<" SWF4 DoActionTag push type "<<(int)it2->data_byte);
							break;
					}
					it2++;
				}
				break;
			}
			case 0x99: // ActionJump
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionJump "<<it->data_int16);
				int skip = it->data_int16;
				if (skip < 0)
				{
					while (skip < 0 && it != actionlist.begin())
					{
						skip += it->getFullLength();
						it--;
					}
				}
				else
				{
					while (skip > 0 && it != actionlist.end())
					{
						it++;
						skip -= it->getFullLength();
					}
				}
				break;
			}
			case 0x9a: // ActionGetURL2
			{
				asAtom at=PopStack(stack);
				asAtom au=PopStack(stack);
				tiny_string target = asAtomHandler::toString(at,clip->getSystemState());
				tiny_string url = asAtomHandler::toString(au,clip->getSystemState());
				if (it->data_flag1) //LoadTargetFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadTargetFlag");
				if (it->data_flag2) //LoadVariablesFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadVariablesFlag");
				if (it->data_byte) //SendVarsMethod
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with SendVarsMethod");
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetURL2 "<<url<<" "<<target);
				clip->getSystemState()->openPageInBrowser(url,target);
				break;
			}
			case 0x9b: // ActionDefineFunction
			{
				std::vector<uint32_t> paramnames;
				for (uint32_t i = 1; i < it->data_string.size(); i++)
				{
					paramnames.push_back(clip->getSystemState()->getUniqueStringId(it->data_string[i].lowercase()));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineFunction "<<it->data_string.front()<<" "<<it->data_string.size()-1);
				AVM1Function* f = Class<IFunction>::getAVM1Function(clip->getSystemState(),clip,context,paramnames,it->data_actionlist,locals);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()));
				if (it->data_string.front() == "")
				{
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
				{
					uint32_t nameID = clip->getSystemState()->getUniqueStringId(it->data_string.front());
					clip->AVM1SetFunction(nameID,_MR(f));
				}
				break;
			}
			case 0x9d: // ActionIf
			{
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionIf "<<asAtomHandler::toDebugString(a)<<" "<<it->data_int16);
				if (asAtomHandler::toInt(a))
				{
					int skip = it->data_int16;
					if (skip < 0)
					{
						while (skip < 0 && it != actionlist.begin())
						{
							skip += it->getFullLength();
							it--;
						}
					}
					else
					{
						while (skip > 0 && it != actionlist.end())
						{
							it++;
							skip -= it->getFullLength();
						}
					}
				}
				break;
			}
			case 0x9e: // ActionCall
			{
				asAtom a = PopStack(stack);
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,clip->getSystemState());
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCall label "<<s);
					clip->AVM1ExecuteFrameActionsFromLabel(s);
				}
				else
				{
					uint32_t frame = asAtomHandler::toUInt(a);
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCall frame "<<frame);
					clip->AVM1ExecuteFrameActions(frame);
				}
				break;
			}
			case 0x9f: // ActionGotoFrame2
			{
				asAtom a = PopStack(stack);
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,clip->getSystemState());
					if (it->data_uint16)
						LOG(LOG_NOT_IMPLEMENTED,"AVM1: GotFrame2 with bias and label:"<<asAtomHandler::toDebugString(a));
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoFrame2 label "<<s);
					clip->AVM1gotoFrameLabel(s);
					
				}
				else
				{
					uint32_t frame = asAtomHandler::toUInt(a)+it->data_uint16;
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoFrame2 "<<frame);
					clip->AVM1gotoFrame(frame,!it->data_flag2,true);
				}
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->state.FP<<" SWF4+ DoActionTag "<<hex<<(int)it->actionCode);
				break;
		}
	}
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" executeActions done");
	Log::calls_indent--;
}
