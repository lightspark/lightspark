#include "actions.h"

using namespace std;

DoActionTag::DoActionTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	ACTIONRECORDHEADER ah(in);
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
	throw "WIP";
}
