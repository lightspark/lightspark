#include "tags.h"
#include "swftypes.h"

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	switch(h>>6)
	{
		case 1:
			std::cout << "position " << f.tellg() << std::endl;
			return new ShowFrameTag(h,f);
		case 2:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineShapeTag(h,f);
	//	case 4:
	//		return new PlaceObjectTag(h,f);
		case 9:
			std::cout << "position " << f.tellg() << std::endl;
			return new SetBackgroundColorTag(h,f);
		case 11:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineTextTag(h,f);
		case 26:
			std::cout << "position " << f.tellg() << std::endl;
			return new PlaceObject2Tag(h,f);
		case 39:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineSpriteTag(h,f);
		case 45:
			std::cout << "position " << f.tellg() << std::endl;
			return new SoundStreamHead2Tag(h,f);
		case 48:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineFont2Tag(h,f);
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

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineSprite" << std::endl;
	in.ignore(getSize());
}

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineFont2" << std::endl;
	in.ignore(getSize());
}

DefineTextTag::DefineTextTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineText" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB SoundStreamHead2" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	in >> ShapeId >> ShapeBounds >> Shapes;
}

/*PlaceObjectTag::PlaceObjectTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << std::hex << Header << std::endl;
	in >> CharacterId;
	std::cout << CharacterId << std::endl;
	throw "help3";
}*/

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	BitStream bs(in);
	PlaceFlagHasClipAction=UB(1,bs);
	PlaceFlagHasClipDepth=UB(1,bs);
	PlaceFlagHasName=UB(1,bs);
	PlaceFlagHasRatio=UB(1,bs);
	PlaceFlagHasColorTransform=UB(1,bs);
	PlaceFlagHasMatrix=UB(1,bs);
	PlaceFlagHasCharacter=UB(1,bs);
	PlaceFlagMove=UB(1,bs);
	in >> Depth;
	if(PlaceFlagHasClipAction+PlaceFlagHasName+PlaceFlagHasRatio+PlaceFlagMove)
	{
		throw "unsupported has";
	}
	if(PlaceFlagHasCharacter)
		in >> CharacterId;
	if(PlaceFlagHasMatrix)
		in >> Matrix;
	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;
	if(PlaceFlagHasColorTransform)
		in >> ColorTransform;
}

