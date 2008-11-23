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
	std::cout << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
}

std::ostream& operator<<(const std::ostream& s, const RGB& r)
{
	std::cout << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
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

std::istream& operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		throw "Line array extended not supported\n";
	v.LineStyles=new LINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		throw "Fill array extended not supported\n";
	v.FillStyles=new FILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	s >> v.FillStyles >> v.LineStyles;
	throw "help2";
}

std::istream& operator>>(std::istream& s, LINESTYLE& v)
{
	s >> v.Width >> v.Color;
	std::cout << "Line " << v.Width/20 << v.Color;
	return s;
}

std::istream& operator>>(std::istream& s, FILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType!=0)
		throw "unsupported fill type";
	s >> v.Color;
	std::cout << v.Color << std::endl;
	return s;
}
