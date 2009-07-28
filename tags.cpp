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

#define GL_GLEXT_PROTOTYPES
#include <vector>
#include <list>
#include <algorithm>
#include "abc.h"
#include "tags.h"
#include "actions.h"
#include "geometry.h"
#include "swftypes.h"
#include "swf.h"
#include "input.h"
#include "logger.h"
#include "frame.h"
#include <GL/gl.h>

using namespace std;

long timeDiff(timespec& s, timespec& d);

extern __thread SystemState* sys;
extern __thread RenderThread* rt;
extern __thread ParseThread* pt;

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	LOG(TRACE,"Reading tag type: " << (h>>6));
	switch(h>>6)
	{
		case 0:
			LOG(TRACE, "EndTag at position " << f.tellg());
			return new EndTag(h,f);
		case 1:
			return new ShowFrameTag(h,f);
		case 2:
			return new DefineShapeTag(h,f);
	//	case 4:
	//		return new PlaceObjectTag(h,f);
		case 9:
			return new SetBackgroundColorTag(h,f);
		case 10:
			return new DefineFontTag(h,f);
		case 11:
			return new DefineTextTag(h,f);
		case 12:
			return new DoActionTag(h,f);
		case 13:
			return new DefineFontInfoTag(h,f);
		case 14:
			return new DefineSoundTag(h,f);
		case 15:
			return new StartSoundTag(h,f);
		case 19:
			return new SoundStreamBlockTag(h,f);
		case 20:
			return new DefineBitsLosslessTag(h,f);
		case 21:
			return new DefineBitsJPEG2Tag(h,f);
		case 22:
			return new DefineShape2Tag(h,f);
		case 24:
			return new ProtectTag(h,f);
		case 26:
			return new PlaceObject2Tag(h,f);
		case 28:
			return new RemoveObject2Tag(h,f);
		case 32:
			return new DefineShape3Tag(h,f);
		case 34:
			return new DefineButton2Tag(h,f);
		case 36:
			return new DefineBitsLossless2Tag(h,f);
		case 37:
			return new DefineEditTextTag(h,f);
		case 39:
			return new DefineSpriteTag(h,f);
		case 41:
			return new SerialNumberTag(h,f);
		case 43:
			return new FrameLabelTag(h,f);
		case 45:
			return new SoundStreamHead2Tag(h,f);
		case 46:
			return new DefineMorphShapeTag(h,f);
		case 48:
			return new DefineFont2Tag(h,f);
		case 56:
			return new ExportAssetsTag(h,f);
		case 59:
			return new DoInitActionTag(h,f);
		case 60:
			return new DefineVideoStreamTag(h,f);
		case 65:
			return new ScriptLimitsTag(h,f);
		case 69:
			return new FileAttributesTag(h,f);
		case 73:
			return new DefineFontAlignZonesTag(h,f);
		case 75:
			return new DefineFont3Tag(h,f);
		case 76:
			return new SymbolClassTag(h,f);
		case 77:
			return new MetadataTag(h,f);
		case 78:
			return new DefineScalingGridTag(h,f);
		case 82:
			return new DoABCTag(h,f);
		case 83:
			return new DefineShape4Tag(h,f);
		case 86:
			return new DefineSceneAndFrameLabelDataTag(h,f);
		case 87:
			return new DefineBinaryDataTag(h,f);
		case 88:
			return new DefineFontNameTag(h,f);
		default:
			LOG(NOT_IMPLEMENTED,"Unsupported tag type " << (h>>6));
			return new UnimplementedTag(h,f);
	}
}

RemoveObject2Tag::RemoveObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	in >> Depth;
}

void RemoveObject2Tag::execute(MovieClip* parent, list <pair<PlaceInfo, IDisplayListElem*> >& ls)
{
	list <pair<PlaceInfo, IDisplayListElem*> >::iterator it=ls.begin();

	for(it;it!=ls.end();it++)
	{
		if(it->second->Depth==Depth)
		{
			/*SWFObject* t=dynamic_cast<SWFObject*>(*it);
			if(t!=NULL)
				LOG(NO_INFO,"Removing " << t->getName());*/
			ls.erase(it);
			break;
		}
	}
}

SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	in >> BackgroundColor;
}

DefineEditTextTag::DefineEditTextTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	//setVariableByName("text",SWFObject(new Integer(0)));
	in >> CharacterID >> Bounds;
	LOG(TRACE,"DefineEditTextTag ID " << CharacterID);
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
	LOG(NOT_IMPLEMENTED,"Sync to variable name " << VariableName);
	if(HasText)
		in >> InitialText;
}

void DefineEditTextTag::Render()
{
	LOG(NOT_IMPLEMENTED,"DefineEditTextTag: Render");
}

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	ISWFObject* target_bak=pt->parsingTarget;
	pt->parsingTarget=this;

	list < pair<PlaceInfo, IDisplayListElem*> >* bak=pt->parsingDisplayList;
	pt->parsingDisplayList=&displayList;
	in >> SpriteID >> FrameCount;
	_totalframes=FrameCount;
	state.max_FP=FrameCount;

	LOG(TRACE,"DefineSprite ID: " << SpriteID);
	TagFactory factory(in);
	Tag* tag;
	do
	{
		tag=factory.readTag();
		switch(tag->getType())
		{
			case DICT_TAG:
				LOG(ERROR,"Dictionary tag inside a sprite. Should not happen.");
				break;
			case DISPLAY_LIST_TAG:
				addToFrame(dynamic_cast<DisplayListTag*>(tag));
				break;
			case SHOW_TAG:
			{
				frames.push_back(cur_frame);
				cur_frame=Frame(&dynamicDisplayList);
				break;
			}
			case CONTROL_TAG:
				LOG(ERROR,"Control tag inside a sprite. Should not happen.");
				break;
		}
	}
	while(tag->getType()!=END_TAG);

	if(frames.size()!=FrameCount && FrameCount!=1)
		LOG(ERROR,"Inconsistent frame count " << FrameCount);
	pt->parsingDisplayList=bak;

	pt->parsingTarget=target_bak;
	LOG(TRACE,"EndDefineSprite ID: " << SpriteID);
}

void ignore(istream& i, int count)
{
	char* buf=new char[count];

	i.read(buf,count);

	delete[] buf;
}

DefineFontTag::DefineFontTag(RECORDHEADER h, std::istream& in):FontTag(h,in)
{
	LOG(TRACE,"DefineFont");
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

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in):FontTag(h,in)
{
	LOG(TRACE,"DefineFont2");
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
	UI16 t;
	if(FontFlagsWideOffsets)
	{
		LOG(ERROR,"Not supported wide font offsets...Aborting");
	}
	else
	{
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
	//Stub implement kerninfo merda merda
	ignore(in,KerningCount*4);
}

DefineBitsLosslessTag::DefineBitsLosslessTag(RECORDHEADER h, istream& in):DictionaryTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> CharacterId >> BitmapFormat >> BitmapWidth >> BitmapHeight;

	if(BitmapFormat==3)
		in >> BitmapColorTableSize;

	//TODO: read bitmap data
	ignore(in,dest-in.tellg());
}

DefineBitsLossless2Tag::DefineBitsLossless2Tag(RECORDHEADER h, istream& in):DictionaryTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> CharacterId >> BitmapFormat >> BitmapWidth >> BitmapHeight;

	if(BitmapFormat==3)
		in >> BitmapColorTableSize;

	//TODO: read bitmap data
	ignore(in,dest-in.tellg());
}

DefineTextTag::DefineTextTag(RECORDHEADER h, istream& in):DictionaryTag(h,in)
{
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;
	LOG(TRACE,"DefineText ID " << CharacterId);

	TEXTRECORD t(this);
	while(1)
	{
		in >> t;
		if(t.TextRecordType+t.StyleFlagsHasFont+t.StyleFlagsHasColor+t.StyleFlagsHasYOffset+t.StyleFlagsHasXOffset==0)
			break;
		TextRecords.push_back(t);
	}
}

void DefineTextTag::Render()
{
	LOG(TRACE,"DefineText Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		FontTag* font=NULL;
		int count=0;
		std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
		std::vector < GLYPHENTRY >::iterator it2;
		for(it;it!=TextRecords.end();it++)
		{
			if(it->StyleFlagsHasFont)
			{
				DictionaryTag* it3=root->dictionaryLookup(it->FontID);
				font=dynamic_cast<FontTag*>(it3);
				if(font==NULL)
					LOG(ERROR,"Should be a FontTag");
			}
			it2 = it->GlyphEntries.begin();
			for(it2;it2!=(it->GlyphEntries.end());it2++)
			{
				vector<Shape> new_shapes;
				font->genGlyphShape(new_shapes,it2->GlyphIndex);
				for(int i=0;i<new_shapes.size();i++)
				{
					new_shapes[i].id=count;
					cached.push_back(new_shapes[i]);
				}
				count++;
			}
		}
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}
	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

	std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
	std::vector < GLYPHENTRY >::iterator it2;
	int count=0;
	int shapes_done=0;
	int x=0,y=0;
	float matrix[16];
	TextMatrix.get4DMatrix(matrix);

	//Build a fake FILLSTYLEs
	FILLSTYLE f;
	f.FillStyleType=0x00;
	f.Color=it->TextColor;
	FILLSTYLE clearStyle;
	clearStyle.FillStyleType=0x00;
	clearStyle.Color=RGBA(0,0,0,0);
	glPushMatrix();
	glMultMatrixf(matrix);
	float scale_cur=1;
	for(it;it!=TextRecords.end();it++)
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

		for(it2;it2!=(it->GlyphEntries.end());it2++)
		{
			while(cached[shapes_done].id==count)
			{
				if(cached[shapes_done].color==1)
					cached[shapes_done].style=&f;
				else if(cached[shapes_done].color==0)
					cached[shapes_done].style=&clearStyle;
				else
					abort();

				cached[shapes_done].Render(x2/scale_cur,y2/scale_cur);
				shapes_done++;
				if(shapes_done==cached.size())
					break;
			}
			x2+=it2->GlyphAdvance;
			count++;
		}
	}

	glEnable(GL_BLEND);
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(TRACE,"DefineShapeTag");
	Shapes.version=1;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape2Tag::DefineShape2Tag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(TRACE,"DefineShape2Tag");
	Shapes.version=2;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape3Tag::DefineShape3Tag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(TRACE,"DefineShape3Tag");
	Shapes.version=3;
	in >> ShapeId >> ShapeBounds >> Shapes;
	texture=0;
}

DefineShape4Tag::DefineShape4Tag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(TRACE,"DefineShape4Tag");
	Shapes.version=4;
	in >> ShapeId >> ShapeBounds >> EdgeBounds;
	BitStream bs(in);
	UB(5,bs);
	UsesFillWindingRule=UB(1,bs);
	UsesNonScalingStrokes=UB(1,bs);
	UsesScalingStrokes=UB(1,bs);
	in >> Shapes;
}

int crossProd(const Vector2& a, const Vector2& b)
{
	return a.x*b.y-a.y*b.x;
}

DefineMorphShapeTag::DefineMorphShapeTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> CharacterId >> StartBounds >> EndBounds >> Offset >> MorphFillStyles >> MorphLineStyles >> StartEdges >> EndEdges;
	if(in.tellg()<dest)
		ignore(in,dest-in.tellg());
}

std::ostream& operator<<(std::ostream& s, const Vector2& p)
{
	s << "{ "<< p.x << ',' << p.y << " } [" << p.index  << ']' << std::endl;
	return s;
}

void FromShaperecordListToShapeVector(SHAPERECORD* cur, vector<Shape>& shapes);

void DefineMorphShapeTag::Render()
{
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(EndEdges.ShapeRecords);

	FromShaperecordListToShapeVector(cur,shapes);

	for(int i=0;i<shapes.size();i++)
		shapes[i].BuildFromEdges(MorphFillStyles.FillStyles);

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
		it->Render();

	glEnable(GL_BLEND);
	glPushMatrix();
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

void DefineShapeTag::Render()
{
	LOG(TRACE,"DefineShape Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToShapeVector(cur,cached);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles);

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

//	float matrix[16];
//	Matrix.get4DMatrix(matrix);
	//Apply local transformation
	glPushMatrix();
//	glMultMatrixf(matrix);

	std::vector < Shape >::iterator it=cached.begin();
	for(it;it!=cached.end();it++)
		it->Render();

	glEnable(GL_BLEND);
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

void DefineShape2Tag::Render()
{
	LOG(TRACE,"DefineShape2 Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToShapeVector(cur,cached);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles);

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

	//Apply local transformation
	glPushMatrix();
	
	std::vector < Shape >::iterator it=cached.begin();
	for(it;it!=cached.end();it++)
	{
		if(it->color >= Shapes.FillStyles.FillStyleCount)
		{
			it->style=new FILLSTYLE;
			it->style->FillStyleType=0x00;
			it->style->Color=RGB(255,0,0);
			it->color=1;
			LOG(NOT_IMPLEMENTED,"Orrible HACK");
		}
		it->Render();
	}

	glEnable(GL_BLEND);
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

void DefineShape4Tag::Render()
{
	LOG(TRACE,"DefineShape4 Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToShapeVector(cur,cached);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles);

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}
	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

	std::vector < Shape >::iterator it=cached.begin();
	for(it;it!=cached.end();it++)
		it->Render();

	glEnable(GL_BLEND);
	glPushMatrix();
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

void DefineShape3Tag::Render()
{
	LOG(TRACE,"DefineShape3 Render "<< ShapeId);
/*	if(texture==0)
	{
		glPushAttrib(GL_TEXTURE_BIT);
		glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D,texture);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (ShapeBounds.Xmax-ShapeBounds.Xmin)/10, 
				(ShapeBounds.Ymax-ShapeBounds.Ymin)/10, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		glPopAttrib();
	}*/
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToShapeVector(cur,cached);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles);

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D, texture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glDisable(GL_BLEND);
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT);

	std::vector < Shape >::iterator it=cached.begin();
	for(it;it!=cached.end();it++)
	{
		if(it->color >= Shapes.FillStyles.FillStyleCount)
		{
			it->style=new FILLSTYLE;
			it->style->FillStyleType=0x00;
			it->style->Color=RGB(255,0,0);
			it->color=1;
			LOG(NOT_IMPLEMENTED,"Orrible HACK");
		}
		it->Render();
	}

	glEnable(GL_BLEND);
	glPushMatrix();
	glLoadIdentity();
	GLenum draw_buffers[]={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT2_EXT};
	glDrawBuffers(2,draw_buffers);
	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glColor3f(0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2i(0,0);
		glTexCoord2f(1,1);
		glVertex2i(rt->width,0);
		glTexCoord2f(1,0);
		glVertex2i(rt->width,rt->height);
		glTexCoord2f(0,0);
		glVertex2i(0,rt->height);
	glEnd();
	glPopMatrix();
}

void FromShaperecordListToDump(SHAPERECORD* cur)
{
	ofstream f("record_dump");

	int startX=0;
	int startY=0;

	while(cur)
	{
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				startX+=cur->DeltaX;
				startY+=cur->DeltaY;
				f << startX << ' ' << startY << endl;
			}
			else
			{
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				f << startX << ' ' << startY << endl;

				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				f << startX << ' ' << startY << endl;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;
				f << "//Move To" << endl;
				f << startX << ' ' << startY << endl;
			}
			if(cur->StateLineStyle)
			{
				f << "//LS=" << cur->LineStyle << endl;
			}
			if(cur->StateFillStyle1)
			{
				f << "//FS1=" << cur->FillStyle1 << endl;
			}
			if(cur->StateFillStyle0)
			{
				f << "//FS0=" << cur->FillStyle0 << endl;
			}
		}
		cur=cur->next;
	}
	f.close();
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void FromShaperecordListToShapeVector(SHAPERECORD* cur, vector<Shape>& shapes)
{
	int startX=0;
	int startY=0;
	int count=0;
	int color0=0;
	int color1=0;

	while(cur)
	{
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(startX,startY,count);
				startX+=cur->DeltaX;
				startY+=cur->DeltaY;
				Vector2 p2(startX,startY,count+1);
				int color;
				for(int k=0;k<2;k++)
				{
					bool new_shape=true;
					if(k==0)
						color=color0;
					else
						color=color1;

					for(int i=0;i<shapes.size();i++)
					{
						if(shapes[i].color==color)
						{
							if(shapes[i].outline.back()==p1)
							{
								shapes[i].outline.push_back(p2);
								//cout << "Adding edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								new_shape=false;
								break;
							}
							else if(shapes[i].outline.front()==p2)
							{
								shapes[i].outline.insert(shapes[i].outline.begin(),p1);
								//cout << "Adding edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								new_shape=false;
								break;
							}

							if(shapes[i].outline.back()==p2 &&
								!(*(shapes[i].outline.rbegin()+1)==p1))
							{
								shapes[i].outline.push_back(p1);
								//cout << "Adding edge to shape " << i << endl;
								//cout << p2 << p1 << endl;
								new_shape=false;
								break;
							}
							else if(shapes[i].outline.front()==p1 &&
								!(shapes[i].outline[1]==p2)) 
							{
								shapes[i].outline.insert(shapes[i].outline.begin(),p2);
								//cout << "Adding edge to shape " << i << endl;
								//cout << p2 << p1 << endl;
								new_shape=false;
								break;
							}
						}
					}
					if(new_shape)
					{
						//cout << "Adding edge to new shape " << shapes.size() << endl;
						//cout << p1 << p2 << endl;
						shapes.push_back(Shape());
						shapes.back().outline.push_back(p1);
						shapes.back().outline.push_back(p2);
						shapes.back().color=color;
					}
				}
				count++;
			}
			else
			{
				Vector2 p1(startX,startY,count);
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				Vector2 p2(startX,startY,count+1);
				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				Vector2 p3(startX,startY,count+2);
				int color;
				for(int k=0;k<2;k++)
				{
					bool new_shape=true;
					if(k==0)
						color=color0;
					else
						color=color1;

					for(int i=0;i<shapes.size();i++)
					{
						if(shapes[i].color==color)
						{
							if(shapes[i].outline.back()==p1)
							{
								shapes[i].outline.push_back(p2);
								shapes[i].outline.push_back(p3);
								//cout << "Adding curved edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								//cout << p2 << p3 << endl;
								new_shape=false;
								break;
							}
							else if(shapes[i].outline.front()==p3)
							{
								shapes[i].outline.insert(shapes[i].outline.begin(),p2);
								shapes[i].outline.insert(shapes[i].outline.begin(),p1);
								//cout << "Adding curved edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								//cout << p2 << p3 << endl;
								new_shape=false;
								break;
							}

							if(shapes[i].outline.back()==p3 &&
								!(*(shapes[i].outline.rbegin()+1)==p2))
							{
								shapes[i].outline.push_back(p2);
								shapes[i].outline.push_back(p1);
								//cout << "Adding curved edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								//cout << p2 << p3 << endl;
								new_shape=false;
								break;
							}
							else if(shapes[i].outline.front()==p1 &&
								!(shapes[i].outline[1]==p2))
							{
								shapes[i].outline.insert(shapes[i].outline.begin(),p2);
								shapes[i].outline.insert(shapes[i].outline.begin(),p3);
								//cout << "Adding curved edge to shape " << i << endl;
								//cout << p1 << p2 << endl;
								//cout << p2 << p3 << endl;
								new_shape=false;
								break;
							}
						}
					}
					if(new_shape)
					{
						//cout << "Adding edge to new shape" << endl;
						//cout << p1 << p2 << endl;
						//cout << p2 << p3 << endl;
						shapes.push_back(Shape());
						shapes.back().outline.push_back(p1);
						shapes.back().outline.push_back(p2);
						shapes.back().outline.push_back(p3);
						shapes.back().color=color;
					}
				}
				count+=2;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;
				count++;
				//cout << "Move " << startX << ' ' << startY << endl;
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
		cur=cur->next;
	}

	//Let's join shape pieces together
	for(int i=0;i<shapes.size();i++)
	{
		for(int j=i+1;j<shapes.size();j++)
		{
			if(shapes[i].color==shapes[j].color)
			{
				if(shapes[i].outline.back()==shapes[j].outline.front())
				{
					shapes[i].outline.insert(shapes[i].outline.end(),
							shapes[j].outline.begin()+1,shapes[j].outline.end());
					shapes.erase(shapes.begin()+j);
					j--;
					continue;
				}
				else if(shapes[i].outline.front()==shapes[j].outline.back())
				{
					shapes[i].outline.insert(shapes[i].outline.begin(),
							shapes[j].outline.begin(),shapes[j].outline.end()-1);
					shapes.erase(shapes.begin()+j);
					j--;
					continue;
				}
				else if(shapes[i].outline.back()==shapes[j].outline.back() &&    //This should not match with
						shapes[i].outline[shapes[i].outline.size()-2]!=	//the reverse of the same edge
						shapes[j].outline[shapes[j].outline.size()-2])
				{
					shapes[i].outline.insert(shapes[i].outline.end(),
							shapes[j].outline.rbegin()+1,shapes[j].outline.rend());
					shapes.erase(shapes.begin()+j);
					j--;
					continue;
				}
				else if(shapes[i].outline.front()==shapes[j].outline.front() &&
						shapes[i].outline[1]!=shapes[j].outline[1])
				{
					shapes[i].outline.insert(shapes[i].outline.begin(),
							shapes[j].outline.rbegin(),shapes[j].outline.rend()-1);
					shapes.erase(shapes.begin()+j);
					j--;
					continue;
				}
			}
		}
	}
	sort(shapes.begin(),shapes.end());
}

void DefineFont3Tag::genGlyphShape(vector<Shape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToShapeVector(cur,s);

	for(int i=0;i<s.size();i++)
	{
		for(int j=0;j<s[i].outline.size();j++)
			s[i].outline[j]/=20;
		s[i].BuildFromEdges(NULL);
	}

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

void DefineFont2Tag::genGlyphShape(vector<Shape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToShapeVector(cur,s);

	for(int i=0;i<s.size();i++)
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

void DefineFontTag::genGlyphShape(vector<Shape>& s,int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToShapeVector(cur,s);

	for(int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL);

	//Should check fill state
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(TRACE,"ShowFrame");
}

bool PlaceObject2Tag::list_orderer(const pair<PlaceInfo, IDisplayListElem*> a, int d)
{
	return a.second->Depth<d;
}

void PlaceObject2Tag::execute(MovieClip* parent, list < pair< PlaceInfo, IDisplayListElem*> >& ls)
{
	//TODO: support clipping
	if(ClipDepth!=0)
		return;

	PlaceInfo infos;
	//Find if this id is already on the list
	list < pair<PlaceInfo, IDisplayListElem*> >::iterator it=ls.begin();
	for(it;it!=ls.end();it++)
	{
		if(it->second->Depth==Depth)
		{
			infos=it->first;
			if(!PlaceFlagMove)
			{
				LOG(ERROR,"Depth already used already on displaylist");
				abort();
			}
			break;
		}
	}

	if(PlaceFlagHasMatrix)
		infos.Matrix=Matrix;

	IDisplayListElem* toAdd=NULL;
	if(PlaceFlagHasCharacter)
	{
		LOG(TRACE,"Placing ID " << CharacterId);
		DictionaryTag* dict=parent->root->dictionaryLookup(CharacterId);
		toAdd=dict->instance();
		if(toAdd==NULL)
			abort();

		if(PlaceFlagHasColorTransform)
			toAdd->ColorTransform=ColorTransform;

		if(PlaceFlagHasRatio)
			toAdd->Ratio=Ratio;

		if(PlaceFlagHasClipDepth)
			toAdd->ClipDepth=ClipDepth;

		toAdd->root=parent->root;
		toAdd->Depth=Depth;
		if(!PlaceFlagMove)
		{
			list<pair<PlaceInfo, IDisplayListElem*> >::iterator it=lower_bound(ls.begin(),ls.end(),Depth,list_orderer);
			ls.insert(it,make_pair(infos,toAdd));
		}
	}

	if(PlaceFlagHasName)
	{
		//Set a variable on the parent to link this object
		LOG(NO_INFO,"Registering ID " << CharacterId << " with name " << Name);
		if(!(PlaceFlagMove))
		{
			if(toAdd->constructor)
				toAdd->constructor->call(toAdd,NULL);
			parent->setVariableByName(Name,toAdd);
		}
		else
			LOG(ERROR, "Moving of registered objects not really supported");
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
				ls.erase(it);
				list<pair<PlaceInfo, IDisplayListElem*> >::iterator it=lower_bound(ls.begin(),ls.end(),Depth,list_orderer);
				ls.insert(it,make_pair(infos,toAdd));
			}
		}
		else
			LOG(ERROR,"no char to move at depth " << Depth << " name " << Name);
	}
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	LOG(TRACE,"PlaceObject2");

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

	if(PlaceFlagHasCharacter && CharacterId==0)
		abort();
}

void SetBackgroundColorTag::execute()
{
	pt->root->setBackground(BackgroundColor);
}

FrameLabelTag::FrameLabelTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	in >> Name;
	if(pt->version>=6)
	{
		UI8 NamedAnchor=in.peek();
		if(NamedAnchor==1)
			in >> NamedAnchor;
	}
}

void FrameLabelTag::execute(MovieClip* parent, list < pair< PlaceInfo, IDisplayListElem*> >& ls)
{
	LOG(NOT_IMPLEMENTED,"TODO: FrameLabel exec");
	//sys.currentClip->frames[sys.currentClip->state.FP].setLabel(Name);
}

DefineButton2Tag::DefineButton2Tag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in),IdleToOverUp(false)
{
	state=BUTTON_UP;
	in >> ButtonId;
	BitStream bs(in);
	UB(7,bs);
	TrackAsMenu=UB(1,bs);
	in >> ActionOffset;

	BUTTONRECORD br;

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

IDisplayListElem* DefineButton2Tag::instance()
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

DefineVideoStreamTag::DefineVideoStreamTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(NO_INFO,"DefineVideoStreamTag");
	in >> CharacterID >> NumFrames >> Width >> Height;
	BitStream bs(in);
	UB(4,bs);
	VideoFlagsDeblocking=UB(3,bs);
	VideoFlagsSmoothing=UB(1,bs);
	in >> CodecID;
}

void DefineVideoStreamTag::Render()
{
	LOG(NO_INFO,"DefineVideoStreamTag Render");
	glColor3f(1,0,0);
	glBegin(GL_QUADS);
		glVertex2i(0,0);
		glVertex2i(Width,0);
		glVertex2i(Width,Height);
		glVertex2i(0,Height);
	glEnd();
}

DefineBinaryDataTag::DefineBinaryDataTag(RECORDHEADER h,std::istream& s):DictionaryTag(h,s)
{
	LOG(TRACE,"DefineBinaryDataTag");
	int size=getSize();
	s >> Tag >> Reserved;
	cout << Tag << endl;
	size -= sizeof(Tag)+sizeof(Reserved);
	data=new uint8_t[size];
	s.read((char*)data,size);
}

FileAttributesTag::FileAttributesTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(TRACE,"FileAttributesTag Tag");
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
	{
		//TODO: should move to single VM model
		sem_init(&ABCVm::sem_ex,0,1);
	}
}

