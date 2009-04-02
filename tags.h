/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef TAGS_H
#define TAGS_H

#include <list>
#include <vector>
#include <iostream>
#include "swftypes.h"
#include "input.h"

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
	virtual void printInfo(int t=0){ std::cerr << (Header>>6) << std::endl; throw "No Info"; }
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
	virtual UI16 getDepth() const=0;
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
	RenderTag(RECORDHEADER h,std::istream& s):Tag(h,s){ }
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
	void printInfo(int t=0);
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
	void printInfo(int t=0);
};

class DefineMorphShapeTag: public RenderTag
{
private:
	UI16 CharacterId;
	RECT StartBounds;
	RECT EndBounds;
	UI32 Offset;
	MORPHFILLSTYLEARRAY MorphFillStyles;
	MORPHLINESTYLEARRAY MorphLineStyles;
	SHAPE StartEdges;
	SHAPE EndEdges;
public:
	DefineMorphShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return CharacterId; }
	virtual void Render(int layer);
	void printInfo(int t=0);
};


class DefineEditTextTag: public Tag
{
public:
	DefineEditTextTag(RECORDHEADER h, std::istream& s);
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

class RemoveObject2Tag: public Tag
{
private:
	UI16 Depth;

public:
	RemoveObject2Tag(RECORDHEADER h, std::istream& in);
};

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
	UI16 getDepth() const
	{
		return Depth;
	}

	void printInfo(int t=0);
};

class FrameLabelTag: public DisplayListTag
{
private:
	STRING Name;
public:
	FrameLabelTag(RECORDHEADER h, std::istream& in);
	void Render( );
	UI16 getDepth() const
	{
		return 0;
	}

	void printInfo(int t=0){ std::cerr << "FrameLabel Info" << std::endl; }
};

class SetBackgroundColorTag: public ControlTag
{
private:
	RGB BackgroundColor;
public:
	SetBackgroundColorTag(RECORDHEADER h, std::istream& in);
	void execute( );
};

class SoundStreamHead2Tag: public Tag
{
public:
	SoundStreamHead2Tag(RECORDHEADER h, std::istream& in);
};

class BUTTONCONDACTION;

class DefineButton2Tag: public RenderTag, public IActiveObject
{
private:
	UI16 ButtonId;
	UB ReservedFlags;
	UB TrackAsMenu;
	UI16 ActionOffset;
	std::vector<BUTTONRECORD> Characters;
	std::vector<BUTTONCONDACTION> Actions;
	enum BUTTON_STATE { BUTTON_UP=0, BUTTON_OVER};
	BUTTON_STATE state;

	//Transition flags
	bool IdleToOverUp;
public:
	DefineButton2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ButtonId; }
	virtual void Render(int layer);
	virtual void MouseEvent(int x, int y);

	void printInfo(int t=0);
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
	virtual void Render(int glyph)=0;
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
	void Render(int glyph);
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
	void Render(int glyph);
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
	void printInfo(int t=0);
};

class DefineSpriteTag: public RenderTag
{
private:
	UI16 SpriteID;
	UI16 FrameCount;
	//std::vector < Tag* > ControlTags;
	MovieClip clip;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return SpriteID; }
	virtual void Render(int layer);
	void printInfo(int t=0);
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
