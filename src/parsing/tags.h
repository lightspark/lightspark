/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef PARSING_TAGS_H
#define PARSING_TAGS_H 1

#include "compat.h"
#include <vector>
#include <iostream>
#include "swftypes.h"
#include "backends/geometry.h"
#include "backends/decoder.h"
#include "scripting/flash/display/flashdisplay.h"

namespace lightspark
{
class Class_base;
class RootMovieClip;
class DisplayObjectContainer;
class DefineSpriteTag;
class AdditionalDataTag;

enum TAGTYPE {TAG=0,DISPLAY_LIST_TAG,SHOW_TAG,CONTROL_TAG,DICT_TAG,FRAMELABEL_TAG,SYMBOL_CLASS_TAG,ACTION_TAG,ABC_TAG,END_TAG,AVM1ACTION_TAG,AVM1INITACTION_TAG};

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
	Tag(RECORDHEADER h):Header(h) {}
	virtual TAGTYPE getType() const{ return TAG; }
	virtual ~Tag(){}
};

class EndTag:public Tag
{
public:
	EndTag(RECORDHEADER h, std::istream& s):Tag(h){}
	TAGTYPE getType() const override { return END_TAG; }
};

class DisplayListTag: public Tag
{
public:
	DisplayListTag(RECORDHEADER h):Tag(h){}
	TAGTYPE getType() const override { return DISPLAY_LIST_TAG; }
	virtual void execute(DisplayObjectContainer* parent,bool inskipping) =0;
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
	tiny_string bindingclassname;
	RootMovieClip* loadedFrom;
	uint32_t nameID;
	DictionaryTag(RECORDHEADER h, RootMovieClip* root):Tag(h),bindedTo(nullptr),loadedFrom(root),nameID(UINT32_MAX) { }
	TAGTYPE getType() const override { return DICT_TAG; }
	virtual int getId() const=0;
	virtual ASObject* instance(Class_base* c=nullptr) { return nullptr; }
	virtual MATRIX MapToBounds(const MATRIX& mat) { return mat; }
	virtual MATRIX MapToBoundsForButton(const MATRIX& mat) { return MapToBounds(mat); }
	virtual void resizeCompleted() {}
};

/*
 * See p.53ff in the SWF spec. Those tags are ::executed directly after parsing
 * and then delete'ed.
 */
class ControlTag: public Tag
{
public:
	ControlTag(RECORDHEADER h):Tag(h){}
	TAGTYPE getType() const override { return CONTROL_TAG; }
	virtual void execute(RootMovieClip* root) const=0;
};

/*
 * Initiates an action. Action is executed after a frame is parsed.
 */
class ActionTag: public ControlTag
{
public:
	ActionTag(RECORDHEADER h):ControlTag(h){}
	TAGTYPE getType() const override { return ACTION_TAG; }
	void execute(RootMovieClip* root) const override =0;
};

class AVM1ActionTag: public Tag
{
private:
	std::vector<uint8_t> actions;
	uint32_t startactionpos;
public:
	AVM1ActionTag(RECORDHEADER h, std::istream& s,RootMovieClip* root, AdditionalDataTag* datatag);
	TAGTYPE getType() const override { return AVM1ACTION_TAG; }
	void execute(MovieClip* clip, AVM1context *context);
	bool empty() { return actions.empty(); }
};
class AVM1InitActionTag: public ControlTag
{
private:
	UI16_SWF SpriteId;
	std::vector<uint8_t> actions;
	uint32_t startactionpos;
public:
	AVM1InitActionTag(RECORDHEADER h, std::istream& s,RootMovieClip* root, AdditionalDataTag* datatag);
	TAGTYPE getType() const override { return AVM1INITACTION_TAG; }
	void execute(RootMovieClip* root) const override;
	bool empty() { return actions.empty(); }
	void executeDirect(MovieClip *clip) const;
};

class DefineShapeTag: public DictionaryTag
{
friend class Shape;
protected:
	UI16_SWF ShapeId;
	RECT ShapeBounds;
	SHAPEWITHSTYLE Shapes;
	tokensVector* tokens;
	TextureChunk chunk;
	DefineShapeTag(RECORDHEADER h,int v,RootMovieClip* root);
public:
	DefineShapeTag(RECORDHEADER h,std::istream& in, RootMovieClip* root);
	~DefineShapeTag();
	virtual int getId() const{ return ShapeId; }
	ASObject* instance(Class_base* c=nullptr);
	MATRIX MapToBoundsForButton(const MATRIX& mat) override;
	void resizeCompleted() override;
};

class DefineShape2Tag: public DefineShapeTag
{
protected:
	DefineShape2Tag(RECORDHEADER h, int v, RootMovieClip* root):DefineShapeTag(h,v,root){}
public:
	DefineShape2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
};

class DefineShape3Tag: public DefineShape2Tag
{
protected:
	DefineShape3Tag(RECORDHEADER h, int v, RootMovieClip* root):DefineShape2Tag(h,v,root){}
public:
	DefineShape3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	virtual int getId() const { return ShapeId; }
};

class DefineShape4Tag: public DefineShape3Tag
{
private:
	RECT EdgeBounds;
	UB UsesFillWindingRule;
	UB UsesNonScalingStrokes;
	UB UsesScalingStrokes;
public:
	DefineShape4Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	virtual int getId() const { return ShapeId; }
};

class DefineMorphShapeTag: public DictionaryTag
{
	friend class TokenContainer;
protected:
	UI16_SWF CharacterId;
	RECT StartBounds;
	RECT EndBounds;
	MORPHFILLSTYLEARRAY MorphFillStyles;
	MORPHLINESTYLEARRAY MorphLineStyles;
	SHAPE StartEdges;
	SHAPE EndEdges;
	DefineMorphShapeTag(RECORDHEADER h, RootMovieClip* root, int version):DictionaryTag(h,root),MorphLineStyles(version){}
public:
	DefineMorphShapeTag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	int getId() const { return CharacterId; }
	virtual ASObject* instance(Class_base* c=NULL);
};

class DefineMorphShape2Tag: public DefineMorphShapeTag
{
private:
	RECT StartEdgeBounds;
	RECT EndEdgeBounds;
	UB UsesNonScalingStrokes;
	UB UsesScalingStrokes;
public:
	DefineMorphShape2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
};

class DefineEditTextTag: public DictionaryTag
{
friend class TextField;
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
	TextData textData;
public:
	DefineEditTextTag(RECORDHEADER h, std::istream& s, RootMovieClip* root);
	int getId() const override { return CharacterID; }
	ASObject* instance(Class_base* c=nullptr) override;
	MATRIX MapToBounds(const MATRIX& mat) override;
};

class MemoryStreamCache;

class DefineSoundTag: public DictionaryTag
{
private:
	UI16_SWF SoundId;
	char SoundFormat;
	char SoundRate;
	char SoundSize;
	char SoundType;
	UI32_SWF SoundSampleCount;
	_R<MemoryStreamCache> SoundData;
	int realSampleRate;
public:
	DefineSoundTag(RECORDHEADER h, std::istream& s, RootMovieClip* root);
	virtual int getId() const { return SoundId; }
	ASObject* instance(Class_base* c=nullptr) override;
	LS_AUDIO_CODEC getAudioCodec() const;
	number_t getDurationInMS() const;
	int getSampleRate() const;
	int getChannels() const;
	_R<MemoryStreamCache> getSoundData() const;
	std::streambuf *createSoundStream() const;
	_NR<SoundChannel> soundchanel;
	// indicates if this channel is attached to a Sound object
	bool isAttached;
};

class StartSoundTag: public DisplayListTag
{
private:
	UI16_SWF SoundId;
	SOUNDINFO SoundInfo;

	void play(DefineSoundTag *soundTag);
public:
	StartSoundTag(RECORDHEADER h, std::istream& s);
	void execute(DisplayObjectContainer* parent,bool inskipping) override;
};

class SoundStreamHeadTag: public DisplayListTag
{
friend class SoundStreamBlockTag;
private:
	int StreamSoundCompression;
	int StreamSoundRate;
	char StreamSoundSize;
	char StreamSoundType;
	UI16_SWF StreamSoundSampleCount;
	SI16_SWF LatencySeek;
public:
	SoundStreamHeadTag(RECORDHEADER h, std::istream& s, RootMovieClip* root,DefineSpriteTag* sprite);
	_R<MemoryStreamCache> SoundData;
	void setSoundChannel(Sprite* spr, bool autoplay);
	void execute(DisplayObjectContainer *parent,bool inskipping) override {}
};

class SoundStreamBlockTag: public DisplayListTag
{
public:
	SoundStreamBlockTag(RECORDHEADER h, std::istream& in, RootMovieClip* root,DefineSpriteTag* sprite);
	static void decodeSoundBlock(StreamCache *cache, LS_AUDIO_CODEC codec, unsigned char* buf, int len);
	void execute(DisplayObjectContainer *parent,bool inskipping) override {}
};

class ShowFrameTag: public Tag
{
public:
	ShowFrameTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const override { return SHOW_TAG; }
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
	void execute(DisplayObjectContainer* parent,bool inskipping) override;
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
	CXFORMWITHALPHA ColorTransformWithAlpha;
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
	CLIPACTIONS ClipActions;
	PlaceObject2Tag(RECORDHEADER h,uint32_t v):DisplayListTag(h),ClipActions(v,nullptr),placedTag(nullptr),NameID(BUILTIN_STRINGS::EMPTY){}
	virtual void setProperties(DisplayObject* obj, DisplayObjectContainer* parent) const;
	DictionaryTag* placedTag;
public:
	STRING Name;
	uint32_t NameID;
	PlaceObject2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root, AdditionalDataTag* datatag);
	void execute(DisplayObjectContainer* parent,bool inskipping) override;
};

class PlaceObject3Tag: public PlaceObject2Tag
{
private:
	bool PlaceFlagOpaqueBackground;
	bool PlaceFlagHasVisible;
	bool PlaceFlagHasImage;
	bool PlaceFlagHasClassName;
	bool PlaceFlagHasCacheAsBitmap;
	bool PlaceFlagHasBlendMode;
	bool PlaceFlagHasFilterList;
	STRING ClassName;
	FILTERLIST SurfaceFilterList;
	UI8 BlendMode;
	UI8 BitmapCache;
	UI8 Visible;
	RGBA BackgroundColor;

public:
	PlaceObject3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	void setProperties(DisplayObject* obj, DisplayObjectContainer* parent) const;
};

class FrameLabelTag: public Tag
{
public:
	STRING Name;
	FrameLabelTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const override { return FRAMELABEL_TAG; }
};

class SetBackgroundColorTag: public ControlTag
{
private:
	RGB BackgroundColor;
public:
	SetBackgroundColorTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root) const;
};

class DefineButtonTag: public DictionaryTag
{
private:
	UI16_SWF ButtonId;
	UB ReservedFlags;
	bool TrackAsMenu;
	std::vector<BUTTONRECORD> Characters;
public:
	std::vector<BUTTONCONDACTION> condactions;
	DefineButtonTag(RECORDHEADER h, std::istream& in, int version, RootMovieClip* root, AdditionalDataTag* datatag);
	virtual int getId() const { return ButtonId; }
	ASObject* instance(Class_base* c=NULL);
};

class KERNINGRECORD
{
};

class DefineBinaryDataTag: public DictionaryTag
{
private:
	UI16_SWF Tag;
	UI32_SWF Reserved;
	uint8_t* bytes;
	uint32_t len;
public:
	DefineBinaryDataTag(RECORDHEADER h,std::istream& s,RootMovieClip* root);
	~DefineBinaryDataTag() { delete[] bytes; }
	virtual int getId() const {return Tag;}
	ASObject* instance(Class_base* c=NULL);
};

class FontTag: public DictionaryTag
{
friend class DefineFontInfoTag;
protected:
	UI16_SWF FontID;
	std::vector<SHAPE> GlyphShapeTable;
	std::vector<uint16_t> CodeTable;
	tiny_string fontname;
	bool FontFlagsSmallText;
	bool FontFlagsShiftJIS;
	bool FontFlagsANSI;
	bool FontFlagsItalic;
	bool FontFlagsBold;
public:
	/* Multiply the coordinates of the SHAPEs by this
	 * value to get a resolution of 1024*20th pixel
	 * DefineFont3Tag sets 1 here, the rest set 20
	 */
	const int scaling;
	FontTag(RECORDHEADER h, int _scaling,RootMovieClip* root):DictionaryTag(h,root), scaling(_scaling) {}
	const std::vector<SHAPE>& getGlyphShapes() const
	{
		return GlyphShapeTable;
	}
	int getId() const { return FontID; }
	ASObject* instance(Class_base* c=NULL);
	const tiny_string getFontname() const { return fontname;}
	virtual void fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, RGB textColor, uint32_t leading,uint32_t startpos) const=0;
	bool hasGlyphs(const tiny_string text) const;
};

class DefineFontTag: public FontTag
{
	friend class DefineTextTag; 
protected:
	std::vector<uint16_t> OffsetTable;
public:
	DefineFontTag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	void fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, RGB textColor, uint32_t leading,uint32_t startpos) const;
};

class DefineFontInfoTag: public Tag
{
public:
	DefineFontInfoTag(RECORDHEADER h, std::istream& in,RootMovieClip* root);
};

class DefineFont2Tag: public FontTag
{
	friend class DefineTextTag; 
private:
	std::vector<uint32_t> OffsetTable;
	bool FontFlagsHasLayout;
	bool FontFlagsWideOffsets;
	bool FontFlagsWideCodes;
	LANGCODE LanguageCode;
	UI16_SWF NumGlyphs;
	uint32_t CodeTableOffset;
	SI16_SWF FontAscent;
	SI16_SWF FontDescent;
	SI16_SWF FontLeading;
	std::vector < SI16_SWF > FontAdvanceTable;
	std::vector < RECT > FontBoundsTable;
	UI16_SWF KerningCount;
	std::vector <KERNINGRECORD> FontKerningTable;

public:
	DefineFont2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	void fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, RGB textColor, uint32_t leading,uint32_t startpos) const;
};

class DefineFont3Tag: public FontTag
{
private:
	std::vector<uint32_t> OffsetTable;
	UB FontFlagsHasLayout;
	UB FontFlagsWideOffsets;
	UB FontFlagsWideCodes;
	LANGCODE LanguageCode;
	UI16_SWF NumGlyphs;
	uint32_t CodeTableOffset;
	SI16_SWF FontAscent;
	SI16_SWF FontDescent;
	SI16_SWF FontLeading;
	std::vector < SI16_SWF > FontAdvanceTable;
	std::vector < RECT > FontBoundsTable;
	UI16_SWF KerningCount;
	std::vector <KERNINGRECORD> FontKerningTable;

public:
	DefineFont3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	void fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, RGB textColor, uint32_t leading,uint32_t startpos) const;
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
	DefineFont4Tag(RECORDHEADER h, std::istream& in,RootMovieClip* root);
	virtual int getId() const { return FontID; }
	ASObject* instance(Class_base* c=nullptr);
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
	mutable tokensVector tokens;
	void computeCached() const;
public:
	int version;
	DefineTextTag(RECORDHEADER h, std::istream& in,RootMovieClip* root,int v=1);
	int getId() const { return CharacterId; }
	ASObject* instance(Class_base* c=nullptr);
};

class DefineText2Tag: public DefineTextTag
{
public:
	DefineText2Tag(RECORDHEADER h, std::istream& in,RootMovieClip* root) : DefineTextTag(h,in,root,2) {}
};

class DefineSpriteTag: public DictionaryTag, public FrameContainer
{
private:
	UI16_SWF SpriteID;
	UI16_SWF FrameCount;
	uint32_t soundstartframe;
public:
	SoundStreamHeadTag* soundheadtag;
	DefineSpriteTag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	~DefineSpriteTag();
	virtual int getId() const { return SpriteID; }
	virtual ASObject* instance(Class_base* c=nullptr);
	void setSoundStartFrame(uint32_t frame) 
	{
		if (soundstartframe == UINT32_MAX)
			soundstartframe=frame;
	}
};

class ProtectTag: public ControlTag
{
public:
	ProtectTag(RECORDHEADER h, std::istream& in);
	void execute(RootMovieClip* root) const{}
};

class BitmapContainer;

class BitmapTag: public DictionaryTag
{
protected:
        _R<BitmapContainer> bitmap;
    void loadBitmap(uint8_t* inData, int datasize, const uint8_t *tablesData=nullptr, int tablesLen=0);
public:
	BitmapTag(RECORDHEADER h,RootMovieClip* root);
	ASObject* instance(Class_base* c=nullptr);
        _R<BitmapContainer> getBitmap() const;
};

class JPEGTablesTag: public Tag
{
private:
	static uint8_t* JPEGTables;
	static int tableSize;
public:
	JPEGTablesTag(RECORDHEADER h, std::istream& in);
	static const uint8_t* getJPEGTables();
	static int getJPEGTableSize();
};

class DefineBitsLosslessTag: public BitmapTag
{
private:
	UI16_SWF CharacterId;
	UI8 BitmapFormat;
	UI16_SWF BitmapWidth;
	UI16_SWF BitmapHeight;
	UI8 BitmapColorTableSize;
	//ZlibBitmapData;
public:
	DefineBitsLosslessTag(RECORDHEADER h, std::istream& in, int version, RootMovieClip* root);
	int getId() const{ return CharacterId; }
};

class DefineBitsTag: public BitmapTag
{
private:
	UI16_SWF CharacterId;
public:
	DefineBitsTag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	int getId() const{ return CharacterId; }
};

class DefineBitsJPEG2Tag: public BitmapTag
{
private:
	UI16_SWF CharacterId;
public:
	DefineBitsJPEG2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	int getId() const{ return CharacterId; }
};

class DefineBitsJPEG3Tag: public BitmapTag
{
private:
	UI16_SWF CharacterId;
	uint8_t* alphaData;
public:
	DefineBitsJPEG3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	~DefineBitsJPEG3Tag();
	int getId() const{ return CharacterId; }
};

class DefineScalingGridTag: public Tag
{
public:
	UI16_SWF CharacterId;
	RECT Splitter;
	DefineScalingGridTag(RECORDHEADER h, std::istream& in);
};

class AdditionalDataTag: public Tag
{
public:
	AdditionalDataTag(RECORDHEADER h, std::istream& in);
	~AdditionalDataTag()
	{
		delete bytes;
	}
	uint8_t* bytes;
	uint32_t numbytes;
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
	void execute(RootMovieClip* root) const override;
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
class VideoFrameTag;
class DefineVideoStreamTag: public DictionaryTag
{
friend class Video;
private:
	UI16_SWF CharacterID;
	UI16_SWF NumFrames;
	UI16_SWF Width;
	UI16_SWF Height;
	UB VideoFlagsReserved;
	UB VideoFlagsDeblocking;
	UB VideoFlagsSmoothing;
	UI8 VideoCodecID;
	vector<VideoFrameTag*> frames;
public:
	DefineVideoStreamTag(RECORDHEADER h, std::istream& in, RootMovieClip* root);
	~DefineVideoStreamTag();
	int getId() const override { return CharacterID; }
	ASObject* instance(Class_base* c=nullptr) override;
	void setFrameData(VideoFrameTag* tag);
	VideoFrameTag* getFrame(uint32_t frame) const { return frames[frame]; }
};
class VideoFrameTag: public DisplayListTag
{
private:
	UI16_SWF StreamID;
	UI16_SWF FrameNum;
	uint8_t* framedata;
	uint32_t numbytes;
public:
	VideoFrameTag(RECORDHEADER h, std::istream& in);
	~VideoFrameTag();
	void execute(DisplayObjectContainer* parent,bool inskipping) override;
	uint8_t* getData() { return framedata; }
	uint32_t getNumBytes() { return numbytes+AV_INPUT_BUFFER_PADDING_SIZE; }
	uint32_t getFrameNumber() { return FrameNum; }
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

class ScriptLimitsTag: public ControlTag
{
private:
	UI16_SWF MaxRecursionDepth;
	UI16_SWF ScriptTimeoutSeconds;
public:
	ScriptLimitsTag(RECORDHEADER h, std::istream& in);
	TAGTYPE getType() const override { return ABC_TAG; }
	void execute(RootMovieClip* root) const override;
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
public:
	UB UseDirectBlit;
	UB UseGPU;
	UB HasMetadata;
	UB ActionScript3;
	UB UseNetwork;
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

class ExportAssetsTag: public Tag
{
public:
	ExportAssetsTag(RECORDHEADER h, std::istream& in, RootMovieClip *root);
};

class NameCharacterTag: public Tag
{
public:
	NameCharacterTag(RECORDHEADER h, std::istream& in, RootMovieClip *root);
};

class TagFactory
{
private:
	std::istream& f;
	bool firstTag;
public:
	TagFactory(std::istream& in):f(in),firstTag(true){}
	/**
	 * The RootMovieClip that is the owner of the content.
	 * It is needed to solve references to other tags during construction
	 */
	Tag* readTag(RootMovieClip* root,DefineSpriteTag* sprite=nullptr);
};

}

#endif /* PARSING_TAGS_H */
