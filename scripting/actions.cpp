/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "actions.h"
#include "logger.h"
#include "swf.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

void lightspark::ignore(istream& i, int count);

ExportAssetsTag::ExportAssetsTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NO_INFO,"ExportAssetsTag Tag");
	in >> Count;
	Tags.resize(Count);
	Names.resize(Count);
	for(int i=0;i<Count;i++)
	{
		in >> Tags[i] >> Names[i];
		DictionaryTag* d=pt->root->dictionaryLookup(Tags[i]);
		if(d==NULL)
			throw ParseException("ExportAssetsTag: id not defined in Dictionary");
		//TODO:new interface based model
		//pt->root->setVariableByString(Names[i],d->instance());
	}
}

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	LOG(LOG_CALLS,"DoActionTag");
	int dest=in.tellg();
	dest+=h.getLength();

	while(1)
	{
		ACTIONRECORDHEADER ah(in);
		if(ah.ActionCode==0)
			break;
		else
			actions.push_back(ah.createTag(in));
		if(actions.back()==NULL)
		{
			actions.pop_back();
			LOG(LOG_ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
	}
}

void DoActionTag::execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& ls)
{
	Depth=0x20000;
	ls.push_back(make_pair(PlaceInfo(),this));
}

void DoActionTag::Render()
{
	LOG(LOG_NOT_IMPLEMENTED,"AVM1 not supported");
/*
	ts = get_current_time_ms();
	ExecutionContext* exec_bak=rt->execContext;
	rt->execContext=this;
	for(unsigned int i=0;i<actions.size();i++)
	{
		actions[i]->Execute();
		if(jumpOffset<0)
		{
			int off=-jumpOffset;
			while(off>0)
			{
				off-=actions[i]->Length;
				i--;
			}
			if(off<0)
				LOG(LOG_ERROR,"Invalid jump offset");
		}
		else if(jumpOffset>0)
		{
			while(jumpOffset>0)
			{
				i++;
				jumpOffset-=actions[i]->Length;
			}
			if(jumpOffset<0)
				LOG(LOG_ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	td = get_current_time_ms();
	sys->fps_prof->action_time=td-ts;
*/
}

DoInitActionTag::DoInitActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	LOG(LOG_CALLS,"DoInitActionTag");
	int dest=in.tellg();
	dest+=h.getLength();
	in >> SpriteID;
	while(1)
	{
		ACTIONRECORDHEADER ah(in);
		if(ah.ActionCode==0)
			break;
		else
			actions.push_back(ah.createTag(in));
		if(actions.back()==NULL)
		{
			actions.pop_back();
			LOG(LOG_ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
	}
}

void DoInitActionTag::execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& ls)
{
	Depth=0x10000;
	ls.push_back(make_pair(PlaceInfo(),this));
}

void DoInitActionTag::Render()
{
	LOG(LOG_NOT_IMPLEMENTED,"AVM1 not supported");
/*
	ts = get_current_time_ms();
	ExecutionContext* exec_bak=rt->execContext;
	rt->execContext=this;
	for(unsigned int i=0;i<actions.size();i++)
	{
		actions[i]->Execute();
		if(jumpOffset<0)
		{
			int off=-jumpOffset;
			while(off>0)
			{
				off-=actions[i]->Length;
				i--;
			}
			if(off<0)
				LOG(LOG_ERROR,"Invalid jump offset");
		}
		else if(jumpOffset>0)
		{
			while(jumpOffset>0)
			{
				i++;
				jumpOffset-=actions[i]->Length;
			}
			if(jumpOffset<0)
				LOG(LOG_ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	td=get_current_time_ms();
	sys->fps_prof->action_time=td-ts;
*/
}

ACTIONRECORDHEADER::ACTIONRECORDHEADER(std::istream& in)
{
	in >> ActionCode;
	if(ActionCode>=0x80)
		in >> Length;
	else
		Length=0;
}

ActionTag* ACTIONRECORDHEADER::createTag(std::istream& in)
{
	ActionTag* t=NULL;
	switch(ActionCode)
	{
		case 0x06:
			t=new ActionPlay;
			break;
		case 0x07:
			t=new ActionStop;
			break;
		case 0x0b:
			t=new ActionSubtract;
			break;
		case 0x0c:
			t=new ActionMultiply;
			break;
		case 0x0d:
			t=new ActionDivide;
			break;
		case 0x12:
			t=new ActionNot;
			break;
		case 0x13:
			t=new ActionStringEquals;
			break;
		case 0x15:
			t=new ActionStringExtract;
			break;
		case 0x17:
			t=new ActionPop;
			break;
		case 0x18:
			t=new ActionToInteger;
			break;
		case 0x1c:
			t=new ActionGetVariable;
			break;
		case 0x1d:
			t=new ActionSetVariable;
			break;
		case 0x20:
			t=new ActionSetTarget2;
			break;
		case 0x21:
			t=new ActionStringAdd;
			break;
		case 0x22:
			t=new ActionGetProperty;
			break;
		case 0x23:
			t=new ActionSetProperty;
			break;
		case 0x24:
			t=new ActionCloneSprite;
			break;
		case 0x26:
			t=new ActionTrace;
			break;
		case 0x2b:
			t=new ActionCastOp;
			break;
		case 0x2c:
			t=new ActionImplementsOp;
			break;
		case 0x33:
			t=new ActionAsciiToChar;
			break;
		case 0x34:
			t=new ActionGetTime;
			break;
		case 0x3a:
			t=new ActionDelete;
			break;
		case 0x3c:
			t=new ActionDefineLocal;
			break;
		case 0x3d:
			t=new ActionCallFunction;
			break;
		case 0x3f:
			t=new ActionModulo;
			break;
		case 0x3e:
			t=new ActionReturn;
			break;
		case 0x40:
			t=new ActionNewObject;
			break;
		case 0x42:
			t=new ActionInitArray;
			break;
		case 0x43:
			t=new ActionInitObject;
			break;
		case 0x44:
			t=new ActionTypeOf;
			break;
		case 0x46:
			t=new ActionEnumerate;
			break;
		case 0x47:
			t=new ActionAdd2;
			break;
		case 0x48:
			t=new ActionLess2;
			break;
		case 0x49:
			t=new ActionEquals2;
			break;
		case 0x4a:
			t=new ActionToNumber;
			break;
		case 0x4b:
			t=new ActionToString;
			break;
		case 0x4c:
			t=new ActionPushDuplicate;
			break;
		case 0x4e:
			t=new ActionGetMember;
			break;
		case 0x4f:
			t=new ActionSetMember;
			break;
		case 0x50:
			t=new ActionIncrement;
			break;
		case 0x51:
			t=new ActionDecrement;
			break;
		case 0x52:
			t=new ActionCallMethod;
			break;
		case 0x53:
			t=new ActionNewMethod;
			break;
		case 0x54:
			t=new ActionInstanceOf;
			break;
		case 0x55:
			t=new ActionEnumerate2;
			break;
		case 0x60:
			t=new ActionBitAnd;
			break;
		case 0x61:
			t=new ActionBitOr;
			break;
		case 0x62:
			t=new ActionBitXor;
			break;
		case 0x63:
			t=new ActionBitLShift;
			break;
		case 0x64:
			t=new ActionBitRShift;
			break;
		case 0x66:
			t=new ActionStrictEquals;
			break;
		case 0x67:
			t=new ActionGreater;
			break;
		case 0x68:
			t=new ActionStringGreater;
			break;
		case 0x69:
			t=new ActionExtends;
			break;
		case 0x81:
			t=new ActionGotoFrame(in);
			break;
		case 0x83:
			t=new ActionGetURL(in);
			break;
		case 0x87:
			t=new ActionStoreRegister(in);
			break;
		case 0x88:
			t=new ActionConstantPool(in);
			break;
		case 0x8b:
			t=new ActionSetTarget(in);
			break;
		case 0x8c:
			t=new ActionGoToLabel(in);
			break;
//		case 0x8e:
//			t=new ActionDefineFunction2(in,this);
//			break;
		case 0x94:
			t=new ActionWith(in);
			break;
		case 0x96:
			t=new ActionPush(in,this);
			break;
		case 0x99:
			t=new ActionJump(in);
			break;
		case 0x9a:
			t=new ActionGetURL2(in);
			break;
//		case 0x9b:
//			t=new ActionDefineFunction(in,this);
//			break;
		case 0x9d:
			t=new ActionIf(in);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"Unsupported ActionCode " << (int)ActionCode << " with length " << (int)Length << " bytes");
			ignore(in,Length);
			t=NULL;
			break;
	}
	if(t)
	{
		t->Length+=Length;
		if(ActionCode>=0x80)
			t->Length+=2;
	}
	return t;
}

RunState::RunState():FP(0),next_FP(0),stop_FP(0)
{
}

void ActionStop::Execute()
{
	/*LOG(LOG_CALLS,"ActionStop");
	rt->currentClip->state.next_FP=rt->currentClip->state.FP;
	rt->currentClip->state.stop_FP=true;*/
}

ActionDefineFunction::ActionDefineFunction(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams;
	LOG(LOG_CALLS,"Defining function " << FunctionName);
	params.resize(NumParams);
	for(int i=0;i<NumParams;i++)
	{
		in >> params[i];
	}
	in >> CodeSize;
	streampos dest=in.tellg();
	dest+=CodeSize;
	Length+=CodeSize;
	while(CodeSize)
	{
		ACTIONRECORDHEADER ah(in);
		if(ah.ActionCode==0)
			LOG(LOG_ERROR,"End action in function")
		else
			functionActions.push_back(ah.createTag(in));
		if(functionActions.back()==NULL)
		{
			functionActions.pop_back();
			LOG(LOG_ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
		if(in.tellg()==dest)
			break;
		else if(in.tellg()>dest)
		{
			LOG(LOG_ERROR,"CodeSize not consistent");
			break;
		}
	}
}

/*ASObject* ActionDefineFunction2::call(ASObject* obj, ASObject* const* args, int argslen, int level)
{
	retValue=new Undefined;
	if(retValue->getObjectType()!=T_UNDEFINED)
		LOG(LOG_ERROR,"Not valid condition");
	ExecutionContext* exec_bak=rt->execContext;
	rt->execContext=this;
	LOG(LOG_CALLS,"Calling Function2 " << FunctionName);
	for(int i=0;i<args->size();i++)
		LOG(LOG_CALLS,"Arg "<<i<<"="<<args->at(i)->toString());
	for(int i=0;i<NumParams;i++)
	{
		if(Parameters[i].Register==0)
			LOG(LOG_ERROR,"Parameter not in register")
		else
		{
			rt->execContext->regs[Parameters[i].Register]=args->at(i);
		}
	}
	int used_regs=1;
	if(PreloadThisFlag)
	{
		LOG(LOG_CALLS,"Preload this");
		abort();
		//rt->execContext->regs[used_regs]=rt->currentClip;
		used_regs++;
	}
	if(PreloadArgumentsFlag)
		LOG(LOG_CALLS,"Preload arguments "<<used_regs);
	if(PreloadSuperFlag)
		LOG(LOG_CALLS,"Preload super "<<used_regs);
	if(PreloadRootFlag)
		LOG(LOG_CALLS,"Preload root "<<used_regs);
	if(PreloadParentFlag)
		LOG(LOG_CALLS,"Preload parent "<<used_regs);
	if(PreloadGlobalFlag)
		LOG(LOG_CALLS,"Preload global "<<used_regs);

	for(unsigned int i=0;i<functionActions.size();i++)
	{
		functionActions[i]->Execute();
		if(jumpOffset<0)
		{
			int off=-jumpOffset;
			while(off>0)
			{
				off-=functionActions[i]->Length;
				i--;
			}
			if(off<0)
				LOG(LOG_ERROR,"Invalid jump offset");
			jumpOffset=0;
		}
		else if(jumpOffset>0)
		{
			while(jumpOffset>0)
			{
				i++;
				jumpOffset-=functionActions[i]->Length;
			}
			if(jumpOffset<0)
				LOG(LOG_ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	return retValue;
}*/

ActionDefineFunction2::ActionDefineFunction2(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams >> RegisterCount;
	LOG(LOG_CALLS,"Defining function2 " << FunctionName);
	BitStream bs(in);
	PreloadParentFlag=UB(1,bs);
	PreloadRootFlag=UB(1,bs);
	SuppressSuperFlag=UB(1,bs);
	PreloadSuperFlag=UB(1,bs);
	SuppressArgumentsFlag=UB(1,bs);
	PreloadArgumentsFlag=UB(1,bs);
	SuppressThisFlag=UB(1,bs);
	PreloadThisFlag=UB(1,bs);
	UB(7,bs);
	PreloadGlobalFlag=UB(1,bs);
	Parameters.resize(NumParams);
	for(int i=0;i<NumParams;i++)
	{
		in >> Parameters[i].Register >> Parameters[i].ParamName;
	}
	in >> CodeSize;
	Length+=CodeSize;
	streampos dest=in.tellg();
	dest+=CodeSize;
	while(CodeSize)
	{
		ACTIONRECORDHEADER ah(in);
		if(ah.ActionCode==0)
			LOG(LOG_ERROR,"End action in function")
		else
			functionActions.push_back(ah.createTag(in));
		if(functionActions.back()==NULL)
		{
			functionActions.pop_back();
			LOG(LOG_ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
		functionActions.back()->print();
		if(in.tellg()==dest)
			break;
		else if(in.tellg()>dest)
		{
			LOG(LOG_ERROR,"CodeSize not consistent with file offset " << in.tellg());
			break;
		}
	}
}

void ActionPushDuplicate::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionPushDuplicate");
}

void ActionSetProperty::Execute()
{
	abort();
/*	ASObject* value=rt->vm.stack.pop();
	int index=rt->vm.stack.pop()->toInt();
	tiny_string target=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionSetProperty to: " << target << " index " << index);
	ASObject* owner;
	ASObject* obj=sys->getVariableByQName(target,"",owner);
	if(owner)
	{
		switch(index)
		{
			case 2:
				obj->setVariableByQName("_scalex","",value);
				LOG(LOG_CALLS,"setting to " << value->toNumber());
				break;
			case 5:
				ret=obj->getVariableByName("_totalframes");
				LOG(NO_INFO,"setting to " << ret->toInt());
				break;
			case 12:
				ret=obj->getVariableByName("_framesloaded");
				LOG(NO_INFO,"setting to " << ret->toInt());
				break;
			default:
				LOG(LOG_ERROR,"Not supported property index "<< index);
				break;
		}
	}*/
}

void ActionGetProperty::Execute()
{
	abort();
/*	int index=rt->vm.stack.pop()->toInt();
	tiny_string target=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionGetProperty from: " << target << " index " << index);
	ASObject* owner;
	ASObject* obj=sys->getVariableByQName(target,"",owner);
	ASObject* ret;
	if(owner)
	{
		switch(index)
		{
			case 5:
				ret=obj->getVariableByQName("_totalframes","",owner);
				LOG(LOG_CALLS,"returning " << ret->toInt());
				break;
			case 12:
				ret=obj->getVariableByQName("_framesloaded","",owner);
				LOG(LOG_CALLS,"returning " << ret->toInt());
				break;
			default:
				LOG(LOG_ERROR,"Not supported property index "<< index);
				break;
		}
	}
	else
		ret=new Undefined;
	rt->vm.stack.push(ret);*/
}

void ActionDecrement::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionDecrement");
}

void ActionExtends::Execute()
{
	abort();
/*	LOG(LOG_NOT_IMPLEMENTED,"ActionExtends");
	ASObject* super_cons=rt->vm.stack.pop();
	ASObject* sub_cons=rt->vm.stack.pop();
	ASObject* sub_ob=sub_cons;
	ASObject* super_ob=super_cons;
	if(sub_ob==NULL)
		abort();
	if(super_ob==NULL)
		abort();
	ASObject* prot=new ASObject();
	prot->super=super_ob;
	sub_ob->prototype=prot;*/
}

void ActionTypeOf::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionTypeOf");
}

void ActionEnumerate::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionEnumerate");
}

void ActionGetTime::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionGetTime");
}

void ActionInstanceOf::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionInstanceOf");
}

void ActionImplementsOp::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionImplementsOp");
}

void ActionBitAnd::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionBitAnd");
}

void ActionBitOr::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionBitOr");
}

void ActionBitXor::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionBitXor");
}

void ActionBitLShift::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionBitLShift");
}

void ActionBitRShift::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionBitRShift");
}

void ActionEnumerate2::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionEnumerate2");
}

void ActionToString::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionToString");
}

void ActionToNumber::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionToNumber");
}

void ActionCastOp::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionCastOp");
}

void ActionIncrement::Execute()
{
	abort();
/*	float a=rt->vm.stack.pop()->toNumber();
	LOG(LOG_CALLS,"ActionIncrement: " << a);
	rt->vm.stack.push(new Number(a+1));*/
}

void ActionGreater::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionGreater");
	ASObject* arg1=rt->vm.stack.pop();
	ASObject* arg2=rt->vm.stack.pop();
	if(arg2->isGreater(arg1))
	{
		LOG(LOG_CALLS,"Greater");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(LOG_CALLS,"Not Greater");
		rt->vm.stack.push(new Integer(0));
	}*/
}

void ActionStringGreater::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionStringGreater");
}

void ActionAdd2::Execute()
{
	abort();
/*	ASObject* arg1=rt->vm.stack.pop();
	ASObject* arg2=rt->vm.stack.pop();

	if(arg1->getObjectType()==T_STRING || arg2->getObjectType()==T_STRING)
	{
		string tmp(arg2->toString());
		tmp+=arg1->toString();
		rt->vm.stack.push(new ASString(tmp));
		LOG(LOG_CALLS,"ActionAdd2 (string concatenation): " << rt->vm.stack(0)->toString());
	}
	else
	{
		rt->vm.stack.push(new Number(arg1->toNumber()+arg2->toNumber()));
		LOG(LOG_CALLS,"ActionAdd2 returning: " << arg1->toNumber() + arg2->toNumber());
	}*/
}

void ActionCloneSprite::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionCloneSprite");
}

void ActionTrace::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionTrace");
}

void ActionDefineLocal::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionDefineLocal");
	ASObject* value=rt->vm.stack.pop();
	tiny_string name=rt->vm.stack.pop()->toString();
	rt->currentClip->setVariableByQName(name,"",value);*/
}

void ActionNewObject::Execute()
{
	abort();
/*	tiny_string varName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionNewObject: name " << varName);
	ASObject* owner;
	ASObject* type=sys->getVariableByQName(varName,"",owner);
	if(owner)
	{
		if(type->getObjectType()!=T_UNDEFINED)
			LOG(LOG_ERROR,"ActionNewObject: no such object");
		int numArgs=rt->vm.stack.pop()->toInt();
		if(numArgs)
			LOG(LOG_ERROR,"There are arguments");
		ASObject* c=type->getVariableByQName("constructor","",owner);
		if(c->getObjectType()!=T_FUNCTION)
			LOG(LOG_ERROR,"Constructor is not a function");
		Function* f=static_cast<Function*>(c);
		if(f==NULL)
			LOG(LOG_ERROR,"Not possible error");

		ASObject* obj=new ASObject;
		f->call(obj,NULL);
		rt->vm.stack.push(obj);
	}
	else
		rt->vm.stack.push(new Undefined);*/
}

void ActionReturn::Execute()
{
	abort();
//	LOG(LOG_CALLS,"ActionReturn");
//	rt->execContext->retValue=rt->vm.stack.pop();
}

void ActionPop::Execute()
{
	abort();
//	tiny_string popped=rt->vm.stack.pop()->toString();
//	LOG(LOG_CALLS,"ActionPop: " << popped);
}

void ActionToInteger::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"ActionToInteger");
}

void ActionCallMethod::Execute()
{
	abort();
/*	tiny_string methodName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionCallMethod: " << methodName);
	ASObject* obj=rt->vm.stack.pop();
	int numArgs=rt->vm.stack.pop()->toInt();
	ASObject** args=new[numArgs];
	for(int i=0;i<numArgs;i++)
		args[i]=rt->vm.stack.pop();
	ASObject* owner;
	ASObject* ret=rt->currentClip->getVariableByQName(methodName,"",owner);
	if(owner)
	{
		IFunction* f=ret->toFunction();
		if(f==0)
		{
			LOG(LOG_ERROR,"No such function");
			rt->vm.stack.push(new Undefined);
		}
		else
		{
			ASObject* ret=f->call(NULL,&args);
			rt->vm.stack.push(ret);
		}
	}
	else
		rt->vm.stack.push(new Undefined);*/
}

void ActionCallFunction::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionCallFunction");

	tiny_string funcName=rt->vm.stack.pop()->toString();
	int numArgs=rt->vm.stack.pop()->toInt();
	ASObject** args=new[numArgs];
	for(int i=0;i<numArgs;i++)
		args[i]=rt->vm.stack.pop();
	ASObject* owner;
	ASObject* ret=rt->currentClip->getVariableByQName(funcName,"",owner);
	if(owner)
	{
		IFunction* f=ret->toFunction();
		if(f==0)
		{
			LOG(LOG_ERROR,"No such function");
			rt->vm.stack.push(new Undefined);
		}
		else
		{
			ASObject* ret=f->call(NULL,&args);
			rt->vm.stack.push(ret);
		}
	}
	else
		rt->vm.stack.push(new Undefined);
	LOG(LOG_CALLS,"ActionCallFunction: End");*/
}

void ActionDefineFunction::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionDefineFunction: " << FunctionName);
	if(FunctionName.isNull())
		rt->vm.stack.push(this);
	else
		rt->currentClip->setVariableByQName((const char*)FunctionName,"",this);*/
}

void ActionDefineFunction2::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionDefineFunction2: " << FunctionName);
	if(FunctionName.isNull())
		rt->vm.stack.push(this);
	else
		rt->currentClip->setVariableByQName((const char*)FunctionName,"",this);*/
}

void ActionLess2::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionLess2");
	ASObject* arg1=rt->vm.stack.pop();
	ASObject* arg2=rt->vm.stack.pop();
	if(arg2->isLess(arg1))
	{
		LOG(LOG_CALLS,"Less");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(LOG_CALLS,"Not Less");
		rt->vm.stack.push(new Integer(0));
	}*/
}

void ActionStrictEquals::Execute()
{
	abort();
/*	LOG(LOG_NOT_IMPLEMENTED,"ActionStrictEquals");
	ASObject* arg1=rt->vm.stack.pop();
	ASObject* arg2=rt->vm.stack.pop();
	if(arg1->isEqual(arg2))
	{
		LOG(LOG_CALLS,"Equal");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(LOG_CALLS,"Not Equal");
		rt->vm.stack.push(new Integer(0));
	}*/
}

void ActionEquals2::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionEquals2");
	ASObject* arg1=rt->vm.stack.pop();
	ASObject* arg2=rt->vm.stack.pop();
	if(arg1->isEqual(arg2))
	{
		LOG(LOG_CALLS,"Equal");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(LOG_CALLS,"Not Equal");
		rt->vm.stack.push(new Integer(0));
	}*/
}

void ActionJump::Execute()
{
	LOG(LOG_CALLS,"ActionJump: " << BranchOffset);
	//rt->execContext->setJumpOffset(BranchOffset);
}

void ActionDelete::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionDelete");
}

void ActionNewMethod::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionNewMethod");
}

void ActionAsciiToChar::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionStringAdd");
}

void ActionStringAdd::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionStringAdd");
}

void ActionStringExtract::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionStringExtract");
}

void ActionIf::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionIf");
	int cond=rt->vm.stack.pop()->toInt();
	if(cond)
		rt->execContext->setJumpOffset(Offset);*/
}

void ActionModulo::Execute()
{
	abort();
/*	int a=rt->vm.stack.pop()->toInt();
	int b=rt->vm.stack.pop()->toInt();
	rt->vm.stack.push(new Number(b%a));
	LOG(LOG_CALLS,"ActionDivide: return " << b << "%" << a << "="<< b%a);*/
}

void ActionDivide::Execute()
{
	abort();
/*	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b/a));
	LOG(LOG_CALLS,"ActionDivide: return " << b << "/" << a << "="<< b/a);*/
}

void ActionMultiply::Execute()
{
	abort();
/*	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b*a));
	LOG(LOG_CALLS,"ActionMultiply: return " << b*a);*/
}

void ActionSubtract::Execute()
{
	abort();
/*	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b-a));
	LOG(LOG_CALLS,"ActionSubtract: return " << b-a);*/
}

void ActionNot::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionNot");
	double a=rt->vm.stack.pop()->toNumber();
	if(a==0)
		rt->vm.stack.push(new Integer(1));
	else
		rt->vm.stack.push(new Integer(0));*/
}

void ActionInitArray::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionInitArray");
}

void ActionInitObject::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionInitObject");
}

void ActionStringEquals::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionStringEquals");
}

void ActionSetVariable::Execute()
{
	abort();
/*	ASObject* obj=rt->vm.stack.pop();
	tiny_string varName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionSetVariable: name " << varName);
	rt->currentClip->setVariableByQName(varName,"",obj);*/
}

void ActionSetTarget2::Execute()
{
	abort();
}

void ActionGetVariable::Execute()
{
	abort();
/*	tiny_string varName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionGetVariable: " << varName);
	ASObject* owner;
	ASObject* object=rt->currentClip->getVariableByQName(varName,"",owner);
	if(!owner)
	{
		//Looks in Global
		LOG(LOG_CALLS,"NOT implemented, trying Global");
		object=rt->vm.Global.getVariableByQName(varName,"",owner);
	}
	if(!owner)
	{
		//Check for special vars
		if(varName=="_global")
		{
			object=&rt->vm.Global;
			owner=object;
		}
	}

	if(owner)
		rt->vm.stack.push(object);
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"NOT found, pushing undefined");
		rt->vm.stack.push(new Undefined);
	}*/
}

void ActionToggleQuality::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionToggleQuality");
}

ActionGotoFrame::ActionGotoFrame(std::istream& in)
{
	in >> Frame;
}

ActionJump::ActionJump(std::istream& in)
{
	in >> BranchOffset;
}

ActionWith::ActionWith(std::istream& in)
{
	in >> Size;
}

ActionIf::ActionIf(std::istream& in)
{
	in >> Offset;
}

ActionConstantPool::ActionConstantPool(std::istream& in)
{
	in >> Count;

	STRING s;
	LOG(LOG_TRACE,"ConstantPool: Reading " << Count <<  " constants");
	for(int i=0;i<Count;i++)
	{
		in >> s;
		ConstantPool.push_back(s);
	}
}

ActionSetTarget::ActionSetTarget(std::istream& in)
{
	in >> TargetName;
}

ActionGoToLabel::ActionGoToLabel(std::istream& in)
{
	in >> Label;
}

ActionStoreRegister::ActionStoreRegister(std::istream& in)
{
	in >> RegisterNumber;
}

void ActionStoreRegister::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionStoreRegister "<< (int)RegisterNumber);
	if(RegisterNumber>10)
		LOG(LOG_ERROR,"Register index too big");
	rt->execContext->regs[RegisterNumber]=rt->vm.stack(0);*/
}

ActionPush::ActionPush(std::istream& in, ACTIONRECORDHEADER* h)
{
	int r=h->Length;
	while(r)
	{
		in >> Type;
		r--;
		switch(Type)
		{
			case 0:
			{
				STRING tmp;
				in >> tmp;
				//Objects.push_back(Class<ASString>::getInstanceS(true,(const char*)tmp)->obj);
				r-=(tmp.size()+1);
				LOG(LOG_CALLS,"Push: Read string " << tmp);
				break;
			}
			case 1:
			{
				FLOAT tmp;
				in >> tmp;
				//Objects.push_back(new Number(tmp));
				r-=4;
				LOG(LOG_CALLS,"Push: Read float " << tmp);
				break;
			}
			case 2:
			{
				//Objects.push_back(new Null);
				LOG(LOG_TRACE,"Push: null");
				break;
			}
			case 3:
			{
				//Objects.push_back(new Undefined);
				LOG(LOG_TRACE,"Push: undefined");
				break;
			}
			case 4:
			{
				UI8 tmp;
				in >> tmp;
				//RegisterNumber* n=new RegisterNumber(tmp);
				//Objects.push_back(n);
				r--;
				LOG(LOG_TRACE,"Push: Read reg number " << (int)tmp);
				break;
			}
			case 5:
			{
				UI8 tmp;
				in >> tmp;
				//Objects.push_back(new Integer(tmp));
				r--;
				LOG(LOG_TRACE,"Push: Read bool " << (int)tmp);
				break;
			}
			case 6:
			{
				DOUBLE tmp;
				in >> tmp;
				//Objects.push_back(new Number(tmp));
				r-=8;
				LOG(LOG_TRACE,"Push: Read double " << tmp);
				break;
			}
			case 7:
			{
				UI32 tmp;
				in >> tmp;
				//Objects.push_back(new Integer(tmp));
				r-=4;
				LOG(LOG_TRACE,"Push: Read integer " << tmp);
				break;
			}
			case 8:
			{
				UI8 i;
				in >> i;
				//ConstantReference* c=new ConstantReference(i);
				//Objects.push_back(c);
				r--;
				LOG(LOG_TRACE,"Push: Read constant index " << (int)i);
				break;
			}
			case 9:
			{
				UI16 i;
				in >> i;
				//ConstantReference* c=new ConstantReference(i);
				//Objects.push_back(c);
				r-=2;
				LOG(LOG_TRACE,"Push: Read long constant index " << (int)i);
				break;
			}
			default:
				LOG(LOG_ERROR,"Push type: " << (int)Type);
				ignore(in,r);
				r=0;
				break;
		}
	}
}

void ActionPush::print()
{
	LOG(LOG_TRACE,"ActionPush");
}

void ActionWith::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Exec: ActionWith");
}

void ActionGetMember::Execute()
{
	abort();
/*	tiny_string memberName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionGetMember: " << memberName);
	ASObject* obj=rt->vm.stack.pop();
	ASObject* owner;
	ASObject* ret=obj->getVariableByQName(memberName,"",owner);
	if(owner)
		rt->vm.stack.push(ret);
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"NOT found, pushing undefined");
		rt->vm.stack.push(new Undefined);
	}*/
}

void ActionSetMember::Execute()
{
	abort();
/*	ASObject* value=rt->vm.stack.pop();
	tiny_string memberName=rt->vm.stack.pop()->toString();
	LOG(LOG_CALLS,"ActionSetMember: " << memberName);
	ASObject* obj=rt->vm.stack.pop();
	obj->setVariableByQName(memberName,"",value);*/
}

void ActionPush::Execute()
{
	abort();
/*	LOG(LOG_CALLS,"ActionPush");
	for(int i=0;i<Objects.size();i++)
	{
		LOG(LOG_CALLS,"\t " << Objects[i]->toString());
		rt->vm.stack.push(Objects[i]->clone());
	}*/
}

ActionGetURL::ActionGetURL(std::istream& in)
{
	in >> UrlString >> TargetString;
}

ActionGetURL2::ActionGetURL2(std::istream& in)
{
	LOG(LOG_NOT_IMPLEMENTED,"GetURL2");
	in >> Reserved;
}

void ActionGetURL::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"GetURL: exec");
}

void ActionGetURL2::Execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"GetURL2: exec");
}

void ActionPlay::Execute()
{
	/*LOG(LOG_CALLS,"ActionPlay");
	rt->currentClip->state.next_FP=rt->currentClip->state.FP;
	rt->currentClip->state.stop_FP=false;*/
}

void ActionGotoFrame::Execute()
{
	/*LOG(LOG_CALLS,"ActionGoto");
	rt->currentClip->state.next_FP=Frame;
	rt->currentClip->state.stop_FP=false;*/
}

void ActionConstantPool::Execute()
{
	LOG(LOG_CALLS,"ActionConstantPool");
	//rt->vm.setConstantPool(ConstantPool);	
}

void ActionSetTarget::Execute()
{
	abort();
}

void ActionGoToLabel::Execute()
{
	LOG(LOG_CALLS,"ActionGoToLabel");
}

std::istream& lightspark::operator >>(std::istream& stream, BUTTONCONDACTION& v)
{
	stream >> v.CondActionSize;

	BitStream bs(stream);
	
	v.CondIdleToOverDown=UB(1,bs);
	v.CondOutDownToIdle=UB(1,bs);
	v.CondOutDownToOverDown=UB(1,bs);
	v.CondOverDownToOutDown=UB(1,bs);
	v.CondOverDownToOverUp=UB(1,bs);
	v.CondOverUpToOverDown=UB(1,bs);
	v.CondOverUpToIdle=UB(1,bs);
	v.CondIdleToOverUp=UB(1,bs);

	v.CondKeyPress=UB(7,bs);
	v.CondOutDownToIdle=UB(1,bs);

	while(1)
	{
		ACTIONRECORDHEADER ah(stream);
		if(ah.ActionCode==0)
			break;
		else
			v.Actions.push_back(ah.createTag(stream));
	}

	return stream;
}

