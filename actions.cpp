/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "actions.h"
#include "logger.h"
#include "swf.h"

extern __thread SystemState* sys;
extern __thread RenderThread* rt;

using namespace std;
long timeDiff(timespec& s, timespec& d);

void ignore(istream& i, int count);

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	LOG(CALLS,"DoActionTag");
	int dest=in.tellg();
	if((h&0x3f)==0x3f)
		dest+=Length;
	else
		dest+=h&0x3f;

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
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
	}
}

int DoActionTag::getDepth() const
{
	//The last, after any other valid depth and initactions
	return 0x20000;
}

void DoActionTag::printInfo(int t)
{
	for(unsigned int i=0;i<actions.size();i++)
		actions[i]->print();
}

void DoActionTag::Render()
{
	timespec ts,td;
	clock_gettime(CLOCK_REALTIME,&ts);
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
				LOG(ERROR,"Invalid jump offset");
		}
		else if(jumpOffset>0)
		{
			while(jumpOffset>0)
			{
				i++;
				jumpOffset-=actions[i]->Length;
			}
			if(jumpOffset<0)
				LOG(ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	clock_gettime(CLOCK_REALTIME,&td);
	sys->fps_prof->action_time=timeDiff(ts,td);
}

DoInitActionTag::DoInitActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	LOG(CALLS,"DoInitActionTag");
	int dest=in.tellg();
	if((h&0x3f)==0x3f)
		dest+=Length;
	else
		dest+=h&0x3f;
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
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
	}
}

int DoInitActionTag::getDepth() const
{
	//The first after any other valid depth
	return 0x10000;
}

void DoInitActionTag::Render()
{
	timespec ts,td;
	clock_gettime(CLOCK_REALTIME,&ts);
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
				LOG(ERROR,"Invalid jump offset");
		}
		else if(jumpOffset>0)
		{
			while(jumpOffset>0)
			{
				i++;
				jumpOffset-=actions[i]->Length;
			}
			if(jumpOffset<0)
				LOG(ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	clock_gettime(CLOCK_REALTIME,&td);
	sys->fps_prof->action_time=timeDiff(ts,td);
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
			return t;
		case 0x0b:
			t=new ActionSubtract;
			return t;
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
		case 0x1c:
			t=new ActionGetVariable;
			break;
		case 0x1d:
			t=new ActionSetVariable;
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
		case 0x64:
			t=new ActionBitRShift;
			break;
		case 0x66:
			t=new ActionStrictEquals;
			break;
		case 0x67:
			t=new ActionGreater;
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
		case 0x8e:
			t=new ActionDefineFunction2(in,this);
			break;
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
		case 0x9b:
			t=new ActionDefineFunction(in,this);
			break;
		case 0x9d:
			t=new ActionIf(in);
			break;
		default:
			LOG(NOT_IMPLEMENTED,"Unsupported ActionCode " << (int)ActionCode);
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

RunState::RunState():FP(0),stop_FP(0)
{
}

void ActionStop::Execute()
{
	LOG(CALLS,"ActionStop");
	rt->currentClip->state.next_FP=rt->currentClip->state.FP;
	rt->currentClip->state.stop_FP=true;
}

ActionDefineFunction::ActionDefineFunction(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams;
	LOG(CALLS,"Defining function " << FunctionName);
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
			LOG(ERROR,"End action in function")
		else
			functionActions.push_back(ah.createTag(in));
		if(functionActions.back()==NULL)
		{
			functionActions.pop_back();
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
		if(in.tellg()==dest)
			break;
		else if(in.tellg()>dest)
		{
			LOG(ERROR,"CodeSize not consistent");
			break;
		}
	}
}

ISWFObject* ActionDefineFunction2::call(ISWFObject* obj, arguments* args)
{
	retValue=new Undefined;
	if(retValue->getObjectType()!=T_UNDEFINED)
		LOG(ERROR,"Not valid condition");
	ExecutionContext* exec_bak=rt->execContext;
	rt->execContext=this;
	LOG(CALLS,"Calling Function2 " << FunctionName);
	for(int i=0;i<args->size();i++)
		LOG(CALLS,"Arg "<<i<<"="<<args->at(i)->toString());
	for(int i=0;i<NumParams;i++)
	{
		//cout << "Reg " << (int)Parameters[i].Register << " for " <<  Parameters[i].ParamName << endl;
		if(Parameters[i].Register==0)
			LOG(ERROR,"Parameter not in register")
		else
		{
			rt->execContext->regs[Parameters[i].Register]=args->at(i);
		}
	}
	int used_regs=1;
	if(PreloadThisFlag)
	{
		LOG(CALLS,"Preload this");
		rt->execContext->regs[used_regs]=rt->currentClip;
		used_regs++;
	}
	if(PreloadArgumentsFlag)
		LOG(CALLS,"Preload arguments "<<used_regs);
	if(PreloadSuperFlag)
		LOG(CALLS,"Preload super "<<used_regs);
	if(PreloadRootFlag)
		LOG(CALLS,"Preload root "<<used_regs);
	if(PreloadParentFlag)
		LOG(CALLS,"Preload parent "<<used_regs);
	if(PreloadGlobalFlag)
		LOG(CALLS,"Preload global "<<used_regs);

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
				LOG(ERROR,"Invalid jump offset");
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
				LOG(ERROR,"Invalid jump offset");
		}
	}
	rt->execContext=exec_bak;
	return retValue;
}

ActionDefineFunction2::ActionDefineFunction2(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams >> RegisterCount;
	LOG(CALLS,"Defining function2 " << FunctionName);
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
			LOG(ERROR,"End action in function")
		else
			functionActions.push_back(ah.createTag(in));
		if(functionActions.back()==NULL)
		{
			functionActions.pop_back();
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
		functionActions.back()->print();
		if(in.tellg()==dest)
			break;
		else if(in.tellg()>dest)
		{
			LOG(ERROR,"CodeSize not consistent with file offset " << in.tellg());
			break;
		}
	}
}

void ActionPushDuplicate::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionPushDuplicate");
}

void ActionSetProperty::Execute()
{
	ISWFObject* value=rt->vm.stack.pop();
	int index=rt->vm.stack.pop()->toInt();
	string target=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionSetProperty to: " << target << " index " << index);
	bool found;
	ISWFObject* obj=sys->getVariableByName(target,found);
	if(found)
	{
		switch(index)
		{
			case 2:
				obj->setVariableByName("_scalex",value);
				LOG(CALLS,"setting to " << value->toNumber());
				break;
	/*		case 5:
				ret=obj->getVariableByName("_totalframes");
				LOG(NO_INFO,"setting to " << ret->toInt());
				break;
			case 12:
				ret=obj->getVariableByName("_framesloaded");
				LOG(NO_INFO,"setting to " << ret->toInt());
				break;*/
			default:
				LOG(ERROR,"Not supported property index "<< index);
				break;
		}
	}
}

void ActionGetProperty::Execute()
{
	int index=rt->vm.stack.pop()->toInt();
	string target=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionGetProperty from: " << target << " index " << index);
	bool found;
	ISWFObject* obj=sys->getVariableByName(target,found);
	ISWFObject* ret;
	if(found)
	{
		switch(index)
		{
			case 5:
				ret=obj->getVariableByName("_totalframes",found);
				LOG(CALLS,"returning " << ret->toInt());
				break;
			case 12:
				ret=obj->getVariableByName("_framesloaded",found);
				LOG(CALLS,"returning " << ret->toInt());
				break;
			default:
				LOG(ERROR,"Not supported property index "<< index);
				break;
		}
	}
	else
		ret=new Undefined;
	rt->vm.stack.push(ret);
}

void ActionDecrement::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionDecrement");
}

void ActionExtends::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionExtends");
}

void ActionTypeOf::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionTypeOf");
}

void ActionGetTime::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionGetTime");
}

void ActionInstanceOf::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionInstanceOf");
}

void ActionImplementsOp::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionImplementsOp");
}

void ActionBitAnd::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionBitAnd");
}

void ActionBitRShift::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionBitRShift");
}

void ActionEnumerate2::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionEnumerate2");
}

void ActionToString::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionToString");
}

void ActionToNumber::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionToNumber");
}

void ActionCastOp::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionCastOp");
}

void ActionIncrement::Execute()
{
	float a=rt->vm.stack.pop()->toNumber();
	LOG(CALLS,"ActionIncrement: " << a);
	rt->vm.stack.push(new Number(a+1));
}

void ActionGreater::Execute()
{
	LOG(CALLS,"ActionGreater");
	ISWFObject* arg1=rt->vm.stack.pop();
	ISWFObject* arg2=rt->vm.stack.pop();
	if(arg2->isGreater(arg1))
	{
		LOG(CALLS,"Greater");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Greater");
		rt->vm.stack.push(new Integer(0));
	}
}

void ActionAdd2::Execute()
{
	ISWFObject* arg1=rt->vm.stack.pop();
	ISWFObject* arg2=rt->vm.stack.pop();

	if(arg1->getObjectType()==T_STRING || arg2->getObjectType()==T_STRING)
	{
		rt->vm.stack.push(new ASString(arg2->toString()+arg1->toString()));
		LOG(CALLS,"ActionAdd2 (string concatenation): " << rt->vm.stack(0)->toString());
	}
	else
	{
		rt->vm.stack.push(new Number(arg1->toNumber()+arg2->toNumber()));
		LOG(CALLS,"ActionAdd2 returning: " << arg1->toNumber() + arg2->toNumber());
	}
}

void ActionCloneSprite::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionCloneSprite");
}

void ActionDefineLocal::Execute()
{
	LOG(CALLS,"ActionDefineLocal");
	ISWFObject* value=rt->vm.stack.pop();
	string name=rt->vm.stack.pop()->toString();
	rt->currentClip->setVariableByName(name,value);
}

void ActionNewObject::Execute()
{
	string varName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionNewObject: name " << varName);
	bool found;
	ISWFObject* type=sys->getVariableByName(varName,found);
	if(found)
	{
		if(type->getObjectType()!=T_UNDEFINED)
			LOG(ERROR,"ActionNewObject: no such object");
		int numArgs=rt->vm.stack.pop()->toInt();
		if(numArgs)
			LOG(ERROR,"There are arguments");
		ISWFObject* c=type->getVariableByName("constructor",found);
		if(c->getObjectType()!=T_FUNCTION)
			LOG(ERROR,"Constructor is not a function");
		Function* f=dynamic_cast<Function*>(c);
		if(f==NULL)
			LOG(ERROR,"Not possible error");

		ISWFObject* obj=type->clone();
		f->call(obj,NULL);
		rt->vm.stack.push(obj);
	}
	else
		rt->vm.stack.push(new Undefined);
}

void ActionReturn::Execute()
{
	LOG(CALLS,"ActionReturn");
	rt->execContext->retValue=rt->vm.stack.pop();
}

void ActionPop::Execute()
{
	string popped=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionPop: " << popped);
}

void ActionCallMethod::Execute()
{
	string methodName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionCallMethod: " << methodName);
	ISWFObject* obj=rt->vm.stack.pop();
	int numArgs=rt->vm.stack.pop()->toInt();
	arguments args(numArgs);
	for(int i=0;i<numArgs;i++)
		args.at(i)=rt->vm.stack.pop();
	bool found;
	ISWFObject* ret=rt->currentClip->getVariableByName(methodName,found);
	if(found)
	{
		IFunction* f=ret->toFunction();
		if(f==0)
		{
			LOG(ERROR,"No such function");
			rt->vm.stack.push(new Undefined);
		}
		else
		{
			ISWFObject* ret=f->call(NULL,&args);
			rt->vm.stack.push(ret);
		}
	}
	else
		rt->vm.stack.push(new Undefined);
}

void ActionCallFunction::Execute()
{
	LOG(CALLS,"ActionCallFunction");

	string funcName=rt->vm.stack.pop()->toString();
	int numArgs=rt->vm.stack.pop()->toInt();
	arguments args(numArgs);;
	for(int i=0;i<numArgs;i++)
		args.at(i)=rt->vm.stack.pop();
	bool found;
	ISWFObject* ret=rt->currentClip->getVariableByName(funcName,found);
	if(found)
	{
		IFunction* f=ret->toFunction();
		if(f==0)
		{
			LOG(ERROR,"No such function");
			rt->vm.stack.push(new Undefined);
		}
		else
		{
			ISWFObject* ret=f->call(NULL,&args);
			rt->vm.stack.push(ret);
		}
	}
	else
		rt->vm.stack.push(new Undefined);
	LOG(CALLS,"ActionCallFunction: End");
}

void ActionDefineFunction::Execute()
{
	LOG(CALLS,"ActionDefineFunction: " << FunctionName);
	if(FunctionName.isNull())
		rt->vm.stack.push(this);
	else
		rt->currentClip->setVariableByName(FunctionName,this);
}

ISWFObject* ActionDefineFunction::call(ISWFObject* obj, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"ActionDefineFunction: Call");
	return NULL;
}

void ActionDefineFunction2::Execute()
{
	LOG(CALLS,"ActionDefineFunction2: " << FunctionName);
	if(FunctionName.isNull())
		rt->vm.stack.push(this);
	else
		rt->currentClip->setVariableByName(FunctionName,this);
}

void ActionLess2::Execute()
{
	LOG(CALLS,"ActionLess2");
	ISWFObject* arg1=rt->vm.stack.pop();
	ISWFObject* arg2=rt->vm.stack.pop();
	if(arg2->isLess(arg1))
	{
		LOG(CALLS,"Less");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Less");
		rt->vm.stack.push(new Integer(0));
	}
}

void ActionStrictEquals::Execute()
{
	LOG(NOT_IMPLEMENTED,"ActionStrictEquals");
	ISWFObject* arg1=rt->vm.stack.pop();
	ISWFObject* arg2=rt->vm.stack.pop();
	if(arg1->isEqual(arg2))
	{
		LOG(CALLS,"Equal");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Equal");
		rt->vm.stack.push(new Integer(0));
	}
}

void ActionEquals2::Execute()
{
	LOG(CALLS,"ActionEquals2");
	ISWFObject* arg1=rt->vm.stack.pop();
	ISWFObject* arg2=rt->vm.stack.pop();
	if(arg1->isEqual(arg2))
	{
		LOG(CALLS,"Equal");
		rt->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Equal");
		rt->vm.stack.push(new Integer(0));
	}
}

void ActionJump::Execute()
{
	LOG(CALLS,"ActionJump: " << BranchOffset);
	rt->execContext->setJumpOffset(BranchOffset);
}

void ActionDelete::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionDelete");
}

void ActionNewMethod::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionNewMethod");
}

void ActionAsciiToChar::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionStringAdd");
}

void ActionStringAdd::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionStringAdd");
}

void ActionStringExtract::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionStringExtract");
}

void ActionIf::Execute()
{
	LOG(CALLS,"ActionIf");
	int cond=rt->vm.stack.pop()->toInt();
	if(cond)
		rt->execContext->setJumpOffset(Offset);
}

void ActionModulo::Execute()
{
	int a=rt->vm.stack.pop()->toInt();
	int b=rt->vm.stack.pop()->toInt();
	rt->vm.stack.push(new Number(b%a));
	LOG(CALLS,"ActionDivide: return " << b << "%" << a << "="<< b%a);
}

void ActionDivide::Execute()
{
	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b/a));
	LOG(CALLS,"ActionDivide: return " << b << "/" << a << "="<< b/a);
}

void ActionMultiply::Execute()
{
	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b*a));
	LOG(CALLS,"ActionMultiply: return " << b*a);
}

void ActionSubtract::Execute()
{
	double a=rt->vm.stack.pop()->toNumber();
	double b=rt->vm.stack.pop()->toNumber();
	rt->vm.stack.push(new Number(b-a));
	LOG(CALLS,"ActionSubtract: return " << b-a);
}

void ActionNot::Execute()
{
	LOG(CALLS,"ActionNot");
	double a=rt->vm.stack.pop()->toNumber();
	if(a==0)
		rt->vm.stack.push(new Integer(1));
	else
		rt->vm.stack.push(new Integer(0));
}

void ActionInitArray::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionInitArray");
}

void ActionInitObject::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionInitObject");
}

void ActionStringEquals::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionStringEquals");
}

void ActionSetVariable::Execute()
{
	ISWFObject* obj=rt->vm.stack.pop();
	string varName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionSetVariable: name " << varName);
	rt->currentClip->setVariableByName(varName,obj);
}

void ActionGetVariable::Execute()
{
	string varName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionGetVariable: " << varName);
	bool found;
	ISWFObject* object=rt->currentClip->getVariableByName(varName,found);
	if(found)
		rt->vm.stack.push(object);
	else
	{
		LOG(CALLS,"NOT found, pushing undefined");
		rt->vm.stack.push(new Undefined);
	}
}

void ActionToggleQuality::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionToggleQuality");
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
	LOG(TRACE,"ConstantPool: Reading " << Count <<  " constants");
	for(int i=0;i<Count;i++)
	{
		in >> s;
		ConstantPool.push_back(s);
	}
}

ActionStoreRegister::ActionStoreRegister(std::istream& in)
{
	in >> RegisterNumber;
}

void ActionStoreRegister::Execute()
{
	LOG(CALLS,"ActionStoreRegister "<< (int)RegisterNumber);
	if(RegisterNumber>10)
		LOG(ERROR,"Register index too big");
	rt->execContext->regs[RegisterNumber]=rt->vm.stack(0);
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
				Objects.push_back(new ASString(tmp));
				r-=(tmp.size()+1);
				LOG(CALLS,"Push: Read string " << tmp);
				break;
			}
			case 1:
			{
				FLOAT tmp;
				in >> tmp;
				Objects.push_back(new Number(tmp));
				r-=4;
				LOG(CALLS,"Push: Read float " << tmp);
				break;
			}
			case 2:
			{
				Objects.push_back(new Null);
				LOG(TRACE,"Push: null");
				break;
			}
			case 3:
			{
				Objects.push_back(new Undefined);
				LOG(TRACE,"Push: undefined");
				break;
			}
			case 4:
			{
				UI8 tmp;
				in >> tmp;
				RegisterNumber* n=new RegisterNumber(tmp);
				Objects.push_back(n);
				r--;
				LOG(TRACE,"Push: Read reg number " << (int)tmp);
				break;
			}
			case 5:
			{
				UI8 tmp;
				in >> tmp;
				Objects.push_back(new Integer(tmp));
				r--;
				LOG(TRACE,"Push: Read bool " << (int)tmp);
				break;
			}
			case 6:
			{
				DOUBLE tmp;
				in >> tmp;
				Objects.push_back(new Number(tmp));
				r-=8;
				LOG(TRACE,"Push: Read double " << tmp);
				break;
			}
			case 7:
			{
				UI32 tmp;
				in >> tmp;
				Objects.push_back(new Integer(tmp));
				r-=4;
				LOG(TRACE,"Push: Read integer " << tmp);
				break;
			}
			case 8:
			{
				UI8 i;
				in >> i;
				ConstantReference* c=new ConstantReference(i);
				Objects.push_back(c);
				r--;
				LOG(TRACE,"Push: Read constant index " << (int)i);
				break;
			}
			case 9:
			{
				UI16 i;
				in >> i;
				ConstantReference* c=new ConstantReference(i);
				Objects.push_back(c);
				r-=2;
				LOG(TRACE,"Push: Read long constant index " << (int)i);
				break;
			}
			default:
				LOG(ERROR,"Push type: " << (int)Type);
				ignore(in,r);
				r=0;
				break;
		}
	}
}

void ActionPush::print()
{
	LOG(TRACE,"ActionPush");
}

void ActionWith::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionWith");
}

void ActionGetMember::Execute()
{
	string memberName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionGetMember: " << memberName);
	ISWFObject* obj=rt->vm.stack.pop();
	bool found;
	ISWFObject* ret=obj->getVariableByName(memberName,found);
	if(found)
		rt->vm.stack.push(ret);
	else
		rt->vm.stack.push(new Undefined);
}

void ActionSetMember::Execute()
{
	ISWFObject* value=rt->vm.stack.pop();
	string memberName=rt->vm.stack.pop()->toString();
	LOG(CALLS,"ActionSetMember: " << memberName);
	ISWFObject* obj=rt->vm.stack.pop();
	obj->setVariableByName(memberName,value);
}

void ActionPush::Execute()
{
	LOG(CALLS,"ActionPush");
	for(int i=0;i<Objects.size();i++)
	{
		LOG(CALLS,"\t " << Objects[i]->toString());
		rt->vm.stack.push(Objects[i]->clone());
	}
}

ActionGetURL::ActionGetURL(std::istream& in)
{
	in >> UrlString >> TargetString;
}

ActionGetURL2::ActionGetURL2(std::istream& in)
{
	LOG(NOT_IMPLEMENTED,"GetURL2");
	in >> Reserved;
}

void ActionGetURL::Execute()
{
	LOG(NOT_IMPLEMENTED,"GetURL: exec");
}

void ActionGetURL2::Execute()
{
	LOG(NOT_IMPLEMENTED,"GetURL2: exec");
}

void ActionPlay::Execute()
{
	LOG(CALLS,"ActionPlay");
	rt->currentClip->state.next_FP=rt->currentClip->state.FP;
	rt->currentClip->state.stop_FP=false;
}

void ActionGotoFrame::Execute()
{
	LOG(CALLS,"ActionGoto");
	rt->currentClip->state.next_FP=Frame;
	rt->currentClip->state.stop_FP=false;
}

void ActionConstantPool::Execute()
{
	LOG(CALLS,"ActionConstantPool");
	rt->vm.setConstantPool(ConstantPool);	
}

std::istream& operator>>(std::istream& stream, BUTTONCONDACTION& v)
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

