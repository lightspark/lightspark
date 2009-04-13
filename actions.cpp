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
	for(unsigned int i=0;i<actions.size();i++)
		actions[i]->Execute();
/*	for(unsigned int i=0;i<actions.size();i++)
		actions[i]->print();*/
}

ACTIONRECORDHEADER::ACTIONRECORDHEADER(std::istream& in)
{
	in >> ActionCode;
	if(ActionCode>=0x80)
		in >> Length;
}

ActionTag* ACTIONRECORDHEADER::createTag(std::istream& in)
{
	switch(ActionCode)
	{
		case 0x06:
			return new ActionPlay;
			break;
		case 0x07:
			return new ActionStop;
			break;
		case 0x0b:
			return new ActionSubtract;
			break;
		case 0x0c:
			return new ActionMultiply;
			break;
		case 0x0d:
			return new ActionDivide;
			break;
		case 0x12:
			return new ActionNot;
			break;
		case 0x13:
			return new ActionStringEquals;
			break;
		case 0x15:
			return new ActionStringExtract;
			break;
		case 0x17:
			return new ActionPop;
			break;
		case 0x1c:
			return new ActionGetVariable;
			break;
		case 0x1d:
			return new ActionSetVariable;
			break;
		case 0x21:
			return new ActionStringAdd;
			break;
		case 0x22:
			return new ActionGetProperty;
			break;
		case 0x24:
			return new ActionCloneSprite;
			break;
		case 0x3c:
			return new ActionDefineLocal;
			break;
		case 0x3d:
			return new ActionCallFunction;
			break;
		case 0x3e:
			return new ActionReturn;
			break;
		case 0x40:
			return new ActionNewObject;
			break;
		case 0x47:
			return new ActionAdd2;
			break;
		case 0x48:
			return new ActionLess2;
			break;
		case 0x49:
			return new ActionEquals2;
			break;
		case 0x4c:
			return new ActionPushDuplicate;
			break;
		case 0x4e:
			return new ActionGetMember;
			break;
		case 0x4f:
			return new ActionSetMember;
			break;
		case 0x50:
			return new ActionIncrement;
			break;
		case 0x51:
			return new ActionDecrement;
			break;
		case 0x52:
			return new ActionCallMethod;
			break;
		case 0x67:
			return new ActionGreater;
			break;
		case 0x81:
			return new ActionGotoFrame(in);
			break;
		case 0x83:
			return new ActionGetURL(in);
			break;
		case 0x87:
			return new ActionStoreRegister(in);
			break;
		case 0x88:
			return new ActionConstantPool(in);
			break;
		case 0x8e:
			return new ActionDefineFunction2(in,this);
			break;
		case 0x94:
			return new ActionWith(in);
			break;
		case 0x96:
			return new ActionPush(in,this);
			break;
		case 0x99:
			return new ActionJump(in);
			break;
		case 0x9a:
			return new ActionGetURL2(in);
			break;
		case 0x9b:
			return new ActionDefineFunction(in,this);
			break;
		case 0x9d:
			return new ActionIf(in);
			break;
		default:
			LOG(ERROR,"Unsopported ActionCode " << (int)ActionCode);
			break;
	}
}

RunState::RunState():FP(0),stop_FP(0)
{
}

void ActionStop::Execute()
{
	LOG(CALLS,"ActionStop");
	sys->currentState->next_FP=sys->currentState->FP;
	sys->currentState->stop_FP=true;
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
	int dest=in.tellg();
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
	sys->vm.registerFunction(this);
}

void ActionDefineFunction2::call()
{
	for(unsigned int i=0;i<functionActions.size();i++)
		functionActions[i]->Execute();
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
	//LOG(NOT_IMPLEMENTED,"ActionDefineFunction2: read function code");
	//ignore(in,CodeSize);
	int dest=in.tellg();
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
	sys->vm.registerFunction(this);
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
	LOG(NOT_IMPLEMENTED,"Exec: ActionIncrement");
}

void ActionGreater::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionGreater");
}

void ActionAdd2::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionAdd2");
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
	LOG(NOT_IMPLEMENTED,"Exec: ActionNewObject");
}

void ActionReturn::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionReturn");
}

void ActionPop::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionPop");
}

void ActionCallMethod::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionCallMethod");
}

void ActionCallFunction::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionCallFunction");

	STRING funcName=sys->vm.stack.pop()->toString();
	int numArgs=sys->vm.stack.pop()->toInt();
	if(numArgs!=0)
		LOG(NOT_IMPLEMENTED,"There are args");
	FunctionTag* f=sys->vm.getFunctionByName(funcName);
	f->call();
}

void ActionDefineFunction::Execute()
{
	LOG(CALLS,"ActionDefineFunction: Null Execution");
}

void ActionDefineFunction2::Execute()
{
	LOG(CALLS,"ActionDefineFunction2: Null Execution");
}

void ActionLess2::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionLess2");
}

void ActionEquals2::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionEquals2");
}

void ActionJump::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionJump");
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
	LOG(NOT_IMPLEMENTED,"Exec: ActionIf");
}

void ActionDivide::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionDivide");
}

void ActionMultiply::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionSubtract");
}

void ActionSubtract::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionSubtract");
}

void ActionNot::Execute()
{
	LOG(NOT_IMPLEMENTED,"Exec: ActionNot");
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
	sys->renderTarget->setVariableByName(varName,obj);
}

void ActionGetVariable::Execute()
{
	STRING varName=sys->vm.stack.pop()->toString();
	LOG(CALLS,"ActionGetVariable: name " << varName);
	SWFObject object=sys->renderTarget->getVariableByName(varName);
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
	LOG(NOT_IMPLEMENTED,"Exec: ActionStoreRegister "<<RegisterNumber);
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
			case 5:
			{
				UI8 tmp;
				in >> tmp;
				UI32* d=new UI32(tmp);
				Objects.push_back(d);
				r--;
				LOG(TRACE,"Push: Read bool " << *d);
				break;
			}
			case 6:
			{
				DOUBLE* d=new DOUBLE;
				in >> *d;
				Objects.push_back(d);
				r-=8;
				LOG(TRACE,"Push: Read double " << *d);
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
				LOG(NOT_IMPLEMENTED,"Push type: " << (int)Type);
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
		sys->vm.stack.push(Objects[i]);
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
	sys->currentState->next_FP=sys->currentState->FP;
	sys->currentState->stop_FP=false;
}

void ActionGotoFrame::Execute()
{
	LOG(CALLS,"ActionGoto");
	sys->currentState->next_FP=Frame;
	sys->currentState->stop_FP=false;
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

