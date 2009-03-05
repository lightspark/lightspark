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
	throw "Action getdepth";
	return 0;
}

void DoActionTag::Render()
{
	throw "Action render";
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
		case 129:
			return new ActionGotoFrame(in);
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
	state.FP=-1;
}

ActionGotoFrame::ActionGotoFrame(std::istream& in)
{
	in >> Frame;
	cout << "dest " << Frame << endl;
}

void ActionGotoFrame::Execute()
{
	state.FP=Frame-1;
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
