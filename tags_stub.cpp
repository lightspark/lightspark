#include <istream>
#include "tags.h"

using namespace std;

void ignore(istream& i, int count);

ProtectTag::ProtectTag(RECORDHEADER h, istream& in):ControlTag(h,in)
{
	std::cout << "STUB Protect" << std::endl;
	if((h&0x3f)==0x3f)
		ignore(in,Length);
	else
		ignore(in,h&0x3f);
}
