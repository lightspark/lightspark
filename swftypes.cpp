#include "swftypes.h"

RECT::RECT()
{
}

RECT::RECT(FILE* in)
{
	BitStream s(in);
	NBits=UB(5,s);
	Xmin=SB(NBits,s);
	Xmax=SB(NBits,s);
	Ymin=SB(NBits,s);
	Ymax=SB(NBits,s);
}

std::ostream& operator<<(const std::ostream& s, const RECT& r)
{
	std::cout << "{" << (int)r.Xmin << "," << r.Xmax << "," << r.Ymin << "," << r.Ymax << "}";
}
