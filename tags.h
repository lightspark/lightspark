#ifndef TAGS_H
#define TAGS_H

#include <list>
#include <vector>
#include <iostream>
#include "swftypes.h"

enum TAGTYPE {TAG=0,DISPLAY_LIST_TAG,SHOW_TAG,CONTROL_TAG,RENDER_TAG,END_TAG};

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
	virtual TAGTYPE getType(){ return TAG; }
	//virtual int getId(){return 0;} 
};

class EndTag:public Tag
{
public:
	EndTag(RECORDHEADER h, std::istream& s):Tag(h,s){ std::cout <<"endtag" <<std::endl;}
	virtual TAGTYPE getType() { return END_TAG; }
};

class DisplayListTag: public Tag
{
public:
	bool add_to_list;
	DisplayListTag(RECORDHEADER h, std::istream& s):Tag(h,s),add_to_list(true){}
	virtual TAGTYPE getType(){ return DISPLAY_LIST_TAG; }
	virtual UI16 getDepth()=0;
	virtual void Render()=0;
};

class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return CONTROL_TAG; }
	virtual void execute()=0;
};

class RenderTag: public Tag
{
public:
	RenderTag(RECORDHEADER h,std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return RENDER_TAG; }
	virtual int getId(){return 0;} 
	virtual void Render(int layer){std::cout << "default render" << std::endl; };
};

class DefineShapeTag: public RenderTag
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render(int layer);
};

class DefineShape2Tag: public RenderTag
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render(int layer);
};

class DefineSoundTag: public Tag
{
public:
	DefineSoundTag(RECORDHEADER h, std::istream& s);
};

class StartSoundTag: public Tag
{
public:
	StartSoundTag(RECORDHEADER h, std::istream& s);
};

class ShowFrameTag: public Tag
{
public:
	ShowFrameTag(RECORDHEADER h, std::istream& in);
	virtual TAGTYPE getType(){ return SHOW_TAG; }
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


class PlaceObject2Tag: public DisplayListTag
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
	void Render( );
	UI16 getDepth()
	{
		return Depth;
	}
};

class SetBackgroundColorTag: public ControlTag
{
private:
	RGB BackgroundColor;
public:
	SetBackgroundColorTag(RECORDHEADER h, std::istream& in);
	virtual void execute();
};

class SoundStreamHead2Tag: public Tag
{
public:
	SoundStreamHead2Tag(RECORDHEADER h, std::istream& in);
};

class KERNINGRECORD
{
};

class FontTag: public RenderTag
{
protected:
	UI16 FontID;
public:
	FontTag(RECORDHEADER h,std::istream& s):RenderTag(h,s){}
	virtual void Render(int glyph,const RGBA& color,int layer)=0;
};

class DefineFontTag: public FontTag
{
	friend class DefineTextTag; 
protected:
	std::vector<UI16> OffsetTable;
	std::vector < SHAPE > GlyphShapeTable;

public:
	DefineFontTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return FontID; }
	void Render(int glyph,const RGBA& color,int layer);
};

class DefineFontInfoTag: public Tag
{
public:
	DefineFontInfoTag(RECORDHEADER h, std::istream& in);
};


class DefineFont2Tag: public FontTag
{
	friend class DefineTextTag; 
private:
	std::vector<UI32> OffsetTable;
	std::vector < SHAPE > GlyphShapeTable;
	UB FontFlagsHasLayout;
	UB FontFlagsShiftJIS;
	UB FontFlagsSmallText;
	UB FontFlagsANSI;
	UB FontFlagsWideOffsets;
	UB FontFlagsWideCodes;
	UB FontFlagsItalic;
	UB FontFlagsBold;
	LANGCODE LanguageCode;
	UI8 FontNameLen;
	std::vector <UI8> FontName;
	UI16 NumGlyphs;
	UI32 CodeTableOffset;
	std::vector <UI16> CodeTable;
	SI16 FontAscent;
	SI16 FontDescent;
	SI16 FontLeading;
	std::vector < SI16 > FontAdvanceTable;
	std::vector < RECT > FontBoundsTable;
	UI16 KerningCount;
	std::vector <KERNINGRECORD> FontKerningTable;

public:
	DefineFont2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return FontID; }
	void Render(int glyph ,const RGBA& color,int layer);
};

class DefineTextTag: public RenderTag
{
	friend class GLYPHENTRY;
private:
	UI16 CharacterId;
	RECT TextBounds;
	MATRIX TextMatrix;
	UI8 GlyphBits;
	UI8 AdvanceBits;
	std::vector < TEXTRECORD > TextRecords;
public:
	DefineTextTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return CharacterId; }
	virtual void Render(int layer);
};

class DefineSpriteTag: public RenderTag
{
private:
	UI16 SpriteID;
	UI16 FrameCount;
	//std::vector < Tag* > ControlTags;
	std::list < DisplayListTag* > displayList;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return SpriteID; }
	virtual void Render(int layer);
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
