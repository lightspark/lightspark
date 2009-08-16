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
#include "flashdisplay.h"
#include "flashtext.h"
#include <GL/gl.h>

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
};

class EndTag:public Tag
{
public:
	EndTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType() { return END_TAG; }
};

class DisplayListTag: public Tag
{
public:
	DisplayListTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return DISPLAY_LIST_TAG; }
	virtual void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& list)=0;
};

class DictionaryTag: public Tag
{
protected:
	std::vector<Shape> cached;
public:
	DictionaryTag(RECORDHEADER h,std::istream& s):Tag(h,s){ }
	virtual TAGTYPE getType(){ return DICT_TAG; }
	virtual int getId(){return 0;} 
	virtual IDisplayListElem* instance() { return NULL; } 
};

class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return CONTROL_TAG; }
	virtual void execute()=0;
};

class DefineShapeTag: public DictionaryTag, public IDisplayListElem
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();

	IDisplayListElem* instance()
	{
		return new DefineShapeTag(*this);
	}
};

class DefineShape2Tag: public DictionaryTag, public IDisplayListElem
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();

	IDisplayListElem* instance()
	{
		return new DefineShape2Tag(*this);
	}
};

class DefineShape3Tag: public DictionaryTag, public IDisplayListElem
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
	GLuint texture;
public:
	DefineShape3Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();

	IDisplayListElem* instance()
	{
		return new DefineShape3Tag(*this);
	}
};

class DefineShape4Tag: public DictionaryTag, public IDisplayListElem
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	RECT EdgeBounds;
	UB UsesFillWindingRule;
	UB UsesNonScalingStrokes;
	UB UsesScalingStrokes;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape4Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();

	IDisplayListElem* instance()
	{
		return new DefineShape4Tag(*this);
	}
};

class DefineMorphShapeTag: public DictionaryTag
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
};


class DefineEditTextTag: public DictionaryTag, public ASObject
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

class RemoveObject2Tag: public DisplayListTag
{
private:
	UI16 Depth;

public:
	RemoveObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& list);
};

class PlaceObject2Tag: public DisplayListTag
{
private:
	static bool list_orderer(const std::pair<PlaceInfo, IDisplayListElem*> a, int d);
//	Number _scalex;

//	ISWFObject* wrapped;
//	ISWFObject* parent;

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
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;

public:
	STRING Name;
	PlaceObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& list);
/*	void setWrapped(ISWFObject* w)
	{
		wrapped=w;
	}*/

};

class FrameLabelTag: public DisplayListTag
{
private:
	STRING Name;
public:
	FrameLabelTag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& list);
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

class DefineButton2Tag: public DictionaryTag, public IDisplayListElem
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
	virtual void handleEvent(Event*);

	IDisplayListElem* instance();
};

class KERNINGRECORD
{
};

class DefineBinaryDataTag: public DictionaryTag, public ASObject
{
private:
	UI16 Tag;
	UI32 Reserved;
	uint8_t* data;
public:
	DefineBinaryDataTag(RECORDHEADER h,std::istream& s);
	~DefineBinaryDataTag(){delete[] data;}
	virtual int getId(){return Tag;} 
};

class FontTag: public DictionaryTag, public ASFont
{
protected:
	UI16 FontID;
public:
	FontTag(RECORDHEADER h,std::istream& s):DictionaryTag(h,s){}
	virtual void genGlyphShape(std::vector<Shape>& s, int glyph)=0;
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
	virtual void genGlyphShape(std::vector<Shape>& s, int glyph);
};

class DefineFontInfoTag: public Tag
{
public:
	DefineFontInfoTag(RECORDHEADER h, std::istream& in);
};

class DefineFont2Tag: public FontTag
{
	friend class DefineTextTag; 
protected:
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
	virtual void genGlyphShape(std::vector<Shape>& s, int glyph);
};

class DefineFont3Tag: public DefineFont2Tag
{
public:
	DefineFont3Tag(RECORDHEADER h, std::istream& in):DefineFont2Tag(h,in){}
	virtual int getId(){ return FontID; }
	virtual void genGlyphShape(std::vector<Shape>& s, int glyph);
};

class DefineTextTag: public DictionaryTag, public IDisplayListElem
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

	IDisplayListElem* instance()
	{
		return new DefineTextTag(*this);
	}
};

class DefineSpriteTag: public DictionaryTag, public MovieClip
{
private:
	UI16 SpriteID;
	UI16 FrameCount;
	//std::vector < Tag* > ControlTags;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return SpriteID; }

	IDisplayListElem* instance()
	{
		return new DefineSpriteTag(*this);
	}

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

class DefineBitsLosslessTag: public DictionaryTag
{
private:
	UI16 CharacterId;
	UI8 BitmapFormat;
	UI16 BitmapWidth;
	UI16 BitmapHeight;
	UI8 BitmapColorTableSize;
	//ZlibBitmapData;
public:
	DefineBitsLosslessTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return CharacterId; }
};

class DefineBitsJPEG2Tag: public Tag
{
public:
	DefineBitsJPEG2Tag(RECORDHEADER h, std::istream& in);
};

class DefineBitsLossless2Tag: public DictionaryTag, public ASObject
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

class DefineScalingGridTag: public Tag
{
public:
	UI16 CharacterId;
	RECT Splitter;
	DefineScalingGridTag(RECORDHEADER h, std::istream& in);
};

class UnimplementedTag: public Tag
{
public:
	UnimplementedTag(RECORDHEADER h, std::istream& in);
};

class DefineSceneAndFrameLabelDataTag: public Tag
{
public:
	DefineSceneAndFrameLabelDataTag(RECORDHEADER h, std::istream& in);
};

class DefineFontNameTag: public Tag
{
public:
	DefineFontNameTag(RECORDHEADER h, std::istream& in);
};

class DefineFontAlignZonesTag: public Tag
{
public:
	DefineFontAlignZonesTag(RECORDHEADER h, std::istream& in);
};

class DefineVideoStreamTag: public DictionaryTag
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
private:
	UB UseDirectBlit;
	UB UseGPU;
	UB HasMetadata;
	UB ActionScript3;
	UB UseNetwork;
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
