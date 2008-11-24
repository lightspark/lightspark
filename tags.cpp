#include "tags.h"
#include "swftypes.h"

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	switch(h>>6)
	{
		case 2:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineShapeTag(h,f);
	//	case 4:
	//		return new PlaceObjectTag(h,f);
		case 9:
			std::cout << "position " << f.tellg() << std::endl;
			return new SetBackgroundColorTag(h,f);
		case 45:
			std::cout << "position " << f.tellg() << std::endl;
			return new SoundStreamHead2Tag(h,f);

		default:
			std::cout << "position " << f.tellg() << std::endl;
			std::cout << (h>>6) << std::endl;
			throw "unsupported tag";
			break;
	}
}


SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	in >> BackgroundColor;
	std::cout << BackgroundColor << std::endl;
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB SoundStreamHead2" << std::endl;
	in.ignore(h&0x3f);
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::streampos start=in.tellg();
	in >> ShapeId >> ShapeBounds >> Shapes;
	std::streampos end=in.tellg();
	std::cout << "tag len " << end-start << " " << getSize() << std::endl;
}

PlaceObjectTag::PlaceObjectTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << std::hex << Header << std::endl;
	in >> CharacterId;
	std::cout << CharacterId << std::endl;
	throw "help3";
}

