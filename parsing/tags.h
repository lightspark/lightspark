/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

enum TAGTYPE {TAG=0,DISPLAY_LIST_TAG,SHOW_TAG,CONTROL_TAG,DICT_TAG,FRAMELABEL_TAG,END_TAG};

void ignore(std::istream& i, int count);
void FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords, std::vector<GeomShape>& shapes);

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
	virtual void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list)=0;
};

class DictionaryTag: public Tag
{
protected:
	std::vector<GeomShape> cached;
public:
	Class_base* bindedTo;
	RootMovieClip* loadedFrom;
	DictionaryTag(RECORDHEADER h):Tag(h),bindedTo(NULL),loadedFrom(NULL){ }
	virtual TAGTYPE getType()const{ return DICT_TAG; }
	virtual int getId()=0;
	virtual ASObject* instance() const { return NULL; };
	void setLoadedFrom(RootMovieClip* r){loadedFrom=r;}
};

class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h):Tag(h){}
	virtual TAGTYPE getType()const{ return CONTROL_TAG; }
	virtual void execute(RootMovieClip* root)=0;
};

class DefineShapeTag: public DictionaryTag, public DisplayObject
{
protected:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
	DefineShapeTag(RECORDHEADER h):DictionaryTag(h){};
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	virtual void inputRender();
	virtual Vector2 debugRender(FTFont* font, bool deep);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		//Apply transformation with the current matrix
		getMatrix().multiply2D(ShapeBounds.Xmin/20,ShapeBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(ShapeBounds.Xmax/20,ShapeBounds.Ymax/20,xmax,ymax);
		//TODO: adapt for rotation
		return true;
	}

	ASObject* instance() const
	{
		DefineShapeTag* ret=new DefineShapeTag(*this);
		ret->setPrototype(Class<Shape>::getClass());
		return ret;
	}
};

class DefineShape2Tag: public DefineShapeTag
{
protected:
	DefineShape2Tag(RECORDHEADER h):DefineShapeTag(h){};
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
	virtual Vector2 debugRender(FTFont* font, bool deep);
	ASObject* instance() const
	{
		DefineShape2Tag* ret=new DefineShape2Tag(*this);
		ret->setPrototype(Class<Shape>::getClass());
		return ret;
	}
};

class DefineShape3Tag: public DefineShape2Tag
{
protected:
	DefineShape3Tag(RECORDHEADER h):DefineShape2Tag(h){};
public:
	DefineShape3Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual Vector2 debugRender(FTFont* font, bool deep);
	ASObject* instance() const
	{
		DefineShape3Tag* ret=new DefineShape3Tag(*this);
		ret->setPrototype(Class<Shape>::getClass());
		return ret;
	}
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
	ASObject* instance() const
	{
		DefineShape4Tag* ret=new DefineShape4Tag(*this);
		ret->setPrototype(Class<Shape>::getClass());
		return ret;
	}
};

class DefineMorphShapeTag: public DictionaryTag, public MorphShape
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
	virtual ASObject* instance() const;
};


class DefineEditTextTag: public DictionaryTag, public TextField
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
	virtual Vector2 debugRender(FTFont* font, bool deep);
	ASObject* instance() const;
};

class DefineSoundTag: public DictionaryTag, public Sound
{
private:
	UI16 SoundId;
	char SoundFormat;
	char SoundRate;
	char SoundSize;
	char SoundType;
	UI32 SoundSampleCount;
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
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list);
};

class PlaceObject2Tag: public DisplayListTag
{
protected:
	class list_orderer
	{
	public:
		bool operator()(const std::pair<PlaceInfo, DisplayObject*>& a, int d);
		bool operator()(int d, const std::pair<PlaceInfo, DisplayObject*>& a);
		bool operator()(const std::pair<PlaceInfo, DisplayObject*>& a, const std::pair<PlaceInfo, DisplayObject*>& b);
	};

	bool PlaceFlagHasClipAction;
	bool PlaceFlagHasClipDepth;
	bool PlaceFlagHasName;
	bool PlaceFlagHasRatio;
	bool PlaceFlagHasColorTransform;
	bool PlaceFlagHasMatrix;
	bool PlaceFlagHasCharacter;
	bool PlaceFlagMove;
	UI16 Depth;
	UI16 CharacterId;
	MATRIX Matrix;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;
	PlaceObject2Tag(RECORDHEADER h):DisplayListTag(h){}

public:
	STRING Name;
	PlaceObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list);
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
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list);
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
	virtual Vector2 debugRender(FTFont* font, bool deep);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw UnsupportedException("DefineButton2Tag::getBounds");
	}
	virtual void handleEvent(Event*);

	ASObject* instance() const;
};

class KERNINGRECORD
{
};

class DefineBinaryDataTag: public DictionaryTag, public ByteArray
{
private:
	UI16 Tag;
	UI32 Reserved;
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

class FontTag: public DictionaryTag
{
protected:
	UI16 FontID;
public:
	FontTag(RECORDHEADER h):DictionaryTag(h){}
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph)=0;
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
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph);
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
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph);
};

class DefineFont3Tag: public FontTag
{
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
	DefineFont3Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return FontID; }
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph);
};

class DefineTextTag: public DictionaryTag, public DisplayObject
{
	friend class GLYPHENTRY;
private:
	UI16 CharacterId;
	RECT TextBounds;
	MATRIX TextMatrix;
	UI8 GlyphBits;
	UI8 AdvanceBits;
	std::vector < TEXTRECORD > TextRecords;
	//Override the usual vector, we need special shapes
	std::vector<GlyphShape> cached;
public:
	DefineTextTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return CharacterId; }
	virtual void Render();
	virtual void inputRender();
	virtual Vector2 debugRender(FTFont* font, bool deep);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		getMatrix().multiply2D(TextBounds.Xmin/20,TextBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(TextBounds.Xmax/20,TextBounds.Ymax/20,xmax,ymax);
		
		TextMatrix.multiply2D(xmin,ymin,xmin,ymin);
		TextMatrix.multiply2D(xmax,ymax,xmax,ymax);
		//TODO: adapt for rotation
		return true;
	}
	ASObject* instance() const
	{
		return new DefineTextTag(*this);
	}
};

class DefineSpriteTag: public DictionaryTag, public MovieClip
{
private:
	UI16 SpriteID;
	UI16 FrameCount;
public:
	DefineSpriteTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return SpriteID; }
	virtual Vector2 debugRender(FTFont* font, bool deep);
	virtual ASObject* instance() const;
};

class ProtectTag: public ControlTag
{
public:
	ProtectTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root){};
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

class DefineBitsLossless2Tag: public DictionaryTag, public Bitmap
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
	virtual ASObject* instance() const;
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		return false;
	}
	virtual void Render();
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
	UI16 MaxRecursionDepth;
	UI16 ScriptTimeoutSeconds;
public:
	ScriptLimitsTag(RECORDHEADER h, std::istream& in);
};

class ProductInfoTag: public Tag
{
private:
	UI32 ProductId;
	UI32 Edition;
	UI8 MajorVersion;
	UI8 MinorVersion;
	UI32 MinorBuild;
	UI32 MajorBuild;
	UI32 CompileTimeHi, CompileTimeLo;
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
	UI16 ReservedWord;
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
