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

#include <vector>
#include <list>
#include <algorithm>
#include <sstream>
#ifdef __MINGW32__
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "scripting/abc.h"
#include "parsing/tags.h"
#include "backends/geometry.h"
#include "backends/security.h"
#include "backends/streamcache.h"
#include "swftypes.h"
#include "logger.h"
#include "compat.h"
#include "parsing/streams.h"
#include "scripting/class.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/MorphShape.h"
#include "scripting/flash/display/SimpleButton.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/avm1/avm1sound.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1media.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/flash/filters/flashfilters.h"
#include "backends/audio.h"
#include "backends/rendering.h"

#undef RGB

// BitmapFormat values in DefineBitsLossless(2) tag
#define LOSSLESS_BITMAP_PALETTE 3
#define LOSSLESS_BITMAP_RGB15 4
#define LOSSLESS_BITMAP_RGB24 5

// Adobe seems to place legacy displayobjects at negative depths starting at -16384
// see https://www.kirupa.com/developer/actionscript/depths2.htm
#define LEGACY_DEPTH_START -16384

using namespace std;
using namespace lightspark;

uint8_t* JPEGTablesTag::JPEGTables = nullptr;
int JPEGTablesTag::tableSize = 0;

Tag* TagFactory::readTag(RootMovieClip* root, DefineSpriteTag *sprite)
{
	RECORDHEADER h;

	bool done = false;
	AdditionalDataTag* datatag = nullptr;
	Tag* ret=nullptr;
	while (!done)
	{
		done = true;
		//Catch eofs
		try
		{
			f >> h;
		}
		catch (ifstream::failure& e) {
			if(!f.eof()) //Only handle eof
				throw;
			f.clear();
			LOG(LOG_INFO,"Simulating EndTag at EOF @ " << f.tellg());
			return new EndTag(h,f);
		}

		unsigned int expectedLen=h.getLength();
		unsigned int start=f.tellg();
		LOG(LOG_TRACE,"Reading tag type: " << h.getTagType() << " at byte " << start << " with length " << expectedLen << " bytes");
		switch(h.getTagType())
		{
			case 0:
				ret=new EndTag(h,f);
				break;
			case 1:
				ret=new ShowFrameTag(h,f);
				break;
			case 2:
				ret=new DefineShapeTag(h,f,root);
				break;
				//	case 4:
				//		ret=new PlaceObjectTag(h,f);
			case 6:
				ret=new DefineBitsTag(h,f,root);
				break;
			case 7:
				ret=new DefineButtonTag(h,f,1,root,datatag);
				if (datatag)
					delete datatag;
				datatag=nullptr;
				break;
			case 8:
				ret=new JPEGTablesTag(h,f);
				break;
			case 9:
				ret=new SetBackgroundColorTag(h,f);
				break;
			case 10:
				ret=new DefineFontTag(h,f,root);
				break;
			case 11:
				ret=new DefineTextTag(h,f,root);
				break;
			case 12:
				ret=new AVM1ActionTag(h,f,root,datatag);
				if (datatag)
					delete datatag;
				datatag=nullptr;
				break;
			case 13:
				ret=new DefineFontInfoTag(h,f,root);
				break;
			case 14:
				ret=new DefineSoundTag(h,f,root);
				break;
			case 15:
				ret=new StartSoundTag(h,f);
				break;
			case 17:
				ret=new DefineButtonSoundTag(h,f,root);
				break;
			case 18:
				ret=new SoundStreamHeadTag(h,f,root,sprite);
				break;
			case 19:
				ret=new SoundStreamBlockTag(h,f,root,sprite);
				break;
			case 20:
				ret=new DefineBitsLosslessTag(h,f,1,root);
				break;
			case 21:
				ret=new DefineBitsJPEG2Tag(h,f,root);
				break;
			case 22:
				ret=new DefineShape2Tag(h,f,root);
				break;
			case 24:
				ret=new ProtectTag(h,f);
				break;
			case 26:
				ret=new PlaceObject2Tag(h,f,root,datatag);
				if (datatag)
					delete datatag;
				datatag=nullptr;
				break;
			case 28:
				ret=new RemoveObject2Tag(h,f);
				break;
			case 32:
				ret=new DefineShape3Tag(h,f,root);
				break;
			case 33:
				ret=new DefineText2Tag(h,f,root);
				break;
			case 34:
				ret=new DefineButtonTag(h,f,2,root,datatag);
				if (datatag)
					delete datatag;
				datatag=nullptr;
				break;
			case 35:
				ret=new DefineBitsJPEG3Tag(h,f,root);
				break;
			case 36:
				ret=new DefineBitsLosslessTag(h,f,2,root);
				break;
			case 37:
				ret=new DefineEditTextTag(h,f,root);
				break;
			case 39:
				ret=new DefineSpriteTag(h,f,root);
				break;
			case 40:
				ret=new NameCharacterTag(h,f,root);
				break;
			case 41:
				ret=new ProductInfoTag(h,f);
				break;
			case 43:
				ret=new FrameLabelTag(h,f);
				break;
			case 45:
				ret=new SoundStreamHeadTag(h,f,root,sprite);
				break;
			case 46:
				ret=new DefineMorphShapeTag(h,f,root);
				break;
			case 48:
				ret=new DefineFont2Tag(h,f,root);
				break;
			case 56:
				ret=new ExportAssetsTag(h,f,root);
				break;
			case 58:
				ret=new EnableDebuggerTag(h,f);
				break;
			case 59:
				ret=new AVM1InitActionTag(h,f,root,datatag);
				if (datatag)
					delete datatag;
				datatag=nullptr;
				break;
			case 60:
				ret=new DefineVideoStreamTag(h,f,root);
				break;
			case 61:
				ret=new VideoFrameTag(h, f, root);
				break;
			case 63:
				ret=new DebugIDTag(h,f);
				break;
			case 64:
				ret=new EnableDebugger2Tag(h,f);
				break;
			case 65:
				ret=new ScriptLimitsTag(h,f);
				break;
			case 69:
				//FileAttributes tag is mandatory on version>=8 and must be the first tag
				if(!firstTag)
					LOG(LOG_ERROR,"FileAttributes tag not in the beginning");
				ret=new FileAttributesTag(h,f);
				break;
			case 70:
				ret=new PlaceObject3Tag(h,f,root);
				break;
			case 72:
				ret=new DoABCTag(h,f);
				break;
			case 73:
				ret=new DefineFontAlignZonesTag(h,f);
				break;
			case 74:
				ret=new CSMTextSettingsTag(h,f);
				break;
			case 75:
				ret=new DefineFont3Tag(h,f,root);
				break;
			case 76:
				ret=new SymbolClassTag(h,f);
				break;
			case 77:
				ret=new MetadataTag(h,f);
				break;
			case 78:
				ret=new DefineScalingGridTag(h,f);
				break;
			case 82:
				ret=new DoABCDefineTag(h,f);
				break;
			case 83:
				ret=new DefineShape4Tag(h,f,root);
				break;
			case 84:
				ret=new DefineMorphShape2Tag(h,f,root);
				break;
			case 86:
				ret=new DefineSceneAndFrameLabelDataTag(h,f);
				break;
			case 87:
				ret=new DefineBinaryDataTag(h,f,root);
				break;
			case 88:
				ret=new DefineFontNameTag(h,f);
				break;
			case 91:
				ret=new DefineFont4Tag(h,f,root);
				break;
			case 253:
				// this is an undocumented tag that seems to be used for obfuscation
				// when placed before a DoActionTag, the bytes in this tag will be interpreted as actionscript code
				datatag=new AdditionalDataTag(h,f);
				done = false;
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"Unsupported tag type " << h.getTagType());
				ret=new UnimplementedTag(h,f);
				break;
		}
		firstTag=false;

		unsigned int end=f.tellg();

		unsigned int actualLen=end-start;

		if(actualLen<expectedLen)
		{
			LOG(LOG_ERROR,"Error while reading tag " << h.getTagType() << ". Size=" << actualLen << " expected: " << expectedLen);
			ignore(f,expectedLen-actualLen);
		}
		else if(actualLen>expectedLen)
		{
			LOG(LOG_ERROR,"Error while reading tag " << h.getTagType() << ". Size=" << actualLen << " expected: " << expectedLen);
			// Adobe also seems to ignore this
			//throw ParseException("Malformed SWF file");
		}

		root->loaderInfo->setBytesLoaded(f.tellg());
	}
	if (datatag)
	{
		LOG(LOG_NOT_IMPLEMENTED,"additionaldatag (type 253) found, but not used in subsequent tag:"<<h.getTagType());
	}

	return ret;
}

RemoveObject2Tag::RemoveObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	in >> Depth;
	LOG(LOG_TRACE,"RemoveObject2 Depth: " << Depth);
}

void RemoveObject2Tag::execute(DisplayObjectContainer* parent,bool inskipping)
{
	parent->LegacyChildRemoveDeletionMark(LEGACY_DEPTH_START+Depth);
	parent->deleteLegacyChildAt(LEGACY_DEPTH_START+Depth,inskipping);
}

SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	in >> BackgroundColor;
}

DefineEditTextTag::DefineEditTextTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h,root)
{
	//setVariableByName("text",SWFObject(new Integer(0)));
	in >> CharacterID >> Bounds;
	LOG(LOG_TRACE,"DefineEditTextTag ID " << CharacterID);
	BitStream bs(in);
	HasText=UB(1,bs);
	WordWrap=UB(1,bs);
	Multiline=UB(1,bs);
	Password=UB(1,bs);
	ReadOnly=UB(1,bs);
	HasTextColor=UB(1,bs);
	HasMaxLength=UB(1,bs);
	HasFont=UB(1,bs);
	HasFontClass=UB(1,bs);
	AutoSize=UB(1,bs);
	HasLayout=UB(1,bs);
	NoSelect=UB(1,bs);
	Border=UB(1,bs);
	WasStatic=UB(1,bs);
	HTML=UB(1,bs);
	UseOutlines=UB(1,bs);
	if(HasFont)
	{
		in >> FontID;
		if(HasFontClass)
			in >> FontClass;
		in >> FontHeight;
		textData.fontSize = twipsToPixels(FontHeight);
	}
	if(HasTextColor)
	{
		in >> TextColor;
		textData.textColor.Red = TextColor.Red;
		textData.textColor.Green = TextColor.Green;
		textData.textColor.Blue = TextColor.Blue;
	}
	if(HasMaxLength)
		in >> MaxLength;
	if(HasLayout)
	{
		in >> Align >> LeftMargin >> RightMargin >> Indent >> Leading;
		switch(Align)
		{
			case 0:
				textData.align = TextData::AS_NONE;
				if (AutoSize)
					textData.autoSize = TextData::AS_LEFT;
				break;
			case 1:
				textData.autoSize = TextData::AS_RIGHT;
				if (AutoSize)
					textData.autoSize = TextData::AS_RIGHT;
				break;
			case 2:
				textData.autoSize = TextData::AS_CENTER;
				if (AutoSize)
					textData.autoSize = TextData::AS_CENTER;
				break;
			case 3:
				LOG(LOG_NOT_IMPLEMENTED,"DefineEditTextTag:Align justify on ID "<<CharacterID);
				break;
		}
		if (LeftMargin != 0)
			LOG(LOG_NOT_IMPLEMENTED,"DefineEditTextTag:LeftMargin on ID "<<CharacterID);
		if (RightMargin != 0)
			LOG(LOG_NOT_IMPLEMENTED,"DefineEditTextTag:RightMargin on ID "<<CharacterID);
		if (Indent != 0)
			LOG(LOG_NOT_IMPLEMENTED,"DefineEditTextTag:Indent on ID "<<CharacterID);
	}
	in >> VariableName;
	if (AutoSize && textData.autoSize == TextData::AS_NONE)
		textData.autoSize = TextData::AS_LEFT;
	if(HasText)
	{
		in >> InitialText;
		textData.setText((const char*)InitialText);
	}
	textData.wordWrap = WordWrap;
	textData.multiline = Multiline;
	textData.border = Border;
	if (textData.border)
	{
		textData.borderColor = RGB(0,0,0);
		// it seems that textfields with border are filled with white background
		textData.background = true;
		textData.backgroundColor = RGB(0xff,0xff,0xff);
	}
	textData.width = (Bounds.Xmax-Bounds.Xmin)/20;
	textData.height = (Bounds.Ymax-Bounds.Ymin)/20;
	textData.leading = Leading/20;
	textData.isPassword = Password;
}

ASObject* DefineEditTextTag::instance(Class_base* c)
{
	if(c==nullptr)
	{
		if (loadedFrom->usesActionScript3)
			c=Class<TextField>::getClass(loadedFrom->getSystemState());
		else
			c=Class<AVM1TextField>::getClass(loadedFrom->getSystemState());
	}
	if (HasFont && textData.fontID!=this->FontID)
	{
		FontTag* fonttag =  dynamic_cast<FontTag*>(loadedFrom->dictionaryLookup(FontID));
		if (fonttag)
		{
			textData.font = fonttag->getFontname();
			textData.fontID = fonttag->getId();
		}
	}
	//TODO: check
	assert_and_throw(bindedTo==nullptr);
	TextField* ret= loadedFrom->usesActionScript3 ? 
					new (c->memoryAccount) TextField(loadedFrom->getInstanceWorker(), c, textData, !NoSelect, ReadOnly,VariableName,this) :
					new (c->memoryAccount) AVM1TextField(loadedFrom->getInstanceWorker(),c, textData, !NoSelect, ReadOnly,VariableName,this);
	if (HTML)
		ret->setHtmlText((const char*)InitialText);
	return ret;
}

MATRIX DefineEditTextTag::MapToBounds(const MATRIX &mat)
{
	MATRIX m (1,1,0,0,Bounds.Xmin/20,Bounds.Ymin/20);
	return mat.multiplyMatrix(m);
}

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h,root)
{
	soundheadtag=nullptr;
	soundstartframe=UINT32_MAX;
	in >> SpriteID >> FrameCount;

	LOG(LOG_TRACE,"DefineSprite ID: " << SpriteID);
	//Create a non top level TagFactory
	TagFactory factory(in);
	Tag* tag;
	bool done=false;
	bool empty=true;
	do
	{
		tag=factory.readTag(root,this);
		/* We need no locking here, because the vm can only
		 * access this object after construction
		 */
		switch(tag->getType())
		{
			case DICT_TAG:
				delete tag;
				throw ParseException("Dictionary tag inside a sprite. Should not happen.");
			case DISPLAY_LIST_TAG:
				addToFrame(static_cast<DisplayListTag*>(tag));
				empty=false;
				break;
			case SHOW_TAG:
			{
				delete tag;
				frames.emplace_back(Frame());
				empty=true;
				break;
			}
			case DEFINESCALINGGRID_TAG:
			{
				root->addToScalingGrids(static_cast<DefineScalingGridTag*>(tag));
				break;
			}
			case BACKGROUNDCOLOR_TAG:
			case SYMBOL_CLASS_TAG:
			case ABC_TAG:
			case CONTROL_TAG:
				delete tag;
				throw ParseException("Control tag inside a sprite. Should not happen.");
			case ACTION_TAG:
				LOG(LOG_NOT_IMPLEMENTED,"ActionTag inside sprite "<< SpriteID);
				delete tag;
				break;
			case FRAMELABEL_TAG:
				addFrameLabel(frames.size()-1,static_cast<FrameLabelTag*>(tag)->Name);
				delete tag;
				empty=false;
				break;
			case AVM1ACTION_TAG:
				if (!(static_cast<AVM1ActionTag*>(tag)->empty()))
				{
					addToFrame(static_cast<AVM1ActionTag*>(tag));
					empty=false;
				}
				break;
			case AVM1INITACTION_TAG:
				LOG(LOG_NOT_IMPLEMENTED,"InitActionTag inside sprite "<< SpriteID);
				delete tag;
				break;
			case ENABLEDEBUGGER_TAG:
			case METADATA_TAG:
			case FILEATTRIBUTES_TAG:
				delete tag;
				break;
			case TAG:
				delete tag;
				LOG(LOG_NOT_IMPLEMENTED,"Unclassified tag inside Sprite?");
				break;
			case BUTTONSOUND_TAG:
			{
				if (!static_cast<DefineButtonSoundTag*>(tag)->button)
					delete tag;
				break;
			}
			case END_TAG:
				delete tag;
				done=true;
				if(empty && frames.size()!=FrameCount)
					frames.pop_back();
				break;
		}
	}
	while(!done);

	if(frames.size()!=FrameCount)
	{
		//This condition is not critical as Sprites are not executed while being parsed
		LOG(LOG_CALLS,"Inconsistent frame count in Sprite ID " << SpriteID);
	}

	setFramesLoaded(frames.size());
	if (soundheadtag)
		soundheadtag->SoundData->markFinished(true);
	LOG(LOG_TRACE,"DefineSprite done for ID: " << SpriteID);
}

DefineSpriteTag::~DefineSpriteTag()
{
	//This is the actual parsed tag so it also has to clean up the tag in the FrameContainer
	for(auto it=frames.begin();it!=frames.end();++it)
		it->destroyTags();
}

ASObject* DefineSpriteTag::instance(Class_base* c)
{
	Class_base* retClass=nullptr;
	if(c)
		retClass=c;
	else if(bindedTo)
		retClass=bindedTo;
	else if (!loadedFrom->usesActionScript3)
		retClass=Class<AVM1MovieClip>::getClass(loadedFrom->getSystemState());
	else
		retClass=Class<MovieClip>::getClass(loadedFrom->getSystemState());
	MovieClip* spr = !loadedFrom->usesActionScript3 ?
				new (retClass->memoryAccount) AVM1MovieClip(loadedFrom->getInstanceWorker(),retClass, *this, this->getId()) :
				new (retClass->memoryAccount) MovieClip(loadedFrom->getInstanceWorker(),retClass, *this, this->getId());
	if (soundheadtag)
		soundheadtag->setSoundChannel(spr);
	if (soundstartframe != UINT32_MAX)
		spr->setSoundStartFrame(this->soundstartframe);
	spr->loadedFrom=this->loadedFrom;
	// initactions are always executed in Vm thread, no need to check here
	// spr->loadedFrom->AVM1checkInitActions(spr);
	spr->setScalingGrid();
	return spr;
}

void lightspark::ignore(istream& i, int count)
{
	char* buf=new char[count];

	i.read(buf,count);

	delete[] buf;
}

DefineFontInfoTag::DefineFontInfoTag(RECORDHEADER h, std::istream& in,RootMovieClip* root):Tag(h)
{
	LOG(LOG_TRACE,"DefineFontInfoTag");
	UI16_SWF FontID;
	in >> FontID;
	FontTag* fonttag = dynamic_cast<FontTag*>(root->dictionaryLookup(FontID));
	if (!fonttag)
	{
		LOG(LOG_ERROR,"DefineFontInfoTag font not found:"<<FontID);
		ignore(in,h.getLength()-2);
		return;
	}
	UI8 FontNameLen;
	in >> FontNameLen;
	fonttag->fontname = tiny_string(in,FontNameLen);

	BitStream bs(in);
	UB(2,bs);
	fonttag->FontFlagsSmallText = UB(1,bs);
	fonttag->FontFlagsShiftJIS = UB(1,bs);
	fonttag->FontFlagsANSI = UB(1,bs);
	fonttag->FontFlagsItalic = UB(1,bs);
	fonttag->FontFlagsBold = UB(1,bs);

	bool FontFlagsWideCodes = UB(1,bs);
	int NumGlyphs = fonttag->getGlyphShapes().size();
	if(FontFlagsWideCodes)
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI16_SWF t;
			in >> t;
			fonttag->CodeTable.push_back(t);
		}
	}
	else
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI8 t;
			in >> t;
			fonttag->CodeTable.push_back(t);
		}
	}
	root->registerEmbeddedFont(fonttag->getFontname(),fonttag);
}

FontTag::FontTag(RECORDHEADER h, int _scaling, RootMovieClip* root):DictionaryTag(h,root), scaling(_scaling)
{
	FILLSTYLE fs(1);
	fs.FillStyleType = SOLID_FILL;
	fs.Color = RGBA(255,255,255,255);
	fillStyles.push_back(fs);
}

ASObject* FontTag::instance(Class_base* c)
{ 
	Class_base* retClass=nullptr;
	if(c)
		retClass=c;
	else if(bindedTo)
		retClass=bindedTo;
	else
		retClass=Class<ASFont>::getClass(loadedFrom->getSystemState());

	ASFont* ret=new (retClass->memoryAccount) ASFont(loadedFrom->getInstanceWorker(),retClass);
	LOG(LOG_NOT_IMPLEMENTED,"FontTag::instance doesn't handle all font properties");
	ret->SetFont(fontname,FontFlagsBold,FontFlagsItalic,true,false);
	return ret;
}

const TextureChunk* FontTag::getCharTexture(const CharIterator& chrIt, int fontpixelsize,uint32_t& codetableindex)
{
	assert (*chrIt != 13 && *chrIt != 10);
	int tokenscaling = fontpixelsize * this->scaling;
	codetableindex=UINT32_MAX;

	for (unsigned int i = 0; i < CodeTable.size(); i++)
	{
		if (CodeTable[i] == *chrIt)
		{
			codetableindex=i;
			auto it = getGlyphShapes().at(i).scaledtexturecache.find(tokenscaling);
			if (it == getGlyphShapes().at(i).scaledtexturecache.end())
			{
				const std::vector<SHAPERECORD>& sr = getGlyphShapes().at(i).ShapeRecords;
				number_t ystart = getRenderCharStartYPos()/1024.0f;
				ystart *=number_t(tokenscaling);
				MATRIX glyphMatrix(number_t(tokenscaling)/1024.0f, number_t(tokenscaling)/1024.0f, 0, 0,0,ystart);
				tokensVector tmptokens;
				TokenContainer::FromShaperecordListToShapeVector(sr,tmptokens,fillStyles,glyphMatrix);
				number_t xmin, xmax, ymin, ymax;
				if (!TokenContainer::boundsRectFromTokens(tmptokens,0.05,xmin,xmax,ymin,ymax))
					return nullptr;
				CairoTokenRenderer r(tmptokens,MATRIX()
							, xmin, ymin, xmax, ymax
							, 1, 1
							, false, false
							, 0.05,1.0
							, ColorTransformBase()
							, SMOOTH_MODE::SMOOTH_SUBPIXEL,AS_BLENDMODE::BLENDMODE_NORMAL,0,0);
				uint8_t* buf = r.getPixelBuffer();
				CharacterRenderer* renderer = new CharacterRenderer(buf,abs(xmax),abs(ymax));
				//force creation of buffer if neccessary
				renderer->upload(true);
				//Get the texture to be sure it's allocated when the upload comes
				renderer->getTexture();
				uint32_t w,h;
				renderer->sizeNeeded(w,h);
				getSys()->getRenderThread()->loadChunkBGRA(renderer->getTexture(),w,h,buf);
				renderer->uploadFence();
				it = getGlyphShapes().at(i).scaledtexturecache.insert(make_pair(tokenscaling,renderer)).first;
			}
			return &(*it).second->getTexture();
		}
	}
	return nullptr;
}

bool FontTag::hasGlyphs(const tiny_string text) const
{
	if (!CodeTable.size())
	{
		// always return false if CodeTable is empty;
		return false;
	}
	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		bool found = false;
		if (*it <= 0x20)
			continue;
		for (unsigned int i = 0; i < CodeTable.size(); i++)
		{
			if (CodeTable[i] == *it)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

number_t DefineFontTag::getRenderCharStartYPos() const 
{
	return 1024;
}

number_t DefineFontTag::getRenderCharAdvance(uint32_t index) const
{
	return 0;
}

void DefineFontTag::getTextBounds(const tiny_string& text, int fontpixelsize, number_t& width, number_t& height)
{
	int tokenscaling = fontpixelsize * this->scaling;
	width=0;
	height=tokenscaling;
	number_t tmpwidth=0;

	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		if (*it == 13 || *it == 10)
		{
			if (width < tmpwidth)
				width = tmpwidth;
			tmpwidth = 0;
			height+=tokenscaling;
		}
		else
		{
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					tmpwidth += tokenscaling;
					break;
				}
			}
		}
	}
	if (width < tmpwidth)
		width = tmpwidth;
}

DefineFontTag::DefineFontTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):FontTag(h, 20, root)
{
	LOG(LOG_TRACE,"DefineFont");
	in >> FontID;

	UI16_SWF t;
	int NumGlyphs=0;
	in >> t;
	OffsetTable.push_back(t);
	NumGlyphs=t/2;

	for(int i=1;i<NumGlyphs;i++)
	{
		in >> t;
		OffsetTable.push_back(t);
	}

	for(int i=0;i<NumGlyphs;i++)
	{
		SHAPE t(0,true);
		in >> t;
		GlyphShapeTable.push_back(t);
	}
	root->registerEmbeddedFont("",this);
}

void DefineFontTag::fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, const list<FILLSTYLE>& fillstyleColor, int32_t leading, int32_t startposx, int32_t startposy)
{
	Vector2 curPos;

	int tokenscaling = fontpixelsize * this->scaling;
	curPos.y = 1024 + startposy*1024;

	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		if (*it == 13 || *it == 10)
		{
			curPos.x = 0;
			curPos.y += leading*1024;
		}
		else
		{
			bool found = false;
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					const std::vector<SHAPERECORD>& sr = getGlyphShapes().at(i).ShapeRecords;
					Vector2 glyphPos = curPos*tokenscaling;
					MATRIX glyphMatrix(tokenscaling, tokenscaling, 0, 0,
							   glyphPos.x+startposx*1024*20,
							   glyphPos.y);
					TokenContainer::FromShaperecordListToShapeVector(sr,tokens,fillstyleColor,glyphMatrix);
					curPos.x += tokenscaling;
					found = true;
					break;
				}
			}
			if (!found)
				LOG(LOG_INFO,"DefineFontTag:Character not found:"<<(int)*it<<" "<<text<<" "<<this->getFontname()<<" "<<CodeTable.size());
		}
	}
}
number_t DefineFont2Tag::getRenderCharStartYPos() const
{
	return (1024+this->FontLeading/2.0);
}

number_t DefineFont2Tag::getRenderCharAdvance(uint32_t index) const
{
	if (index < FontAdvanceTable.size())
		return number_t(FontAdvanceTable[index])/1024.0;
	return 0;
}

void DefineFont2Tag::getTextBounds(const tiny_string& text, int fontpixelsize, number_t& width, number_t& height)
{
	int tokenscaling = fontpixelsize * this->scaling;
	width=0;
	height= (number_t(FontAscent+FontDescent+FontLeading)/1024.0)* tokenscaling;
	number_t tmpwidth=0;

	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		if (*it == 13 || *it == 10)
		{
			if (width < tmpwidth)
				width = tmpwidth;
			tmpwidth = 0;
			height+=tokenscaling+ (number_t(this->FontLeading)/1024.0)*this->scaling;
		}
		else
		{
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					if (FontFlagsHasLayout)
						tmpwidth += number_t(FontAdvanceTable[i])/1024.0 * fontpixelsize;
					else
						tmpwidth += tokenscaling;
					break;
				}
			}
		}
	}
	if (width < tmpwidth)
		width = tmpwidth;
}

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):FontTag(h, 20, root)
{
	LOG(LOG_TRACE,"DefineFont2");
	in >> FontID;
	BitStream bs(in);
	FontFlagsHasLayout = UB(1,bs);
	FontFlagsShiftJIS = UB(1,bs);
	FontFlagsSmallText = UB(1,bs);
	FontFlagsANSI = UB(1,bs);
	FontFlagsWideOffsets = UB(1,bs);
	FontFlagsWideCodes = UB(1,bs);
	FontFlagsItalic = UB(1,bs);
	FontFlagsBold = UB(1,bs);
	UI8 FontNameLen;
	std::vector <UI8> FontName;
	in >> LanguageCode >> FontNameLen;
	for(int i=0;i<FontNameLen;i++)
	{
		UI8 t;
		in >> t;
		FontName.push_back(t);
	}
	FontName.push_back(UI8(0));// ensure that fontname ends with \0
	fontname = tiny_string((const char*)FontName.data(),true);

	in >> NumGlyphs;
	if(FontFlagsWideOffsets)
	{
		UI32_SWF t;
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		in >> t;
		CodeTableOffset=t;
	}
	else
	{
		UI16_SWF t;
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		in >> t;
		CodeTableOffset=t;
	}
	for(int i=0;i<NumGlyphs;i++)
	{
		SHAPE t(0,true);
		in >> t;
		GlyphShapeTable.push_back(t);
	}
	if(FontFlagsWideCodes)
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI16_SWF t;
			in >> t;
			CodeTable.push_back(t);
		}
	}
	else
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI8 t;
			in >> t;
			CodeTable.push_back(t);
		}
	}
	if(FontFlagsHasLayout)
	{
		in >> FontAscent >> FontDescent >> FontLeading;

		for(int i=0;i<NumGlyphs;i++)
		{
			SI16_SWF t;
			in >> t;
			FontAdvanceTable.push_back(t);
		}
		for(int i=0;i<NumGlyphs;i++)
		{
			RECT t;
			in >> t;
			FontBoundsTable.push_back(t);
		}
		in >> KerningCount;
	}
	//TODO: implmented Kerning support
	ignore(in,KerningCount*4);
	root->registerEmbeddedFont(getFontname(),this);
}

void DefineFont2Tag::fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, const list<FILLSTYLE>& fillstyleColor, int32_t leading, int32_t startposx, int32_t startposy)
{
	Vector2 curPos;

	int tokenscaling = fontpixelsize * this->scaling;
	curPos.y = (1024+this->FontLeading/2.0)+startposy*1024;

	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		if (*it == 13 || *it == 10)
		{
			curPos.x = 0;
			curPos.y += leading*1024;
		}
		else
		{
			bool found = false;
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					const std::vector<SHAPERECORD>& sr = getGlyphShapes().at(i).ShapeRecords;
					Vector2 glyphPos = curPos*tokenscaling;
					MATRIX glyphMatrix(tokenscaling, tokenscaling, 0, 0,
							   glyphPos.x+startposx*1024*20,
							   glyphPos.y);
					TokenContainer::FromShaperecordListToShapeVector(sr,tokens,fillstyleColor,glyphMatrix);
					if (FontFlagsHasLayout)
						curPos.x += FontAdvanceTable[i];
					else
						curPos.x += tokenscaling;
					found = true;
					break;
				}
			}
			if (!found)
				LOG(LOG_INFO,"DefineFont2Tag:Character not found:"<<(int)*it<<" "<<text<<" "<<this->getFontname()<<" "<<CodeTable.size());
		}
	}
}

number_t DefineFont3Tag::getRenderCharStartYPos() const
{
	return FontAscent;
}

number_t DefineFont3Tag::getRenderCharAdvance(uint32_t index) const
{
	if (index < FontAdvanceTable.size())
		return number_t(FontAdvanceTable[index])/1024.0/20.0;
	return 0;
}

void DefineFont3Tag::getTextBounds(const tiny_string& text, int fontpixelsize, number_t& width, number_t& height)
{
	int tokenscaling = fontpixelsize * this->scaling;
	width=0;
	height= (number_t(FontAscent+FontDescent)/1024.0/20.0)* tokenscaling;
	number_t tmpwidth=0;

	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		if (*it == 13 || *it == 10)
		{
			if (width < tmpwidth)
				width = tmpwidth;
			tmpwidth = 0;
			height+=tokenscaling+ (number_t(this->FontLeading)/1024.0/20.0)*this->scaling;
		}
		else
		{
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					if (FontFlagsHasLayout)
						tmpwidth += number_t(FontAdvanceTable[i])/1024.0/20.0 * tokenscaling;
					else
					{
						const std::vector<SHAPERECORD>& sr = getGlyphShapes().at(i).ShapeRecords;
						number_t ystart = getRenderCharStartYPos()/1024.0f;
						ystart *=number_t(tokenscaling);
						MATRIX glyphMatrix(number_t(tokenscaling)/1024.0f, number_t(tokenscaling)/1024.0f, 0, 0,0,ystart);
						tokensVector tmptokens;
						TokenContainer::FromShaperecordListToShapeVector(sr,tmptokens,fillStyles,glyphMatrix);
						number_t xmin, xmax, ymin, ymax;
						if (TokenContainer::boundsRectFromTokens(tmptokens,0.05,xmin,xmax,ymin,ymax))
							tmpwidth += xmax-xmin;
						else
							tmpwidth += tokenscaling/2.0;
					}
					break;
				}
			}
		}
	}
	if (width < tmpwidth)
		width = ceil(tmpwidth);
}

DefineFont3Tag::DefineFont3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):FontTag(h, 1, root),CodeTableOffset(0)
{
	LOG(LOG_TRACE,"DefineFont3");
	in >> FontID;
	BitStream bs(in);
	FontFlagsHasLayout = UB(1,bs);
	FontFlagsShiftJIS = UB(1,bs);
	FontFlagsSmallText = UB(1,bs);
	FontFlagsANSI = UB(1,bs);
	FontFlagsWideOffsets = UB(1,bs);
	FontFlagsWideCodes = UB(1,bs);
	FontFlagsItalic = UB(1,bs);
	FontFlagsBold = UB(1,bs);
	UI8 FontNameLen;
	std::vector <UI8> FontName;
	in >> LanguageCode >> FontNameLen;
	for(int i=0;i<FontNameLen;i++)
	{
		UI8 t;
		in >> t;
		FontName.push_back(t);
	}
	FontName.push_back(UI8(0));// ensure that fontname ends with \0
	fontname = tiny_string((const char*)FontName.data(),true);
	in >> NumGlyphs;
	streampos offsetReference = in.tellg();
	if(FontFlagsWideOffsets)
	{
		UI32_SWF t;
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		//CodeTableOffset is missing with zero NumGlyphs
		if(NumGlyphs)
		{
			in >> t;
			CodeTableOffset=t;
		}
	}
	else
	{
		UI16_SWF t;
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		//CodeTableOffset is missing with zero NumGlyphs
		if(NumGlyphs)
		{
			in >> t;
			CodeTableOffset=t;
		}
	}

	GlyphShapeTable.resize(NumGlyphs);
	for(int i=0;i<NumGlyphs;i++)
	{
		//It seems legal to have 1 byte records. We ignore
		//them, because 1 byte is too short to encode a SHAPE.
		if ((i < NumGlyphs-1) && 
		    (OffsetTable[i+1]-OffsetTable[i] == 1))
		{
			char ignored;
			in.get(ignored);
			continue;
		}

		in >> GlyphShapeTable[i];
	}

	//sanity check the stream position
	streampos expectedPos = offsetReference + (streampos)CodeTableOffset;
	if (in.tellg() != expectedPos)
	{
		LOG(LOG_ERROR, "Malformed SWF file: unexpected offset in DefineFont3 tag:"<<in.tellg()<<" "<<expectedPos<<" "<<NumGlyphs);
		if (in.tellg() < expectedPos)
		{
			//Read too few bytes => We can still continue
			ignore(in, expectedPos-in.tellg());
		}
		else
		{
			//Read too many bytes => fail
			assert(in.tellg() == expectedPos);
		}
	}

	for(int i=0;i<NumGlyphs;i++)
	{
		UI16_SWF t;
		in >> t;
		CodeTable.push_back(t);
	}
	if(FontFlagsHasLayout)
	{
		in >> FontAscent >> FontDescent >> FontLeading;

		for(int i=0;i<NumGlyphs;i++)
		{
			SI16_SWF t;
			in >> t;
			FontAdvanceTable.push_back(t);
		}
		for(int i=0;i<NumGlyphs;i++)
		{
			RECT t;
			in >> t;
			FontBoundsTable.push_back(t);
		}
		in >> KerningCount;
	}
	//TODO: implment Kerning support
	ignore(in,KerningCount* (FontFlagsWideCodes ? 6 : 4));
	root->registerEmbeddedFont(getFontname(),this);

}

void DefineFont3Tag::fillTextTokens(tokensVector &tokens, const tiny_string text, int fontpixelsize, const list<FILLSTYLE>& fillstyleColor, int32_t leading, int32_t startposx, int32_t startposy)
{
	Vector2 curPos;

	int tokenscaling = fontpixelsize * this->scaling;
	curPos.y = getRenderCharStartYPos()*this->scaling;
	for (CharIterator it = text.begin(); it != text.end(); it++)
	{
		assert (*it != 13 && *it != 10);
		if (*it == 13 || *it == 10)
		{
			LOG(LOG_ERROR,"fillTextToken called for char \\r or \\n, should not happen:"<<text);
		}
		else
		{
			bool found = false;
			for (unsigned int i = 0; i < CodeTable.size(); i++)
			{
				if (CodeTable[i] == *it)
				{
					const std::vector<SHAPERECORD>& sr = getGlyphShapes().at(i).ShapeRecords;
					Vector2 glyphPos = curPos*tokenscaling;
					MATRIX glyphMatrix(tokenscaling, tokenscaling, 0, 0,
							   glyphPos.x+startposx*1024*20,
							   glyphPos.y+startposy*1024*20);
					TokenContainer::FromShaperecordListToShapeVector(sr,tokens,fillstyleColor,glyphMatrix);
					if (FontFlagsHasLayout)
						curPos.x += FontAdvanceTable[i];
					found = true;
					break;
				}
			}
			if (!found)
				LOG(LOG_INFO,"DefineFont3Tag:Character not found:"<<(int)*it<<" "<<text<<" "<<this->getFontname()<<" "<<CodeTable.size());
		}
	}
}

DefineFont4Tag::DefineFont4Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h,root)
{
	LOG(LOG_TRACE,"DefineFont4");
	int dest=in.tellg();
	dest+=h.getLength();

	in >> FontID;
	BitStream bs(in);
	UB(5,bs); /* reserved */
	FontFlagsHasFontData = UB(1,bs);
	FontFlagsItalic = UB(1,bs);
	FontFlagsBold = UB(1,bs);
	in >> FontName;

	if(FontFlagsHasFontData)
		LOG(LOG_NOT_IMPLEMENTED,"DefineFont4Tag with FontData");
	ignore(in,dest-in.tellg());
}
ASObject* DefineFont4Tag::instance(Class_base* c)
{ 
	tiny_string fontname = FontName;
	Class_base* retClass=nullptr;
	if(c)
		retClass=c;
	else if(bindedTo)
		retClass=bindedTo;
	else
		retClass=Class<ASFont>::getClass(loadedFrom->getSystemState());

	ASFont* ret=new (retClass->memoryAccount) ASFont(loadedFrom->getInstanceWorker(),retClass);
	LOG(LOG_NOT_IMPLEMENTED,"DefineFont4Tag::instance doesn't handle all font properties");
	ret->SetFont(fontname,FontFlagsBold,FontFlagsItalic,FontFlagsHasFontData,false);
	return ret;
}

BitmapTag::BitmapTag(RECORDHEADER h,RootMovieClip* root):DictionaryTag(h,root),bitmap(_MR(new BitmapContainer(root->getSystemState()->tagsMemory)))
{
}

BitmapTag::~BitmapTag()
{
	bitmap.reset();
}

_NR<BitmapContainer> BitmapTag::getBitmap() const {
	return bitmap;
}
void BitmapTag::loadBitmap(uint8_t* inData, int datasize, const uint8_t *tablesData, int tablesLen)
{
	if (datasize < 4)
		return;
	else if((inData[0]&0x80) && inData[1]=='P' && inData[2]=='N' && inData[3]=='G')
		bitmap->fromPNG(inData,datasize);
	else if(inData[0]==0xff && inData[1]==0xd8 && inData[2]==0xff)
		bitmap->fromJPEG(inData,datasize,tablesData,tablesLen);
	else if(inData[0]=='G' && inData[1]=='I' && inData[2]=='F' && inData[3]=='8')
		bitmap->fromGIF(inData,datasize,loadedFrom->getSystemState());
	else if(inData[0]==0xff && inData[1]==0xd9)
		// I've found swf files with broken jpegs that start with the jpeg "end of file" magic bytes and two times the "begin of file" magic bytes
		// so we just ignore the first 4 bytes
		// TODO check if libjpeg has a better common way to deal with invalid headers
		loadBitmap(inData+4, datasize-4, tablesData, tablesLen);
	else
		LOG(LOG_ERROR,"unknown image format for ID "<<getId());
}
DefineBitsLosslessTag::DefineBitsLosslessTag(RECORDHEADER h, istream& in, int version, RootMovieClip* root):BitmapTag(h,root),BitmapColorTableSize(0)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> CharacterId >> BitmapFormat >> BitmapWidth >> BitmapHeight;

	if(BitmapFormat==LOSSLESS_BITMAP_PALETTE)
		in >> BitmapColorTableSize;

	string cData;
	size_t cSize = dest-in.tellg(); //rest of this tag
	cData.resize(cSize);
	in.read(&cData[0], cSize);
	istringstream cDataStream(cData);
	zlib_filter zf(cDataStream.rdbuf());
	istream zfstream(&zf);

	if (BitmapFormat == LOSSLESS_BITMAP_RGB15 ||
	    BitmapFormat == LOSSLESS_BITMAP_RGB24)
	{
		size_t size = BitmapWidth * BitmapHeight * 4;
		uint8_t* inData=new(nothrow) uint8_t[size];
		zfstream.read((char*)inData,size);
		assert(!zfstream.fail() && !zfstream.eof());

		BitmapContainer::BITMAP_FORMAT format;
		if (BitmapFormat == LOSSLESS_BITMAP_RGB15)
			format = BitmapContainer::RGB15;
		else if (version == 1)
			format = BitmapContainer::RGB32;
		else
			format = BitmapContainer::ARGB32;

		bitmap->fromRGB(inData, BitmapWidth, BitmapHeight, format);
	}
	else if (BitmapFormat == LOSSLESS_BITMAP_PALETTE)
	{
		unsigned numColors = BitmapColorTableSize+1;

		/* Bitmap rows are 32 bit aligned */
		uint32_t stride = BitmapWidth;
		while (stride % 4 != 0)
			stride++;

		unsigned int paletteBPP;
		if (version == 1)
			paletteBPP = 3;
		else
			paletteBPP = 4;

		size_t size = paletteBPP*numColors + stride*BitmapHeight;
		uint8_t* inData=new(nothrow) uint8_t[size];
		zfstream.read((char*)inData,size);
		assert(!zfstream.fail() && !zfstream.eof());

		uint8_t *palette = inData;
		uint8_t *pixelData = inData + paletteBPP*numColors;
		bitmap->fromPalette(pixelData, BitmapWidth, BitmapHeight, stride, palette, numColors, paletteBPP);
		delete[] inData;
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"DefineBitsLossless(2)Tag with unsupported BitmapFormat " << BitmapFormat);
	}
}

ASObject* BitmapTag::instance(Class_base* c)
{
	//Flex imports bitmaps using BitmapAsset as the base class, which is derived from bitmap
	//Also BitmapData is used in the wild though, so support both cases

	Class_base* realClass=(c)?c:bindedTo;
	Class_base* classRet = nullptr;
	if (loadedFrom->usesActionScript3)
	{
		classRet = Class<BitmapData>::getClass(loadedFrom->getSystemState());
		if(!realClass)
			return new (classRet->memoryAccount) BitmapData(loadedFrom->getInstanceWorker(),classRet, bitmap);
		if(realClass->isSubClass(Class<Bitmap>::getClass(realClass->getSystemState())))
		{
			BitmapData* ret=new (classRet->memoryAccount) BitmapData(loadedFrom->getInstanceWorker(),classRet, bitmap);
			Bitmap* bitmapRet= new (realClass->memoryAccount) Bitmap(loadedFrom->getInstanceWorker(),realClass,_MR(ret));
			return bitmapRet;
		}
		else
			return new (classRet->memoryAccount) BitmapData(loadedFrom->getInstanceWorker(),realClass, bitmap);
	}
	else
	{
		classRet = Class<AVM1BitmapData>::getClass(loadedFrom->getSystemState());
		if(!realClass)
			return new (classRet->memoryAccount) AVM1BitmapData(loadedFrom->getInstanceWorker(),classRet, bitmap);
		if(realClass->isSubClass(Class<AVM1Bitmap>::getClass(realClass->getSystemState())))
		{
			AVM1BitmapData* ret=new (classRet->memoryAccount) AVM1BitmapData(loadedFrom->getInstanceWorker(),classRet, bitmap);
			Bitmap* bitmapRet= new (realClass->memoryAccount) AVM1Bitmap(loadedFrom->getInstanceWorker(),realClass,_MR(ret));
			return bitmapRet;
		}
		else
			return new (classRet->memoryAccount) AVM1BitmapData(loadedFrom->getInstanceWorker(),realClass, bitmap);
	}

	if(realClass->isSubClass(Class<BitmapData>::getClass(realClass->getSystemState())))
	{
		classRet = realClass;
	}

	return new (classRet->memoryAccount) BitmapData(loadedFrom->getInstanceWorker(),classRet, bitmap);
}

DefineTextTag::DefineTextTag(RECORDHEADER h, istream& in, RootMovieClip* root,int v):DictionaryTag(h,root),version(v)
{
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;
	assert(v==1 || v==2);
	if(v==1)
		LOG(LOG_TRACE,"DefineText ID " << CharacterId);
	else if(v==2)
		LOG(LOG_TRACE,"DefineText2 ID " << CharacterId);

	TEXTRECORD t(this);
	while(1)
	{
		in >> t;
		if(t.TextRecordType+t.StyleFlagsHasFont+t.StyleFlagsHasColor+t.StyleFlagsHasYOffset+t.StyleFlagsHasXOffset==0)
			break;
		TextRecords.push_back(t);
	}
}

ASObject* DefineTextTag::instance(Class_base* c)
{
	/* we cannot call computeCached in the constructor
	 * because loadedFrom is not available there for dictionary lookups
	 */
	if(tokens.empty())
		computeCached();

	if(c==nullptr)
		c=Class<StaticText>::getClass(loadedFrom->getSystemState());

	StaticText* ret=new (c->memoryAccount) StaticText(loadedFrom->getInstanceWorker(),c, tokens,TextBounds,this->getId());
	return ret;
}

void DefineTextTag::computeCached()
{
	if(!tokens.empty())
		return;

	FontTag* curFont = nullptr;
	Vector2f curPos;

	/*
	 * All coordinates are scaled into 1024*20*20 units per pixel.
	 * This is scaled back to pixels by cairo. (1024 is the glyph
	 * EM square scale, 20 is twips-per-pixel and the second 20
	 * comes from TextHeight, which is also in twips)
	 */
	const int twipsScaling = 1024*20;
	const int pixelScaling = 1024*20*20;

	// Scale the translation component of TextMatrix.
	MATRIX scaledTextMatrix = TextMatrix;
	scaledTextMatrix.translate((TextMatrix.getTranslateX()-TextBounds.Xmin/20) *pixelScaling,(TextMatrix.getTranslateY()-TextBounds.Ymin/20) *pixelScaling);

	for(size_t i=0; i< TextRecords.size();++i)
	{
		if(TextRecords[i].StyleFlagsHasFont)
		{
			DictionaryTag* it3=loadedFrom->dictionaryLookup(TextRecords[i].FontID);
			curFont=dynamic_cast<FontTag*>(it3);
			assert_and_throw(curFont);
		}
		assert_and_throw(curFont);
		FILLSTYLE fs(1);
		fs.FillStyleType = SOLID_FILL;
		fs.Color = RGBA(0,0,0,255);
		if(TextRecords[i].StyleFlagsHasColor)
			fs.Color = TextRecords[i].TextColor;
		else if (!fillStyles.empty())
			fs.Color = fillStyles.back().Color;
		fillStyles.push_back(fs);
		if(TextRecords[i].StyleFlagsHasXOffset)
		{
			curPos.x = TextRecords[i].XOffset;
		}
		if(TextRecords[i].StyleFlagsHasYOffset)
		{
			curPos.y = TextRecords[i].YOffset;
		}
		/*
		 * In DefineFont3Tags, shape's coordinates are 1024*20 times pixels size,
		 * in all former DefineFont*Tags, its just 1024 times. We scale everything here
		 * to 1024*20, so curFont->scaling=20 for DefineFont2Tags and DefineFontTags.
		 * And, of course, scale by the TextHeight.
		 */
		int scaling = TextRecords[i].TextHeight * curFont->scaling;
		for(uint32_t j=0;j<TextRecords[i].GlyphEntries.size();++j)
		{
			const GLYPHENTRY& ge = TextRecords[i].GlyphEntries[j];
			// use copy of shaperecords to modify fillstyle
			std::vector<SHAPERECORD> sr = curFont->getGlyphShapes().at(ge.GlyphIndex).ShapeRecords;
			//set fillstyle of each glyph to fillstyle of this TextRecord
			for (auto it = sr.begin(); it != sr.end(); it++)
				(*it).FillStyle0 = i+1;
			Vector2f glyphPos = curPos*twipsScaling;

			MATRIX glyphMatrix(scaling, scaling, 0, 0, 
						glyphPos.x,
						glyphPos.y);

			//Apply glyphMatrix first, then scaledTextMatrix
			glyphMatrix = scaledTextMatrix.multiplyMatrix(glyphMatrix);

			TokenContainer::FromShaperecordListToShapeVector(sr,tokens,fillStyles,glyphMatrix,list<LINESTYLE2>(),this->TextBounds);
			curPos.x += ge.GlyphAdvance;
		}
	}
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h,int v,RootMovieClip* root):DictionaryTag(h,root),Shapes(v),tokens(nullptr)
{
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in,RootMovieClip* root):DictionaryTag(h,root),Shapes(1),tokens(nullptr)
{
	LOG(LOG_TRACE,"DefineShapeTag");
	in >> ShapeId >> ShapeBounds >> Shapes;

	if (root->version < 8)
	{
		for (auto& s : Shapes.FillStyles.FillStyles)
		{
			if (s.FillStyleType == REPEATING_BITMAP)
				s.FillStyleType = NON_SMOOTHED_REPEATING_BITMAP;
			else if (s.FillStyleType == CLIPPED_BITMAP)
				s.FillStyleType = NON_SMOOTHED_CLIPPED_BITMAP;
		}
	}
}

DefineShapeTag::~DefineShapeTag()
{
	if (tokens)
		delete tokens;
}

ASObject *DefineShapeTag::instance(Class_base *c)
{
	if (!tokens)
	{
		tokens = new tokensVector();
		for (auto it = Shapes.FillStyles.FillStyles.begin(); it != Shapes.FillStyles.FillStyles.end(); it++)
		{
			it->ShapeBounds = ShapeBounds;
		}
		for (auto it = Shapes.LineStyles.LineStyles2.begin(); it != Shapes.LineStyles.LineStyles2.end(); it++)
		{
			it->FillType.ShapeBounds = ShapeBounds;
		}
		TokenContainer::FromShaperecordListToShapeVector(Shapes.ShapeRecords,*tokens,Shapes.FillStyles.FillStyles,MATRIX(),Shapes.LineStyles.LineStyles2,ShapeBounds);
	}
	Shape* ret=nullptr;
	if(c==nullptr)
	{
		ret= loadedFrom->usesActionScript3 ?
					Class<Shape>::getInstanceSNoArgs(loadedFrom->getInstanceWorker()):
					Class<AVM1Shape>::getInstanceSNoArgs(loadedFrom->getInstanceWorker());
	}
	else
	{
		ret= loadedFrom->usesActionScript3 ?
					 new (c->memoryAccount) Shape(loadedFrom->getInstanceWorker(),c):
					new (c->memoryAccount) AVM1Shape(loadedFrom->getInstanceWorker(),c);
	}
	ret->setupShape(this, 1.0f/20.0f);
	return ret;
}

void DefineShapeTag::resizeCompleted()
{
	this->chunk.makeEmpty();
}

DefineShape2Tag::DefineShape2Tag(RECORDHEADER h, std::istream& in,RootMovieClip* root):DefineShapeTag(h,2,root)
{
	LOG(LOG_TRACE,"DefineShape2Tag");
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape3Tag::DefineShape3Tag(RECORDHEADER h, std::istream& in,RootMovieClip* root):DefineShape2Tag(h,3,root)
{
	LOG(LOG_TRACE,"DefineShape3Tag");
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape4Tag::DefineShape4Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DefineShape3Tag(h,4,root)
{
	LOG(LOG_TRACE,"DefineShape4Tag");
	in >> ShapeId >> ShapeBounds >> EdgeBounds;
	BitStream bs(in);
	UB(5,bs);
	UsesFillWindingRule=UB(1,bs);
	UsesNonScalingStrokes=UB(1,bs);
	UsesScalingStrokes=UB(1,bs);
	in >> Shapes;
}

DefineMorphShapeTag::DefineMorphShapeTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h, root),
	MorphLineStyles(1)
{
	LOG(LOG_TRACE,"DefineMorphShapeTag");
	UI32_SWF Offset;
	in >> CharacterId >> StartBounds >> EndBounds >> Offset >> MorphFillStyles >> MorphLineStyles;
	try
	{
		in >> StartEdges >> EndEdges;
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Invalid data for morph shape");
	}
}

ASObject* DefineMorphShapeTag::instance(Class_base* c)
{
	assert_and_throw(bindedTo==nullptr);
	if(c==nullptr)
		c=Class<MorphShape>::getClass(loadedFrom->getSystemState());
	MorphShape* ret=new (c->memoryAccount) MorphShape(loadedFrom->getInstanceWorker(),c, this);
	return ret;
}

void DefineMorphShapeTag::getTokensForRatio(tokensVector& tokens, uint32_t ratio)
{
	auto it = tokensmap.find(ratio);
	if (it==tokensmap.end())
	{
		it = tokensmap.insert(make_pair(ratio,tokensVector())).first;
		TokenContainer::FromDefineMorphShapeTagToShapeVector(this,it->second,ratio);
	}
	tokens.filltokens.assign(it->second.filltokens.begin(),it->second.filltokens.end());
	tokens.stroketokens.assign(it->second.stroketokens.begin(),it->second.stroketokens.end());
}

DefineMorphShape2Tag::DefineMorphShape2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DefineMorphShapeTag(h, root, 2)
{
	LOG(LOG_TRACE,"DefineMorphShape2Tag");
	UI32_SWF Offset;
	in >> CharacterId >> StartBounds >> EndBounds >> StartEdgeBounds >> EndEdgeBounds;
	BitStream bs(in);
	UB(6,bs);
	UsesNonScalingStrokes=UB(1,bs);
	UsesScalingStrokes=UB(1,bs);
	in >> Offset >> MorphFillStyles >> MorphLineStyles;
	try
	{
		in >> StartEdges >> EndEdges;
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Invalid data for morph shape");
	}
}

//void DefineFont3Tag::genGlyphShape(vector<GeomShape>& s, int glyph)
//{
//	SHAPE& shape=GlyphShapeTable[glyph];
//	FromShaperecordListToShapeVector(shape.ShapeRecords,s);

/*	for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);*/

	//Should check fill state

/*		s.back().graphic.filled0=true;
		s.back().graphic.filled1=false;
		s.back().graphic.stroked=false;

		if(i->state.validFill0)
		{
			if(i->state.fill0!=1)
				LOG(ERROR,"Not valid fill style for font");
		}

		if(i->state.validFill1)
		{
			LOG(ERROR,"Not valid fill style for font");
		}

		if(i->state.validStroke)
		{
			if(i->state.stroke)
			{
				s.back().graphic.stroked=true;
				LOG(ERROR,"Not valid stroke style for font");
//				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
			}
			else
				s.back().graphic.stroked=false;
		}
		else
			s.back().graphic.stroked=false;*/
//}

//void DefineFont2Tag::genGlyphShape(vector<GeomShape>& s, int glyph)
//{
	//SHAPE& shape=GlyphShapeTable[glyph];
	//FromShaperecordListToShapeVector(shape.ShapeRecords,s);

	/*for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);*/

	//Should check fill state

/*		s.back().graphic.filled0=true;
		s.back().graphic.filled1=false;
		s.back().graphic.stroked=false;

		if(i->state.validFill0)
		{
			if(i->state.fill0!=1)
				LOG(ERROR,"Not valid fill style for font");
		}

		if(i->state.validFill1)
		{
			LOG(ERROR,"Not valid fill style for font");
		}

		if(i->state.validStroke)
		{
			if(i->state.stroke)
			{
				s.back().graphic.stroked=true;
				LOG(ERROR,"Not valid stroke style for font");
//				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
			}
			else
				s.back().graphic.stroked=false;
		}
		else
			s.back().graphic.stroked=false;*/
//}

//void DefineFontTag::genGlyphShape(vector<GeomShape>& s,int glyph)
//{
	/*SHAPE& shape=GlyphShapeTable[glyph];
	FromShaperecordListToShapeVector(shape.ShapeRecords,s);

	for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);*/

	//Should check fill state
//}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"ShowFrame");
}

void PlaceObject2Tag::setProperties(DisplayObject* obj, DisplayObjectContainer* parent) const
{
	assert_and_throw(obj);

	//TODO: move these three attributes in PlaceInfo
	if(PlaceFlagHasColorTransform)
	{
		obj->colorTransform=_NR<ColorTransform>(Class<ColorTransform>::getInstanceS(obj->getInstanceWorker(),this->ColorTransformWithAlpha));
	}

	if(PlaceFlagHasRatio)
		obj->Ratio=Ratio;

	if(PlaceFlagHasClipDepth)
		obj->ClipDepth=LEGACY_DEPTH_START+ClipDepth;

	if(PlaceFlagHasName)
	{
		//Set a variable on the parent to link this object
		LOG(LOG_TRACE,"Registering ID " << CharacterId << " with name " << Name);
		if(!PlaceFlagMove)
		{
			obj->name = NameID;
		}
		else
			LOG(LOG_ERROR, "Moving of registered objects not really supported");
	}
	else if (!PlaceFlagMove)
	{
		//Remove the automatic name set by the DisplayObject constructor
		obj->name = BUILTIN_STRINGS::EMPTY;
	}
}

void PlaceObject2Tag::execute(DisplayObjectContainer* parent, bool inskipping)
{
	if(!PlaceFlagHasCharacter && !PlaceFlagMove)
	{
		LOG(LOG_ERROR,"Invalid PlaceObject2Tag that does nothing");
		return;
	}
	bool exists = parent->hasLegacyChildAt(LEGACY_DEPTH_START+Depth);
	uint32_t nameID = 0;
	DisplayObject* currchar=nullptr;
	if (exists)
	{
		currchar = parent->getLegacyChildAt(LEGACY_DEPTH_START+Depth);
		nameID = currchar->name;
		if (parent->LegacyChildRemoveDeletionMark(LEGACY_DEPTH_START+Depth) && currchar->getTagID() != CharacterId)
		{
			parent->deleteLegacyChildAt(LEGACY_DEPTH_START+Depth,inskipping);
			exists = false;
		}
	}
	bool newInstance = false;
	if(PlaceFlagHasCharacter && (!exists || (currchar->getTagID() != CharacterId)))
	{
		//A new character must be placed
		LOG(LOG_TRACE,"Placing ID " << CharacterId);

		if(placedTag==nullptr)
		{
			// tag for CharacterID may be defined after this tag was defined, so we look for it again in the dictionary
			RootMovieClip* root = parent->loadedFrom;
			if (root)
				placedTag=root->dictionaryLookup(CharacterId);
		}
		if(placedTag==nullptr)
		{
			LOG(LOG_ERROR,"no tag to place:"<<CharacterId);
			throw RunTimeException("No tag to place");
		}

		placedTag->loadedFrom->checkBinding(placedTag);

		DisplayObject *toAdd = nullptr;
		if(PlaceFlagHasName)
		{
			nameID = NameID;
		}
		if (!exists)
		{
			// check if we can reuse the DisplayObject from the last declared frame (only relevant if we are moving backwards in the timeline)
			toAdd = parent->getLastFrameChildAtDepth(LEGACY_DEPTH_START+Depth);
			if (toAdd)
				toAdd->incRef();
		}
		if (!toAdd)
		{
			//We can create the object right away
			ASObject* instance = placedTag->instance();
			if (!placedTag->bindedTo)
				instance->setIsInitialized();
			if (instance->is<BitmapData>())
				toAdd = parent->loadedFrom->usesActionScript3 ?
							Class<Bitmap>::getInstanceS(instance->getInstanceWorker(),_R<BitmapData>(instance->as<BitmapData>())) :
							Class<AVM1Bitmap>::getInstanceS(instance->getInstanceWorker(),_R<AVM1BitmapData>(instance->as<AVM1BitmapData>()));
			else
				toAdd=dynamic_cast<DisplayObject*>(instance);
			if(!toAdd && instance)
			{
				//We ignore weird tags. I have seen ASFont
				//(from a DefineFont) being added by PlaceObject2.
				LOG(LOG_NOT_IMPLEMENTED, "Adding non-DisplayObject to display list");
				instance->decRef();
				return;
			}
			newInstance = true;
			if(!PlaceFlagHasName)
				nameID = BUILTIN_STRINGS::EMPTY;
		}
		assert_and_throw(toAdd);

		if (currchar && !PlaceFlagHasMatrix) // reuse matrix of existing DispayObject at this depth
			Matrix = currchar->getMatrix();

		toAdd->loadedFrom=placedTag->loadedFrom;
		//The matrix must be set before invoking the constructor
		toAdd->setLegacyMatrix(Matrix);
		toAdd->legacy = true;
		if (toAdd->placeFrame == UINT32_MAX && parent->is<MovieClip>())
			toAdd->placeFrame = parent->as<MovieClip>()->state.FP;

		setProperties(toAdd, parent);

		if(exists)
		{
			if(!PlaceFlagHasColorTransform) // reuse colortransformation of existing DispayObject at this depth
				toAdd->colorTransform= parent->getLegacyChildAt(LEGACY_DEPTH_START+Depth)->colorTransform;

			if(PlaceFlagMove || (currchar->getTagID() != CharacterId))
			{
				parent->deleteLegacyChildAt(LEGACY_DEPTH_START+Depth,inskipping);

				if (toAdd->is<MovieClip>() && PlaceFlagHasClipAction)
					toAdd->as<MovieClip>()->setupActions(ClipActions);
				/* parent becomes the owner of toAdd */
				parent->insertLegacyChildAt(LEGACY_DEPTH_START+Depth,toAdd,inskipping);
				currchar=toAdd;
			}
			else
				LOG(LOG_ERROR,"Invalid PlaceObject2Tag that overwrites an object without moving at depth "<<LEGACY_DEPTH_START+Depth);
		}
		else
		{
			if (toAdd->is<MovieClip>() && PlaceFlagHasClipAction)
				toAdd->as<MovieClip>()->setupActions(ClipActions);
			/* parent becomes the owner of toAdd */
			parent->insertLegacyChildAt(LEGACY_DEPTH_START+Depth,toAdd,inskipping);
			currchar=toAdd;
		}
	}
	else
	{
		if (currchar)
			setProperties(currchar, parent);
		if (PlaceFlagHasMatrix)
		{
			if(placedTag==nullptr && currchar)
			{
				// tag for CharacterID may be defined after this tag was defined, so we look for it again in the dictionary
				RootMovieClip* root = parent->loadedFrom;
				if (root)
					placedTag=root->dictionaryLookup(currchar->getTagID());
			}
			parent->transformLegacyChildAt(LEGACY_DEPTH_START+Depth,Matrix);
		}
	}
	if (exists && (currchar->getTagID() == CharacterId) && nameID) // reuse name of existing DispayObject at this depth
	{
		currchar->name = nameID;
		currchar->incRef();
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=currchar->name;
		objName.ns.emplace_back(parent->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom v = asAtomHandler::fromObject(currchar);
		parent->setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED,nullptr,parent->getInstanceWorker());
	}
	if (exists && PlaceFlagHasClipAction)
	{
		parent->setupClipActionsAt(LEGACY_DEPTH_START+Depth,ClipActions);
	}
	
	if (exists && PlaceFlagHasRatio)
		parent->checkRatioForLegacyChildAt(LEGACY_DEPTH_START + Depth, Ratio, inskipping);
	else if (exists && PlaceFlagHasCharacter)
		parent->checkRatioForLegacyChildAt(LEGACY_DEPTH_START + Depth, 0, inskipping);
	
	if(PlaceFlagHasColorTransform)
		parent->checkColorTransformForLegacyChildAt(LEGACY_DEPTH_START+Depth,ColorTransformWithAlpha);
	if (newInstance && PlaceFlagHasClipAction && this->ClipActions.AllEventFlags.ClipEventConstruct && currchar)
	{
		// TODO not sure if this is the right place to handle Construct events
		std::map<uint32_t,asAtom> m;
		for (auto it = this->ClipActions.ClipActionRecords.begin();it != this->ClipActions.ClipActionRecords.end(); it++)
		{
			if (it->EventFlags.ClipEventConstruct)
			{
				AVM1context context;
				ACTIONRECORD::executeActions(currchar ,&context,it->actions,it->startactionpos,m);
			}
		}
	}
	if (PlaceFlagHasClipAction && this->ClipActions.AllEventFlags.ClipEventInitialize && currchar)
	{
		// TODO not sure if this is the right place to handle Initialize events
		std::map<uint32_t,asAtom> m;
		for (auto it = this->ClipActions.ClipActionRecords.begin();it != this->ClipActions.ClipActionRecords.end(); it++)
		{
			if (it->EventFlags.ClipEventInitialize)
			{
				AVM1context context;
				ACTIONRECORD::executeActions(currchar ,&context,it->actions,it->startactionpos,m);
			}
		}
	}
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root, AdditionalDataTag *datatag):DisplayListTag(h),ClipActions(root->version,datatag),placedTag(nullptr)
{
	LOG(LOG_TRACE,"PlaceObject2");
	uint32_t startpos = in.tellg();
	BitStream bs(in);
	PlaceFlagHasClipAction=UB(1,bs);
	PlaceFlagHasClipDepth=UB(1,bs);
	PlaceFlagHasName=UB(1,bs);
	PlaceFlagHasRatio=UB(1,bs);
	PlaceFlagHasColorTransform=UB(1,bs);
	PlaceFlagHasMatrix=UB(1,bs);
	PlaceFlagHasCharacter=UB(1,bs);
	PlaceFlagMove=UB(1,bs);
	in >> Depth;
	if(PlaceFlagHasCharacter)
		in >> CharacterId;

	if(PlaceFlagHasMatrix)
		in >> Matrix;

	if(PlaceFlagHasColorTransform)
		in >> ColorTransformWithAlpha;

	if(PlaceFlagHasRatio)
		in >> Ratio;

	if(PlaceFlagHasName)
	{
		in >> Name;
		NameID =root->getSystemState()->getUniqueStringId(Name);
	}
	else
		NameID = BUILTIN_STRINGS::EMPTY;

	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;

	if(PlaceFlagHasClipAction)
	{
		ClipActions.dataskipbytes = (uint32_t(in.tellg())-startpos)+h.getHeaderSize();
		in >> ClipActions;
	}
}

PlaceObject3Tag::PlaceObject3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):PlaceObject2Tag(h,root->version)
{
	LOG(LOG_TRACE,"PlaceObject3");
	uint32_t start = in.tellg();

	BitStream bs(in);
	PlaceFlagHasClipAction=UB(1,bs);
	PlaceFlagHasClipDepth=UB(1,bs);
	PlaceFlagHasName=UB(1,bs);
	PlaceFlagHasRatio=UB(1,bs);
	PlaceFlagHasColorTransform=UB(1,bs);
	PlaceFlagHasMatrix=UB(1,bs);
	PlaceFlagHasCharacter=UB(1,bs);
	PlaceFlagMove=UB(1,bs);
	UB(1,bs); //Reserved
	PlaceFlagOpaqueBackground=UB(1,bs);
	PlaceFlagHasVisible=UB(1,bs);
	PlaceFlagHasImage=UB(1,bs);
	PlaceFlagHasClassName=UB(1,bs);
	PlaceFlagHasCacheAsBitmap=UB(1,bs);
	PlaceFlagHasBlendMode=UB(1,bs);
	PlaceFlagHasFilterList=UB(1,bs);

	in >> Depth;
	if(PlaceFlagHasClassName)// || (PlaceFlagHasImage && PlaceFlagHasCharacter)) // spec is wrong here
	{
		in >> ClassName;
		LOG(LOG_NOT_IMPLEMENTED,"ClassName in PlaceObject3 not yet supported:"<<ClassName);
	}

	if(PlaceFlagHasCharacter)
		in >> CharacterId;

	if(PlaceFlagHasMatrix)
		in >> Matrix;

	if(PlaceFlagHasColorTransform)
		in >> ColorTransformWithAlpha;

	if(PlaceFlagHasRatio)
		in >> Ratio;

	if(PlaceFlagHasName)
	{
		in >> Name;
		NameID =root->getSystemState()->getUniqueStringId(Name);
	}
	else
		NameID = BUILTIN_STRINGS::EMPTY;

	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;

	if(PlaceFlagHasFilterList)
		in >> SurfaceFilterList;

	if(PlaceFlagHasBlendMode)
		in >> BlendMode;

	if(PlaceFlagHasCacheAsBitmap)
	{
		uint32_t end = in.tellg();
		// adobe seems to allow PlaceFlagHasCacheAsBitmap set without actual BitmapCache value
		if (end-start == this->Header.getLength())
			BitmapCache=1;
		else
			in >> BitmapCache;
	}
	if(PlaceFlagHasVisible)
		in >> Visible;
	if(PlaceFlagOpaqueBackground)
		in >> BackgroundColor;

	if(PlaceFlagHasClipAction)
		in >> ClipActions;

	assert_and_throw(!(PlaceFlagHasCharacter && CharacterId==0))

}

void PlaceObject3Tag::setProperties(DisplayObject *obj, DisplayObjectContainer *parent) const
{
	PlaceObject2Tag::setProperties(obj,parent);
	if (PlaceFlagHasBlendMode)
		obj->setBlendMode(BlendMode);
	if (PlaceFlagHasVisible)
		obj->setVisible(Visible);
	if (PlaceFlagOpaqueBackground) // it seems that adobe ignores the alpha value
		obj->opaqueBackground=asAtomHandler::fromUInt((BackgroundColor.Red<<16)|(BackgroundColor.Green<<8)|(BackgroundColor.Blue));
	obj->cacheAsBitmap=this->BitmapCache;
	obj->setFilters(this->SurfaceFilterList);
}

void SetBackgroundColorTag::execute(RootMovieClip* root) const
{
	root->setBackground(BackgroundColor);
}

ProductInfoTag::ProductInfoTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"ProductInfoTag Tag");

	in >> ProductId >> Edition >> MajorVersion >> MinorVersion >> 
	MinorBuild >> MajorBuild >> CompileTimeLo >> CompileTimeHi;

	uint64_t longlongTime = CompileTimeHi;
	longlongTime<<=32;
	longlongTime|=CompileTimeLo;

	LOG(LOG_INFO,"SWF Info:" << 
	endl << "\tProductId:\t\t" << ProductId <<
	endl << "\tEdition:\t\t" << Edition <<
	endl << "\tVersion:\t\t" << int(MajorVersion) << "." << int(MinorVersion) << "." << MajorBuild << "." << MinorBuild <<
	endl << "\tCompileTime:\t\t" << longlongTime);
}

FrameLabelTag::FrameLabelTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	in >> Name;
	if (Header.getLength() > uint32_t(Name.size()+1))
	{
		UI8 NamedAnchor=in.peek();
		if(NamedAnchor==1)
			in >> NamedAnchor;
	}
}
DefineButtonSoundTag::DefineButtonSoundTag(RECORDHEADER h, istream& in, RootMovieClip* root):Tag(h)
{
	LOG(LOG_TRACE,"DefineButtonSoundTag Tag");

	UI16_SWF ButtonId;
	in >> ButtonId;
	in >> SoundID0_OverUpToIdle;
	if (SoundID0_OverUpToIdle)
		in >> SoundInfo0_OverUpToIdle;
	in >> SoundID1_IdleToOverUp;
	if (SoundID1_IdleToOverUp)
		in >> SoundInfo1_IdleToOverUp;
	in >> SoundID2_OverUpToOverDown;
	if (SoundID2_OverUpToOverDown)
		in >> SoundInfo2_OverUpToOverDown;
	in >> SoundID3_OverDownToOverUp;
	if (SoundID3_OverDownToOverUp)
		in >> SoundInfo3_OverDownToOverUp;
	button = dynamic_cast<DefineButtonTag*>(root->dictionaryLookup(ButtonId));
	if (!button)
		LOG(LOG_ERROR,"Button ID not found for DefineButtonSoundTag "<<ButtonId);
	else
		button->sounds = this;
}

DefineButtonTag::DefineButtonTag(RECORDHEADER h, std::istream& in, int version, RootMovieClip* root, AdditionalDataTag *datatag):DictionaryTag(h,root)
{
	sounds=nullptr;
	in >> ButtonId;
	int len = Header.getLength()-2;
	UI16_SWF ActionOffset;
	int pos = in.tellg();
	if (version > 1)
	{
		BitStream bs(in);
		UB(7,bs);
		TrackAsMenu=UB(1,bs);
		pos = in.tellg();
		len-=1;
		in >> ActionOffset;
	}
	else
	{
		TrackAsMenu=false;
		ActionOffset=0;
	}

	BUTTONRECORD br(version);

	do
	{
		in >> br;
		if(br.isNull())
			break;
		Characters.push_back(br);
	}
	while(true);
	
	int realactionoffset = (((int)in.tellg())-pos);
	len -= realactionoffset;
	int datatagskipbytes = Header.getHeaderSize()+realactionoffset + 2 + (version > 1 ? 1 : 0);
	if (root->usesActionScript3)
	{
		// ignore actions when using ActionScript3
		ignore(in,len);
		return;
	}
	if (version == 1)
	{
		BUTTONCONDACTION a;
		a.CondOverDownToOverUp=true; // clicked indicator
		a.startactionpos=0;
		a.actions.resize(len+ (datatag ? datatag->numbytes+datatagskipbytes : 0)+1,0);
		if (datatag)
		{
			a.startactionpos=datatag->numbytes+datatagskipbytes;
			memcpy(a.actions.data(),datatag->bytes,datatag->numbytes);
		}
		in.read((char*)a.actions.data()+a.startactionpos,len);
		condactions.push_back(a);
	}
	else if(ActionOffset)
	{
		if (ActionOffset != realactionoffset)
			throw ParseException("Malformed SWF file, DefineButtonTag: invalid ActionOffset");
		while (true)
		{
			pos = in.tellg();
			BUTTONCONDACTION r;
			in>>r;
			len -= (((int)in.tellg())-pos);
			pos = in.tellg();
			int codesize = (r.CondActionSize ? r.CondActionSize-4 : len);
			r.actions.resize(codesize+ (datatag ? datatag->numbytes+datatagskipbytes+4 : 0)+1,0);
			r.startactionpos=0;
			if (datatag)
			{
				r.startactionpos=datatag->numbytes+datatagskipbytes+4;
				memcpy(r.actions.data(),datatag->bytes,datatag->numbytes);
			}
			in.read((char*)r.actions.data()+r.startactionpos,codesize);
			datatagskipbytes+= codesize+4;
			len -= (((int)in.tellg())-pos);
			condactions.push_back(r);
			if (r.CondActionSize == 0)
				break;
		}
		if (len > 0)
		{
			LOG(LOG_ERROR,"DefineButtonTag "<<ButtonId<<": bytes available after reading all BUTTONCONDACTION entries:"<<len);
			ignore(in,len);
		}
	}
}

DefineButtonTag::~DefineButtonTag()
{
	if (sounds)
		delete sounds;
}

ASObject* DefineButtonTag::instance(Class_base* c)
{
	DisplayObject* states[4] = {nullptr, nullptr, nullptr, nullptr};
	bool isSprite[4] = {false, false, false, false};
	uint32_t curDepth[4];

	/* There maybe multiple DisplayObjects for one state. The official
	 * implementation seems to create a Sprite in that case with
	 * all DisplayObjects as children.
	 */

	auto i = Characters.begin();
	for(;i != Characters.end(); ++i)
	{
		for(int j=0;j<4;++j)
		{
			if(j==0 && !i->ButtonStateDown)
				continue;
			if(j==1 && !i->ButtonStateHitTest)
				continue;
			if(j==2 && !i->ButtonStateOver)
				continue;
			if(j==3 && !i->ButtonStateUp)
				continue;
			DictionaryTag* dict=loadedFrom->dictionaryLookup(i->CharacterID);
			loadedFrom->checkBinding(dict);

			//We can create the object right away
			DisplayObject* state=dynamic_cast<DisplayObject*>(dict->instance());
			assert_and_throw(state);
			//The matrix must be set before invoking the constructor
			state->setLegacyMatrix(dict->MapToBounds(i->PlaceMatrix));
			state->legacy=true;
			state->name = BUILTIN_STRINGS::EMPTY;
			state->setScalingGrid();
			state->loadedFrom=this->loadedFrom;
			if (i->ButtonHasBlendMode && i->buttonVersion == 2)
				state->setBlendMode(i->BlendMode);
			if (i->ButtonHasFilterList)
				state->setFilters(i->FilterList);
			if (i->ColorTransform.isfilled())
				state->colorTransform=_NR<ColorTransform>(Class<ColorTransform>::getInstanceS(loadedFrom->getInstanceWorker(),i->ColorTransform));

			if(states[j] == nullptr)
			{
				states[j] = state;
				curDepth[j] = i->PlaceDepth;
			}
			else
			{
				if(!isSprite[j])
				{
					Sprite* spr = Class<Sprite>::getInstanceSNoArgs(loadedFrom->getInstanceWorker());
					spr->loadedFrom=this->loadedFrom;
					spr->constructionComplete();
					spr->afterConstruction();
					spr->insertLegacyChildAt(LEGACY_DEPTH_START+curDepth[j],states[j]);
					states[j] = spr;
					spr->name = BUILTIN_STRINGS::EMPTY;
					isSprite[j] = true;
				}
				Sprite* spr = Class<Sprite>::cast(states[j]);
				spr->insertLegacyChildAt(LEGACY_DEPTH_START+i->PlaceDepth,state);
			}
		}
	}
	Class_base* realClass=(c)?c:bindedTo;

	if(realClass==nullptr)
	{
		if (!loadedFrom->usesActionScript3)
			realClass=Class<AVM1SimpleButton>::getClass(loadedFrom->getSystemState());
		else
			realClass=Class<SimpleButton>::getClass(loadedFrom->getSystemState());
	}
	SimpleButton* ret= !loadedFrom->usesActionScript3 ?
				new (realClass->memoryAccount) AVM1SimpleButton(loadedFrom->getInstanceWorker(), realClass, states[0], states[1], states[2], states[3],this) :
				new (realClass->memoryAccount) SimpleButton(loadedFrom->getInstanceWorker(), realClass, states[0], states[1], states[2], states[3],this);
	ret->setScalingGrid();
	return ret;
}


DefineVideoStreamTag::DefineVideoStreamTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h, root)
{
	in >> CharacterID >> NumFrames >> Width >> Height;
	BitStream bs(in);
	UB(4,bs);
	VideoFlagsDeblocking=UB(3,bs);
	VideoFlagsSmoothing=UB(1,bs);
	in >> VideoCodecID;
	frames.resize(NumFrames);
}

DefineVideoStreamTag::~DefineVideoStreamTag()
{
}

ASObject* DefineVideoStreamTag::instance(Class_base* c)
{
	Class_base* classRet = nullptr;
	if(c)
		classRet=c;
	else if(bindedTo)
		classRet=bindedTo;
	else if (!loadedFrom->usesActionScript3)
		classRet=Class<AVM1Video>::getClass(loadedFrom->getSystemState());
	else
		classRet=Class<Video>::getClass(loadedFrom->getSystemState());

	if (!loadedFrom->usesActionScript3)
		return new (classRet->memoryAccount) AVM1Video(loadedFrom->getInstanceWorker(),classRet, Width, Height,NumFrames ? this : nullptr);
	else
		return new (classRet->memoryAccount) Video(loadedFrom->getInstanceWorker(),classRet, Width, Height,NumFrames ? this : nullptr);
}

void DefineVideoStreamTag::setFrameData(VideoFrameTag* tag)
{
	assert_and_throw(tag->getFrameNumber()<NumFrames);
	frames[tag->getFrameNumber()]=tag;
}

DefineBinaryDataTag::DefineBinaryDataTag(RECORDHEADER h,std::istream& s,RootMovieClip* root):DictionaryTag(h,root)
{
	LOG(LOG_TRACE,"DefineBinaryDataTag");
	int size=h.getLength();
	s >> Tag >> Reserved;
	size -= sizeof(Tag)+sizeof(Reserved);
	bytes=new uint8_t[size];
	len=size;
	s.read((char*)bytes,size);
}

ASObject* DefineBinaryDataTag::instance(Class_base* c)
{
	uint8_t* b = new uint8_t[len];
	memcpy(b,bytes,len);

	Class_base* classRet = nullptr;
	if(c)
		classRet=c;
	else if(bindedTo)
		classRet=bindedTo;
	else
		classRet=Class<ByteArray>::getClass(loadedFrom->getSystemState());

	ByteArray* ret=new (classRet->memoryAccount) ByteArray(loadedFrom->getInstanceWorker(),classRet, b, len);
	return ret;
}

FileAttributesTag::FileAttributesTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"FileAttributesTag Tag");
	BitStream bs(in);
	UB(1,bs);
	UseDirectBlit=UB(1,bs);
	UseGPU=UB(1,bs);
	HasMetadata=UB(1,bs);
	ActionScript3=UB(1,bs);
	UB(2,bs);
	UseNetwork=UB(1,bs);
	UB(24,bs);
}

DefineSoundTag::DefineSoundTag(RECORDHEADER h, std::istream& in, RootMovieClip* root):DictionaryTag(h,root),SoundData(new MemoryStreamCache(root->getSystemState())),realSampleRate(0),isAttached(false)
{
	LOG(LOG_TRACE,"DefineSound Tag");
	in >> SoundId;
	BitStream bs(in);
	SoundFormat=UB(4,bs);
	SoundRate=UB(2,bs);
	SoundSize=UB(1,bs);
	SoundType=UB(1,bs);
	uint8_t sndinfo = SoundFormat<<4|SoundRate<<2|SoundSize<<1|SoundType;
	in >> SoundSampleCount;
	unsigned int soundDataLength = h.getLength()-7;
	
	if (SoundFormat == LS_AUDIO_CODEC::ADPCM)
	{
		// split ADPCM sound into packets and embed packets into an flv container so that ffmpeg can properly handle it
		uint32_t bpos = 0;
		uint8_t flv[soundDataLength+25];
		
		// flv header
		flv[bpos++] = 'F'; flv[bpos++] = 'L'; flv[bpos++] = 'V';
		flv[bpos++] = 0x01;
		flv[bpos++] = 0x04; // indicate audio tag presence
		flv[bpos++] = 0x00; flv[bpos++] = 0x00; flv[bpos++] = 0x00; flv[bpos++] = 0x09; // DataOffset, end of flv header
		
		flv[bpos++] = 0x00; flv[bpos++] = 0x00; flv[bpos++] = 0x00; flv[bpos++] = 0x00; // PreviousTagSize0
		
		SoundData->append(flv,bpos);
		uint32_t timestamp=0;
		int adpcmcodesize = UB(2,bs);
		int32_t bitstoprocess=soundDataLength*8-2;
		uint32_t datalenbits = (16+6+(adpcmcodesize+2)*4095)*(SoundType+1)-6;// 6 bits are stored in the first byte of the packet (with the bps)
		while (bitstoprocess > 6)
		{
			bpos=0;
			flv[bpos++] = 0x08; // audio tag indicator
			uint32_t datasizepos=bpos;
			bpos += 3; // skip bytes for dataSize, will be filled later
			flv[bpos++] = (timestamp>>16) &0xff; // timestamp
			flv[bpos++] = (timestamp>> 8) &0xff;
			flv[bpos++] = (timestamp    ) &0xff;
			flv[bpos++] = (timestamp>>24) &0xff; // timestamp extended
			timestamp += 4096 *1000 / getSampleRate();
			flv[bpos++] = 0x00; flv[bpos++] = 0x00; flv[bpos++] = 0x00; // StreamID
			
			uint32_t datalenstart=bpos;
			flv[bpos++] = sndinfo; // sound format
			
			// ADPCM packet data
			flv[bpos++] = ((adpcmcodesize<<6) | UB(6,bs))&0xff;
			bitstoprocess-=6;
			for (uint32_t i = 0; (i < datalenbits/8) && (bitstoprocess > 8); i++)
			{
				flv[bpos++] = UB(8,bs)&0xff;
				bitstoprocess-=8;
			}
			uint32_t additionalbits = min(uint32_t(datalenbits%8),uint32_t(bitstoprocess));
			if (additionalbits)
			{
				flv[bpos++] = UB(additionalbits,bs);
				bitstoprocess-=additionalbits;
			}
			// compute and set dataSize
			uint32_t dataSize = bpos-datalenstart;
			flv[datasizepos++] = (dataSize>>16) &0xff;
			flv[datasizepos++] = (dataSize>> 8) &0xff;
			flv[datasizepos++] = (dataSize    ) &0xff;
			SoundData->append(flv,bpos);
			
			// PreviousTagSize
			uint32_t l = bpos;
			flv[0] = (l>>24) &0xff;
			flv[1] = (l>>16) &0xff;
			flv[2] = (l>> 8) &0xff;
			flv[3] = (l    ) &0xff;
			SoundData->append(flv,4);
		}
	}
	else
	{
		//TODO: get rid of the temporary copy
		uint8_t* tmp = new uint8_t[soundDataLength];
		in.read((char *)tmp, soundDataLength);
		unsigned char *tmpp = tmp;
		// it seems that adobe allows zeros at the beginning of the sound data
		// at least for MP3 we ignore them, otherwise ffmpeg will not work properly
		if (SoundFormat == LS_AUDIO_CODEC::MP3)
		{
			while (*tmpp == 0 && soundDataLength)
			{
				soundDataLength--;
				tmpp++;
			}
		}
		SoundData->append(tmpp, soundDataLength);
		delete[] tmp;
	}
	SoundData->markFinished();

#ifdef ENABLE_LIBAVCODEC
	// it seems that ffmpeg doesn't properly detect PCM data, so we only autodetect the sample rate for MP3
	if (SoundFormat == LS_AUDIO_CODEC::MP3 && soundDataLength >= 8192)
	{
		// detect real sample rate regardless of value provided in the tag
		std::streambuf *sbuf = SoundData->createReader();
		istream s(sbuf);
		FFMpegStreamDecoder* streamDecoder=new FFMpegStreamDecoder(nullptr,root->getSystemState()->getEngineData(),s,0);
		realSampleRate  = streamDecoder->getAudioSampleRate();
		delete streamDecoder;
		delete sbuf;
	}
#endif
}

ASObject* DefineSoundTag::instance(Class_base* c)
{
	Class_base* retClass=nullptr;
	if (c)
		retClass=c;
	else if(bindedTo)
		retClass=bindedTo;
	else if (!loadedFrom->usesActionScript3)
		retClass=Class<AVM1Sound>::getClass(loadedFrom->getSystemState());
	else
		retClass=Class<Sound>::getClass(loadedFrom->getSystemState());

	if (!loadedFrom->usesActionScript3)
		return new (retClass->memoryAccount) AVM1Sound(loadedFrom->getInstanceWorker(), retClass, SoundData,
			AudioFormat(getAudioCodec(), getSampleRate(), getChannels()),getDurationInMS());
	else
		return new (retClass->memoryAccount) Sound(loadedFrom->getInstanceWorker(), retClass, SoundData,
			AudioFormat(getAudioCodec(), getSampleRate(), getChannels()),getDurationInMS());
}

LS_AUDIO_CODEC DefineSoundTag::getAudioCodec() const
{
	return (LS_AUDIO_CODEC)SoundFormat;
}
number_t DefineSoundTag::getDurationInMS() const
{
	return ((number_t)SoundSampleCount/(number_t)getSampleRate())*1000.0;
}
int DefineSoundTag::getSampleRate() const
{
	if (realSampleRate)
		return realSampleRate;
	switch(SoundRate)
	{
		case 0:
			return 5512;
		case 1:
			return 11025;
		case 2:
			return 22050;
		case 3:
			return 44100;
	}

	// not reached
	assert(false && "invalid sample rate");
	return 0;
}

int DefineSoundTag::getChannels() const
{
	return (int)SoundType + 1;
}

_R<MemoryStreamCache> DefineSoundTag::getSoundData() const
{
	return SoundData;
}

std::streambuf *DefineSoundTag::createSoundStream() const
{
	return SoundData->createReader();
}

SoundChannel* DefineSoundTag::createSoundChannel(const SOUNDINFO* soundinfo)
{
	return Class<SoundChannel>::getInstanceS(loadedFrom->getInstanceWorker(),1,SoundData, AudioFormat(getAudioCodec(), getSampleRate(), getChannels()),soundinfo);
}

StartSoundTag::StartSoundTag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	LOG(LOG_TRACE,"StartSound Tag");
	in >> SoundId >> SoundInfo;
}

void StartSoundTag::execute(DisplayObjectContainer *parent, bool inskipping)
{
	if (inskipping) // it seems that StartSoundTags are not executed if we are skipping the frame due to a gotoframe action
		return;
	DefineSoundTag *soundTag = dynamic_cast<DefineSoundTag *>(parent->loadedFrom->dictionaryLookup(SoundId));
	if (!soundTag)
		return;

	if (SoundInfo.HasOutPoint || SoundInfo.HasInPoint)
	{
		LOG(LOG_NOT_IMPLEMENTED, "StartSoundTag: some modifiers not supported");
	}
	SoundChannel* sound=nullptr;
	if (parent->is<Sprite>())
	{
		sound = parent->as<Sprite>()->getSoundChannel();
		if (sound && sound->fromSoundTag != soundTag) // sprite is currently not bound to this sound tag
			sound = nullptr;
	}
	if (!sound)
	{
		sound = Class<SoundChannel>::getInstanceS(soundTag->loadedFrom->getInstanceWorker(),1,
			soundTag->getSoundData(),
			AudioFormat(soundTag->getAudioCodec(),
						soundTag->getSampleRate(),
						soundTag->getChannels()),
			&this->SoundInfo);
		sound->fromSoundTag = soundTag;
		if (parent->is<Sprite>())
			parent->as<Sprite>()->setSound(sound,false);
	}
	if (SoundInfo.SyncStop)
	{
		sound->threadAbort();
		return;
	}
	// it seems that adobe ignores the StartSoundTag if SoundInfo.SyncNoMultiple is set and the tag is attached to a Sound object created by actionscript
	if (SoundInfo.SyncNoMultiple && soundTag->isAttached)
		return;
	if (this->SoundInfo.SyncNoMultiple && sound->isPlaying())
		return;
	// it seems that sounds are not played if we get here by gotoandstop
	if (parent->is<MovieClip>() && parent->as<MovieClip>()->state.stop_FP && parent->as<MovieClip>()->state.explicit_FP)
		return;
	sound->play(0);
}

ScriptLimitsTag::ScriptLimitsTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	LOG(LOG_TRACE,"ScriptLimitsTag Tag");
	in >> MaxRecursionDepth >> ScriptTimeoutSeconds;
	LOG(LOG_INFO,"MaxRecursionDepth: " << MaxRecursionDepth << ", ScriptTimeoutSeconds: " << ScriptTimeoutSeconds);
}

DebugIDTag::DebugIDTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"DebugIDTag Tag");
	for(int i = 0; i < 16; i++)
		in >> DebugId[i];

	//Note the switch to hex formatting on the ostream, and switch back to dec
	LOG(LOG_INFO,"DebugId " << hex <<
		int(DebugId[0]) << int(DebugId[1]) << int(DebugId[2]) << int(DebugId[3]) << "-" <<
		int(DebugId[4]) << int(DebugId[5]) << "-" <<
		int(DebugId[6]) << int(DebugId[7]) << "-" <<
		int(DebugId[8]) << int(DebugId[9]) << "-" <<
		int(DebugId[10]) << int(DebugId[11]) << int(DebugId[12]) << int(DebugId[13]) << int(DebugId[14]) << int(DebugId[15]) <<
		dec);
}

EnableDebuggerTag::EnableDebuggerTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"EnableDebuggerTag Tag");
	DebugPassword = "";
	if(h.getLength() > 0)
		in >> DebugPassword;
	LOG(LOG_INFO,"Debugger enabled, password: " << DebugPassword);
}

EnableDebugger2Tag::EnableDebugger2Tag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"EnableDebugger2Tag Tag");
	in >> ReservedWord;

	DebugPassword = "";
	if(h.getLength() > sizeof(ReservedWord))
		in >> DebugPassword;
	LOG(LOG_INFO,"Debugger enabled, reserved: " << ReservedWord << ", password: " << DebugPassword);
}

ExportAssetsTag::ExportAssetsTag(RECORDHEADER h, std::istream& in,RootMovieClip* root):Tag(h)
{
	LOG(LOG_TRACE,"ExportAssets Tag");
	UI16_SWF count;
	in>>count;
	for (uint32_t i = 0; i < count; i++)
	{
		UI16_SWF tagid;
		STRING tagname;
		in >> tagid >> tagname;
		DictionaryTag* tag = root->dictionaryLookup(tagid);
		if (tag)
			tag->nameID = root->getSystemState()->getUniqueStringId(tagname);
		else
			LOG(LOG_ERROR,"ExportAssetsTag: tag not found:"<<tagid<<" "<<tagname);
	}
}
NameCharacterTag::NameCharacterTag(RECORDHEADER h, istream &in, RootMovieClip *root):Tag(h)
{
	UI16_SWF tagid;
	STRING tagname;
	in >> tagid >> tagname;
	DictionaryTag* tag = root->dictionaryLookup(tagid);
	if (tag)
		tag->nameID = root->getSystemState()->getUniqueStringId(tagname);
	else
		LOG(LOG_ERROR,"NameCharacterTag: tag not found:"<<tagid<<" "<<tagname);
}

MetadataTag::MetadataTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"MetadataTag Tag");
	in >> XmlString;
	string XmlStringStd = XmlString;

	pugi::xml_document xml;
	if (xml.load_buffer((const unsigned char*)XmlStringStd.c_str(), XmlStringStd.length()).status == pugi::status_ok)
	{
		pugi::xml_node_iterator it1 = xml.root().begin();
		ostringstream output;
		for (;it1 != xml.root().end(); it1++)
		{
			pugi::xml_node_iterator it2 = it1->begin();
			for (;it2 != it1->end(); it2++)
			{
				if (it2->type() == pugi::node_element)
				{
					output << endl << "\t" << xml.name() << ":\t\t" << xml.value();
				}
			}
		}
		LOG(LOG_INFO, "SWF Metadata:" << output.str());
	}
	else
	{
		LOG(LOG_INFO, "SWF Metadata:" << XmlStringStd);
	}
}

JPEGTablesTag::JPEGTablesTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	tableSize=Header.getLength();
	if (tableSize != 0)
	{
		if (JPEGTables == NULL)
		{
			JPEGTables=new(nothrow) uint8_t[tableSize];
			in.read((char*)JPEGTables, tableSize);
		}
		else
		{
			LOG(LOG_ERROR, "Malformed SWF file: duplicated JPEGTables tag");
			skip(in);
		}
	}
}

const uint8_t* JPEGTablesTag::getJPEGTables()
{
	return JPEGTables;
}

int JPEGTablesTag::getJPEGTableSize()
{
	return tableSize;
}

DefineBitsTag::DefineBitsTag(RECORDHEADER h, std::istream& in,RootMovieClip* root):BitmapTag(h,root)
{
	LOG(LOG_TRACE,"DefineBitsTag Tag");
	if (JPEGTablesTag::getJPEGTables() == NULL)
	{
		LOG(LOG_ERROR, "Malformed SWF file: JPEGTable was expected before DefineBits");
		// try to continue anyway
	}

	in >> CharacterId;
	//Read image data
	int dataSize=Header.getLength()-2;
	uint8_t *inData=new(nothrow) uint8_t[dataSize];
	in.read((char*)inData,dataSize);
	loadBitmap(inData,dataSize,JPEGTablesTag::getJPEGTables(),JPEGTablesTag::getJPEGTableSize());
	delete[] inData;
}

DefineBitsJPEG2Tag::DefineBitsJPEG2Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):BitmapTag(h,root)
{
	LOG(LOG_TRACE,"DefineBitsJPEG2Tag Tag");
	in >> CharacterId;
	//Read image data
	int dataSize=Header.getLength()-2;
	uint8_t* inData=new(nothrow) uint8_t[dataSize];
	in.read((char*)inData,dataSize);
	loadBitmap(inData,dataSize);
	delete[] inData;
}

DefineBitsJPEG3Tag::DefineBitsJPEG3Tag(RECORDHEADER h, std::istream& in, RootMovieClip* root):BitmapTag(h,root),alphaData(NULL)
{
	LOG(LOG_TRACE,"DefineBitsJPEG3Tag Tag");
	UI32_SWF dataSize;
	in >> CharacterId >> dataSize;
	//Read image data
	uint8_t* inData=new(nothrow) uint8_t[dataSize];
	in.read((char*)inData,dataSize);

	loadBitmap(inData,dataSize);
	delete[] inData;

	//Read alpha data (if any)
	int alphaSize=Header.getLength()-dataSize-6;
	if(alphaSize>0) //If less that 0 the consistency check on tag size will stop later
	{
		//Create a zlib filter
		string alphaData;
		alphaData.resize(alphaSize);
		in.read(&alphaData[0], alphaSize);
		istringstream alphaStream(alphaData);
		zlib_filter zf(alphaStream.rdbuf());
		istream zfstream(&zf);
		zfstream.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

		vector<char> alphaDataUncompressed;
		alphaDataUncompressed.resize(bitmap->getHeight()*bitmap->getWidth());
		
		//Catch the exception if the stream ends
		try
		{
			zfstream.read(alphaDataUncompressed.data(),bitmap->getHeight()*bitmap->getWidth());
		}
		catch(std::exception& e)
		{
			LOG(LOG_ERROR, "Exception while parsing Alpha data in DefineBitsJPEG3");
		}
		uint8_t* d = bitmap->getData();
		//Set alpha
		for(int32_t i=0;i<bitmap->getHeight()*bitmap->getWidth();i++)
		{
			d[i*4+3]=alphaDataUncompressed[i];
		}
	}
}

DefineBitsJPEG3Tag::~DefineBitsJPEG3Tag()
{
	delete[] alphaData;
}

DefineSceneAndFrameLabelDataTag::DefineSceneAndFrameLabelDataTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	LOG(LOG_TRACE,"DefineSceneAndFrameLabelDataTag");
	in >> SceneCount;
	Offset.resize(SceneCount);
	Name.resize(SceneCount);
	for(uint32_t i=0;i<SceneCount;++i)
	{
		in >> Offset[i] >> Name[i];
	}
	in >> FrameLabelCount;
	FrameNum.resize(FrameLabelCount);
	FrameLabel.resize(FrameLabelCount);
	for(uint32_t i=0;i<FrameLabelCount;++i)
	{
		in >> FrameNum[i] >> FrameLabel[i];
	}
}

void DefineSceneAndFrameLabelDataTag::execute(RootMovieClip* root) const
{
	for(size_t i=0;i<SceneCount;++i)
	{
		root->addScene(i,Offset[i],Name[i]);
	}
	for(size_t i=0;i<FrameLabelCount;++i)
	{
		root->addFrameLabel(FrameNum[i],FrameLabel[i]);
	}
}

SoundStreamHeadTag::SoundStreamHeadTag(RECORDHEADER h, std::istream& in, RootMovieClip *root, DefineSpriteTag* sprite):DisplayListTag(h),SoundData(new MemoryStreamCache(root->getSystemState()))
{
	BitStream bs(in);
	UB(4,bs);
	UB(2,bs); //PlaybackSoundRate
	UB(1,bs); //PlaybackSoundSize
	UB(1,bs); //PlaybackSoundType
	StreamSoundCompression = UB(4,bs);
	switch (UB(2,bs))
	{
		case 0: StreamSoundRate = 5512; break;
		case 1: StreamSoundRate = 11025; break;
		case 2: StreamSoundRate = 22050; break;
		case 3: StreamSoundRate = 44100; break;
	}
	StreamSoundSize = UB(1,bs);
	StreamSoundType = UB(1,bs);
	in>>StreamSoundSampleCount;
	if (StreamSoundCompression == LS_AUDIO_CODEC::MP3) 
		in>>LatencySeek;
	SoundData->setConstant();
	if (StreamSoundSampleCount == 0) // ignore sounds with no samples
		return;
	if (sprite)
	{
		sprite->soundheadtag = this;
	}
	else if (root)
	{
		setSoundChannel(root);
	}
}

SoundStreamHeadTag::~SoundStreamHeadTag()
{
	SoundData.reset();
}

void SoundStreamHeadTag::setSoundChannel(Sprite *spr)
{
	SoundChannel *schannel = Class<SoundChannel>::getInstanceS(spr->getInstanceWorker(),1,
								SoundData,
								AudioFormat(LS_AUDIO_CODEC(StreamSoundCompression),StreamSoundRate,StreamSoundType+1),
								nullptr, nullptr, true);
	spr->setSound(schannel,true);
}


SoundStreamBlockTag::SoundStreamBlockTag(RECORDHEADER h, std::istream& in, RootMovieClip *root, DefineSpriteTag *sprite):DisplayListTag(h)
{
	int len = Header.getLength();
	uint8_t* inData=new(nothrow) uint8_t[len];
	in.read((char*)inData,len);
	if (sprite)
	{
		if (!sprite->soundheadtag)
			throw ParseException("SoundStreamBlock tag without SoundStreamHeadTag.");
		decodeSoundBlock(sprite->soundheadtag->SoundData.getPtr(),(LS_AUDIO_CODEC)sprite->soundheadtag->StreamSoundCompression,inData,len);
		sprite->setSoundStartFrame();
	}
	else if (root)
		root->appendSound(inData,len, root->getFramesLoaded());
	delete[] inData;
}
void SoundStreamBlockTag::decodeSoundBlock(StreamCache* cache, LS_AUDIO_CODEC codec, unsigned char* buf, int len)
{
	cache->setTerminated(false);
	switch (codec)
	{
		case LS_AUDIO_CODEC::LINEAR_PCM_PLATFORM_ENDIAN:
		case LS_AUDIO_CODEC::LINEAR_PCM_LE:
		case LS_AUDIO_CODEC::ADPCM:
			cache->append(buf,len);
			break;
		case LS_AUDIO_CODEC::MP3:
		{
			// skip 4 bytes (SampleCount and SeekSamples)
			if (len> 4)
				cache->append(buf+4,len-4);
			break;
		}
		default:
			LOG(LOG_NOT_IMPLEMENTED,"decoding sound block format "<<(int)codec);
			break;
	}
}

AVM1ActionTag::AVM1ActionTag(RECORDHEADER h, istream &s, RootMovieClip *root, AdditionalDataTag *datatag):DisplayListTag(h)
{
	// ActionTags are ignored if FileAttributes.actionScript3 is set
	if (root->usesActionScript3)
	{
		skip(s);
		return; 
	}
	startactionpos=0;
	actions.resize(Header.getLength()+ (datatag ? datatag->numbytes+Header.getHeaderSize() : 0)+1,0);
	if (datatag)
	{
		startactionpos=datatag->numbytes+Header.getHeaderSize();
		memcpy(actions.data(),datatag->bytes,datatag->numbytes);
	}
	s.read((char*)actions.data()+startactionpos,Header.getLength());
}

void AVM1ActionTag::execute(DisplayObjectContainer* parent, bool inskipping)
{
	if (parent->is<MovieClip>() && !inskipping)// && !parent->as<MovieClip>()->state.stop_FP)
	{
		AVM1scriptToExecute script;
		setActions(script);
		script.avm1context = parent->as<MovieClip>()->getCurrentFrame()->getAVM1Context();
		script.event_name_id = UINT32_MAX;
		script.isEventScript = false;
		parent->incRef(); // will be decreffed after script handler was executed 
		script.clip=parent->as<MovieClip>();
		parent->getSystemState()->stage->AVM1AddScriptToExecute(script);
	}
}

void AVM1ActionTag::setActions(AVM1scriptToExecute& script) const
{
	script.actions = &actions;
	script.startactionpos = startactionpos;
}

AVM1InitActionTag::AVM1InitActionTag(RECORDHEADER h, istream &s, RootMovieClip *root, AdditionalDataTag* datatag):ControlTag(h)
{
	// InitActionTags are ignored if clip uses actionscript3
	if (root->usesActionScript3)
	{
		skip(s);
		return; 
	}
	startactionpos=0;
	actions.resize(Header.getLength()+ (datatag ? datatag->numbytes+Header.getHeaderSize()+2 : 0)+1,0);
	if (datatag)
	{
		startactionpos=datatag->numbytes+Header.getHeaderSize()+2;// 2 bytes for SpriteID
		memcpy(actions.data(),datatag->bytes,datatag->numbytes);
	}
	s >> SpriteId;
	s.read((char*)actions.data()+startactionpos,Header.getLength()-2);
	root->AVM1registerInitActionTag(SpriteId,this);
}

void AVM1InitActionTag::execute(RootMovieClip *root) const
{
	
	DefineSpriteTag* sprite = dynamic_cast<DefineSpriteTag*>(root->dictionaryLookup(SpriteId));
	if (!sprite)
	{
		LOG(LOG_ERROR,"sprite not found for InitActionTag:"<<SpriteId);
		return;
	}
	MovieClip* o = sprite->instance(nullptr)->as<MovieClip>();
	getVm(root->getSystemState())->addEvent(NullRef,_MR(new (root->getSystemState()->unaccountedMemory) AVM1InitActionEvent(root,_MR(o))));
}

void AVM1InitActionTag::executeDirect(MovieClip* clip) const
{
	DefineSpriteTag* sprite = dynamic_cast<DefineSpriteTag*>(clip->loadedFrom->dictionaryLookup(SpriteId));
	if (!sprite)
	{
		LOG(LOG_ERROR,"sprite not found for InitActionTag:"<<SpriteId);
		return;
	}
	std::map<uint32_t,asAtom> m;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" initActions "<< clip->toDebugString()<<" "<<sprite->getId());
	ACTIONRECORD::executeActions(clip,sprite->getAVM1Context(),actions,startactionpos,m,true);
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" initActions done "<< clip->toDebugString()<<" "<<sprite->getId());
}

AdditionalDataTag::AdditionalDataTag(RECORDHEADER h, istream &in):Tag(h)
{
	numbytes = h.getLength();
	if (numbytes)
	{
		bytes = new uint8_t[numbytes];
		in.read((char*)bytes,numbytes);
	}
}
VideoFrameTag::VideoFrameTag(RECORDHEADER h, istream &in, RootMovieClip* root) : DisplayListTag(h)
{
	in >> StreamID >> FrameNum;
	framedata=nullptr;
	numbytes = h.getLength()-4;
	if (numbytes)
	{
		framedata = new uint8_t[numbytes+AV_INPUT_BUFFER_PADDING_SIZE];
		memset(framedata+numbytes,0,AV_INPUT_BUFFER_PADDING_SIZE);
		in.read((char*)framedata,numbytes);

		DefineVideoStreamTag* videotag=dynamic_cast<DefineVideoStreamTag*>(root->dictionaryLookup(StreamID));
		if (videotag)
			videotag->setFrameData(this);
		else
			LOG(LOG_ERROR,"VideoFrameTag: no corresponding video found "<<StreamID);
	}
}

VideoFrameTag::~VideoFrameTag()
{
	if (framedata)
		delete[] framedata;
}

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	LOG(LOG_CALLS,"DoABCTag");

	RootMovieClip* root=getParseThread()->getRootMovie();
	context=new ABCContext(root, in, getVm(root->getSystemState()));

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCTag::execute(RootMovieClip* root) const
{
	LOG(LOG_CALLS,"ABC Exec");
	/* currentVM will free the context*/
	if (root->getInstanceWorker()->isPrimordial)
	{
		if (root == root->getSystemState()->mainClip)
			getVm(root->getSystemState())->addEvent(NullRef,_MR(new (root->getSystemState()->unaccountedMemory) ABCContextInitEvent(context,false)));
		else
			context->exec(false);
	}
	else
	{
		root->getInstanceWorker()->addABCContext(context);
		context->exec(false);
	}
}

DoABCDefineTag::DoABCDefineTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> Flags >> Name;
	LOG(LOG_CALLS,"DoABCDefineTag Name: " << Name);

	RootMovieClip* root=getParseThread()->getRootMovie();
	context=new ABCContext(root, in, getVm(root->getSystemState()));

	int pos=in.tellg();
	if(dest!=pos)
	{
		LOG(LOG_ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
		throw ParseException("Not complete ABC data");
	}
}

void DoABCDefineTag::execute(RootMovieClip* root) const
{
	LOG(LOG_CALLS,"ABC Exec " << Name);
	/* currentVM will free the context*/
	// some swf files have multiple abc tags without the "lazy" flag.
	// if the swf file also has a SymbolClass, we just ignore them and execute all abc tags lazy.
	// the real start of the main class is done when the symbol with id 0 is detected in SymbolClass tag
	bool lazy = root->hasSymbolClass || ((int32_t)Flags)&1;
	if (root->getInstanceWorker()->isPrimordial)
	{
		if (root == root->getSystemState()->mainClip)
			getVm(root->getSystemState())->addEvent(NullRef,_MR(new (root->getSystemState()->unaccountedMemory) ABCContextInitEvent(context,lazy)));
		else
			context->exec(lazy);
	}
	else
	{
		root->getInstanceWorker()->addABCContext(context);
		context->exec(lazy);
	}
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, istream& in):ControlTag(h)
{
	LOG(LOG_TRACE,"SymbolClassTag");
	in >> NumSymbols;

	Tags.resize(NumSymbols);
	Names.resize(NumSymbols);

	for(int i=0;i<NumSymbols;i++)
		in >> Tags[i] >> Names[i];
}

void SymbolClassTag::execute(RootMovieClip* root) const
{
	LOG(LOG_TRACE,"SymbolClassTag Exec");

	bool isSysRoot = root == root->getSystemState()->mainClip;
	for(int i=0;i<NumSymbols;i++)
	{
		LOG(LOG_CALLS,"Binding " << Tags[i] << ' ' << Names[i]);
		tiny_string className((const char*)Names[i],true);
		if(Tags[i]==0)
		{
			root->hasMainClass=true;
			root->incRef();
			ASWorker* worker = root->getInstanceWorker();
			if (!worker->isPrimordial && !isSysRoot)
			{
				getVm(root->getSystemState())->buildClassAndInjectBase(className.raw_buf(),_MR(root));
				worker->state ="running";
				worker->incRef();
				getVm(root->getSystemState())->addEvent(_MR(worker),_MR(Class<Event>::getInstanceS(root->getInstanceWorker(),"workerState")));
			}
			else if (!isSysRoot)
				getVm(root->getSystemState())->addBufferEvent(NullRef, _MR(new (root->getSystemState()->unaccountedMemory) BindClassEvent(_MR(root),className)));
			else
				getVm(root->getSystemState())->addEvent(NullRef, _MR(new (root->getSystemState()->unaccountedMemory) BindClassEvent(_MR(root),className)));
		}
		else
		{
			DictionaryTag* tag = root->dictionaryLookup(Tags[i]);
			root->addBinding(className, tag);
			if (!isSysRoot)
				getVm(root->getSystemState())->addBufferEvent(NullRef, _MR(new (root->getSystemState()->unaccountedMemory) BindClassEvent(tag,className)));
			else
				getVm(root->getSystemState())->addEvent(NullRef, _MR(new (root->getSystemState()->unaccountedMemory) BindClassEvent(tag,className)));
		}
	}
}

void ScriptLimitsTag::execute(RootMovieClip* root) const
{
	if (root != root->getSystemState()->mainClip)
		return;
	ASWorker* worker = root->getInstanceWorker();
	if (MaxRecursionDepth > worker->limits.max_recursion)
	{
		delete[] worker->stacktrace;
		worker->stacktrace = new stacktrace_entry[MaxRecursionDepth];
	}
	worker->limits.max_recursion = MaxRecursionDepth;
	worker->limits.script_timeout = ScriptTimeoutSeconds;
}
DefineScalingGridTag::DefineScalingGridTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	in >> CharacterId >> Splitter;
}

void DefineScalingGridTag::execute(RootMovieClip* root) const
{
	root->addToScalingGrids(this);
}
