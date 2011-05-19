/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef TAGS_H
#define TAGS_H

#include "compat.h"
#include <list>
#include <vector>
#include <iostream>
#include "swftypes.h"
#include "backends/input.h"
#include "backends/geometry.h"
#include "scripting/flashdisplay.h"
#include "scripting/flashtext.h"
#include "scripting/flashutils.h"
#include "scripting/flashmedia.h"
#include "scripting/class.h"

namespace lightspark
{

enum TAGTYPE {TAG=0,DISPLAY_LIST_TAG,SHOW_TAG,CONTROL_TAG,DICT_TAG,FRAMELABEL_TAG,ABC_TAG,END_TAG};

void ignore(std::istream& i, int count);

class Tag
{
protected:
	RECORDHEADER Header;
	void skip(std::istream& in) const
	{
		ignore(in,Header.getLength());
	}
public:
	Tag(RECORDHEADER h):Header(h)
	{
	}
	virtual TAGTYPE getType() const{ return TAG; }
	virtual ~Tag(){}
};

class EndTag:public Tag
{
public:
	EndTag(RECORDHEADER h, std::istream& s):Tag(h){}
	virtual TAGTYPE getType() const{ return END_TAG; }
};

class DisplayListTag: public Tag
{
public:
	DisplayListTag(RECORDHEADER h):Tag(h){}
	virtual TAGTYPE getType() const{ return DISPLAY_LIST_TAG; }
	virtual void execute(DisplayObjectContainer* parent)=0;
};

class DictionaryTag: public Tag
{
public:
	/*
	   Pointer to the class we are binded to

	   This must ONLY be used when creating instances of the tag
           There may be more than a class binded to the same tag. Only
	   the first one is reported here. This behaviour is tested.
	*/
	Class_base* bindedTo;
	RootMovieClip* loadedFrom;
	DictionaryTag(RECORDHEADER h):Tag(h),bindedTo(NULL),loadedFrom(NULL){ }
	virtual TAGTYPE getType()const{ return DICT_TAG; }
	virtual int getId()=0;
	virtual ASObject* instance() const { return NULL; };
	void setLoadedFrom(RootMovieClip* r){loadedFrom=r;}
};

/*
 * See p.53ff in the SWF spec. Those tags are ::executed directly after parsing
 * and then delete'ed.
 */
class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h):Tag(h){}
	virtual TAGTYPE getType()const{ return CONTROL_TAG; }
	virtual void execute(RootMovieClip* root)=0;
};

class DefineShapeTag: public DictionaryTag
{
protected:
	UI16_SWF ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
	/* tokens are computed from Shapes */
	std::vector<GeomToken> tokens;
	DefineShapeTag(RECORDHEADER h,int v):DictionaryTag(h),Shapes(v) {}
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	ASObject* instance() const
	{
		Shape* ret=new Shape(tokens, 1.0f/20.0f);
		ret->setPrototype(Class<Shape>::getClass());
		return ret;
	}
};

class DefineShape2Tag: public DefineShapeTag
{
protected:
	DefineShape2Tag(RECORDHEADER h, int v):DefineShapeTag(h,v){};
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
};

class DefineShape3Tag: public DefineShape2Tag
{
protected:
	DefineShape3Tag(RECORDHEADER h,int v):DefineShape2Tag(h,v){};
public:
	DefineShape3Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
};

class DefineShape4Tag: public DefineShape3Tag
{
private:
	RECT EdgeBounds;
	UB UsesFillWindingRule;
	UB UsesNonScalingStrokes;
	UB UsesScalingStrokes;
public:
	DefineShape4Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
};

class DefineMorphShapeTag: public DictionaryTag, public MorphShape
{
private:
	UI16_SWF CharacterId;
	RECT StartBounds;
	RECT EndBounds;
	UI32_SWF Offset;
	MORPHFILLSTYLEARRAY MorphFillStyles;
	MORPHLINESTYLEARRAY MorphLineStyles;
	SHAPE StartEdges;
	SHAPE EndEdges;
public:
	DefineMorphShapeTag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterId; }
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		return false;
	}
	virtual ASObject* instance() const;
};


class DefineEditTextTag: public DictionaryTag, public TextField
{
private:
	UI16_SWF CharacterID;
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
	UI16_SWF FontID;
	STRING FontClass;
	UI16_SWF FontHeight;
	RGBA TextColor;
	UI16_SWF MaxLength;
	UI8 Align;
	UI16_SWF LeftMargin;
	UI16_SWF RightMargin;
	UI16_SWF Indent;
	SI16_SWF Leading;
	STRING VariableName;
	STRING InitialText;

public:
	DefineEditTextTag(RECORDHEADER h, std::istream& s);
	int getId(){ return CharacterID; }
	void Render(bool maskEnabled);
	ASObject* instance() const;
};

class DefineSoundTag: public DictionaryTag, public Sound
{
private:
	UI16_SWF SoundId;
	char SoundFormat;
	char SoundRate;
	char SoundSize;
	char SoundType;
	UI32_SWF SoundSampleCount;
public:
	DefineSoundTag(RECORDHEADER h, std::istream& s);
	virtual int getId() { return SoundId; }
	ASObject* instance() const;
};

class StartSoundTag: public Tag
{
public:
	StartSoundTag(RECORDHEADER h, std::istream& s);
};

class SoundStreamHeadTag: public Tag
{
public:
	SoundStreamHeadTag(RECORDHEADER h, std::istream& s);
};

class ShowFrameTag: public Tag
{
public:
	ShowFrameTag(RECORDHEADER h, std::istream& in);
	virtual TAGTYPE getType()const{ return SHOW_TAG; }
};

/*class PlaceObjectTag: public Tag
{
private:
	UI16_SWF CharacterId;
	UI16_SWF Depth;
	MATRIX Matrix;
	CXFORM ColorTransform;
public:
	PlaceObjectTag(RECORDHEADER h, std::istream& in);
};*/

class RemoveObject2Tag: public DisplayListTag
{
private:
	UI16_SWF Depth;

public:
	RemoveObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(DisplayObjectContainer* parent);
};

class PlaceObject2Tag: public DisplayListTag
{
protected:
	bool PlaceFlagHasClipAction;
	bool PlaceFlagHasClipDepth;
	bool PlaceFlagHasName;
	bool PlaceFlagHasRatio;
	bool PlaceFlagHasColorTransform;
	bool PlaceFlagHasMatrix;
	bool PlaceFlagHasCharacter;
	bool PlaceFlagMove;
	UI16_SWF Depth;
	UI16_SWF CharacterId;
	MATRIX Matrix;
	CXFORMWITHALPHA ColorTransform;
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
	CLIPACTIONS ClipActions;
	PlaceObject2Tag(RECORDHEADER h):DisplayListTag(h){}
	void setProperties(DisplayObject* obj, DisplayObjectContainer* parent) const;

public:
	STRING Name;
	PlaceObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(DisplayObjectContainer* parent);
};

class PlaceObject3Tag: public PlaceObject2Tag
{
private:
	bool PlaceFlagHasImage;
	bool PlaceFlagHasClassName;
	bool PlaceFlagHasCacheAsBitmap;
	bool PlaceFlagHasBlendMode;
	bool PlaceFlagHasFilterList;
	STRING ClassName;
	FILTERLIST SurfaceFilterList;
	UI8 BlendMode;
	UI8 BitmapCache;

public:
	PlaceObject3Tag(RECORDHEADER h, std::istream& in);
};

class FrameLabelTag: public Tag
{
public:
	STRING Name;
	FrameLabelTag(RECORDHEADER h, std::istream& in);
	virtual TAGTYPE getType()const{ return FRAMELABEL_TAG; }
};

class SetBackgroundColorTag: public ControlTag
{
private:
	RGB BackgroundColor;
public:
	SetBackgroundColorTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root);
};

class SoundStreamHead2Tag: public Tag
{
public:
	SoundStreamHead2Tag(RECORDHEADER h, std::istream& in);
};

class BUTTONCONDACTION;

class DefineButton2Tag: public DictionaryTag, public DisplayObject
{
private:
	UI16_SWF ButtonId;
	UB ReservedFlags;
	UB TrackAsMenu;
	UI16_SWF ActionOffset;
	std::vector<BUTTONRECORD> Characters;
	std::vector<BUTTONCONDACTION> Actions;
	enum BUTTON_STATE { BUTTON_UP=0, BUTTON_OVER};
	BUTTON_STATE state;

	//Transition flags
	bool IdleToOverUp;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
public:
	DefineButton2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ButtonId; }
	void Render(bool maskEnabled);
	virtual void handleEvent(Event*);

	ASObject* instance() const;
};

class KERNINGRECORD
{
};

class DefineBinaryDataTag: public DictionaryTag, public ByteArray
{
private:
	UI16_SWF Tag;
	UI32_SWF Reserved;
public:
	DefineBinaryDataTag(RECORDHEADER h,std::istream& s);
	virtual int getId(){return Tag;} 

	ASObject* instance() const
	{
		DefineBinaryDataTag* ret=new DefineBinaryDataTag(*this);
		ret->setPrototype(Class<ByteArray>::getClass());
		return ret;
	}
};

class FontTag: public DictionaryTag, public Font
{
protected:
	UI16_SWF FontID;
	std::vector<SHAPE> GlyphShapeTable;
public:
	/* Multiply the coordinates of the SHAPEs by this
	 * value to get a resolution of 1024*20th pixel
	 * DefineFont3Tag sets 1 here, the rest set 20
	 */
	const int scaling;
	FontTag(RECORDHEADER h, int _scaling):DictionaryTag(h), scaling(_scaling) {}
	const std::vector<SHAPE>& getGlyphShapes() const
	{
		return GlyphShapeTable;
	}
	int getId() { return FontID; }
};

class DefineFontTag: public FontTag
{
	friend class DefineTextTag; 
protected:
	std::vector<uint16_t> OffsetTable;
public:
	DefineFontTag(RECORDHEADER h, std::istream& in);
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
	std::vector<uint32_t> OffsetTable;
	bool FontFlagsHasLayout;
	bool FontFlagsShiftJIS;
	bool FontFlagsSmallText;
	bool FontFlagsANSI;
	bool FontFlagsWideOffsets;
	bool FontFlagsWideCodes;
	bool FontFlagsItalic;
	bool FontFlagsBold;
	LANGCODE LanguageCode;
	UI8 FontNameLen;
	std::vector <UI8> FontName;
	UI16_SWF NumGlyphs;
	uint32_t CodeTableOffset;
	std::vector <uint16_t> CodeTable;
	SI16_SWF FontAscent;
	SI16_SWF FontDescent;
	SI16_SWF FontLeading;
	std::vector < SI16_SWF > FontAdvanceTable;
	std::vector < RECT > FontBoundsTable;
	UI16_SWF KerningCount;
	std::vector <KERNINGRECORD> FontKerningTable;

public:
	DefineFont2Tag(RECORDHEADER h, std::istream& in);
};

class DefineFont3Tag: public FontTag
{
private:
	std::vector<uint32_t> OffsetTable;
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
	UI16_SWF NumGlyphs;
	uint32_t CodeTableOffset;
	std::vector <UI16_SWF> CodeTable;
	SI16_SWF FontAscent;
	SI16_SWF FontDescent;
	SI16_SWF FontLeading;
	std::vector < SI16_SWF > FontAdvanceTable;
	std::vector < RECT > FontBoundsTable;
	UI16_SWF KerningCount;
	std::vector <KERNINGRECORD> FontKerningTable;

public:
	DefineFont3Tag(RECORDHEADER h, std::istream& in);
};

class DefineFont4Tag : public DictionaryTag
{
private:
	UI16_SWF FontID;
	UB FontFlagsHasFontData;
	UB FontFlagsItalic;
	UB FontFlagsBold;
	STRING FontName;
public:
	DefineFont4Tag(RECORDHEADER h, std::istream& in);
	virtual int getId() { return FontID; }
};

class DefineTextTag: public DictionaryTag
{
	friend class GLYPHENTRY;
private:
	UI16_SWF CharacterId;
	RECT TextBounds;
	MATRIX TextMatrix;
	UI8 GlyphBits;
	UI8 AdvanceBits;
	std::vector < TEXTRECORD > TextRecords;
	mutable std::vector<GeomToken> tokens;
	void computeCached() const;
public:
	int version;
	DefineTextTag(RECORDHEADER h, std::istream& in,int v=1);
	int getId(){ return CharacterId; }
	ASObject* instance() const;
};

class DefineText2Tag: public DefineTextTag
{
public:
	DefineText2Tag(RECORDHEADER h, std::istream& in) : DefineTextTag(h,in,2) {}
};

class DefineSpriteTag: public DictionaryTag, public MovieClip
{
private:
	UI16_SWF SpriteID;
	UI16_SWF FrameCount;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return SpriteID; }
	virtual ASObject* instance() const;
};

class ProtectTag: public ControlTag
{
public:
	ProtectTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root){};
};

class DefineBitsLosslessTag: public DictionaryTag, public Bitmap
{
private:
	UI16_SWF CharacterId;
	UI8 BitmapFormat;
	UI16_SWF BitmapWidth;
	UI16_SWF BitmapHeight;
	UI8 BitmapColorTableSize;
	//ZlibBitmapData;
public:
	DefineBitsLosslessTag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterId; }
};

class DefineBitsTag: public DictionaryTag, public Bitmap
{
private:
	UI16_SWF CharacterId;
	uint8_t* data;
public:
	DefineBitsTag(RECORDHEADER h, std::istream& in);
	~DefineBitsTag();
	int getId(){ return CharacterId; }
};

class DefineBitsJPEG2Tag: public DictionaryTag, public Bitmap
{
private:
	UI16_SWF CharacterId;
public:
	DefineBitsJPEG2Tag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterId; }
};

class DefineBitsJPEG3Tag: public DictionaryTag, public Bitmap
{
private:
	UI16_SWF CharacterId;
	uint8_t* alphaData;
public:
	DefineBitsJPEG3Tag(RECORDHEADER h, std::istream& in);
	~DefineBitsJPEG3Tag();
	int getId(){ return CharacterId; }
};

class DefineBitsLossless2Tag: public DictionaryTag, public Bitmap
{
private:
	UI16_SWF CharacterId;
	UI8 BitmapFormat;
	UI16_SWF BitmapWidth;
	UI16_SWF BitmapHeight;
	UI8 BitmapColorTableSize;
	//ZlibBitmapData;
public:
	DefineBitsLossless2Tag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterId; }
	ASObject* instance() const;
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		return false;
	}
	void Render(bool maskEnabled);
};

class DefineScalingGridTag: public Tag
{
public:
	UI16_SWF CharacterId;
	RECT Splitter;
	DefineScalingGridTag(RECORDHEADER h, std::istream& in);
};

class UnimplementedTag: public Tag
{
public:
	UnimplementedTag(RECORDHEADER h, std::istream& in);
};

class DefineSceneAndFrameLabelDataTag: public ControlTag
{
public:
	u32 SceneCount;
	std::vector<u32> Offset;
	std::vector<STRING> Name;
	u32 FrameLabelCount;
	std::vector<u32> FrameNum;
	std::vector<STRING> FrameLabel;
	DefineSceneAndFrameLabelDataTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root);
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

class DefineVideoStreamTag: public DictionaryTag, public Video
{
private:
	UI16_SWF CharacterID;
	UI16_SWF NumFrames;
	UI16_SWF Width;
	UI16_SWF Height;
	UB VideoFlagsReserved;
	UB VideoFlagsDeblocking;
	UB VideoFlagsSmoothing;
	UI8 CodecID;
public:
	DefineVideoStreamTag(RECORDHEADER h, std::istream& in);
	int getId(){ return CharacterID; }
	ASObject* instance() const;
};

class SoundStreamBlockTag: public Tag
{
public:
	SoundStreamBlockTag(RECORDHEADER h, std::istream& in);
};

class MetadataTag: public Tag
{
private:
	STRING XmlString;
public:
	MetadataTag(RECORDHEADER h, std::istream& in);
};

class CSMTextSettingsTag: public Tag
{
public:
	CSMTextSettingsTag(RECORDHEADER h, std::istream& in);
};

class ScriptLimitsTag: public Tag
{
private:
	UI16_SWF MaxRecursionDepth;
	UI16_SWF ScriptTimeoutSeconds;
public:
	ScriptLimitsTag(RECORDHEADER h, std::istream& in);
};

class ProductInfoTag: public Tag
{
private:
	UI32_SWF ProductId;
	UI32_SWF Edition;
	UI8 MajorVersion;
	UI8 MinorVersion;
	UI32_SWF MinorBuild;
	UI32_SWF MajorBuild;
	UI32_SWF CompileTimeHi, CompileTimeLo;
public:
	ProductInfoTag(RECORDHEADER h, std::istream& in);
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

class EnableDebuggerTag: public Tag
{
private:
	STRING DebugPassword;
public:
	EnableDebuggerTag(RECORDHEADER h, std::istream& in);
};

class EnableDebugger2Tag: public Tag
{
private:
	UI16_SWF ReservedWord;
	STRING DebugPassword;
public:
	EnableDebugger2Tag(RECORDHEADER h, std::istream& in);
};

class DebugIDTag: public Tag
{
private:
	UI8 DebugId[16];
public:
	DebugIDTag(RECORDHEADER h, std::istream& in);
};

class TagFactory
{
private:
	std::istream& f;
	bool firstTag;
	bool topLevel;
public:
	TagFactory(std::istream& in, bool t):f(in),firstTag(true),topLevel(t){}
	Tag* readTag();
};

};

#endif
