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
	stack.push(a);
}

asAtom ACTIONRECORD::PopStack(std::stack<asAtom>& stack)
{
	if (stack.empty())
		throw RunTimeException("AVM1: empty stack");
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

void ACTIONRECORD::executeActions(MovieClip *clip,Frame* frame, std::vector<ACTIONRECORD> &actionlist, asAtom* result, asAtom* obj, asAtom *args, uint32_t num_args, const std::vector<uint32_t>& paramnames)
{
	Log::calls_indent++;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" executeActions");
	if (result)
		result->setUndefined();
	std::stack<asAtom> stack;
	asAtom registers[256];
	std::map<uint32_t,asAtom> locals;
	int curdepth = 0;
	int maxdepth= clip->getSystemState()->mainClip->version < 6 ? 8 : 16;
	asAtom* scopestack = g_newa(asAtom, maxdepth);
	scopestack[0] = obj ? *obj : asAtom::fromObject(clip);
	std::vector<ACTIONRECORD>::iterator* scopestackstop = g_newa(std::vector<ACTIONRECORD>::iterator, maxdepth);
	scopestackstop[0] = actionlist.end();
	if (paramnames.size() != num_args)
		LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" num_args and paramnames do not match "<<num_args<<" "<<paramnames.size());
	else
	{
		for (uint32_t i = 0; i < num_args; i++)
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" parameter "<<i<<" "<<clip->getSystemState()->getStringFromUniqueId(paramnames[i])<<" "<<args[i].toDebugString());
			locals[paramnames[i]] = args[i];
		}
	}
	MovieClip *originalclip = clip;
	for (auto it = actionlist.begin(); it != actionlist.end();it++)
	{
		if (curdepth > 0 && it == scopestackstop[curdepth])
		{
			curdepth--;
		}
		
		switch (it->actionCode)
		{
			case 0x04: // ActionNextFrame
			{
				uint32_t frame = clip->state.FP+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNextFrame "<<frame);
				clip->AVM1gotoFrame(frame,false);
				break;
			}
			case 0x05: // ActionPreviousFrame
			{
				uint32_t frame = clip->state.FP > 0 ? clip->state.FP-1 : 0;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPreviousFrame "<<frame);
				clip->AVM1gotoFrame(frame,false);
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
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAdd "<<b<<"+"<<a);
				PushStack(stack,asAtom(a+b));
				break;
			}
			case 0x0b: // ActionSubtract
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSubtract "<<b<<"-"<<a);
				PushStack(stack,asAtom(b-a));
				break;
			}
			case 0x0c: // ActionMultiply
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionMultiply "<<b<<"*"<<a);
				PushStack(stack,asAtom(b*a));
				break;
			}
			case 0x0d: // ActionDivide
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDivide "<<b<<"/"<<a);
				asAtom ret;
				if (a == 0)
				{
					if (clip->getSystemState()->mainClip->version == 4)
						ret = asAtom::fromString(clip->getSystemState(),"#ERROR#");
					else
						ret = asAtom(Number::NaN);
				}
				else
					ret = asAtom(b/a);
				PushStack(stack,ret);
				break;
			}
			case 0x0e: // ActionEquals
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEquals "<<b<<"=="<<a);
				PushStack(stack,asAtom(b==a));
				break;
			}
			case 0x0f: // ActionLess
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionLess "<<b<<"<"<<a);
				PushStack(stack,asAtom(b<a));
				break;
			}
			case 0x10: // ActionAnd
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAnd "<<b<<" && "<<a);
				PushStack(stack,asAtom(b && a));
				break;
			}
			case 0x11: // ActionOr
			{
				number_t a = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				number_t b = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionOr "<<b<<" || "<<a);
				PushStack(stack,asAtom(b || a));
				break;
			}
			case 0x12: // ActionNot
			{
				number_t value = PopStack(stack).AVM1toNumber(clip->getSystemState()->mainClip->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNot "<<value);
				PushStack(stack,asAtom(value == 0));
				break;
			}
			case 0x13: // ActionStringEquals
			{
				tiny_string a = PopStack(stack).toString(clip->getSystemState());
				tiny_string b = PopStack(stack).toString(clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStringEquals "<<a<<" "<<b);
				PushStack(stack,asAtom(a == b));
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionToInteger "<<a.toDebugString());
				PushStack(stack,asAtom(a.toInt()));
				break;
			}
			case 0x1c: // ActionGetVariable
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetVariable "<<name.toDebugString());
				tiny_string s = name.toString(clip->getSystemState());
				asAtom res;
				if (s=="this")
				{
					if(scopestack[curdepth].type != T_INVALID)
						res = scopestack[curdepth];
					else
						res = asAtom::fromObject(clip);
				}
				else
				{
					// variable names are case insensitive
					auto it = locals.find(clip->getSystemState()->getUniqueStringId(s.lowercase()));
					if (it != locals.end()) // local variable
						res = it->second;
				}
				if (res.type == T_INVALID)
				{
					if (!s.startsWith("/") && !s.startsWith(":"))
						res = originalclip->AVM1GetVariable(s);
					else
						res = clip->AVM1GetVariable(s);
				}
				if (res.type == T_INVALID)
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetVariable variable not found:"<<name.toDebugString());
				PushStack(stack,res);
				break;
			}
			case 0x1d: // ActionSetVariable
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetVariable "<<name.toDebugString()<<" "<<value.toDebugString());
				tiny_string s = name.toString(clip->getSystemState());
				// variable names are case insensitive
				auto it = locals.find(clip->getSystemState()->getUniqueStringId(s.lowercase()));
				if (it != locals.end()) // local variable
					it->second = value;
				else if (!s.startsWith("/") && !s.startsWith(":"))
					originalclip->AVM1SetVariable(s,value);
				else
					clip->AVM1SetVariable(s,value);
				break;
			}
			case 0x20: // ActionSetTarget2
			{
				tiny_string s = PopStack(stack).toString(clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetTarget2 "<<s);
				if (s.empty())
					clip = originalclip;
				else
				{
					clip = clip->AVM1GetClipFromPath(s);
					if (!clip)
					{
						LOG(LOG_ERROR,"AVM1: ActionSetTarget2 clip not found:"<<s);
						clip = originalclip;
					}
				}
				break;
			}
			case 0x21: // ActionStringAdd
			{
				tiny_string a = PopStack(stack).toString(clip->getSystemState());
				tiny_string b = PopStack(stack).toString(clip->getSystemState());
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStringAdd "<<b<<"+"<<a);
				asAtom ret = asAtom::fromString(clip->getSystemState(),b+a);
				PushStack(stack,ret);
				break;
			}
			case 0x22: // ActionGetProperty
			{
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetProperty "<<target.toDebugString()<<" "<<index.toDebugString());
				DisplayObject* o = clip;
				if (target.toStringId(clip->getSystemState()) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = target.toString(clip->getSystemState());
					o = clip->AVM1GetClipFromPath(s);
				}
				asAtom ret = asAtom::undefinedAtom;
				if (o)
				{
					asAtom obj = asAtom::fromObject(o);
					switch (index.toInt())
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
						case 13:// name
							DisplayObject::_getter_name(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 20:// xmouse
							DisplayObject::_getMouseX(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						case 21:// ymouse
							DisplayObject::_getMouseY(ret,clip->getSystemState(),obj,nullptr,0);
							break;
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1: GetProperty type:"<<index.toInt());
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetProperty "<<target.toDebugString()<<" "<<index.toDebugString()<<" "<<value.toDebugString());
				DisplayObject* o = clip;
				if (target.toStringId(clip->getSystemState()) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = target.toString(clip->getSystemState());
					o = clip->AVM1GetClipFromPath(s);
				}
				if (o)
				{
					asAtom obj = asAtom::fromObject(o);
					asAtom ret;
					asAtom valueInt = asAtom(value.toInt());
					switch (index.toInt())
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
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1: SetProperty type:"<<index.toInt());
							break;
					}
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetProperty target clip not found:"<<target.toDebugString()<<" "<<index.toDebugString()<<" "<<value.toDebugString());
				break;
			}
			case 0x24: // ActionCloneSprite
			{
				uint32_t depth = PopStack(stack).toUInt();
				asAtom target = PopStack(stack);
				asAtom source = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCloneSprite "<<target.toDebugString()<<" "<<source.toDebugString()<<" "<<depth);
				if (source.is<MovieClip>())
				{
					DefineSpriteTag* tag = (DefineSpriteTag*)clip->getSystemState()->mainClip->dictionaryLookup(source.as<MovieClip>()->getTagID());
					
					AVM1MovieClip* ret=Class<AVM1MovieClip>::getInstanceS(clip->getSystemState(),*tag,source.as<MovieClip>()->getTagID());
					clip->_addChildAt(_MR(ret),depth);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCloneSprite source is no MovieClip:"<<target.toDebugString()<<" "<<source.toDebugString()<<" "<<depth);
				break;
			}
			case 0x25: // ActionRemoveSprite
			{
				asAtom target = PopStack(stack);
				DisplayObject* o = nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionRemoveSprite "<<target.toDebugString());
				if (target.is<DisplayObject>())
				{
					o = target.as<DisplayObject>();
				}
				else
				{
					uint32_t targetID = target.toStringId(clip->getSystemState());
					if (targetID != BUILTIN_STRINGS::EMPTY)
					{
						tiny_string s = target.toString(clip->getSystemState());
						o = clip->AVM1GetClipFromPath(s);
					}
					else
						o = clip;
				}
				if (o && o->getParent())
				{
					o->getParent()->_removeChild(o);
				}
				break;
			}
			case 0x27: // ActionStartDrag
			{
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStartDrag "<<target.toDebugString());
				asAtom lockcenter = PopStack(stack);
				asAtom constrain = PopStack(stack);
				Rectangle* rect = nullptr;
				if (constrain.toInt())
				{
					asAtom y2 = PopStack(stack);
					asAtom x2 = PopStack(stack);
					asAtom y1 = PopStack(stack);
					asAtom x1 = PopStack(stack);
					rect = Class<Rectangle>::getInstanceS(clip->getSystemState());
					asAtom ret;
					asAtom obj = asAtom::fromObject(rect);
					Rectangle::_setTop(ret,clip->getSystemState(),obj,&y1,1);
					Rectangle::_setLeft(ret,clip->getSystemState(),obj,&x1,1);
					Rectangle::_setBottom(ret,clip->getSystemState(),obj,&y2,1);
					Rectangle::_setRight(ret,clip->getSystemState(),obj,&x2,1);
				}
				asAtom obj;
				if (target.is<MovieClip>())
				{
					obj = target;
				}
				else
				{
					tiny_string s = target.toString(clip->getSystemState());
					MovieClip* targetclip = clip->AVM1GetClipFromPath(s);
					if (targetclip)
					{
						obj = asAtom::fromObject(targetclip);
					}
					else
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionStartDrag target clip not found:"<<target.toDebugString());
						break;
					}
				}
				asAtom ret;
				asAtom args[2];
				args[0] = lockcenter;
				if (rect)
					args[1] = asAtom::fromObject(rect);
				Sprite::_startDrag(ret,clip->getSystemState(),obj,args,rect ? 2 : 1);
				break;
			}
			case 0x28: // ActionEndDrag
			{
				asAtom ret;
				asAtom obj = asAtom::fromObject(clip);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEndDrag "<<obj.toDebugString());
				Sprite::_stopDrag(ret,clip->getSystemState(),obj,nullptr,0);
				break;
			}
			case 0x30: // ActionRandomNumber
			{
				int maximum = PopStack(stack).toInt();
				int ret = (((number_t)rand())/(number_t)RAND_MAX)*maximum;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionRandomNumber "<<ret);
				PushStack(stack,asAtom(ret));
				break;
			}
			case 0x34: // ActionGetTime
			{
				gint64 runtime = compat_msectiming()-clip->getSystemState()->startTime;
				asAtom ret((number_t)runtime);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetTime "<<ret.toDebugString());
				PushStack(stack,asAtom(ret));
				break;
			}
			case 0x3a: // ActionDelete
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
				asAtom ret = asAtom::falseAtom;
				if (scriptobject.type == T_OBJECT)
				{
					ASObject* o = scriptobject.getObject();
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=name.toStringId(clip->getSystemState());
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m))
						ret = asAtom::trueAtom;
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete for scriptobject type "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3b: // ActionDelete2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDelete2 "<<name.toDebugString());
				DisplayObject* o = clip;
				asAtom ret = asAtom::falseAtom;
				while (o)
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=name.toStringId(clip->getSystemState());
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m))
					{
						ret = asAtom::trueAtom;
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineLocal "<<name.toDebugString()<<" " <<value.toDebugString());
				uint32_t nameID = clip->getSystemState()->getUniqueStringId(name.toString(clip->getSystemState()).lowercase());
				locals[nameID] = value;
				break;
			}
			case 0x3d: // ActionCallFunction
			{
				asAtom name = PopStack(stack);
				uint32_t numargs = PopStack(stack).toUInt();
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction "<<name.toDebugString()<<" "<<numargs);
				uint32_t nameID = name.toStringId(clip->getSystemState());
				AVM1Function* f =nullptr;
				if (name.type == T_UNDEFINED || name.toStringId(clip->getSystemState()) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction without name "<<name.toDebugString()<<" "<<numargs);
				else
					f =clip->AVM1GetFunction(nameID);
				asAtom ret;
				if (f)
					f->call(&ret,nullptr, args,numargs);
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom builtinfunc;
					clip->getVariableByMultiname(builtinfunc,m);
					if (builtinfunc.is<Function>())
					{
						asAtom obj = asAtom::fromObject(clip);
						builtinfunc.as<Function>()->call(ret,obj,args,numargs);
					}
					else
					{
						clip->getSystemState()->avm1global->getVariableByMultiname(builtinfunc,m);
						if (builtinfunc.is<Function>())
							builtinfunc.as<Function>()->call(ret,asAtom::undefinedAtom,args,numargs);
						else
							LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallFunction function not found "<<name.toDebugString()<<" "<<numargs);
					}
				}
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionModulo "<<x.toDebugString()<<" % "<<y.toDebugString());
				x.modulo(y);
				PushStack(stack,x);
				break;
			}
			case 0x40: // ActionNewObject
			{
				asAtom name = PopStack(stack);
				uint32_t numargs = PopStack(stack).toUInt();
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject "<<name.toDebugString()<<" "<<numargs);
				AVM1Function* f =nullptr;
				uint32_t nameID = name.toStringId(clip->getSystemState());
				if (name.type == T_UNDEFINED || name.toStringId(clip->getSystemState()) == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject without name "<<name.toDebugString()<<" "<<numargs);
				else
					f =clip->AVM1GetFunction(nameID);
				asAtom ret;
				if (f)
				{
					ret = asAtom::fromObject(new_functionObject(f->prototype));
					f->call(nullptr,&ret, args,numargs);
				}
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom cls;
					clip->getSystemState()->avm1global->getVariableByMultiname(cls,m);
					if (cls.is<Class_base>())
					{
						cls.as<Class_base>()->getInstance(ret,true,args,numargs);
					}
					else
						LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionNewObject class not found "<<name.toDebugString()<<" "<<numargs<<" "<<cls.toDebugString());
				}
				PushStack(stack,ret);
				break;
			}

			case 0x41: // ActionDefineLocal2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDefineLocal2 "<<name.toDebugString());
				locals[name.toStringId(clip->getSystemState())] = asAtom::undefinedAtom;
				break;
			}
			case 0x42: // ActionInitArray
			{
				uint32_t numargs = PopStack(stack).toUInt();
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionInitArray "<<numargs);
				Array* ret=Class<Array>::getInstanceSNoArgs(clip->getSystemState());
				ret->resize(numargs);
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					ret->set(i,value,false,false);
				}
				PushStack(stack,asAtom::fromObject(ret));
				break;
			}
			case 0x43: // ActionInitObject
			{
				uint32_t numargs = PopStack(stack).toUInt();
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionInitObject "<<numargs);
				ASObject* ret=Class<ASObject>::getInstanceS(clip->getSystemState());
				//Duplicated keys overwrite the previous value
				multiname propertyName(NULL);
				propertyName.name_type=multiname::NAME_STRING;
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					asAtom name = PopStack(stack);
					uint32_t nameid=name.toStringId(clip->getSystemState());
					ret->setDynamicVariableNoCheck(nameid,value);
				}
				PushStack(stack,asAtom::fromObject(ret));
				break;
			}
			case 0x47: // ActionAdd2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionAdd2 "<<arg2.toDebugString()<<" + "<<arg1.toDebugString());
				arg2.add(arg1,clip->getSystemState(),false);	
				PushStack(stack,arg2);
				break;
			}
			case 0x48: // ActionLess2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionLess2 "<<arg2.toDebugString()<<" < "<<arg1.toDebugString());
				PushStack(stack,asAtom(arg2.isLess(clip->getSystemState(),arg1) == TTRUE));
				break;
			}
			case 0x49: // ActionEquals2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionEquals2 "<<arg2.toDebugString()<<" == "<<arg1.toDebugString());
				if (clip->getSystemState()->mainClip->version <= 5)
				{
					// equals works different on SWF5: Objects without valueOf property are treated as undefined
					if (arg1.getObject() && (arg1.type == T_FUNCTION || !arg1.getObject()->has_valueOf()))
						arg1.setUndefined();
					if (arg2.getObject() && (arg2.type == T_FUNCTION || !arg2.getObject()->has_valueOf()))
						arg2.setUndefined();
				}
				PushStack(stack,asAtom(arg2.isEqual(clip->getSystemState(),arg1)));
				break;
			}
			case 0x4a: // ActionToNumber
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtom(arg.toNumber()));
				break;
			}
			case 0x50: // ActionIncrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionIncrement "<<num.toDebugString());
				PushStack(stack,asAtom(num.AVM1toNumber(clip->getSystemState()->mainClip->version)+1));
				break;
			}
			case 0x51: // ActionDecrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionDecrement "<<num.toDebugString());
				PushStack(stack,asAtom(num.AVM1toNumber(clip->getSystemState()->mainClip->version)-1));
				break;
			}
			case 0x4c: // ActionPushDuplicate
			{
				asAtom a = PeekStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPushDuplicate "<<a.toDebugString());
				PushStack(stack,a);
				break;
			}
			case 0x4e: // ActionGetMember
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom ret;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
				uint32_t nameID = name.toStringId(clip->getSystemState());
				ASObject* o = scriptobject.getObject();
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=nameID;
				m.isAttribute = false;
				switch (scriptobject.type)
				{
					case T_FUNCTION:
						if (nameID == BUILTIN_STRINGS::PROTOTYPE)
							ret = asAtom::fromObject(o->as<IFunction>()->prototype.getPtr());
						else
							o->getVariableByMultiname(ret,m);
						break;
					case T_OBJECT:
					case T_ARRAY:
					case T_CLASS:
						switch (nameID)
						{
							case BUILTIN_STRINGS::PROTOTYPE:
							{
								if (o->getClass())
									ret = asAtom::fromObject(o->getClass()->prototype.getPtr()->getObj());
								else
									LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember for scriptobject without class "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
								break;
							}
							case BUILTIN_STRINGS::STRING_AVM1_TARGET:
								if (o->is<DisplayObject>())
									ret = asAtom::fromString(clip->getSystemState(),o->as<DisplayObject>()->AVM1GetPath());
								else
									ret.setUndefined();
								break;
							default:
								o->getVariableByMultiname(ret,m);
								break;
						}
						break;
					default:
						LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGetMember for scriptobject type "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
						break;
				}
				if (ret.type == T_INVALID)
					ret.setUndefined();
				PushStack(stack,ret);
				break;
			}
			case 0x4f: // ActionSetMember
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetMember "<<scriptobject.toDebugString()<<" " <<name.toDebugString()<<" "<<value.toDebugString());
				if (scriptobject.type == T_OBJECT || scriptobject.type == T_FUNCTION || scriptobject.type == T_ARRAY)
				{
					ASObject* o = scriptobject.getObject();
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=name.toStringId(clip->getSystemState());
					m.isAttribute = false;
					o->setVariableByMultiname(m,value,ASObject::CONST_ALLOWED);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetMember for scriptobject type "<<scriptobject.toDebugString()<<" " <<name.toDebugString());
				break;
			}
			case 0x52: // ActionCallMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				uint32_t numargs = PopStack(stack).toUInt();
				asAtom* args = g_newa(asAtom, numargs);
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod "<<name.toDebugString()<<" "<<numargs<<" "<<scriptobject.toDebugString());
				uint32_t nameID = name.toStringId(clip->getSystemState());
				asAtom ret;
				if (scriptobject.is<MovieClip>())
				{
					AVM1Function* f = scriptobject.as<MovieClip>()->AVM1GetFunction(nameID);
					if (f)
					{
						f->call(&ret,&scriptobject,args,numargs);
						PushStack(stack,ret);
						break;
					}
				}
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=nameID;
				m.isAttribute = false;
				asAtom func;
				scriptobject.toObject(clip->getSystemState())->getVariableByMultiname(func,m);
				if (func.is<Function>())
					func.as<Function>()->call(ret,scriptobject,args,numargs);
				else if (func.is<AVM1Function>())
					func.as<AVM1Function>()->call(&ret,&scriptobject,args,numargs);
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCallMethod function not found "<<scriptobject.toDebugString()<<" "<<name.toDebugString()<<" "<<func.toDebugString());
				PushStack(stack,ret);
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
				registers[it->data_byte] = PeekStack(stack);
				break;
			}
			case 0x88: // ActionConstantPool
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionConstantPool "<<it->data_uint16);
				frame->AVM1SetConstants(clip->getSystemState(),it->data_string);
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
			case 0x8c: // ActionGotoLabel
			{
				tiny_string s(it->data_string[0].raw_buf(),true);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoLabel "<<s);
				clip->AVM1gotoFrameLabel(s);
				break;
			}
			case 0x08: // ActionToggleQuality
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->state.FP<<" SWF3 DoActionTag ActionToggleQuality "<<hex<<(int)it->actionCode);
				break;
			case 0x8b: // ActionSetTarget
			{
				tiny_string s(it->data_string[0].raw_buf(),true);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionSetTarget "<<s);
				if (s.empty())
					clip = originalclip;
				else
				{
					clip = clip->AVM1GetClipFromPath(s);
					if (!clip)
					{
						LOG(LOG_ERROR,"AVM1: ActionSetTarget clip not found:"<<s);
						clip = originalclip;
					}
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
							asAtom a = asAtom::fromStringID(clip->getSystemState()->getUniqueStringId(it2->data_string[0]));
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 0 "<<a.toDebugString());
							break;
						}
						case 1:
						{
							asAtom a(it2->data_float);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 1 "<<a.toDebugString());
							break;
						}
						case 2:
							PushStack(stack,asAtom::nullAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 2 "<<asAtom::nullAtom.toDebugString());
							break;
						case 3:
							PushStack(stack,asAtom::undefinedAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 3 "<<asAtom::undefinedAtom.toDebugString());
							break;
						case 4:
						{
							if (it->data_uint16 > 255)
								throw RunTimeException("AVM1: invalid register number");
							asAtom a = registers[it2->data_uint16];
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 4 "<<a.toDebugString());
							break;
						}
						case 5:
						{
							asAtom a((bool)it2->data_uint16);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 5 "<<a.toDebugString());
							break;
						}
						case 6:
						{
							asAtom a(it2->data_double);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 6 "<<a.toDebugString());
							break;
						}
						case 7:
						{
							asAtom a((int32_t)it2->data_integer);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 7 "<<a.toDebugString());
							break;
						}
						case 8:
						case 9:
						{
							asAtom a = frame->AVM1GetConstant(it2->data_uint16);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionPush 8/9 "<<it2->data_uint16<<" "<<a.toDebugString());
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
				tiny_string target = PopStack(stack).toString(clip->getSystemState());
				tiny_string url = PopStack(stack).toString(clip->getSystemState());
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
				AVM1Function* f = Class<IFunction>::getAVM1Function(clip->getSystemState(),clip,frame,paramnames,it->data_actionlist);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()));
				if (it->data_string.front() == "")
				{
					asAtom a = asAtom::fromObject(f);
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionIf "<<a.toDebugString()<<" "<<it->data_int16);
				if (a.toInt())
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
				if (a.type == T_STRING)
				{
					tiny_string s = a.toString(clip->getSystemState());
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCall label "<<s);
					clip->AVM1ExecuteFrameActionsFromLabel(s);
				}
				else
				{
					uint32_t frame = a.toUInt();
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionCall frame "<<frame);
					clip->AVM1ExecuteFrameActions(frame);
				}
				break;
			}
			case 0x9f: // ActionGotoFrame2
			{
				asAtom a = PopStack(stack);
				if (a.type == T_STRING)
				{
					tiny_string s = a.toString(clip->getSystemState());
					if (it->data_uint16)
						LOG(LOG_NOT_IMPLEMENTED,"AVM1: GotFrame2 with bias and label:"<<a.toDebugString());
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" ActionGotoFrame2 label "<<s);
					clip->AVM1gotoFrameLabel(s);
					
				}
				else
				{
					uint32_t frame = a.toUInt()+it->data_uint16;
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
