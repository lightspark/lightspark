/**************************************************************************
    Lightspark, a free flash player implementation

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
#include "geometry.h"
#include "asobjects.h"

enum TAGTYPE {TAG=0,DISPLAY_LIST_TAG,SHOW_TAG,CONTROL_TAG,DICT_TAG,END_TAG};

void ignore(std::istream& i, int count);

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
	void skip(std::istream& in)
	{
		if((Header&0x3f)==0x3f)
			ignore(in,Length);
		else
			ignore(in,Header&0x3f);
	}
	virtual TAGTYPE getType(){ return TAG; }
	virtual void printInfo(int t=0){ std::cerr << (Header>>6) << std::endl; throw "No Info"; }
};

class EndTag:public Tag
{
public:
	EndTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType() { return END_TAG; }
};

class DisplayListTag: public Tag, public IDisplayListElem
{
public:
	DisplayListTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return DISPLAY_LIST_TAG; }
};

class DictionaryTag: public Tag
{
protected:
	std::vector<Shape> cached;
public:
	DictionaryTag(RECORDHEADER h,std::istream& s):Tag(h,s){ }
	virtual TAGTYPE getType(){ return DICT_TAG; }
	virtual int getId(){return 0;} 
};

class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return CONTROL_TAG; }
	virtual void execute()=0;
};

class DefineShapeTag: public DictionaryTag, public IRenderObject
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	void printInfo(int t=0);
};

class DefineShape2Tag: public DictionaryTag, public IRenderObject
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	void printInfo(int t=0);
};

class DefineShape3Tag: public DictionaryTag, public IRenderObject
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape3Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	void printInfo(int t=0);
};

class DefineMorphShapeTag: public DictionaryTag, public IRenderObject
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
	virtual void Render();
	void printInfo(int t=0);
};


class DefineEditTextTag: public DictionaryTag, public ASObject, public IRenderObject
{
private:
	UI16 CharacterID;
	RECT Bounds;
	UB HasText;
	UB WordWrap;
	UB Multiline;
	UB Password;
	UB ReadOnly;
	UB HasTextColor;
	UB HasMaxLength;
	UB HasFont;
	UB HasFontClass;
	UB AutoSize;
	UB HasLayout;
	UB NoSelect;
	UB Border;
	UB WasStatic;
	UB HTML;
	UB UseOutlines;
	UI16 FontID;
	STRING FontClass;
	UI16 FontHeight;
	RGBA TextColor;
	UI16 MaxLength;
	UI8 Align;
	UI16 LeftMargin;
	UI16 RightMargin;
	UI16 Indent;
	SI16 Leading;
	STRING VariableName;
	STRING InitialText;

public:
	DefineEditTextTag(RECORDHEADER h, std::istream& s);
	virtual int getId(){ return CharacterID; }
	virtual void Render();
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

class PlaceObject2Tag: public DisplayListTag, public ISWFObject_impl
{
private:
	Integer _x;
	Integer _y;
	Double _scalex;

	ISWFObject* wrapped;

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
	int getDepth() const { return Depth; }
	void printInfo(int t=0);

	//SWFObject interface
	STRING getName() { return Name;}
	SWFOBJECT_TYPE getObjectType(){ return T_PLACEOBJECT;}
	//Forwared to placed object, if valid
	SWFObject getVariableByName(const STRING& name);
	void setVariableByName(const STRING& name, const SWFObject& o);
};

class FrameLabelTag: public DisplayListTag
{
private:
	STRING Name;
public:
	FrameLabelTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const
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

class DefineButton2Tag: public DictionaryTag, public IActiveObject, public IRenderObject
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
	virtual void Render();
	virtual void MouseEvent(int x, int y);

	void printInfo(int t=0);
};

class KERNINGRECORD
{
};

class FontTag: public DictionaryTag
{
protected:
	UI16 FontID;
public:
	FontTag(RECORDHEADER h,std::istream& s):DictionaryTag(h,s){}
	virtual void genGliphShape(std::vector<Shape>& s, int glyph)=0;
	virtual TAGTYPE getType(){ return DICT_TAG; }
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
	virtual void genGliphShape(std::vector<Shape>& s, int glyph);
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
	virtual void genGliphShape(std::vector<Shape>& s, int glyph);
};

class DefineTextTag: public DictionaryTag, public IRenderObject
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
	virtual void Render();
	void printInfo(int t=0);
};

class DefineSpriteTag: public DictionaryTag, public ASMovieClip
{
private:
	UI16 SpriteID;
	UI16 FrameCount;
	//std::vector < Tag* > ControlTags;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	SWFOBJECT_TYPE getObjectType(){ return T_WRAPPED;}
	virtual int getId(){ return SpriteID; }
	void printInfo(int t=0);

	//ISWFObject interface
	ISWFObject* clone()
	{
		return new DefineSpriteTag(*this);
	}
};

class ProtectTag: public ControlTag
{
public:
	ProtectTag(RECORDHEADER h, std::istream& in);
	void execute( ){};
};

class DefineBitsLossless2Tag: public DictionaryTag
{
private:
	UI16 CharacterId;
	UI8 BitmapFormat;
	UI16 BitmapWidth;
	UI16 BitmapHeight;
	UI8 BitmapColorTableSize;
	//ZlibBitmapData;
public:
	DefineBitsLossless2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return CharacterId; }
};

class ExportAssetsTag: public Tag
{
public:
	ExportAssetsTag(RECORDHEADER h, std::istream& in);
};

class DefineVideoStreamTag: public DictionaryTag, public IRenderObject
{
private:
	UI16 CharacterID;
	UI16 NumFrames;
	UI16 Width;
	UI16 Height;
	UB VideoFlagsReserved;
	UB VideoFlagsDeblocking;
	UB VideoFlagsSmoothing;
	UI8 CodecID;
public:
	DefineVideoStreamTag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterID; }
	void Render();
};

class SoundStreamBlockTag: public Tag
{
public:
	SoundStreamBlockTag(RECORDHEADER h, std::istream& in);
};

class MetadataTag: public Tag
{
public:
	MetadataTag(RECORDHEADER h, std::istream& in);
};

class ScriptLimitsTag: public Tag
{
public:
	ScriptLimitsTag(RECORDHEADER h, std::istream& in);
};

//Documented by gnash
class SerialNumberTag: public Tag
{
public:
	SerialNumberTag(RECORDHEADER h, std::istream& in);
};

class FileAttributesTag: public Tag
{
public:
	FileAttributesTag(RECORDHEADER h, std::istream& in);
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
