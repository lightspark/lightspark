/**************************************************************************
    Lighspark, a free flash player implementation

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

#include <GL/gl.h>
#include "tags.h"
#include "actions.h"
#include "geometry.h"
#include "swftypes.h"
#include "swf.h"
#include "input.h"
#include "logger.h"
#include <vector>
#include <list>
#include <algorithm>

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

void drawStenciled(const RECT& bounds, bool filled0, bool filled1, const RGBA& color0, const RGBA& color1)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilFunc(GL_EQUAL,6,0xf);
	glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
	if(filled0)
	{
		glColor3ub(color0.Red,color0.Green,color0.Blue);
		glBegin(GL_QUADS);
			glVertex2i(bounds.Xmin,bounds.Ymin);
			glVertex2i(bounds.Xmin,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymax);
			glVertex2i(bounds.Xmax,bounds.Ymin);
		glEnd();
	}
	if(filled1)
	{
		glColor3ub(color1.Red,color1.Green,color1.Blue);
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
	return;
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
				font->genGliphShape(new_shapes,it2->GlyphIndex);
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
	for(it;it!=TextRecords.end();it++)
	{
		if(it->StyleFlagsHasFont)
			cur_height=it->TextHeight;
		glEnable(GL_STENCIL_TEST);
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
//		glPushMatrix();
//		glMultMatrixf(matrix);
		drawStenciled(TextBounds,true,false,it->TextColor,RGBA());
		glClear(GL_STENCIL_BUFFER_BIT);
//		glPopMatrix();
		glDisable(GL_STENCIL_TEST);
	}
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

void DefineShape2Tag::printInfo(int t)
{
	for(int i=0;i<t;i++)
		cerr << '\t';
	cerr << "DefineShape2 Info ID " << ShapeId << endl;
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

class GraphicStatus
{
public:
	bool validStroke;
	int stroke;

	bool validFill0;
	bool validFill1;
	int fill0;
	int fill1;

	GraphicStatus():validFill0(false),validFill1(false),validStroke(false),fill0(0),fill1(0){}
};

enum VTYPE {START_VERTEX=0,END_VERTEX,NORMAL_VERTEX,SPLIT_VERTEX,MERGE_VERTEX};

class Path
{
public:
	int id;
	int skip_tess;

	std::vector< Path > sub_paths;
	std::vector< Vector2 > points;
	VTYPE getVertexType(const Vector2& v);
	bool closed;
	GraphicStatus state;
	Path():closed(false),skip_tess(false){};
};

void fixIndex(list<Vector2>& points)
{
	list<Vector2>::iterator it=points.begin();
	int i=0;
	for(it;it!=points.end();it++)
	{
		it->index=i;
		i++;
	}
}

void fixIndex(std::vector<Vector2>& points)
{
	int size=points.size();
	for(int i=0;i<size;i++)
		points[i].index=i;
}

std::ostream& operator<<(std::ostream& s, const Vector2& p)
{
	s << "{ "<< p.x << ',' << p.y << " } [" << p.index  << ']' << std::endl;
	return s;
}

bool pointInPolygon(FilterIterator start, FilterIterator end, const Vector2& point);

/*VTYPE getVertexType(const Vector2& v,const std::vector<Vector2>& points)
{
	int a=(v.index+1)%points.size();
	int b=(v.index-1+points.size())%points.size();
	FilterIterator ai(points.begin(),points.end(),v.index);
	FilterIterator bi(points.end(),points.end(),v.index);

	if(points[a] < v &&
		points[b] < v)
	{
		if(!pointInPolygon(ai,bi,v))
			return START_VERTEX;
		else
			return SPLIT_VERTEX;
	}
	else if(v < points[a] &&
		points[b] < v)
	{
		return NORMAL_VERTEX;
	}
	else if(points[a] < v &&
		v < points[b])
	{
		return NORMAL_VERTEX;
	}
	else if(v < points[a]&&
		v < points[b])
	{
		if(!pointInPolygon(ai,bi,v))
			return END_VERTEX;
		else
			return MERGE_VERTEX;
	}
	else
		LOG(ERROR,"Impossible vertex type");
}*/

std::ostream& operator<<(std::ostream& s, const GraphicStatus& p)
{
	s << "ValidFill0 "<< p.validFill0 << std::endl;
	s << "ValidFill1 "<< p.validFill1 << std::endl;
	s << "ValidStroke "<< p.validStroke << std::endl;
	return s;
}

void TessellatePathSimple(Path& path, Shape& shape);
void FromPathToShape(Path& path, Shape& shape)
{
	shape.outline=path.points;
	shape.closed=path.closed;

	if(/*path.skip_tess ||*/ !shape.closed || (!path.state.validFill0 && !path.state.validFill1))
		return;

	TessellatePathSimple(path,shape);

	shape.sub_shapes.resize(path.sub_paths.size());
	for(int i=0;i<path.sub_paths.size();i++)
		FromPathToShape(path.sub_paths[i],shape.sub_shapes[i]);
}

void FromShaperecordListToPaths(const SHAPERECORD* cur, std::vector<Path>& paths);

void DefineMorphShapeTag::Render()
{
	std::vector < Path > paths;
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(EndEdges.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		if(i->points.size()==0)
		{
			LOG(TRACE,"Ignoring empty path");
			continue;
		}
		//TODO: Shape construtor from path
		shapes.push_back(Shape());
		FromPathToShape(*i,shapes.back());

		//Fill graphic data
		if(i->state.validFill0)
		{
			if(i->state.fill0)
			{
				shapes.back().graphic.filled0=true;
				if(MorphFillStyles.FillStyles[i->state.fill0-1].FillStyleType==0x0)
					shapes.back().graphic.color0=MorphFillStyles.FillStyles[i->state.fill0-1].EndColor;
				else
					shapes.back().graphic.color0=RGB(0,0,255);
			}
			else
				shapes.back().graphic.filled0=false;
		}
		else
			shapes.back().graphic.filled0=false;


		if(i->state.validFill1)
		{
			if(i->state.fill1)
			{
				shapes.back().graphic.filled1=true;
				if(MorphFillStyles.FillStyles[i->state.fill0-1].FillStyleType==0x0)
					shapes.back().graphic.color1=MorphFillStyles.FillStyles[i->state.fill1-1].EndColor;
				else
					shapes.back().graphic.color1=RGB(0,0,255);
			}
			else
				shapes.back().graphic.filled1=false;
		}
		else
			shapes.back().graphic.filled1=false;

		if(i->state.validStroke)
		{
			if(i->state.stroke)
			{
				shapes.back().graphic.stroked=true;
				shapes.back().graphic.stroke_color=MorphLineStyles.LineStyles[i->state.stroke-1].EndColor;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		else
			shapes.back().graphic.stroked=false;
	}
/*	if(shapes.size()==1)
	{
		if(shapes[0].graphic.filled1 && !shapes[0].graphic.filled0)
		{
			shapes[0].graphic.filled0=shapes[0].graphic.filled1;
			shapes[0].graphic.filled1=0;
			shapes[0].graphic.color0=shapes[0].graphic.color1;
		}
	}*/
	for(unsigned int i=0;i<shapes.size();i++)
	{
		if(shapes[i].graphic.filled1 && !shapes[i].graphic.filled0)
		{
			shapes[i].graphic.filled0=1;
			shapes[i].graphic.filled1=0;
			shapes[i].graphic.color0=shapes[i].graphic.color1;
			shapes[i].winding=0;
		}
	}
	std::vector < Shape >::iterator it=shapes.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=shapes.end();it++)
	{
		it->Render();
		drawStenciled(EndBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);
	}
/*	it=shapes.begin();
	if(it!=shapes.end())
		drawStenciled(EndBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);*/
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

void FromShaperecordListToEdgeVector(const SHAPERECORD* cur, vector<Edge>& edges);

bool lex_order(const Edge& a, const Edge& b)
{
	if(a.p1.index<b.p1.index)
		return true;
	else
		return false;
}

void DefineShapeTag::Render()
{
	LOG(TRACE,"DefineShape Render");
	if(cached.size()==0)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		vector < Edge > edges;
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToEdgeVector(cur,edges);

		ofstream f("edges.dat");

		for(int i=0;i<edges.size();i++)
			f << edges[i].p1.x << ' ' << edges[i].p1.y << endl;

		f.close();

		sort(edges.begin(),edges.end());
		list < Edge > active_edges;

		for(int i=0;i<edges.size();i++)
		{
			const Vector2& p=edges[i].highPoint();
			int x;
			int dr=1000000;
			int dl=1000000;
			list<Edge>::iterator ir;
			list<Edge>::iterator il;
			bool connected_edge=false;
			//Find if current edge follows one already on active_edge list
			for(list<Edge>::iterator j=active_edges.begin();j!=active_edges.end();j++)
			{
				if(j->p1==edges[i].p2)
				{
					active_edges.insert(j,edges[i]);
					connected_edge=true;
					break;
				}
			}
			if(!connected_edge && !active_edges.empty() && active_edges.back().p2==edges[i].p1)
			{
				active_edges.push_back(edges[i]);
				active_edges.sort(lex_order);
				connected_edge=true;
			}
			if(connected_edge)
				continue;

			//Find active edges that are intersected on p.y. We find both the nearest edge on the left and on the right.
			//If there are no edge on either side, the current edge is simply added to the active_edges list
			for(list<Edge>::iterator j=active_edges.begin();j!=active_edges.end();j++)
			{
				if(j->yIntersect(p,x))
				{
					if(x>p.x && dr>(x-p.x))
					{
						ir=j;
						dr=x-p.x;
					}
					else if(x<p.x && dl>(p.x-x))
					{
						il=j;
						dl=p.x-x;
					}
					else if(x==p.x)
						LOG(ERROR,"Intersection at distance zero?");
				}
			}
			if(dl==1000000 || dr==1000000)
			{
				//We are an external edge. Add to the active_edge list
				active_edges.push_back(edges[i]);
			}
			else
			{
			/*	for(list<Edge>::iterator j=active_edges.begin();j!=active_edges.end();j++)
				{
					cout << j->p1 << ' ' << j->p2 << ' ' << j->index << endl;
				}

				//Intersection to handle. We split the nearest intersect edge at (x,p.y) and find
				//a cycle to in the active_edges list
				Shape t;
				t.closed=true;
				list<Edge>::iterator is;
				list<Edge>::iterator id;
				Vector2 last(0,0,-1);
				if(il->p1.index<ir->p1.index)
				{
					is=il;
					id=ir;
					t.outline.push_back(Vector2(dl,p.y,-1));
					last=Vector2(dr,p.y,-1);
				}
				else
				{
					is=ir;
					id=il;
					t.outline.push_back(Vector2(dr,p.y,-1));
					last=Vector2(dl,p.y,-1);
				}

				//Split left edge in two sections
				//active_edges.insert(is,Edge(is->p1,Vector2(dl,p.y,-1),is->index));
				//active_edges.insert(is,Edge(Vector2(dl,p.y,-1),is->p2,is->index));

				while(is!=id)
				{
					t.outline.push_back(is->p2);
					active_edges.erase(is++);
				}

				t.outline.push_back(last);

				//Split right edge in two sections
				list<Edge>::iterator it=id;
				it++;
				//active_edges.insert(it,Edge(Vector2(dr,p.y,-1),id->p2,id->index));
				//active_edges.insert(it,Edge(id->p1,Vector2(dr,p.y,-1),id->index));
				//active_edges.erase(id);*/
			}
		}
		//Build shapes from the active_edges list
		for(list<Edge>::iterator j=active_edges.begin();j!=active_edges.end();)
		{
			Shape t;
			t.closed=false;
			t.outline.push_back(j->p1);
			Vector2 expected=j->p2;
			while(1)
			{
				j++;
				if(j==active_edges.end())
					break;
				if(j->p1==expected)
				{
					t.outline.push_back(j->p1);
					if(j->p2==t.outline.front())
					{
						t.closed=true;
						break;
					}
					expected=j->p2;
				}
				else
					break;
			}

			cached.push_back(t);
		}
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	int count=0;
	for(it;it!=cached.end();it++)
	{
		it->Render(count);
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);
		count++;
	}
/*	it=cached.begin();
	if(it!=cached.end())
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);*/
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
		std::vector < Path > paths;
		std::vector < Shape > shapes;
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToPaths(cur,paths);
		std::vector < Path >::iterator i=paths.begin();
		for(i;i!=paths.end();i++)
		{
			if(i->points.size()==0)
			{
				LOG(TRACE,"Ignoring empty path");
				continue;
			}
			//TODO: Shape construtor from path
			shapes.push_back(Shape());
			FromPathToShape(*i,shapes.back());
			//Fill graphic data
			if(i->state.validFill0)
			{
				if(i->state.fill0)
				{
					shapes.back().graphic.filled0=true;
					if(Shapes.FillStyles.FillStyles[i->state.fill0-1].FillStyleType==0x0)
						shapes.back().graphic.color0=Shapes.FillStyles.FillStyles[i->state.fill0-1].Color;
					else
					{
						shapes.back().graphic.color0=RGB(0,0,255);
						LOG(NOT_IMPLEMENTED,"Fill style " << Shapes.FillStyles.FillStyles[i->state.fill0-1].FillStyleType << "not implemented");
					}
				}
				else
					shapes.back().graphic.filled0=false;
			}
			else
				shapes.back().graphic.filled0=false;


			if(i->state.validFill1)
			{
				if(i->state.fill1)
				{
					shapes.back().graphic.filled1=true;
					if(Shapes.FillStyles.FillStyles[i->state.fill1-1].FillStyleType==0x0)
						shapes.back().graphic.color1=Shapes.FillStyles.FillStyles[i->state.fill1-1].Color;
					else
					{
						shapes.back().graphic.color1=RGB(0,0,255);
						LOG(NOT_IMPLEMENTED,"Fill style " << Shapes.FillStyles.FillStyles[i->state.fill1-1].FillStyleType << "not implemented");
					}
				}
				else
					shapes.back().graphic.filled1=false;
			}
			else
				shapes.back().graphic.filled1=false;

			if(i->state.validStroke)
			{
				if(i->state.stroke)
				{
					shapes.back().graphic.stroked=true;
					shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state.stroke-1].Color;
				}
				else
					shapes.back().graphic.stroked=false;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		for(unsigned int i=0;i<shapes.size();i++)
		{
			if(shapes[i].graphic.filled1 && !shapes[i].graphic.filled0)
			{
				shapes[i].graphic.filled0=1;
				shapes[i].graphic.filled1=0;
				shapes[i].graphic.color0=shapes[i].graphic.color1;
				shapes[i].winding=0;
			}
		}
		cached=shapes;
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);
	}
/*	it=cached.begin();
	if(it!=cached.end())
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);*/
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
		std::vector < Path > paths;
		std::vector < Shape > shapes;
		SHAPERECORD* cur=&(Shapes.ShapeRecords);

		FromShaperecordListToPaths(cur,paths);
		std::vector < Path >::iterator i=paths.begin();
		for(i;i!=paths.end();i++)
		{
			if(i->points.size()==0)
			{
				LOG(TRACE,"Ignoring empty path");
				continue;
			}
			//TODO: Shape construtor from path
			shapes.push_back(Shape());
			FromPathToShape(*i,shapes.back());
			//Fill graphic data
			if(i->state.validFill0)
			{
				if(i->state.fill0)
				{
					shapes.back().graphic.filled0=true;
					if(Shapes.FillStyles.FillStyles[i->state.fill0-1].FillStyleType==0x0)
						shapes.back().graphic.color0=Shapes.FillStyles.FillStyles[i->state.fill0-1].Color;
					else
					{
						shapes.back().graphic.color0=RGB(0,0,255);
						LOG(NOT_IMPLEMENTED,"Fill style " << Shapes.FillStyles.FillStyles[i->state.fill0-1].FillStyleType << "not implemented");
					}
				}
				else
					shapes.back().graphic.filled0=false;
			}
			else
				shapes.back().graphic.filled0=false;


			if(i->state.validFill1)
			{
				if(i->state.fill1)
				{
					shapes.back().graphic.filled1=true;
					if(Shapes.FillStyles.FillStyles[i->state.fill1-1].FillStyleType==0x0)
						shapes.back().graphic.color1=Shapes.FillStyles.FillStyles[i->state.fill1-1].Color;
					else
					{
						shapes.back().graphic.color1=RGB(0,0,255);
						LOG(NOT_IMPLEMENTED,"Fill style " << Shapes.FillStyles.FillStyles[i->state.fill1-1].FillStyleType << "not implemented");
					}
				}
				else
					shapes.back().graphic.filled1=false;
			}
			else
				shapes.back().graphic.filled1=false;

			if(i->state.validStroke)
			{
				if(i->state.stroke)
				{
					shapes.back().graphic.stroked=true;
					shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state.stroke-1].Color;
				}
				else
					shapes.back().graphic.stroked=false;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		for(unsigned int i=0;i<shapes.size();i++)
		{
			if(shapes[i].graphic.filled1 && !shapes[i].graphic.filled0)
			{
				shapes[i].graphic.filled0=1;
				shapes[i].graphic.filled1=0;
				shapes[i].graphic.color0=shapes[i].graphic.color1;
				shapes[i].winding=0;
			}
		}
		cached=shapes;
		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->cache_time+=timeDiff(ts,td);
	}

	std::vector < Shape >::iterator it=cached.begin();
	glEnable(GL_STENCIL_TEST);
	for(it;it!=cached.end();it++)
	{
		it->Render();
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);
	}
/*	it=cached.begin();
	if(it!=cached.end())
		drawStenciled(ShapeBounds,it->graphic.filled0,it->graphic.filled1,it->graphic.color0,it->graphic.color1);*/
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
}

/*void SplitPath(std::vector<Path>& paths,int a, int b)
{
	Path p;
	p.closed=true;
	if(a>b)
	{
		int c=a;
		a=b;
		b=c;
	}
	if(a==b)
		LOG(ERROR,"SplitPath, internal error");

	std::vector<Vector2>::iterator first,end;
	Path* cur_path;
	for(unsigned int i=0;i<paths.size();i++)
	{
		first=find(paths[i].points.begin(),paths[i].points.end(),a);
		end=find(paths[i].points.begin(),paths[i].points.end(),b);

		if(first!=paths[i].points.end() && end!=paths[i].points.end())
		{
			p.state=paths[i].state;
			cur_path=&paths[i];
			break;
		}
	}

	p.points.insert(p.points.begin(),first,end+1);
	
	cur_path->points.erase(first+1,end);
	paths.push_back(p);
}*/

/*bool pointInPolygon(FilterIterator start, FilterIterator end, const Vector2& point)
{
	if(start==end)
		return false;
	FilterIterator jj=start;
	FilterIterator jj2=start;
	jj2++;

	int count=0;
	int dist;

	while(jj2!=end)
	{
		Edge e(*jj,*jj2,-1);
		if(e.yIntersect(point.y,dist,point.x))
		{
			if(dist-point.x>0)
				count++;
		}
		jj=jj2++;
	}
	Edge e(*jj,*start,-1);
	if(e.yIntersect(point.y,dist,point.x))
	{
		if(dist-point.x>0)
			count++;
	}
	return count%2;
}*/

/*void SplitIntersectingPaths(vector<Path>& paths)
{
	for(int k=0;k<paths.size();k++)
	{
		if(paths[k].points.size()==217)
			char a=0;

		vector < Edge > edges;
		bool next_path=false;
		for(int i=1;i<paths[k].points.size();i++)
		{
			Edge t(paths[k].points[i-1],paths[k].points[i],i-1);
			for(int j=0;j<edges.size();j++)
			{
				if(edges[j].edgeIntersect(t))
				{
					//LOG(NOT_IMPLEMENTED,"Self intersecting path");
					//paths[k].points.clear();
					paths[k].skip_tess=true;
					next_path=true;
					break;
				}
			}
			if(next_path)
				break;
			edges.push_back(t);
		}

		//Check for point collisions
		for(int i=0;i<paths[k].points.size();i++)
		{
			for(int j=i;j<paths[k].points.size();j++)
			{
				if(paths[k].points[i]==paths[k].points[j])
				{
					//LOG(NOT_IMPLEMENTED,"Colliding points in path");
					//paths[k].points.clear();
					paths[k].skip_tess=true;
					next_path=true;
					break;
				}
			}
		}

	}
}*/

void FromShaperecordListToEdgeVector(const SHAPERECORD* cur, vector<Edge>& edges)
{
	int count=0;
	int startX=0,startY=0;
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
				edges.push_back(Edge(p1,p2,count));
				count++;
			}
			else
			{
				Vector2 p1(startX,startY,count);
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				Vector2 p2(startX,startY,count+1);
				edges.push_back(Edge(p1,p2,count));
				count++;

				Vector2 p3(startX,startY,count);
				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				Vector2 p4(startX,startY,count+1);
				edges.push_back(Edge(p3,p4,count));
				count++;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;
				count++;
			}
/*			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}
			if(cur->StateFillStyle1)
			{
				cur_path->state.validFill1=true;
				cur_path->state.fill1=cur->FillStyle1;
			}
			if(cur->StateFillStyle0)
			{
				cur_path->state.validFill0=true;
				cur_path->state.fill0=cur->FillStyle0;
			}*/
		}
		cur=cur->next;
	}
}

void FromShaperecordListToPaths(const SHAPERECORD* cur, std::vector<Path>& paths)
{
/*	int vindex=0;
	int startX=0,startY=0;
	paths.push_back(Path());
	Path* cur_path=&paths.back();
	while(cur)
	{
		if(cur->TypeFlag)
		{
			if(cur_path->points.size()==0)
			{
				cur_path->points.push_back(Vector2(startX,startY,vindex));
				vindex++;
			}
			if(cur->StraightFlag)
			{
				startX+=cur->DeltaX;
				startY+=cur->DeltaY;
				Vector2 nv(startX,startY,vindex);
				if(nv==cur_path->points.front())
				{
					cur_path->closed=true;
					GraphicStatus old_status=cur_path->state;
					paths.push_back(Path());
					cur_path=&paths.back();
					cur_path->state=old_status;
					vindex=0;
				}
				else
					cur_path->points.push_back(Vector2(startX,startY,vindex));
				vindex++;
			}
			else
			{
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				if(Vector2(startX,startY,vindex)==cur_path->points.front())
				{
					LOG(ERROR,"Collision during path generation");
					cur_path->closed=true;
				}
				else
					cur_path->points.push_back(Vector2(startX,startY,vindex));
				vindex++;

				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				Vector2 nv(startX,startY,vindex);
				if(nv==cur_path->points.front())
				{
					cur_path->closed=true;
					GraphicStatus old_status=cur_path->state;
					paths.push_back(Path());
					cur_path=&paths.back();
					cur_path->state=old_status;
					vindex=0;
				}
				else
					cur_path->points.push_back(nv);
				vindex++;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;

				Vector2 nv(startX,startY,vindex);
				GraphicStatus old_status=cur_path->state;
				bool found=false;
				for(int i=0;i<paths.size();i++)
				{
					FilterIterator ai(paths[i].points.begin(),paths[i].points.end(),-1);
					FilterIterator bi(paths[i].points.end(),paths[i].points.end(),-1);
					if(pointInPolygon(ai,bi,nv))
					{
						paths[i].sub_paths.push_back(Path());
						cur_path=&paths[i].sub_paths.back();
						found=true;
						break;
					}
				}
				if(!found)
				{
					if(cur_path->points.size()!=0)
					{
						paths.push_back(Path());
						cur_path=&paths.back();
					}
				}
				cur_path->state=old_status;
				vindex=0;
				cur_path->points.push_back(nv);
				vindex=1;

			}
			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}
			if(cur->StateFillStyle1)
			{
				cur_path->state.validFill1=true;
				cur_path->state.fill1=cur->FillStyle1;
			}
			if(cur->StateFillStyle0)
			{
				cur_path->state.validFill0=true;
				cur_path->state.fill0=cur->FillStyle0;
				
			}
		}
		cur=cur->next;
	}
	//SplitIntersectingPaths(paths);*/
}

inline bool pointInTriangle(const Vector2& P,const Vector2& A,const Vector2& B,const Vector2& C)
{
	Vector2 v0 = C - A;
	Vector2 v1 = B - A;
	Vector2 v2 = P - A;

	long dot00 = v0.dot(v0);
	long dot01 = v0.dot(v1);
	long dot02 = v0.dot(v2);
	long dot11 = v1.dot(v1);
	long dot12 = v1.dot(v2);

	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u > 0) && (v > 0) && (u + v < 1);
}


void TessellatePathSimple(Path& path, Shape& shape)
{
/*	if(path.points.size()<3)
	{
		LOG(NO_INFO,"Tessellating a degenerate path");
		return;
	}
	std::vector<Vector2> P=path.points;
	int area=0;
	int i;
	for(i=0; i<P.size()-1;i++)
	{
		area+=(P[i].y+P[i+1].y)*(P[i+1].x-P[i].x)/2;
	}
	area+=(P[i].y+P[0].y)*(P[0].x-P[i].x)/2;

	if(area<0)
	{
		//reverse(P.begin(),P.end());
		shape.winding=1;
	}
	else
		shape.winding=0;
	i=0;
	int count=0;
	while(P.size()>3)
	{
		fixIndex(P);
		int a=(i+1)%P.size();
		int b=(i-1+P.size())%P.size();
		FilterIterator ai(P.begin(),P.end(),i);
		FilterIterator bi(P.end(),P.end(),i);
		bool not_ear=false;
		if(!pointInPolygon(ai,bi,P[i]))
		{
			for(int j=0;j<P.size();j++)
			{
				if(j==i)
					continue;
				if(pointInTriangle(P[j],P[a],P[i],P[b]))
				{
					not_ear=true;
					break;
				}
			}
			if(!not_ear)
			{
				shape.interior.push_back(Triangle(P[a],P[i],P[b]));
				P.erase(P.begin()+i);
				i=0;
			}
			else
			{
				i++;
				i%=P.size();
			}
		}
		else
		{
			i++;
			i%=P.size();
		}
		count++;
		if(count>30000)
			break;
	}
	shape.interior.push_back(Triangle(P[0],P[1],P[2]));*/
}

void DefineFont2Tag::genGliphShape(vector<Shape>& s, int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	std::vector < Path > paths;
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		if(i->points.size()==0)
		{
			LOG(TRACE,"Ignoring empty path");
			continue;
		}
		//TODO: Shape construtor from path
		s.push_back(Shape());
		FromPathToShape(*i,s.back());

		//Fill graphic data
		s.back().graphic.filled0=true;
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
			s.back().graphic.stroked=false;
	}
}

void DefineFontTag::genGliphShape(vector<Shape>& s,int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	std::vector < Path > paths;
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		if(i->points.size()==0)
		{
			LOG(TRACE,"Ignoring empty path");
			continue;
		}
		//TODO: Shape construtor from path
		s.push_back(Shape());
		FromPathToShape(*i,s.back());

		//Fill graphic data
		s.back().graphic.filled0=true;
		s.back().graphic.filled1=false;
		s.back().graphic.stroked=false;

		if(i->state.validFill0)
		{
			if(i->state.fill0!=1)
				LOG(ERROR,"Not valid fill color for font");
		}

		if(i->state.validFill1)
		{
			LOG(ERROR,"Not valid fill color for font");
		}

		if(i->state.validStroke)
		{
			if(i->state.stroke)
			{
				s.back().graphic.stroked=true;
				LOG(ERROR,"Not valid stroke color for font");
//				s.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
			}
			else
				s.back().graphic.stroked=false;
		}
		else
			s.back().graphic.stroked=false;
	}
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	LOG(TRACE,"ShowFrame");
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in),wrapped(NULL),_y(0),_x(0),_scalex(100)
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

		wrapped->setVariableByName("_y",SWFObject(&_y,true));
		wrapped->setVariableByName("_x",SWFObject(&_x,true));
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
		if(!(PlaceFlagMove))
		{
			SWFObject w_this(this);
			//sys->parsingTarget->setVariableByName(Name,w_this);
			sys->setVariableByName(Name,w_this);
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

SWFObject PlaceObject2Tag::getVariableByName(const STRING& name)
{
	if(wrapped)
		return wrapped->getVariableByName(name);
	else
		LOG(ERROR,"No object wrapped");
}

void PlaceObject2Tag::setVariableByName(const STRING& name, const SWFObject& o)
{
	if(wrapped)
		wrapped->setVariableByName(name,o);
	else
		LOG(ERROR,"No object wrapped");
}

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
		UI8 NamedAnchor;
		in >> NamedAnchor;
		if(NamedAnchor==0)
			LOG(ERROR,"Not a named anchor");
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

	BUTTONCONDACTION bca;
	do
	{
		in >> bca;
		Actions.push_back(bca);
	}
	while(!bca.isLast());
}

void DefineButton2Tag::MouseEvent(int x, int y)
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
