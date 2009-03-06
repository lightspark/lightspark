#include "actions.h"

using namespace std;

extern RunState state;

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

UI16 DoActionTag::getDepth()
{
	return 0;
}

void DoActionTag::Render()
{
	for(int i=0;i<actions.size();i++)
		actions[i]->Execute();
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
		case 7:
			return new ActionStop;
			break;
		case 0x81:
			return new ActionGotoFrame(in);
			break;
		case 0x83:
			return new ActionGetURL(in);
			break;
		default:
			cout << (int)ActionCode << endl;
			throw "Unsopported actioncode";
			return NULL;
	}
}

RunState::RunState():FP(0)
{
}

void ActionStop::Execute()
{
	cout << "Stop" << endl;
	state.next_FP=-1;
}

ActionGotoFrame::ActionGotoFrame(std::istream& in)
{
	in >> Frame;
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
	state.next_FP=Frame;
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
