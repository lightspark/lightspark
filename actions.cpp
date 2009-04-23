/**************************************************************************
    Lighspark, a free flash player implementation

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

extern __thread SystemState* sys;

using namespace std;

void ignore(istream& i, int count);

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
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
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
	}
}

UI16 DoActionTag::getDepth() const
{
	return 0;
}

void DoActionTag::printInfo(int t)
{
	for(unsigned int i=0;i<actions.size();i++)
		actions[i]->print();
}

void DoActionTag::Render()
{
	ExecutionContext* exec_bak=sys->execContext;
	sys->execContext=this;
	for(unsigned int i=0;i<actions.size();i++)
	{
		actions[i]->Execute();
		if(jumpOffset<0)
		{
			LOG(NOT_IMPLEMENTED,"Backward jumps");
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
/*	for(unsigned int i=0;i<actions.size();i++)
		actions[i]->print();*/
	sys->execContext=exec_bak;
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
		case 0x24:
			t=new ActionCloneSprite;
			break;
		case 0x3c:
			t=new ActionDefineLocal;
			break;
		case 0x3d:
			t=new ActionCallFunction;
			break;
		case 0x3e:
			t=new ActionReturn;
			break;
		case 0x40:
			t=new ActionNewObject;
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
		case 0x67:
			t=new ActionGreater;
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
			LOG(ERROR,"Unsopported ActionCode " << (int)ActionCode);
			break;
	}
	t->Length+=Length;
	if(ActionCode>=0x80)
		t->Length+=2;
	return t;
}

RunState::RunState():FP(0),stop_FP(0)
{
}

void ActionStop::Execute()
{
	LOG(CALLS,"ActionStop");
	sys->currentClip->state.next_FP=sys->currentClip->state.FP;
	sys->currentClip->state.stop_FP=true;
}

ActionDefineFunction::ActionDefineFunction(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams;
	LOG(NO_INFO,"Defining function " << FunctionName);
	params.resize(NumParams);
	for(int i=0;i<NumParams;i++)
	{
		in >> params[i];
	}
	in >> CodeSize;
	//LOG(NOT_IMPLEMENTED,"ActionDefineFunction2: read function code");
	//ignore(in,CodeSize);
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

SWFObject ActionDefineFunction2::call(ISWFObject* obj, arguments* args)
{
	ExecutionContext* exec_bak=sys->execContext;
	sys->execContext=this;
	LOG(CALLS,"Calling Function2 " << FunctionName);
	for(int i=0;i<NumParams;i++)
	{
		cout << "Reg " << (int)Parameters[i].Register << " for " <<  Parameters[i].ParamName << endl;
		if(Parameters[i].Register==0)
			LOG(ERROR,"Parameter not in register")
		else
		{
			sys->vm.regs[Parameters[i].Register]=args->args[i];
		}
	}
	int used_regs=1;
	if(PreloadThisFlag)
	{
		LOG(CALLS,"Preload this");
		sys->vm.regs[used_regs]=SWFObject(sys->currentClip,true);
		used_regs++;
	}
	if(PreloadArgumentsFlag)
		LOG(NO_INFO,"Preload arguments "<<used_regs);
	if(PreloadSuperFlag)
		LOG(NO_INFO,"Preload super "<<used_regs);
	if(PreloadRootFlag)
		LOG(NO_INFO,"Preload root "<<used_regs);
	if(PreloadParentFlag)
		LOG(NO_INFO,"Preload parent "<<used_regs);
	if(PreloadGlobalFlag)
		LOG(NO_INFO,"Preload global "<<used_regs);

	for(unsigned int i=0;i<functionActions.size();i++)
	{
		functionActions[i]->Execute();
		if(jumpOffset<0)
		{
			LOG(NOT_IMPLEMENTED,"Backward jumps");
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
	sys->execContext=exec_bak;
	return SWFObject(sys->vm.stack.pop());
}

ActionDefineFunction2::ActionDefineFunction2(istream& in,ACTIONRECORDHEADER* h)
{
	in >> FunctionName >> NumParams >> RegisterCount;
	LOG(NO_INFO,"Defining function2 " << FunctionName);
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
		functionActions.back()->print();
		if(functionActions.back()==NULL)
		{
			LOG(ERROR,"Not supported action opcode");
			ignore(in,dest-in.tellg());
			break;
		}
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

void ActionGetProperty::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionGetProperty");
}

void ActionDecrement::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionDecrement");
}

void ActionIncrement::Execute()
{
	float a=sys->vm.stack.pop()->toFloat();
	LOG(CALLS,"ActionIncrement: " << a);
	sys->vm.stack.push(SWFObject(new Double(a+1)));
}

void ActionGreater::Execute()
{
	LOG(CALLS,"ActionGreater");
	SWFObject arg1=sys->vm.stack.pop();
	SWFObject arg2=sys->vm.stack.pop();
	if(arg2.isGreater(arg1))
	{
		LOG(CALLS,"Greater");
		sys->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Greater");
		sys->vm.stack.push(new Integer(0));
	}
}

void ActionAdd2::Execute()
{
	SWFObject arg1=sys->vm.stack.pop();
	SWFObject arg2=sys->vm.stack.pop();

	if(arg1->getObjectType()==T_STRING || arg2->getObjectType()==T_STRING)
	{
		sys->vm.stack.push(SWFObject(new ASString(arg2->toString()+arg1->toString())));
		LOG(CALLS,"ActionAdd2 (string concatenation): " << sys->vm.stack(0)->toString());
	}
	else
	{
		sys->vm.stack.push(SWFObject(new Double(arg1->toFloat()+arg2->toFloat())));
		LOG(CALLS,"ActionAdd2 returning: " << arg1->toFloat() + arg2->toFloat());
	}
}

void ActionCloneSprite::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionCloneSprite");
}

void ActionDefineLocal::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionDefineLocal");
}

void ActionNewObject::Execute()
{
	STRING varName=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionNewObject: name " << varName);
	SWFObject type=sys->getVariableByName(varName);
	if(!type.isDefined())
		LOG(ERROR,"ActionNewObject: no such object");
	int numArgs=sys->vm.stack.pop()->toInt();
	if(numArgs)
		LOG(ERROR,"There are arguments");
	SWFObject c=type->getVariableByName("constructor");
	if(c->getObjectType()!=T_FUNCTION)
		LOG(ERROR,"Constructor is not a function");
	Function* f=dynamic_cast<Function*>(c.getData());
	if(f==NULL)
		LOG(ERROR,"Not possible error");

	ISWFObject* obj=type->clone();
	f->call(obj,NULL);
	sys->vm.stack.push(SWFObject(obj,true));
}

void ActionReturn::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionReturn");
}

void ActionPop::Execute()
{
	STRING popped=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionPop: " << popped);
}

void ActionCallMethod::Execute()
{
	STRING methodName=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionCallMethod: " << methodName);
	SWFObject obj=sys->vm.stack.pop();
	int numArgs=sys->vm.stack.pop()->toInt();
	arguments args;
	for(int i=0;i<numArgs;i++)
		args.args.push_back(sys->vm.stack.pop());
	IFunction* f=obj->getVariableByName(methodName)->toFunction();
	if(f==0)
		LOG(ERROR,"No such function");
	SWFObject ret=f->call(obj.getData(),&args);
	sys->vm.stack.push(ret);
}

void ActionCallFunction::Execute()
{
	LOG(CALLS,"ActionCallFunction");

	STRING funcName=sys->vm.stack.pop()->toString();
	int numArgs=sys->vm.stack.pop()->toInt();
	arguments args;
	for(int i=0;i<numArgs;i++)
		args.args.push_back(sys->vm.stack.pop());
	IFunction* f=sys->currentClip->getVariableByName(funcName)->toFunction();
	if(f==0)
		LOG(ERROR,"No such function");
	SWFObject ret=f->call(NULL,&args);
	sys->vm.stack.push(ret);

	LOG(CALLS,"ActionCallFunction: End");
}

void ActionDefineFunction::Execute()
{
	LOG(CALLS,"ActionDefineFunction: " << FunctionName);
	if(FunctionName.isNull())
		sys->vm.stack.push(SWFObject(this,true));
	else
		sys->currentClip->setVariableByName(FunctionName,SWFObject(this,true));
}

SWFObject ActionDefineFunction::call(ISWFObject* obj, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"ActionDefineFunction: Call");
	return SWFObject();
}

void ActionDefineFunction2::Execute()
{
	LOG(CALLS,"ActionDefineFunction2: " << FunctionName);
	if(FunctionName.isNull())
		sys->vm.stack.push(SWFObject(this,true));
	else
		sys->currentClip->setVariableByName(FunctionName,SWFObject(this,true));
}

void ActionLess2::Execute()
{
	LOG(CALLS,"ActionLess2");
	SWFObject arg1=sys->vm.stack.pop();
	SWFObject arg2=sys->vm.stack.pop();
	if(arg2.isLess(arg1))
	{
		LOG(CALLS,"Less");
		sys->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Less");
		sys->vm.stack.push(new Integer(0));
	}
}

void ActionEquals2::Execute()
{
	LOG(CALLS,"ActionEquals2");
	SWFObject arg1=sys->vm.stack.pop();
	SWFObject arg2=sys->vm.stack.pop();
	if(arg1.equals(arg2))
	{
		LOG(CALLS,"Equal");
		sys->vm.stack.push(new Integer(1));
	}
	else
	{
		LOG(CALLS,"Not Equal");
		sys->vm.stack.push(new Integer(0));
	}
}

void ActionJump::Execute()
{
	LOG(CALLS,"ActionJump: " << BranchOffset);
	sys->execContext->setJumpOffset(BranchOffset);
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
	int cond=sys->vm.stack.pop()->toInt();
	if(cond)
		sys->execContext->setJumpOffset(Offset);
}

void ActionDivide::Execute()
{
	float a=sys->vm.stack.pop()->toFloat();
	float b=sys->vm.stack.pop()->toFloat();
	sys->vm.stack.push(new Double(b/a));
	LOG(CALLS,"ActionDivide: return " << b/a);
}

void ActionMultiply::Execute()
{
	float a=sys->vm.stack.pop()->toFloat();
	float b=sys->vm.stack.pop()->toFloat();
	sys->vm.stack.push(new Double(b*a));
	LOG(CALLS,"ActionMultiply: return " << b*a);
}

void ActionSubtract::Execute()
{
	float a=sys->vm.stack.pop()->toFloat();
	float b=sys->vm.stack.pop()->toFloat();
	sys->vm.stack.push(new Double(b-a));
	LOG(CALLS,"ActionSubtract: return " << b-a);
}

void ActionNot::Execute()
{
	LOG(CALLS,"ActionNot");
	float a=sys->vm.stack.pop()->toFloat();
	if(a==0)
		sys->vm.stack.push(new Integer(1));
	else
		sys->vm.stack.push(new Integer(0));
}

void ActionStringEquals::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionStringEquals");
}

void ActionSetVariable::Execute()
{
	SWFObject obj=sys->vm.stack.pop();
	STRING varName=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionSetVariable: name " << varName);
	sys->currentClip->setVariableByName(varName,obj);
}

void ActionGetVariable::Execute()
{
	STRING varName=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionGetVariable: name " << varName);
	SWFObject object=sys->currentClip->getVariableByName(varName);
	if(!object.isDefined())
	{
		LOG(CALLS,"ActionGetVariable: no such object");
		sys->dumpVariables();
	}
	sys->vm.stack.push(object);
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
	sys->vm.regs[RegisterNumber]=sys->vm.stack(0);
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
				Objects.push_back(new Double(tmp));
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
	LOG(CALLS,"ActionGetMember");
	STRING memberName=sys->vm.stack.pop()->toString();
	SWFObject obj=sys->vm.stack.pop();
	sys->vm.stack.push(obj->getVariableByName(memberName));
}

void ActionSetMember::Execute()
{
	LOG(CALLS,"ActionSetMember");
	SWFObject value=sys->vm.stack.pop();
	STRING memberName=sys->vm.stack.pop()->toString();
	SWFObject obj=sys->vm.stack.pop();
	obj->setVariableByName(memberName,value);
}

void ActionPush::Execute()
{
	LOG(CALLS,"ActionPush");
	for(int i=0;i<Objects.size();i++)
	{
		LOG(CALLS,"\t " << Objects[i]->toString());
		sys->vm.stack.push(Objects[i]->instantiate());
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
	sys->currentClip->state.next_FP=sys->currentClip->state.FP;
	sys->currentClip->state.stop_FP=false;
}

void ActionGotoFrame::Execute()
{
	LOG(CALLS,"ActionGoto");
	sys->currentClip->state.next_FP=Frame;
	sys->currentClip->state.stop_FP=false;
}

void ActionConstantPool::Execute()
{
	LOG(CALLS,"ActionConstantPool");
	sys->vm.setConstantPool(ConstantPool);	
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

