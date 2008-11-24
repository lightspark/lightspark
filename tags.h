#ifndef TAGS_H
#define TAGS_H

#include <iostream>
#include "swftypes.h"

class Tag
{
protected:
	RECORDHEADER Header;
	SI32 Length;
public:
	Tag(RECORDHEADER h, std::istream& s):Header(h)
	{
		if((Header&0x3f)==0x3f)
		{
			std::cout << "long tag" << std::endl;
			s >> Length;
		}
	}
	SI32 getSize()
	{
		if((Header&0x3f)==0x3f)
			return Length;
		else
			return Header&0x3f;
	}
};

class DefineShapeTag: public Tag
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
};

class ShowFrameTag: public Tag
{
public:
	ShowFrameTag(RECORDHEADER h, std::istream& in);
};

/*class PlaceObjectTag: public Tag
{
private:
	UI16 CharacterId;
	UI16 Depth;
	MATRIX Matrix;
	CXFORM ColorTransform;
public:
	PlaceObjectTag(RECORDHEADER h, std::istream& in);
};*/


class PlaceObject2Tag: public Tag
{
private:
	UB PlaceFlagHasClipAction;
	UB PlaceFlagHasClipDepth;
	UB PlaceFlagHasName;
	UB PlaceFlagHasRatio;
	UB PlaceFlagHasColorTransform;
	UB PlaceFlagHasMatrix;
	UB PlaceFlagHasCharacter;
	UB PlaceFlagMove;
	UI16 Depth;
	UI16 CharacterId;
	MATRIX Matrix;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	STRING Name;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;

public:
	PlaceObject2Tag(RECORDHEADER h, std::istream& in);
};

class SetBackgroundColorTag: public Tag
{
private:
	RGB BackgroundColor;
public:
	SetBackgroundColorTag(RECORDHEADER h, std::istream& in);
};

class SoundStreamHead2Tag: public Tag
{
public:
	SoundStreamHead2Tag(RECORDHEADER h, std::istream& in);
};

class DefineFont2Tag: public Tag
{
public:
	DefineFont2Tag(RECORDHEADER h, std::istream& in);
};

class DefineTextTag: public Tag
{
public:
	DefineTextTag(RECORDHEADER h, std::istream& in);
};

class DefineSpriteTag: public Tag
{
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
};


class TagFactory
{
private:
	std::istream& f;
public:
	TagFactory(std::istream& in):f(in){}
	Tag* readTag();
};

#endif
