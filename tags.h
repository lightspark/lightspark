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
			s >> Length;
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


class TagFactory
{
private:
	std::istream& f;
public:
	TagFactory(std::istream& in):f(in){}
	Tag* readTag();
};

#endif
