#include "tags.h"
#include "swftypes.h"

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	switch(h>>6)
	{
		case 2:
			return new DefineShapeTag(h,f);
		case 9:
			return new SetBackgroundColorTag(h,f);
		case 45:
			return new SoundStreamHead2Tag(h,f);

		default:
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
	in >> ShapeId >> ShapeBounds >> Shapes;
	std::cout << ShapeBounds << std::endl;
}
