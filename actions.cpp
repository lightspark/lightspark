#include "actions.h"

extern SystemState sys;

using namespace std;

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	while(1)
	{
		ACTIONRECORDHEADER ah(in);
		if(ah.ActionCode==0)
			break;
		else
			actions.push_back(ah.createTag(in));
	}
}

UI16 DoActionTag::getDepth() const
{
	return 0;
}

void DoActionTag::printInfo(int t)
{
	cerr << "DoAction Info" << endl;
	for(int i=0;i<actions.size();i++)
		actions[i]->print();
}

void DoActionTag::Render()
{
	for(int i=0;i<actions.size();i++)
		actions[i]->Execute();
	for(int i=0;i<actions.size();i++)
		actions[i]->print();
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
		case 0x07:
			return new ActionStop;
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
		case 0x1c:
			return new ActionGetVariable;
			break;
		case 0x1d:
			return new ActionSetVariable;
			break;
		case 0x21:
			return new ActionStringAdd;
			break;
		case 0x81:
			return new ActionGotoFrame(in);
			break;
		case 0x83:
			return new ActionGetURL(in);
			break;
		case 0x88:
			return new ActionConstantPool(in);
			break;
		case 0x96:
			return new ActionPush(in,this);
			break;
		case 0x99:
			return new ActionJump(in);
			break;
		case 0x9d:
			return new ActionIf(in);
			break;
		default:
			cout << (int)ActionCode << endl;
			cout << "@ " << in.tellg() << endl;
			throw "Unsopported actioncode";
			return NULL;
	}
}

RunState::RunState():FP(0),stop_FP(0)
{
}

void ActionStop::Execute()
{
	cout << "Stop" << endl;
	sys.currentState->next_FP=sys.currentState->FP;
	sys.currentState->stop_FP=true;
}

void ActionJump::Execute()
{
	throw "WIP11";
}

void ActionStringAdd::Execute()
{
	throw "WIP11";
}

void ActionStringExtract::Execute()
{
	throw "WIP10";
}

void ActionIf::Execute()
{
	throw "WIP9";
}

void ActionNot::Execute()
{
	throw "WIP8";
}

void ActionStringEquals::Execute()
{
	throw "WIP7";
}

void ActionSetVariable::Execute()
{
	throw "WIP6";
}

void ActionGetVariable::Execute()
{
	throw "WIP5";
}

void ActionToggleQuality::Execute()
{
	throw "WIP4";
}

ActionGotoFrame::ActionGotoFrame(std::istream& in)
{
	in >> Frame;
}

ActionJump::ActionJump(std::istream& in)
{
	in >> BranchOffset;
}

ActionIf::ActionIf(std::istream& in)
{
	in >> Offset;
}

ActionConstantPool::ActionConstantPool(std::istream& in)
{
	in >> Count;

	STRING s;
	for(int i=0;i<Count;i++)
	{
		in >> s;
		ConstantPool.push_back(s);
	}
}

ActionPush::ActionPush(std::istream& in, ACTIONRECORDHEADER* h)
{
	cout << "TODO: ActionPush" << endl;
	in.ignore(h->Length);
	/*in >> Type;

	switch(Type)
	{
		case 8:
			in >> Constant8;
			break;
		default:
			throw "unsupported push";
	}*/
}

void ActionPush::Execute()
{
	throw "WIP3";
}

ActionGetURL::ActionGetURL(std::istream& in)
{
	in >> UrlString >> TargetString;

	cout << UrlString<< endl;
}

void ActionGetURL::Execute()
{
	cout << "Prelevo " << UrlString << endl;
}

void ActionGotoFrame::Execute()
{
	cout << "Goto " << Frame<< endl;
	sys.currentState->next_FP=Frame;
	sys.currentState->stop_FP=false;
}

void ActionConstantPool::Execute()
{
	throw "WIP2";
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
