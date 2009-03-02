#include "actions.h"

using namespace std;

extern State state;

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	while(1)
	{
		ACTIONRECORDHEADER ah(in);
		switch(ah.ActionCode)
		{
			case 0:
				return;
			case 7:
				actions.push_back(new ActionStop);
				break;
			default:
				cout << ah.ActionCode << endl;
				throw "Unsopported actioncode";
		}
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
	cout << (int)ActionCode << endl;
}

State::State():FP(0)
{
}

void ActionStop::Execute()
{
	state.FP=-1;
}
