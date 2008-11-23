#include "swftypes.h"

RECT::RECT()
{
}

/*RECT::RECT(FILE* in)
{
	BitStream s(in);
	NBits=UB(5,s);
	Xmin=SB(NBits,s);
	Xmax=SB(NBits,s);
	Ymin=SB(NBits,s);
	Ymax=SB(NBits,s);
}*/

std::ostream& operator<<(const std::ostream& s, const RECT& r)
{
	std::cout << "{" << (int)r.Xmin << "," << r.Xmax << "," << r.Ymin << "," << r.Ymax << "}";
}

std::istream& operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	v.NBits=UB(5,s);
	v.Xmin=SB(v.NBits,s);
	v.Xmax=SB(v.NBits,s);
	v.Ymin=SB(v.NBits,s);
	v.Ymax=SB(v.NBits,s);
	return stream;
}
