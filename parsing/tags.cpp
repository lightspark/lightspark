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

#include <vector>
#include <list>
#include <algorithm>
#include "scripting/abc.h"
#include "tags.h"
#include "scripting/actions.h"
#include "backends/geometry.h"
#include "backends/rendering.h"
#include "swftypes.h"
#include "swf.h"
#include "logger.h"
#include "frame.h"
#include <GL/glew.h>

#undef RGB

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	
	unsigned int expectedLen=h.getLength();
	unsigned int start=f.tellg();
	Tag* ret=NULL;
	LOG(LOG_TRACE,"Reading tag type: " << h.getTagType() << " at byte " << start << " with length " << expectedLen << " bytes");
	switch(h.getTagType())
	{
		case 0:
			LOG(LOG_TRACE, "EndTag at position " << f.tellg());
			ret=new EndTag(h,f);
			break;
		case 1:
			ret=new ShowFrameTag(h,f);
			break;
		case 2:
			ret=new DefineShapeTag(h,f);
			break;
	//	case 4:
	//		ret=new PlaceObjectTag(h,f);
		case 9:
			ret=new SetBackgroundColorTag(h,f);
			break;
		case 10:
			ret=new DefineFontTag(h,f);
			break;
		case 11:
			ret=new DefineTextTag(h,f);
			break;
		case 12:
			ret=new DoActionTag(h,f);
			break;
		case 13:
			ret=new DefineFontInfoTag(h,f);
			break;
		case 14:
			ret=new DefineSoundTag(h,f);
			break;
		case 15:
			ret=new StartSoundTag(h,f);
			break;
		case 18:
			ret=new SoundStreamHeadTag(h,f);
			break;
		case 19:
			ret=new SoundStreamBlockTag(h,f);
			break;
		case 20:
			ret=new DefineBitsLosslessTag(h,f);
			break;
		case 21:
			ret=new DefineBitsJPEG2Tag(h,f);
			break;
		case 22:
			ret=new DefineShape2Tag(h,f);
			break;
		case 24:
			ret=new ProtectTag(h,f);
			break;
		case 26:
			ret=new PlaceObject2Tag(h,f);
			break;
		case 28:
			ret=new RemoveObject2Tag(h,f);
			break;
		case 32:
			ret=new DefineShape3Tag(h,f);
			break;
		case 34:
			ret=new DefineButton2Tag(h,f);
			break;
		case 36:
			ret=new DefineBitsLossless2Tag(h,f);
			break;
		case 37:
			ret=new DefineEditTextTag(h,f);
			break;
		case 39:
			ret=new DefineSpriteTag(h,f);
			break;
		case 41:
			ret=new ProductInfoTag(h,f);
			break;
		case 43:
			ret=new FrameLabelTag(h,f);
			break;
		case 45:
			ret=new SoundStreamHead2Tag(h,f);
			break;
		case 46:
			ret=new DefineMorphShapeTag(h,f);
			break;
		case 48:
			ret=new DefineFont2Tag(h,f);
			break;
		case 56:
			ret=new ExportAssetsTag(h,f);
			break;
		case 58:
			ret=new EnableDebuggerTag(h,f);
			break;
		case 59:
			ret=new DoInitActionTag(h,f);
			break;
		case 60:
			ret=new DefineVideoStreamTag(h,f);
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
			if(pt->version>=8)
			{
				if(!firstTag)
					LOG(LOG_ERROR,"FileAttributes tag not in the beginning");
			}
			ret=new FileAttributesTag(h,f);
			break;
		case 70:
			ret=new PlaceObject3Tag(h,f);
			break;
		case 73:
			ret=new DefineFontAlignZonesTag(h,f);
			break;
		case 74:
			ret=new CSMTextSettingsTag(h,f);
			break;
		case 75:
			ret=new DefineFont3Tag(h,f);
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
			ret=new DoABCTag(h,f);
			break;
		case 83:
			ret=new DefineShape4Tag(h,f);
			break;
		case 86:
			ret=new DefineSceneAndFrameLabelDataTag(h,f);
			break;
		case 87:
			ret=new DefineBinaryDataTag(h,f);
			break;
		case 88:
			ret=new DefineFontNameTag(h,f);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"Unsupported tag type " << h.getTagType());
			ret=new UnimplementedTag(h,f);
	}

	//Check if this clip is the main clip and if AVM2 has been enabled by a FileAttributes tag
	if(topLevel && firstTag && pt->root==sys)
		sys->needsAVM2(pt->useAVM2);
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
		throw ParseException("Malformed SWF file");
	}
	
	return ret;
}

RemoveObject2Tag::RemoveObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	in >> Depth;
	LOG(LOG_TRACE,"RemoveObject2 Depth: " << Depth);
}

void RemoveObject2Tag::execute(MovieClip* parent, list <pair<PlaceInfo, DisplayObject*> >& ls)
{
	list <pair<PlaceInfo, DisplayObject*> >::iterator it=ls.begin();

	for(;it!=ls.end();it++)
	{
		if(it->second->Depth==Depth)
		{
			it->second->decRef();
			ls.erase(it);
			break;
		}
	}
}

SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h)
{
	in >> BackgroundColor;
}

DefineEditTextTag::DefineEditTextTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
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
	}
	if(HasTextColor)
		in >> TextColor;
	if(HasMaxLength)
		in >> MaxLength;
	if(HasLayout)
	{
		in >> Align >> LeftMargin >> RightMargin >> Indent >> Leading;
	}
	in >> VariableName;
	LOG(LOG_NOT_IMPLEMENTED,"Sync to variable name " << VariableName);
	if(HasText)
		in >> InitialText;
}

void DefineEditTextTag::Render()
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineEditTextTag: Render");
}

Vector2 DefineEditTextTag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0,0.8,0);
	char buf[20];
	snprintf(buf,20,"EditText id=%u",(int)CharacterID);
	font->Render(buf,-1,FTPoint(0,50));
	
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

ASObject* DefineEditTextTag::instance() const
{
	DefineEditTextTag* ret=new DefineEditTextTag(*this);
	//TODO: check
	assert_and_throw(bindedTo==NULL);
	ret->setPrototype(Class<TextField>::getClass());
	return ret;
}

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
{
	in >> SpriteID >> FrameCount;
	setTotalFrames(FrameCount);
	framesLoaded=FrameCount;
	state.max_FP=FrameCount;

	LOG(LOG_TRACE,"DefineSprite ID: " << SpriteID);
	//Create a non top level TagFactory
	TagFactory factory(in, false);
	Tag* tag;
	bool done=false;
	bool empty=true;
	do
	{
		tag=factory.readTag();
		sys->tagsStorage.push_back(tag);
		switch(tag->getType())
		{
			case DICT_TAG:
				throw ParseException("Dictionary tag inside a sprite. Should not happen.");
			case DISPLAY_LIST_TAG:
				addToFrame(static_cast<DisplayListTag*>(tag));
				empty=false;
				break;
			case SHOW_TAG:
			{
				frames.push_back(Frame());
				cur_frame=&frames.back();
				empty=true;
				break;
			}
			case CONTROL_TAG:
				throw ParseException("Control tag inside a sprite. Should not happen.");
			case FRAMELABEL_TAG:
				frames.back().Label=(const char*)static_cast<FrameLabelTag*>(tag)->Name;
				empty=false;
				break;
			case TAG:
				LOG(LOG_NOT_IMPLEMENTED,"Unclassified tag inside Sprite?");
				break;
			case END_TAG:
				done=true;
				if(empty && frames.size()!=FrameCount)
					frames.pop_back();
				break;
		}
	}
	while(!done);

	if(frames.size()!=FrameCount)
	{
		LOG(LOG_ERROR,"Inconsistent frame count " << FrameCount);
		throw ParseException("Invalid frame count in Sprite");
	}

	LOG(LOG_TRACE,"EndDefineSprite ID: " << SpriteID);
}

ASObject* DefineSpriteTag::instance() const
{
	DefineSpriteTag* ret=new DefineSpriteTag(*this);
	//TODO: check
	if(bindedTo)
	{
		//A class is binded to this tag
		ret->setPrototype(bindedTo);
	}
	else
		ret->setPrototype(Class<MovieClip>::getClass());
	ret->bootstrap();
	return ret;
}

Vector2 DefineSpriteTag::debugRender(FTFont* font, bool deep)
{
	if(deep)
		return MovieClip::debugRender(font,deep);
	
	glColor3f(0.5,0,0);
	char buf[20];
	snprintf(buf,20,"Sprite id=%u",(int)SpriteID);
	font->Render(buf,-1,FTPoint(0,50));
	FTBBox tagBox=font->BBox(buf,-1);
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(tagBox.Upper().X(),0);
		glVertex2i(tagBox.Upper().X(),100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(tagBox.Upper().X(),100);
}

void lightspark::ignore(istream& i, int count)
{
	char* buf=new char[count];

	i.read(buf,count);

	delete[] buf;
}

DefineFontTag::DefineFontTag(RECORDHEADER h, std::istream& in):FontTag(h)
{
	LOG(LOG_TRACE,"DefineFont");
	in >> FontID;

	UI16 t;
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
		SHAPE t;
		in >> t;
		GlyphShapeTable.push_back(t);
	}
}

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in):FontTag(h)
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
	in >> LanguageCode >> FontNameLen;
	for(int i=0;i<FontNameLen;i++)
	{
		UI8 t;
		in >> t;
		FontName.push_back(t);
	}
	in >> NumGlyphs;
	if(FontFlagsWideOffsets)
	{
		UI32 t;
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
		UI16 t;
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
		SHAPE t;
		in >> t;
		GlyphShapeTable.push_back(t);
	}
	if(FontFlagsWideCodes)
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI16 t;
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
			SI16 t;
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
}

DefineFont3Tag::DefineFont3Tag(RECORDHEADER h, std::istream& in):FontTag(h)
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
	in >> LanguageCode >> FontNameLen;
	for(int i=0;i<FontNameLen;i++)
	{
		UI8 t;
		in >> t;
		FontName.push_back(t);
	}
	in >> NumGlyphs;
	if(FontFlagsWideOffsets)
	{
		UI32 t;
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
		UI16 t;
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		in >> t;
		CodeTableOffset=t;
	}
	GlyphShapeTable.resize(NumGlyphs);
	for(int i=0;i<NumGlyphs;i++)
	{
		in >> GlyphShapeTable[i];
	}
	for(int i=0;i<NumGlyphs;i++)
	{
		UI16 t;
		in >> t;
		CodeTable.push_back(t);
	}
	if(FontFlagsHasLayout)
	{
		in >> FontAscent >> FontDescent >> FontLeading;

		for(int i=0;i<NumGlyphs;i++)
		{
			SI16 t;
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
	ignore(in,KerningCount*4);
}

DefineBitsLosslessTag::DefineBitsLosslessTag(RECORDHEADER h, istream& in):DictionaryTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> CharacterId >> BitmapFormat >> BitmapWidth >> BitmapHeight;

	if(BitmapFormat==3)
		in >> BitmapColorTableSize;

	//TODO: read bitmap data
	ignore(in,dest-in.tellg());
}

DefineBitsLossless2Tag::DefineBitsLossless2Tag(RECORDHEADER h, istream& in):DictionaryTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> CharacterId >> BitmapFormat >> BitmapWidth >> BitmapHeight;

	if(BitmapFormat==3)
		in >> BitmapColorTableSize;

	//TODO: read bitmap data
	ignore(in,dest-in.tellg());
}

ASObject* DefineBitsLossless2Tag::instance() const
{
	DefineBitsLossless2Tag* ret=new DefineBitsLossless2Tag(*this);
	//TODO: check
	if(bindedTo)
	{
		//A class is binded to this tag
		ret->setPrototype(bindedTo);
	}
	else
		ret->setPrototype(Class<Bitmap>::getClass());
	return ret;
}

void DefineBitsLossless2Tag::Render()
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineBitsLossless2Tag::Render");
}

DefineTextTag::DefineTextTag(RECORDHEADER h, istream& in):DictionaryTag(h)
{
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;
	LOG(LOG_TRACE,"DefineText ID " << CharacterId);

	TEXTRECORD t(this);
	while(1)
	{
		in >> t;
		if(t.TextRecordType+t.StyleFlagsHasFont+t.StyleFlagsHasColor+t.StyleFlagsHasYOffset+t.StyleFlagsHasXOffset==0)
			break;
		TextRecords.push_back(t);
	}
}

void DefineTextTag::inputRender()
{
	if(alpha==0)
		return;
	if(!visible)
		return;
	std::vector < TEXTRECORD >::iterator it=TextRecords.begin();
	if(it==TextRecords.end()) //Nothing to draw
		return;
	std::vector < GLYPHENTRY >::iterator it2;
	int x=0,y=0;

	//Build a fake FILLSTYLEs
	FILLSTYLE f;
	f.FillStyleType=0x00;
	f.Color=it->TextColor;
	MatrixApplier ma(getMatrix());
	ma.concat(TextMatrix);
	//Shapes are defined in twips, so scale then down
	glScalef(0.05,0.05,1);
	
	//The next 1/20 scale is needed by DefineFont3. Should be conditional
	glScalef(0.05,0.05,1);
	float scale_cur=1;
	int count=0;
	unsigned int shapes_done=0;
	it= TextRecords.begin();
	for(;it!=TextRecords.end();it++)
	{
		if(it->StyleFlagsHasFont)
		{
			float scale=it->TextHeight;
			scale/=1024;
			glScalef(scale/scale_cur,scale/scale_cur,1);
			scale_cur=scale;
		}
		it2 = it->GlyphEntries.begin();
		int x2=x,y2=y;
		x2+=(*it).XOffset;
		y2+=(*it).YOffset;

		for(;it2!=(it->GlyphEntries.end());it2++)
		{
			while(shapes_done<cached.size() &&  cached[shapes_done].id==count)
			{
				assert_and_throw(cached[shapes_done].color==1)
				cached[shapes_done].style=&f;

				cached[shapes_done].Render(x2/scale_cur*20,y2/scale_cur*20);
				shapes_done++;
				if(shapes_done==cached.size())
					break;
			}
			x2+=it2->GlyphAdvance;
			count++;
		}
	}
	ma.unapply();
}

void DefineTextTag::Render()
{
	LOG(LOG_TRACE,"DefineText Render");
	if(alpha==0)
		return;
	if(!visible)
		return;
	if(cached.size()==0)
	{
		FontTag* font=NULL;
		int count=0;
		std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
		std::vector < GLYPHENTRY >::iterator it2;
		for(;it!=TextRecords.end();it++)
		{
			if(it->StyleFlagsHasFont)
			{
				DictionaryTag* it3=loadedFrom->dictionaryLookup(it->FontID);
				font=dynamic_cast<FontTag*>(it3);
				if(font==NULL)
					LOG(LOG_ERROR,"Should be a FontTag");
			}
			it2 = it->GlyphEntries.begin();
			for(;it2!=(it->GlyphEntries.end());it2++)
			{
				//TODO: refactor to cache glyphs
				vector<GeomShape> new_shapes;
				font->genGlyphShape(new_shapes,it2->GlyphIndex);
				for(unsigned int i=0;i<new_shapes.size();i++)
					cached.push_back(GlyphShape(new_shapes[i],count));

				count++;
			}
		}
	}
	std::vector < TEXTRECORD >::iterator it=TextRecords.begin();
	if(it==TextRecords.end()) //Nothing to draw
		return;
	std::vector < GLYPHENTRY >::iterator it2;
	int x=0,y=0;

	//Build a fake FILLSTYLEs
	FILLSTYLE f;
	f.FillStyleType=0x00;
	f.Color=it->TextColor;
	MatrixApplier ma(getMatrix());
	ma.concat(TextMatrix);
	//Shapes are defined in twips, so scale then down
	glScalef(0.05,0.05,1);

	if(!isSimple())
		rt->glAcquireTempBuffer(TextBounds.Xmin,TextBounds.Xmax, TextBounds.Ymin,TextBounds.Ymax);

	//The next 1/20 scale is needed by DefineFont3. Should be conditional
	glScalef(0.05,0.05,1);
	float scale_cur=1;
	int count=0;
	unsigned int shapes_done=0;
	for(;it!=TextRecords.end();it++)
	{
		if(it->StyleFlagsHasFont)
		{
			float scale=it->TextHeight;
			scale/=1024;
			glScalef(scale/scale_cur,scale/scale_cur,1);
			scale_cur=scale;
		}
		it2 = it->GlyphEntries.begin();
		int x2=x,y2=y;
		x2+=(*it).XOffset;
		y2+=(*it).YOffset;

		for(;it2!=(it->GlyphEntries.end());it2++)
		{
			while(shapes_done<cached.size() && cached[shapes_done].id==count)
			{
				assert_and_throw(cached[shapes_done].color==1)
				cached[shapes_done].style=&f;

				cached[shapes_done].Render(x2/scale_cur*20,y2/scale_cur*20);
				shapes_done++;
				if(shapes_done==cached.size())
					break;
			}
			x2+=it2->GlyphAdvance;
			count++;
		}
	}

	if(!isSimple())
		rt->glBlitTempBuffer(TextBounds.Xmin,TextBounds.Xmax,TextBounds.Ymin,TextBounds.Ymax);
	ma.unapply();
}

Vector2 DefineTextTag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0,0.4,0.4);
	char buf[20];
	snprintf(buf,20,"Text id=%u",(int)CharacterId);
	font->Render(buf,-1,FTPoint(0,50));
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
{
	LOG(LOG_TRACE,"DefineShapeTag");
	Shapes.version=1;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape2Tag::DefineShape2Tag(RECORDHEADER h, std::istream& in):DefineShapeTag(h)
{
	LOG(LOG_TRACE,"DefineShape2Tag");
	Shapes.version=2;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape3Tag::DefineShape3Tag(RECORDHEADER h, std::istream& in):DefineShape2Tag(h)
{
	LOG(LOG_TRACE,"DefineShape3Tag");
	Shapes.version=3;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape4Tag::DefineShape4Tag(RECORDHEADER h, std::istream& in):DefineShape3Tag(h)
{
	LOG(LOG_TRACE,"DefineShape4Tag");
	Shapes.version=4;
	in >> ShapeId >> ShapeBounds >> EdgeBounds;
	BitStream bs(in);
	UB(5,bs);
	UsesFillWindingRule=UB(1,bs);
	UsesNonScalingStrokes=UB(1,bs);
	UsesScalingStrokes=UB(1,bs);
	in >> Shapes;
}

DefineMorphShapeTag::DefineMorphShapeTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
{
	int dest=in.tellg();
	dest+=h.getLength();
	in >> CharacterId >> StartBounds >> EndBounds >> Offset >> MorphFillStyles >> MorphLineStyles >> StartEdges >> EndEdges;
	if(in.tellg()<dest)
		ignore(in,dest-in.tellg());
}

std::ostream& operator<<(std::ostream& s, const Vector2& p)
{
	s << "{ "<< p.x << ',' << p.y << " }" << std::endl;
	return s;
}

ASObject* DefineMorphShapeTag::instance() const
{
	DefineMorphShapeTag* ret=new DefineMorphShapeTag(*this);
	assert_and_throw(bindedTo==NULL);
	ret->setPrototype(Class<MorphShape>::getClass());
	return ret;
}

void DefineMorphShapeTag::Render()
{
	if(alpha==0)
		return;
	if(!visible)
		return;
	//TODO: implement morph shape support
/*	std::vector < GeomShape > shapes;
	SHAPERECORD* cur=&(EndEdges.ShapeRecords);

	FromShaperecordListToShapeVector(cur,shapes);

//	for(unsigned int i=0;i<shapes.size();i++)
//		shapes[i].BuildFromEdges(MorphFillStyles.FillStyles);

	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);

	rt->glAcquireFramebuffer();

	std::vector < GeomShape >::iterator it=shapes.begin();
	for(;it!=shapes.end();it++)
		it->Render();

	rt->glBlitFramebuffer();
	if(rt->glAcquireIdBuffer())
	{
		std::vector < GeomShape >::iterator it=shapes.begin();
		for(;it!=shapes.end();it++)
			it->Render();
		rt->glReleaseIdBuffer();
	}
	glPopMatrix();*/
}

void DefineShapeTag::inputRender()
{
	if(alpha==0)
		return;
	if(!visible)
		return;

	MatrixApplier ma(getMatrix());
	glScalef(0.05,0.05,1);

	std::vector < GeomShape >::iterator it=cached.begin();
	for(;it!=cached.end();it++)
	{
		assert_and_throw(it->color <= Shapes.FillStyles.FillStyleCount);
		it->Render();
	}
	ma.unapply();
}

void DefineShapeTag::Render()
{
	if(alpha==0)
		return;
	if(!visible)
		return;

	if(cached.size()==0)
	{
		FromShaperecordListToShapeVector(Shapes.ShapeRecords,cached);

		for(unsigned int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(&Shapes.FillStyles.FillStyles);
	}

	MatrixApplier ma(getMatrix());
	glScalef(0.05,0.05,1);

	if(!isSimple())
		rt->glAcquireTempBuffer(ShapeBounds.Xmin,ShapeBounds.Xmax,ShapeBounds.Ymin,ShapeBounds.Ymax);

	std::vector < GeomShape >::iterator it=cached.begin();
	for(;it!=cached.end();it++)
	{
		assert_and_throw(it->color <= Shapes.FillStyles.FillStyleCount);
		it->Render();
	}

	if(!isSimple())
		rt->glBlitTempBuffer(ShapeBounds.Xmin,ShapeBounds.Xmax,ShapeBounds.Ymin,ShapeBounds.Ymax);

	ma.unapply();
}

Vector2 DefineShapeTag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0,0,0.8);
	char buf[20];
	snprintf(buf,20,"Shape id=%u",(int)ShapeId);
	font->Render(buf,-1,FTPoint(0,50));
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

Vector2 DefineShape2Tag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0,0.8,0.8);
	char buf[20];
	snprintf(buf,20,"Shape2 id=%u",(int)ShapeId);
	font->Render(buf,-1,FTPoint(0,50));
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

Vector2 DefineShape3Tag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0.8,0,0.8);
	char buf[20];
	snprintf(buf,20,"Shape3 id=%u",(int)ShapeId);
	font->Render(buf,-1,FTPoint(0,50));
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void lightspark::FromShaperecordListToShapeVector(const vector<SHAPERECORD>& shapeRecords, vector<GeomShape>& shapes)
{
	int startX=0;
	int startY=0;
	unsigned int color0=0;
	unsigned int color1=0;

	ShapesBuilder shapesBuilder;

	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(startX,startY);
				startX+=cur->DeltaX;
				startY+=cur->DeltaY;
				Vector2 p2(startX,startY);
				
				if(color0)
					shapesBuilder.extendOutlineForColor(color0,p1,p2);
				if(color1)
					shapesBuilder.extendOutlineForColor(color1,p1,p2);
			}
			else
			{
				Vector2 p1(startX,startY);
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				Vector2 p2(startX,startY);
				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				Vector2 p3(startX,startY);

				if(color0)
				{
					shapesBuilder.extendOutlineForColor(color0,p1,p2);
					shapesBuilder.extendOutlineForColor(color0,p2,p3);
				}
				if(color1)
				{
					shapesBuilder.extendOutlineForColor(color1,p1,p2);
					shapesBuilder.extendOutlineForColor(color1,p2,p3);
				}
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;
			}
/*			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}*/
			if(cur->StateFillStyle1)
			{
				color1=cur->FillStyle1;
			}
			if(cur->StateFillStyle0)
			{
				color0=cur->FillStyle0;
			}
		}
	}

	shapesBuilder.outputShapes(shapes);
}

void DefineFont3Tag::genGlyphShape(vector<GeomShape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	FromShaperecordListToShapeVector(shape.ShapeRecords,s);

	for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);

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
}

void DefineFont2Tag::genGlyphShape(vector<GeomShape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	FromShaperecordListToShapeVector(shape.ShapeRecords,s);

	for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);

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
}

void DefineFontTag::genGlyphShape(vector<GeomShape>& s,int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	FromShaperecordListToShapeVector(shape.ShapeRecords,s);

	for(unsigned int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);

	//Should check fill state
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"ShowFrame");
}

bool PlaceObject2Tag::list_orderer::operator ()(const pair<PlaceInfo, DisplayObject*>& a, int d)
{
	return a.second->Depth<d;
}

bool PlaceObject2Tag::list_orderer::operator ()(int d, const pair<PlaceInfo, DisplayObject*>& a)
{
	return d<a.second->Depth;
}

bool PlaceObject2Tag::list_orderer::operator ()(const std::pair<PlaceInfo, DisplayObject*>& a, const std::pair<PlaceInfo, DisplayObject*>& b)
{
	return a.second->Depth < b.second->Depth;
}

void PlaceObject2Tag::execute(MovieClip* parent, list < pair< PlaceInfo, DisplayObject*> >& ls)
{
	//TODO: support clipping
	if(ClipDepth!=0)
		return;

	PlaceInfo infos;
	//Find if this id is already on the list
	list < pair<PlaceInfo, DisplayObject*> >::iterator it=ls.begin();
	for(;it!=ls.end();it++)
	{
		if(it->second->Depth==Depth)
		{
			infos=it->first;
			if(!PlaceFlagMove)
				throw ParseException("Depth already used already on displaylist");
			break;
		}
	}

	if(PlaceFlagHasMatrix)
		infos.Matrix=Matrix;

	DisplayObject* toAdd=NULL;
	if(PlaceFlagHasCharacter)
	{
		//As the transformation matrix can change every frame
		//it is saved in a side data structure
		//and assigned to the internal struture every frame
		LOG(LOG_TRACE,"Placing ID " << CharacterId);
		RootMovieClip* localRoot=NULL;
		DictionaryTag* parentDict=dynamic_cast<DictionaryTag*>(parent);
		if(parentDict)
			localRoot=parentDict->loadedFrom;
		else
			localRoot=parent->getRoot();
		DictionaryTag* dict=localRoot->dictionaryLookup(CharacterId);
		toAdd=dynamic_cast<DisplayObject*>(dict->instance());
		assert_and_throw(toAdd);

		//Object should be constructed even if not binded
		if(toAdd->getPrototype() && sys->currentVm)
		{
			//Object expect to have the matrix set when created
			if(PlaceFlagHasMatrix)
				toAdd->setMatrix(Matrix);
			//We now ask the VM to construct this object
			ConstructObjectEvent* e=new ConstructObjectEvent(toAdd,toAdd->getPrototype());
			bool added=sys->currentVm->addEvent(NULL,e);
			if(!added)
			{
				e->decRef();
				throw RunTimeException("Could not add event");
			}
			e->wait();
			e->decRef();
		}

		if(PlaceFlagHasColorTransform)
			toAdd->ColorTransform=ColorTransform;

		if(PlaceFlagHasRatio)
			toAdd->Ratio=Ratio;

		if(PlaceFlagHasClipDepth)
			toAdd->ClipDepth=ClipDepth;

		toAdd->setRoot(parent->getRoot());
		toAdd->Depth=Depth;
		if(!PlaceFlagMove)
		{
			list<pair<PlaceInfo, DisplayObject*> >::iterator it=
				lower_bound< list<pair<PlaceInfo, DisplayObject*> >::iterator, int, list_orderer>
				(ls.begin(),ls.end(),Depth,list_orderer());
			//As we are inserting the object in the list we need to incref it
			toAdd->incRef();
			ls.insert(it,make_pair(infos,toAdd));
		}
	}

	if(PlaceFlagHasName)
	{
		//Set a variable on the parent to link this object
		LOG(LOG_NO_INFO,"Registering ID " << CharacterId << " with name " << Name);
		if(!PlaceFlagMove)
		{
			assert_and_throw(toAdd);
			toAdd->incRef();
			parent->setVariableByQName((const char*)Name,"",toAdd);
		}
		else
			LOG(LOG_ERROR, "Moving of registered objects not really supported");
	}

	//Move the object if relevant
	if(PlaceFlagMove)
	{
		if(it!=ls.end())
		{
			if(!PlaceFlagHasCharacter)
			{
				//if(it2->PlaceFlagHasClipAction)
				if(PlaceFlagHasColorTransform)
					it->second->ColorTransform=ColorTransform;

				if(PlaceFlagHasRatio)
					it->second->Ratio=Ratio;

				if(PlaceFlagHasClipDepth)
					it->second->ClipDepth=ClipDepth;
				it->first=infos;
			}
			else
			{
				it->second->decRef();
				ls.erase(it);
				list<pair<PlaceInfo, DisplayObject*> >::iterator it=lower_bound(ls.begin(),ls.end(),Depth,list_orderer());
				toAdd->incRef();
				ls.insert(it,make_pair(infos,toAdd));
			}
		}
		else
			LOG(LOG_ERROR,"no char to move at depth " << Depth << " name " << Name);
	}

	if(toAdd)
		toAdd->decRef();
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h)
{
	LOG(LOG_TRACE,"PlaceObject2");

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
		in >> ColorTransform;

	if(PlaceFlagHasRatio)
		in >> Ratio;

	if(PlaceFlagHasName)
		in >> Name;

	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;

	if(PlaceFlagHasClipAction)
		in >> ClipActions;

	assert_and_throw(!(PlaceFlagHasCharacter && CharacterId==0))
}

void PlaceObject3Tag::execute(MovieClip* parent, list < pair< PlaceInfo, DisplayObject*> >& ls)
{
	//TODO: support clipping, blending, filters, caching
	if(ClipDepth!=0)
		return;

	PlaceInfo infos;
	//Find if this id is already on the list
	list < pair<PlaceInfo, DisplayObject*> >::iterator it=ls.begin();
	for(;it!=ls.end();it++)
	{
		if(it->second->Depth==Depth)
		{
			infos=it->first;
			if(!PlaceFlagMove)
				throw ParseException("Depth already used already on displaylist");
			break;
		}
	}

	if(PlaceFlagHasMatrix)
		infos.Matrix=Matrix;

	DisplayObject* toAdd=NULL;
	if(PlaceFlagHasCharacter)
	{
		//As the transformation matrix can change every frame
		//it is saved in a side data structure
		//and assigned to the internal struture every frame
		LOG(LOG_TRACE,"Placing ID " << CharacterId);
		RootMovieClip* localRoot=NULL;
		DictionaryTag* parentDict=dynamic_cast<DictionaryTag*>(parent);
		if(parentDict)
			localRoot=parentDict->loadedFrom;
		else
			localRoot=parent->getRoot();
		DictionaryTag* dict=localRoot->dictionaryLookup(CharacterId);
		toAdd=dynamic_cast<DisplayObject*>(dict->instance());
		assert_and_throw(toAdd);

		//Object should be constructed even if not binded
		if(toAdd->getPrototype() && sys->currentVm)
		{
			//Object expect to have the matrix set when created
			if(PlaceFlagHasMatrix)
				toAdd->setMatrix(Matrix);
			//We now ask the VM to construct this object
			ConstructObjectEvent* e=new ConstructObjectEvent(toAdd,toAdd->getPrototype());
			bool added=sys->currentVm->addEvent(NULL,e);
			if(!added)
			{
				e->decRef();
				throw RunTimeException("Could not add event");
			}
			e->wait();
			e->decRef();
		}

		if(PlaceFlagHasColorTransform)
			toAdd->ColorTransform=ColorTransform;

		if(PlaceFlagHasRatio)
			toAdd->Ratio=Ratio;

		if(PlaceFlagHasClipDepth)
			toAdd->ClipDepth=ClipDepth;

		toAdd->setRoot(parent->getRoot());
		toAdd->Depth=Depth;
		if(!PlaceFlagMove)
		{
			list<pair<PlaceInfo, DisplayObject*> >::iterator it=
				lower_bound< list<pair<PlaceInfo, DisplayObject*> >::iterator, int, list_orderer>
				(ls.begin(),ls.end(),Depth,list_orderer());
			//As we are inserting the object in the list we need to incref it
			toAdd->incRef();
			ls.insert(it,make_pair(infos,toAdd));
		}
	}

	if(PlaceFlagHasName)
	{
		//Set a variable on the parent to link this object
		LOG(LOG_NO_INFO,"Registering ID " << CharacterId << " with name " << Name);
		if(!PlaceFlagMove)
		{
			assert_and_throw(toAdd);
			toAdd->incRef();
			parent->setVariableByQName((const char*)Name,"",toAdd);
		}
		else
			LOG(LOG_ERROR, "Moving of registered objects not really supported");
	}

	//Move the object if relevant
	if(PlaceFlagMove)
	{
		if(it!=ls.end())
		{
			if(!PlaceFlagHasCharacter)
			{
				//if(it2->PlaceFlagHasClipAction)
				if(PlaceFlagHasColorTransform)
					it->second->ColorTransform=ColorTransform;

				if(PlaceFlagHasRatio)
					it->second->Ratio=Ratio;

				if(PlaceFlagHasClipDepth)
					it->second->ClipDepth=ClipDepth;
				it->first=infos;
			}
			else
			{
				it->second->decRef();
				ls.erase(it);
				list<pair<PlaceInfo, DisplayObject*> >::iterator it=lower_bound(ls.begin(),ls.end(),Depth,list_orderer());
				toAdd->incRef();
				ls.insert(it,make_pair(infos,toAdd));
			}
		}
		else
			LOG(LOG_ERROR,"no char to move at depth " << Depth << " name " << Name);
	}

	if(toAdd)
		toAdd->decRef();
}

PlaceObject3Tag::PlaceObject3Tag(RECORDHEADER h, std::istream& in):PlaceObject2Tag(h)
{
	LOG(LOG_TRACE,"PlaceObject3");

	BitStream bs(in);
	PlaceFlagHasClipAction=UB(1,bs);
	PlaceFlagHasClipDepth=UB(1,bs);
	PlaceFlagHasName=UB(1,bs);
	PlaceFlagHasRatio=UB(1,bs);
	PlaceFlagHasColorTransform=UB(1,bs);
	PlaceFlagHasMatrix=UB(1,bs);
	PlaceFlagHasCharacter=UB(1,bs);
	PlaceFlagMove=UB(1,bs);
	UB(3,bs); //Reserved
	PlaceFlagHasImage=UB(1,bs);
	PlaceFlagHasClassName=UB(1,bs);
	PlaceFlagHasCacheAsBitmap=UB(1,bs);
	PlaceFlagHasBlendMode=UB(1,bs);
	PlaceFlagHasFilterList=UB(1,bs);

	in >> Depth;
	if(PlaceFlagHasClassName || (PlaceFlagHasImage && PlaceFlagHasCharacter))
		throw ParseException("ClassName in PlaceObject3 not yet supported");

	if(PlaceFlagHasCharacter)
		in >> CharacterId;

	if(PlaceFlagHasMatrix)
		in >> Matrix;

	if(PlaceFlagHasColorTransform)
		in >> ColorTransform;

	if(PlaceFlagHasRatio)
		in >> Ratio;

	if(PlaceFlagHasName)
		in >> Name;

	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;

	if(PlaceFlagHasFilterList)
		in >> SurfaceFilterList;

	if(PlaceFlagHasBlendMode)
		in >> BlendMode;

	if(PlaceFlagHasCacheAsBitmap)
		in >> BitmapCache;

	if(PlaceFlagHasClipAction)
		in >> ClipActions;

	assert_and_throw(!(PlaceFlagHasCharacter && CharacterId==0))
}

void SetBackgroundColorTag::execute(RootMovieClip* root)
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

	LOG(LOG_NO_INFO,"SWF Info:" << 
	endl << "\tProductId: " << ProductId <<
	endl << "\tEdition: " << Edition <<
	endl << "\tVersion: " << UI32(MajorVersion) << "." << UI32(MinorVersion) << "." << MajorBuild << "." << MinorBuild <<
	endl << "\tCompileTime: " << longlongTime);
}

FrameLabelTag::FrameLabelTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	in >> Name;
	if(pt->version>=6)
	{
		UI8 NamedAnchor=in.peek();
		if(NamedAnchor==1)
			in >> NamedAnchor;
	}
}

DefineButton2Tag::DefineButton2Tag(RECORDHEADER h, std::istream& in):DictionaryTag(h),IdleToOverUp(false)
{
	state=BUTTON_UP;
	in >> ButtonId;
	BitStream bs(in);
	UB(7,bs);
	TrackAsMenu=UB(1,bs);
	in >> ActionOffset;

	BUTTONRECORD br(2);

	do
	{
		in >> br;
		Characters.push_back(br);
	}
	while(!br.isNull());

	if(ActionOffset)
	{
		BUTTONCONDACTION bca;
		do
		{
			in >> bca;
			Actions.push_back(bca);
		}
		while(!bca.isLast());
	}
}

ASObject* DefineButton2Tag::instance() const
{
	return new DefineButton2Tag(*this);
}

void DefineButton2Tag::handleEvent(Event* e)
{
	state=BUTTON_OVER;
	IdleToOverUp=true;
}

void DefineButton2Tag::Render()
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineButton2Tag::Render");
	if(alpha==0)
		return;
	if(!visible)
		return;
	/*	for(unsigned int i=0;i<Characters.size();i++)
	{
		if(Characters[i].ButtonStateUp && state==BUTTON_UP)
		{
		}
		else if(Characters[i].ButtonStateOver && state==BUTTON_OVER)
		{
		}
		else
			continue;
		DictionaryTag* r=sys->dictionaryLookup(Characters[i].CharacterID);
		IRenderObject* c=dynamic_cast<IRenderObject*>(r);
		if(c==NULL)
			LOG(ERROR,"Rendering something strange");
		float matrix[16];
		Characters[i].PlaceMatrix.get4DMatrix(matrix);
		glPushMatrix();
		glMultMatrixf(matrix);
		c->Render();
		glPopMatrix();
	}
	for(unsigned int i=0;i<Actions.size();i++)
	{
		if(IdleToOverUp && Actions[i].CondIdleToOverUp)
		{
			IdleToOverUp=false;
		}
		else
			continue;

		for(unsigned int j=0;j<Actions[i].Actions.size();j++)
			Actions[i].Actions[j]->Execute();
	}*/
}

Vector2 DefineButton2Tag::debugRender(FTFont* font, bool deep)
{
	assert(!deep);
	glColor3f(0.8,0.8,0.8);
	char buf[20];
	snprintf(buf,20,"Button2 id=%u",(int)CharacterId);
	font->Render(buf,-1,FTPoint(0,50));
	glBegin(GL_LINE_LOOP);
		glVertex2i(0,0);
		glVertex2i(100,0);
		glVertex2i(100,100);
		glVertex2i(0,100);
	glEnd();
	return Vector2(100,100);
}

DefineVideoStreamTag::DefineVideoStreamTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
{
	LOG(LOG_NO_INFO,"DefineVideoStreamTag");
	in >> CharacterID >> NumFrames >> Width >> Height;
	BitStream bs(in);
	UB(4,bs);
	VideoFlagsDeblocking=UB(3,bs);
	VideoFlagsSmoothing=UB(1,bs);
	in >> CodecID;
}

void DefineVideoStreamTag::Render()
{
	LOG(LOG_NO_INFO,"DefineVideoStreamTag Render");
/*	if(alpha==0)
		return;
	if(!visible)
		return;*/
	glColor4f(1,0,0,0);
	glBegin(GL_QUADS);
		glVertex2i(0,0);
		glVertex2i(Width,0);
		glVertex2i(Width,Height);
		glVertex2i(0,Height);
	glEnd();
}

DefineBinaryDataTag::DefineBinaryDataTag(RECORDHEADER h,std::istream& s):DictionaryTag(h)
{
	LOG(LOG_TRACE,"DefineBinaryDataTag");
	int size=h.getLength();
	s >> Tag >> Reserved;
	size -= sizeof(Tag)+sizeof(Reserved);
	bytes=new uint8_t[size];
	len=size;
	s.read((char*)bytes,size);
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

	if(ActionScript3)
		pt->useAVM2=true;
}

DefineSoundTag::DefineSoundTag(RECORDHEADER h, std::istream& in):DictionaryTag(h)
{
	LOG(LOG_TRACE,"DefineSound Tag");
	in >> SoundId;
	BitStream bs(in);
	SoundFormat=UB(4,bs);
	SoundRate=UB(2,bs);
	SoundSize=UB(1,bs);
	SoundType=UB(1,bs);
	in >> SoundSampleCount;
	//TODO: read and parse actual sound data
	ignore(in,h.getLength()-7);
}

ASObject* DefineSoundTag::instance() const
{
	DefineSoundTag* ret=new DefineSoundTag(*this);
	//TODO: check
	if(bindedTo)
	{
		//A class is binded to this tag
		ret->setPrototype(bindedTo);
	}
	else
		ret->setPrototype(Class<Sound>::getClass());
	return ret;
}

ScriptLimitsTag::ScriptLimitsTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"ScriptLimitsTag Tag");
	in >> MaxRecursionDepth >> ScriptTimeoutSeconds;
	LOG(LOG_NO_INFO,"MaxRecusionDepth: " << MaxRecursionDepth << ", ScriptTimeoutSeconds: " << ScriptTimeoutSeconds);
}

DebugIDTag::DebugIDTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"DebugIDTag Tag");
	for(int i = 0; i < 16; i++)
		in >> DebugId[i];

	//Note the switch to hex formatting on the ostream, and switch back to dec
	LOG(LOG_NO_INFO,"DebugId " << hex <<
		UI32(DebugId[0]) << UI32(DebugId[1]) << UI32(DebugId[2]) << UI32(DebugId[3]) << "-" <<
		UI32(DebugId[4]) << UI32(DebugId[5]) << "-" <<
		UI32(DebugId[6]) << UI32(DebugId[7]) << "-" <<
		UI32(DebugId[8]) << UI32(DebugId[9]) << "-" <<
		UI32(DebugId[10]) << UI32(DebugId[11]) << UI32(DebugId[12]) << UI32(DebugId[13]) << UI32(DebugId[14]) << UI32(DebugId[15]) <<
		dec);
}

EnableDebuggerTag::EnableDebuggerTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"EnableDebuggerTag Tag");
	DebugPassword = "";
	if(h.getLength() > 0)
		in >> DebugPassword;
	LOG(LOG_NO_INFO,"Debugger enabled, password: " << DebugPassword);
}

EnableDebugger2Tag::EnableDebugger2Tag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"EnableDebugger2Tag Tag");
	in >> ReservedWord;

	DebugPassword = "";
	if(h.getLength() > sizeof(ReservedWord))
		in >> DebugPassword;
	LOG(LOG_NO_INFO,"Debugger enabled, reserved: " << ReservedWord << ", password: " << DebugPassword);
}

MetadataTag::MetadataTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_TRACE,"MetadataTag Tag");
	in >> XmlString;
	LOG(LOG_NO_INFO,"MetaData: " << XmlString);
}
