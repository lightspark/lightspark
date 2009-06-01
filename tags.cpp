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
#include <GL/gl.h>

using namespace std;

long timeDiff(timespec& s, timespec& d);

extern __thread SystemState* sys;

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
		case 88:
			return new DefineFontNameTag(h,f);
		default:
			LOG(NOT_IMPLEMENTED,"Unsupported tag type " << (h>>6));
			Tag t(h,f);
			t.skip(f);
			break;
	}
}

RemoveObject2Tag::RemoveObject2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	in >> Depth;

	list<IDisplayListElem*>::iterator it=sys->parsingDisplayList->begin();

	for(it;it!=sys->parsingDisplayList->end();it++)
	{
		if((*it)->getDepth()==Depth)
		{
			/*SWFObject* t=dynamic_cast<SWFObject*>(*it);
			if(t!=NULL)
				LOG(NO_INFO,"Removing " << t->getName());*/
			sys->parsingDisplayList->erase(it);
			break;
		}
	}
}

SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	in >> BackgroundColor;
}

bool list_orderer(const DisplayListTag* a, int d);

DefineEditTextTag::DefineEditTextTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	setVariableByName("text",SWFObject(new Integer(0)));
	in >> CharacterID >> Bounds;
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
	ISWFObject* target_bak=sys->parsingTarget;
	sys->parsingTarget=this;

	list < IDisplayListElem* >* bak=sys->parsingDisplayList;
	sys->parsingDisplayList=&displayList;
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
				addToDisplayList(dynamic_cast<DisplayListTag*>(tag));
				break;
			case SHOW_TAG:
			{
				//TODO: sync maybe not needed
				sem_wait(&sem_frames);
				frames.push_back(Frame(displayList));
				sem_post(&sem_frames);
				break;
			}
			case CONTROL_TAG:
				LOG(ERROR,"Control tag inside a sprite. Should not happen.");
				break;
		}
	}
	while(tag->getType()!=END_TAG);
	if(frames.size()!=FrameCount)
		LOG(ERROR,"Inconsistent frame count");
	sys->parsingDisplayList=bak;

	sys->parsingTarget=target_bak;
	LOG(TRACE,"EndDefineSprite ID: " << SpriteID);
}

void DefineSpriteTag::printInfo(int t)
{
/*	MovieClip* bak=sys.currentClip;
	sys.currentClip=&clip;
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineSprite Info ID " << SpriteID << endl;
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "\tFrame Count " << FrameCount << " real " << clip.frames.size() << endl;
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "\tDisplay List Size " << clip.frames.back().displayList.size() << endl;
	
	list<DisplayListTag*>::iterator it=clip.frames.back().displayList.begin();
	int count=0;
	for(it;it!=clip.frames.back().displayList.end();it++)
	{
		count++;
		(*it)->printInfo(t+1);
		if(count>5 && clip.frames.back().hack)
			break;
	}
	sys.currentClip=bak;*/
}

void drawStenciled(const RECT& bounds, int fill0, int fill1, const FILLSTYLE* style0, const FILLSTYLE* style1)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
	if(fill0)
	{
		glStencilFunc(GL_EQUAL,fill0,0xff);
		if(style0)
			style0->setFragmentProgram();
		else
		{
			glUseProgram(0);
			glColor3f(1,0,0);
		}
		glBegin(GL_QUADS);
			glVertex2i(bounds.Xmin,bounds.Ymin);
			glVertex2i(bounds.Xmin,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymin);
		glEnd();
	}
	if(fill1)
	{
		glStencilFunc(GL_EQUAL,fill1,0xff);
		if(style1)
			style1->setFragmentProgram();
		else
		{
			glUseProgram(0);
			glColor3f(0,1,1);
		}
		glBegin(GL_QUADS);
			glVertex2i(bounds.Xmin,bounds.Ymin);
			glVertex2i(bounds.Xmin,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymin);
		glEnd();
	}
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
	LOG(TRACE,"DefineText");
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;

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
				DictionaryTag* it3=sys->dictionaryLookup(it->FontID);
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
	std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
	std::vector < GLYPHENTRY >::iterator it2;
	int count=0;
	int shapes_done=0;
	int x=0,y=0;//1024;
	int cur_height;
	float matrix[16];
	TextMatrix.get4DMatrix(matrix);
	glEnable(GL_STENCIL_TEST);

	//Build a fake FILLSTYLE
	FILLSTYLE f;
	f.FillStyleType=0x00;
	f.Color=it->TextColor;
	for(it;it!=TextRecords.end();it++)
	{
		if(it->StyleFlagsHasFont)
			cur_height=it->TextHeight;
		it2 = it->GlyphEntries.begin();
		int x2=x,y2=y;
		x2+=(*it).XOffset;
		y2+=(*it).YOffset;
		for(it2;it2!=(it->GlyphEntries.end());it2++)
		{
			glPushMatrix();
			glMultMatrixf(matrix);
			glTranslatef(x2,y2,0);
			float scale=cur_height;
			scale/=1024;
			glScalef(scale,scale,1);
			while(cached[shapes_done].id==count)
			{
				cached[shapes_done].Render();
				shapes_done++;
				if(shapes_done==cached.size())
					break;
			}
			glPopMatrix();
			x2+=it2->GlyphAdvance;
			count++;
		}
	}
	drawStenciled(TextBounds,1,0,&f,NULL);
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void DefineTextTag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineText Info" << endl;
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):DictionaryTag(h,in)
{
	LOG(TRACE,"DefineShapeTag");
	Shapes.version=1;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

void DefineShapeTag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
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

void DefineShape2Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineShape2 Info ID " << ShapeId << endl;
}

void DefineShape4Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineShape4 Info ID " << ShapeId << endl;
}

void DefineShape3Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineShape3 Info ID " << ShapeId << endl;
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

void DefineMorphShapeTag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineMorphShape Info" << endl;
}

std::ostream& operator<<(std::ostream& s, const Vector2& p)
{
	s << "{ "<< p.x << ',' << p.y << " } [" << p.index  << ']' << std::endl;
	return s;
}

void FromShaperecordListToShapeVector(SHAPERECORD* cur, vector<Shape>& shapes, bool& def_color0, bool& def_color1);

void DefineMorphShapeTag::Render()
{
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(EndEdges.ShapeRecords);

	bool def_color0,def_color1;
	FromShaperecordListToShapeVector(cur,shapes,def_color0,def_color1);

	for(int i=0;i<shapes.size();i++)
		shapes[i].BuildFromEdges(MorphFillStyles.FillStyles,def_color0^def_color1);

	sort(shapes.begin(),shapes.end());

	std::vector < Shape >::iterator it=shapes.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=shapes.end();it++)
	{
		it->Render();
		if(it->closed)
			drawStenciled(EndBounds,it->color0,it->color1,it->style0,it->style1);
	}
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void DefineShapeTag::Render()
{
	LOG(TRACE,"DefineShape Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		bool def_color0,def_color1;
		FromShaperecordListToShapeVector(cur,cached,def_color0,def_color1);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles,def_color0^def_color1);

		sort(cached.begin(),cached.end());

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		if(it->closed)
			drawStenciled(ShapeBounds,it->color0,it->color1,it->style0,it->style1);
	}
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void DefineShape2Tag::Render()
{
	LOG(TRACE,"DefineShape2 Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		bool def_color0,def_color1;
		FromShaperecordListToShapeVector(cur,cached,def_color0,def_color1);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles,def_color0^def_color1);

		sort(cached.begin(),cached.end());

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		if(it->color0 >= Shapes.FillStyles.FillStyleCount)
			it->style0=NULL;
		if(it->color1 >= Shapes.FillStyles.FillStyleCount)
			it->style1=NULL;
		LOG(NOT_IMPLEMENTED,"HACK schifoso");
		drawStenciled(ShapeBounds,it->color0,it->color1,it->style0,it->style1);
	}
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void DefineShape4Tag::Render()
{
	LOG(TRACE,"DefineShape4 Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		bool def_color0,def_color1;
		FromShaperecordListToShapeVector(cur,cached,def_color0,def_color1);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles,def_color0^def_color1);

		sort(cached.begin(),cached.end());

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		if(it->closed)
			drawStenciled(ShapeBounds,it->color0,it->color1,it->style0,it->style1);
	}
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void DefineShape3Tag::Render()
{
	LOG(TRACE,"DefineShape3 Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		bool def_color0,def_color1;
		FromShaperecordListToShapeVector(cur,cached,def_color0,def_color1);

		for(int i=0;i<cached.size();i++)
			cached[i].BuildFromEdges(Shapes.FillStyles.FillStyles,def_color0^def_color1);

		sort(cached.begin(),cached.end());

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		if(it->closed)
			drawStenciled(ShapeBounds,it->color0,it->color1,it->style0,it->style1);
	}
	glDisable(GL_STENCIL_TEST);
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes
* * \param def_color0 this will be set if color0 is set in any of the shapes
* * \param def_color1 this will be set if color1 is set in any of the shapes */

void FromShaperecordListToShapeVector(SHAPERECORD* cur, vector<Shape>& shapes,bool& def_color0, bool& def_color1)
{
	int startX=0;
	int startY=0;
	int count=0;
	int color0=0;
	int color1=0;

	def_color0=false;
	def_color1=false;
	bool eos=false; //end of shape
	shapes.push_back(Shape());
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
				shapes.back().edges.push_back(Edge(p1,p2,color0,color1));

				if(p2==shapes.back().edges.front().p1)
					eos=true;
				count++;
			}
			else
			{
				Vector2 p1(startX,startY,count);
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				Vector2 p2(startX,startY,count+1);
				shapes.back().edges.push_back(Edge(p1,p2,color0,color1));
				count++;

				Vector2 p3(startX,startY,count);
				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				Vector2 p4(startX,startY,count+1);
				shapes.back().edges.push_back(Edge(p3,p4,color0,color1));
				if(p4==shapes.back().edges.front().p1)
					eos=true;
				count++;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;

				if(!shapes.back().edges.empty() && !eos)
					abort();
				//add new shape (if current is not empty)
				if(!shapes.back().edges.empty())
					shapes.push_back(Shape());
			}
/*			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}*/
			if(cur->StateFillStyle1)
			{
				color1=cur->FillStyle1;
				def_color1=true;
			}
			if(cur->StateFillStyle0)
			{
				color0=cur->FillStyle0;
				def_color0=true;
			}
		}
		cur=cur->next;
	}
}

void DefineFont3Tag::genGlyphShape(vector<Shape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);

	bool def_color0,def_color1;
	FromShaperecordListToShapeVector(cur,s,def_color0,def_color1);

	for(int i=0;i<s.size();i++)
	{
		for(int j=0;j<s[i].edges.size();j++)
		{
			s[i].edges[j].p1/=20;
			s[i].edges[j].p2/=20;
		}
		s[i].BuildFromEdges(NULL,def_color0^def_color1);
		//TODO: should fill styles be normalized
		s[i].winding=!s[i].winding;
	}

	sort(s.begin(),s.end());

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

	bool def_color0,def_color1;
	FromShaperecordListToShapeVector(cur,s,def_color0,def_color1);

	for(int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL,def_color0^def_color1);

	sort(s.begin(),s.end());

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

	bool def_color0,def_color1;
	FromShaperecordListToShapeVector(cur,s,def_color0,def_color1);

	for(int i=0;i<s.size();i++)
		s[i].BuildFromEdges(NULL,def_color0^def_color1);

	sort(s.begin(),s.end());
	//Should check fill state
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(TRACE,"ShowFrame");
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in),wrapped(NULL),_scalex(100)
{
	LOG(TRACE,"PlaceObject2");
//	LOG(NO_INFO,"Should render with offset " << _x << " " << _y);

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
	list < IDisplayListElem*>::iterator it=sys->parsingDisplayList->begin();
	PlaceObject2Tag* already_on_list=NULL;
	for(it;it!=sys->parsingDisplayList->end();it++)
	{
		if((*it)->getDepth()==Depth)
		{
			already_on_list=dynamic_cast<PlaceObject2Tag*>(*it);
			if(!PlaceFlagMove)
				LOG(NO_INFO,"PlaceObject: MoveFlag not set, but depth is already used");
			break;
		}
	}
	if(PlaceFlagHasCharacter)
	{
		in >> CharacterId;
		DictionaryTag* r=sys->dictionaryLookup(CharacterId);
		//DefineSpriteTag* s=dynamic_cast<DefineSpriteTag*>(r);
		ISWFObject* s=dynamic_cast<ISWFObject*>(r);
		if(s==NULL)
			wrapped=new ASObject();
		else
			wrapped=s->clone();

		wrapped->setVariableByName("_scalex",SWFObject(&_scalex,true));
	}
	if(PlaceFlagHasMatrix)
	{
		in >> Matrix;
		//_scalex=Matrix.ScaleX;
	}
	if(PlaceFlagHasColorTransform)
		in >> ColorTransform;
	if(PlaceFlagHasRatio)
	{
		in >> Ratio;
	}
	if(PlaceFlagHasName)
	{
		in >> Name;
		LOG(NO_INFO,"Registering ID " << CharacterId << " with name " << Name);
		if(Name=="workarea_mc")
			char a=0;
		if(!(PlaceFlagMove))
		{
			sys->parsingTarget->setVariableByName(Name,wrapped);
			//sys->setVariableByName(Name,wrapped);
		}
		else
			LOG(ERROR, "Moving of registered objects not really supported");
	}
	if(PlaceFlagHasClipDepth)
	{
		in >> ClipDepth;
	}
	if(PlaceFlagHasClipAction)
	{
		in >> ClipActions;
	}
	if(PlaceFlagMove)
	{
		if(already_on_list)
		{
			PlaceObject2Tag* it2=already_on_list;
			if(!PlaceFlagHasCharacter)
			{
				//if(it2->PlaceFlagHasClipAction)
				if(!PlaceFlagHasName && it2->PlaceFlagHasName)
				{
					PlaceFlagHasName=it2->PlaceFlagHasName;
					Name=it2->Name;
				}
				if(!PlaceFlagHasCharacter && it2->PlaceFlagHasCharacter)
				{
					PlaceFlagHasCharacter=it2->PlaceFlagHasCharacter;
					CharacterId=it2->CharacterId;
				}
				if(!PlaceFlagHasMatrix && it2->PlaceFlagHasMatrix)
				{
					PlaceFlagHasMatrix=it2->PlaceFlagHasMatrix;
					Matrix=it2->Matrix;
				}
				if(!PlaceFlagHasColorTransform && it2->PlaceFlagHasColorTransform)
				{
					PlaceFlagHasColorTransform=it2->PlaceFlagHasColorTransform;
					ColorTransform=it2->ColorTransform;
				}
				if(!PlaceFlagHasRatio && it2->PlaceFlagHasRatio)
				{
					PlaceFlagHasRatio=it2->PlaceFlagHasRatio;
					Ratio=it2->Ratio;
				}
				if(!PlaceFlagHasClipDepth && it2->PlaceFlagHasClipDepth)
				{
					PlaceFlagHasClipDepth=it2->PlaceFlagHasClipDepth;
					ClipDepth=it2->ClipDepth;
				}
			}
			sys->parsingDisplayList->erase(it);
		}
		else
			LOG(ERROR,"no char to move at depth " << Depth << " name " << Name);
	}
	if(CharacterId==0)
		abort();
}

/*ISWFObject* PlaceObject2Tag::getVariableByName(const string& name, bool& found)
{
	if(wrapped)
		return wrapped->getVariableByName(name,found);
	else
		LOG(ERROR,"No object wrapped");
}

ISWFObject* PlaceObject2Tag::setVariableByName(const string& name, const SWFObject& o)
{
	if(wrapped)
		return wrapped->setVariableByName(name,o);
	else
	{
		LOG(ERROR,"No object wrapped");
		return NULL;
	}
}*/

void PlaceObject2Tag::Render()
{
	//TODO: support clipping
	if(ClipDepth!=0)
		return;

	IRenderObject* it=dynamic_cast<IRenderObject*>(wrapped);
	if(it==NULL)
	{
		DictionaryTag* it2=sys->dictionaryLookup(CharacterId);
		it=dynamic_cast<IRenderObject*>(it2);
	}
	if(it==NULL)
		LOG(ERROR,"Could not find Character in dictionary");
	
	float matrix[16];
	MATRIX m2(Matrix);
	m2.ScaleX*=_scalex/100.0f;
	m2.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);
	it->Render();
	glPopMatrix();
}

void PlaceObject2Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "PlaceObject2 Info ID " << CharacterId << endl;
/*	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "vvvvvvvvvvvvv" << endl;

	sem_wait(&sys.sem_dict);
	std::vector< DictionaryTag* >::iterator it=sys.dictionary.begin();
	for(it;it!=sys.dictionary.end();it++)
	{
		if((*it)->getId()==CharacterId)
			break;
	}
	if(it==sys.dictionary.end())
	{
		throw "Object does not exist";
	}
	sem_post(&sys.sem_dict);

	(*it)->printInfo(t+1);

	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "^^^^^^^^^^^^^" << endl;*/

}

void SetBackgroundColorTag::execute()
{
	sys->setBackground(BackgroundColor);
}

FrameLabelTag::FrameLabelTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	in >> Name;
	if(sys->version>=6)
	{
		UI8 NamedAnchor=in.peek();
		if(NamedAnchor==1)
			in >> NamedAnchor;
	}
}

void FrameLabelTag::Render()
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

void DefineButton2Tag::handleEvent(Event* e)
{
	state=BUTTON_OVER;
	IdleToOverUp=true;
}

void DefineButton2Tag::Render()
{
	for(unsigned int i=0;i<Characters.size();i++)
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
	}
}

void DefineButton2Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineButton2 Info" << endl;
/*	for(unsigned int i=0;i<Characters.size();i++)
	{
		sem_wait(&sys.sem_dict);
		std::vector< DictionaryTag* >::iterator it=sys.dictionary.begin();
		for(it;it!=sys.dictionary.end();it++)
		{
			if((*it)->getId()==Characters[i].CharacterID)
				break;
		}
		if(it==sys.dictionary.end())
		{
			throw "Object does not exist";
		}
		DictionaryTag* c=*it;
		
		sem_post(&sys.sem_dict);
		c->printInfo(t+1);

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
