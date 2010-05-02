/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "flashutils.h"
#include "class.h"
#include <GL/glew.h>

namespace lightspark
{

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
	virtual ~Tag(){}
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
	std::vector<GeomShape> cached;
public:
	Class_base* bindedTo;
	RootMovieClip* loadedFrom;
	DictionaryTag(RECORDHEADER h,std::istream& s):Tag(h,s),bindedTo(NULL),loadedFrom(NULL){ }
	virtual TAGTYPE getType(){ return DICT_TAG; }
	virtual int getId(){return 0;} 
	virtual IInterface* instance() const { return NULL; } 
	void setLoadedFrom(RootMovieClip* r){loadedFrom=r;}
};

class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h, std::istream& s):Tag(h,s){}
	virtual TAGTYPE getType(){ return CONTROL_TAG; }
	virtual void execute(RootMovieClip* root)=0;
};

class DefineShapeTag: public DictionaryTag, public DisplayObject
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShapeTag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		//Apply transformation with the current matrix
		getMatrix().multiply2D(ShapeBounds.Xmin/20,ShapeBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(ShapeBounds.Xmax/20,ShapeBounds.Ymax/20,xmax,ymax);
		//TODO: adapt for rotation
		return true;
	}

	IInterface* instance() const
	{
		return new DefineShapeTag(*this);
	}
};

class DefineShape2Tag: public DictionaryTag, public DisplayObject
{
private:
	UI16 ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in);
	virtual int getId(){ return ShapeId; }
	virtual void Render();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		//Apply transformation with the current matrix
		getMatrix().multiply2D(ShapeBounds.Xmin/20,ShapeBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(ShapeBounds.Xmax/20,ShapeBounds.Ymax/20,xmax,ymax);
		//TODO: adapt for rotation
		return true;
	}

	IInterface* instance() const
	{
		return new DefineShape2Tag(*this);
	}
};

class DefineShape3Tag: public DictionaryTag, public DisplayObject
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		getMatrix().multiply2D(ShapeBounds.Xmin/20,ShapeBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(ShapeBounds.Xmax/20,ShapeBounds.Ymax/20,xmax,ymax);
		return true;
	}

	IInterface* instance() const
	{
		return new DefineShape3Tag(*this);
	}
};

class DefineShape4Tag: public DictionaryTag, public DisplayObject
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		getMatrix().multiply2D(ShapeBounds.Xmin/20,ShapeBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(ShapeBounds.Xmax/20,ShapeBounds.Ymax/20,xmax,ymax);
		return true;
	}

	IInterface* instance() const
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
	//static bool list_orderer(const std::pair<PlaceInfo, IDisplayListElem*>& a, int d);
	//static bool list_orderer(int d, const std::pair<PlaceInfo, IDisplayListElem*>& a);
	class list_orderer
	{
	public:
		bool operator()(const std::pair<PlaceInfo, IDisplayListElem*>& a, int d);
		bool operator()(int d, const std::pair<PlaceInfo, IDisplayListElem*>& a);
		bool operator()(const std::pair<PlaceInfo, IDisplayListElem*>& a, const std::pair<PlaceInfo, IDisplayListElem*>& b);
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

public:
	STRING Name;
	PlaceObject2Tag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& list);
/*	void setWrapped(ASObject* w)
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		abort();
	}
	virtual void handleEvent(Event*);

	IInterface* instance() const;
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
	~DefineBinaryDataTag(){delete[] bytes;}
	virtual int getId(){return Tag;} 

	IInterface* instance() const
	{
		DefineBinaryDataTag* ret=new DefineBinaryDataTag(*this);
		//An object is always linked
		ret->prototype=Class<ByteArray>::getClass();
		ret->prototype->incRef();
		return ret;
	}
};

class FontTag: public DictionaryTag
{
protected:
	UI16 FontID;
public:
	FontTag(RECORDHEADER h,std::istream& s):DictionaryTag(h,s){}
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph)=0;
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
	virtual void genGlyphShape(std::vector<GeomShape>& s, int glyph);
};

class DefineFont3Tag: public DefineFont2Tag
{
public:
	DefineFont3Tag(RECORDHEADER h, std::istream& in):DefineFont2Tag(h,in){}
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		getMatrix().multiply2D(TextBounds.Xmin/20,TextBounds.Ymin/20,xmin,ymin);
		getMatrix().multiply2D(TextBounds.Xmax/20,TextBounds.Ymax/20,xmax,ymax);
		//TODO: adapt for rotation
		return true;
	}

	IInterface* instance() const
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

	IInterface* instance() const
	{
		DefineSpriteTag* ret=new DefineSpriteTag(*this);
		//TODO: check
		if(bindedTo)
		{
			//A class is binded to this tag
			ret->prototype=static_cast<Class_base*>(bindedTo);
		}
		else
		{
			//A default object is always linked
			ret->prototype=Class<MovieClip>::getClass();
		}

		ret->prototype->incRef();
		ret->bootstrap();
		return ret;
	}
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

class CSMTextSettingsTag: public Tag
{
public:
	CSMTextSettingsTag(RECORDHEADER h, std::istream& in);
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

};

#endif
