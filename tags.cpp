#include "tags.h"
#include "geometry.h"
#include "swftypes.h"
#include <vector>
#include <list>
#include <algorithm>
#include <GL/gl.h>

extern std::list < RenderTag* > dictionary;
extern std::list < DisplayListTag* > displayList;

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	switch(h>>6)
	{
		case 0:
			std::cout << "position " << f.tellg() << std::endl;
			return new EndTag(h,f);
		case 1:
			std::cout << "position " << f.tellg() << std::endl;
			return new ShowFrameTag(h,f);
		case 2:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineShapeTag(h,f);
	//	case 4:
	//		return new PlaceObjectTag(h,f);
		case 9:
			std::cout << "position " << f.tellg() << std::endl;
			return new SetBackgroundColorTag(h,f);
		case 10:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineFontTag(h,f);
		case 11:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineTextTag(h,f);
		case 13:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineFontInfoTag(h,f);
		case 14:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineSoundTag(h,f);
		case 15:
			std::cout << "position " << f.tellg() << std::endl;
			return new StartSoundTag(h,f);
		case 22:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineShape2Tag(h,f);
		case 26:
			std::cout << "position " << f.tellg() << std::endl;
			return new PlaceObject2Tag(h,f);
		case 39:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineSpriteTag(h,f);
		case 45:
			std::cout << "position " << f.tellg() << std::endl;
			return new SoundStreamHead2Tag(h,f);
		case 48:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineFont2Tag(h,f);
		default:
			std::cout << "position " << f.tellg() << std::endl;
			std::cout << (h>>6) << std::endl;
			throw "unsupported tag";
			break;
	}
}


SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	in >> BackgroundColor;
	std::cout << BackgroundColor << std::endl;
}

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineSprite" << std::endl;
	in >> SpriteID >> FrameCount;
	if(FrameCount!=1)
		throw "unsopported long sprites";
	TagFactory factory(in);
	Tag* tag;
	do
	{
		tag=factory.readTag();	
		std::cout << "\tsprite tag type "<< tag->getType() <<std::endl;
		if(tag->getType()==DISPLAY_LIST_TAG)
			displayList.push_back(dynamic_cast<DisplayListTag*>(tag));
	}
	while(tag->getType()!=END_TAG);
}

DefineFontTag::DefineFontTag(RECORDHEADER h, std::istream& in):FontTag(h,in)
{
	std::cout << "DefineFont" << std::endl;
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
		std::cout << "font read " << i << std::endl;
		SHAPE t;
		in >> t;
		GlyphShapeTable.push_back(t);
	}
}

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in):FontTag(h,in)
{
	std::cout << "DefineFont2" << std::endl;
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
		throw "help7";
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
		std::cout << "font read " << i << std::endl;
		SHAPE t;
		in >> t;
		GlyphShapeTable.push_back(t);
	}
	if(FontFlagsWideCodes)
	{
		throw "help8";
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
	in.ignore(KerningCount*4);
}

DefineTextTag::DefineTextTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineText" << std::endl;
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;
	while(1)
	{
		TEXTRECORD t(this);
		in >> t;
		if(t.TextRecordType+t.StyleFlagsHasFont+t.StyleFlagsHasColor+t.StyleFlagsHasYOffset+t.StyleFlagsHasXOffset==0)
			break;
		TextRecords.push_back(t);
	}
}

DefineSoundTag::DefineSoundTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineSound" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

DefineFontInfoTag::DefineFontInfoTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineFontInfo" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

StartSoundTag::StartSoundTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB DefineSound" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB SoundStreamHead2" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	Shapes.version=1;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

DefineShape2Tag::DefineShape2Tag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineShape2Tag" << std::endl;
	Shapes.version=2;
	in >> ShapeId >> ShapeBounds >> Shapes;
}

void DefineSpriteTag::Render(int layer)
{
	std::cout << "Render Sprite" << std::endl;
	std::list < DisplayListTag* >::iterator it=displayList.begin();
	for(it;it!=displayList.end();it++)
	{
		(*it)->Render();
	}
}

int crossProd(const Vector2& a, const Vector2& b)
{
	return a.x*b.y-a.y*b.x;
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

	GraphicStatus():validFill0(false),validFill1(false),validStroke(false){}
};

enum VTYPE {START_VERTEX=0,END_VERTEX,NORMAL_VERTEX,SPLIT_VERTEX,MERGE_VERTEX};

class Path
{
public:
	int id;

	std::vector< Vector2 > points;
	VTYPE getVertexType(const Vector2& v);
	bool closed;
	GraphicStatus _state;
	GraphicStatus* state;
	Path():closed(false){ state=&_state;};
	Path(const Path& p):closed(p.closed){state=&_state;points=p.points;_state=p._state;}
};

void fixIndex(std::vector<Vector2>& points)
{
	int size=points.size();
	for(int i=0;i<size;i++)
		points[i].index=i;
}

std::ostream& operator<<(std::ostream& s, const Vector2& p)
{
	std::cout << "{ "<< p.x << ',' << p.y << " } [" << p.index  << ']' << std::endl;
}

bool pointInPolygon(FilterIterator start, FilterIterator end, const Vector2& point);

VTYPE getVertexType(const Vector2& v,const std::vector<Vector2>& points)
{
	//std::cout << "get vertex type on " << v << std::endl;
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
	{
		throw "unknown type";
	}
}

std::ostream& operator<<(std::ostream& s, const GraphicStatus& p)
{
	std::cout << "ValidFill0 "<< p.validFill0 << std::endl;
	std::cout << "ValidFill1 "<< p.validFill1 << std::endl;
	std::cout << "ValidStroke "<< p.validStroke << std::endl;
}

void TessellatePath(Path& path, Shape& shape);
void FromPathToShape(Path& path, Shape& shape)
{
	shape.outline=path.points;
	shape.closed=path.closed;

	if(!shape.closed)
		return;

	TessellatePath(path,shape);
}

void FromShaperecordListToPaths(SHAPERECORD* cur, std::vector<Path>& paths);

void DefineShapeTag::Render(int layer)
{

	std::cout << "Render Shape" << std::endl;
	std::vector < Path > paths;
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(Shapes.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		//TODO: Shape construtor from path
		shapes.push_back(Shape());
		FromPathToShape(*i,shapes.back());

		//Fill graphic data
		if(i->state->validFill0)
		{
			if(i->state->fill0)
			{
				shapes.back().graphic.filled0=true;
				shapes.back().graphic.color0=Shapes.FillStyles.FillStyles[i->state->fill0-1].Color;
				std::cout << shapes.back().graphic.color0 << std::endl;
			}
			else
				shapes.back().graphic.filled0=false;
		}
		else
			shapes.back().graphic.filled0=false;


		if(i->state->validFill1)
		{
			if(i->state->fill1)
			{
				shapes.back().graphic.filled1=true;
				shapes.back().graphic.color1=Shapes.FillStyles.FillStyles[i->state->fill1-1].Color;
				std::cout << shapes.back().graphic.color1 << std::endl;
			}
			else
				shapes.back().graphic.filled1=false;
		}
		else
			shapes.back().graphic.filled1=false;

		if(i->state->validStroke)
		{
			if(i->state->stroke)
			{
				shapes.back().graphic.stroked=true;
				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
				std::cout << shapes.back().graphic.stroke_color << std::endl;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		else
			shapes.back().graphic.stroked=false;
	}
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
//		if(it->filled)
			it->Render(layer);
	}
	std::cout << "fine Render Shape" << std::endl;
}

void DefineShape2Tag::Render(int layer)
{
	std::cout << "Render Shape2" << std::endl;
	std::vector < Path > paths;
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(Shapes.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		//TODO: Shape construtor from path
		shapes.push_back(Shape());
		FromPathToShape(*i,shapes.back());

		//Fill graphic data
		if(i->state->validFill0)
		{
			if(i->state->fill0)
			{
				shapes.back().graphic.filled0=true;
				shapes.back().graphic.color0=Shapes.FillStyles.FillStyles[i->state->fill0-1].Color;
				std::cout << shapes.back().graphic.color0 << std::endl;
			}
			else
				shapes.back().graphic.filled0=false;
		}
		else
			shapes.back().graphic.filled0=false;


		if(i->state->validFill1)
		{
			if(i->state->fill1)
			{
				shapes.back().graphic.filled1=true;
				shapes.back().graphic.color1=Shapes.FillStyles.FillStyles[i->state->fill1-1].Color;
				std::cout << shapes.back().graphic.color1 << std::endl;
			}
			else
				shapes.back().graphic.filled1=false;
		}
		else
			shapes.back().graphic.filled1=false;

		if(i->state->validStroke)
		{
			if(i->state->stroke)
			{
				shapes.back().graphic.stroked=true;
				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
				std::cout << shapes.back().graphic.stroke_color << std::endl;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		else
			shapes.back().graphic.stroked=false;
	}
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
//		if(it->filled)
			it->Render(layer);
	}
	std::cout << "fine Render Shape2" << std::endl;
}

void SplitPath(std::vector<Path>& paths,int a, int b)
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
		throw "cazzo fai";

	std::vector<Vector2>::iterator first,end;
	Path* cur_path;
	for(int i=0;i<paths.size();i++)
	{
		first=find(paths[i].points.begin(),paths[i].points.end(),a);
		end=find(paths[i].points.begin(),paths[i].points.end(),b);

		if(first!=paths[i].points.end() && end!=paths[i].points.end())
		{
			p._state=paths[i]._state;
			cur_path=&paths[i];
			break;
		}
	}

	p.points.insert(p.points.begin(),first,end+1);
	
	cur_path->points.erase(first+1,end);
	paths.push_back(p);
}

bool pointInPolygon(FilterIterator start, FilterIterator end, const Vector2& point)
{
	FilterIterator jj=start;
	FilterIterator jj2=start;
	jj2++;

	int count=0;
	int dist;

	while(jj2!=end)
	{
		Edge e(*jj,*jj2,-1);
		if(e.yIntersect(point.y,dist))
		{
			if(dist-point.x>0)
			{
				count++;
				//std::cout << "Impact edge (" << jj << ',' << jj+1 <<')'<< std::endl;
			}
		}
		jj=jj2++;
	}
	Edge e(*start,*jj,-1);
	if(e.yIntersect(point.y,dist))
	{
		if(dist-point.x>0)
			count++;
	}
	//std::cout << "count " << count << std::endl;
	return count%2;
}

void FromShaperecordListToPaths(SHAPERECORD* cur, std::vector<Path>& paths)
{
	int vindex=0;
	int startX=0,startY=0;
	GraphicStatus* cur_state=NULL;
	while(cur)
	{
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				startX+=cur->DeltaX;
				startY+=cur->DeltaY;
				if(Vector2(startX,startY,vindex)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY,vindex));
				vindex++;
			}
			else
			{
				startX+=cur->ControlDeltaX;
				startY+=cur->ControlDeltaY;
				if(Vector2(startX,startY,vindex)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY,vindex));
				vindex++;

				startX+=cur->AnchorDeltaX;
				startY+=cur->AnchorDeltaY;
				if(Vector2(startX,startY,vindex)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY,vindex));
				vindex++;
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				if(paths.size())
				{
					//std::cout << "2 path" << std::endl;
					GraphicStatus* old_status=paths.back().state;
					paths.push_back(Path());
					cur_state=paths.back().state;
					*cur_state=*old_status;

				}
				else
				{
					//std::cout << "1 path" << std::endl;
					paths.push_back(Path());
					cur_state=paths.back().state;
				}
				startX=cur->MoveDeltaX;
				startY=cur->MoveDeltaY;
				vindex=0;
				paths.back().points.push_back(Vector2(startX,startY,vindex));
				vindex=1;
			}
			if(cur->StateLineStyle)
			{
				cur_state->validStroke=true;
				cur_state->stroke=cur->LineStyle;
				//std::cout << "line style" << std::endl;
			}
			if(cur->StateFillStyle1)
			{
				cur_state->validFill1=true;
				cur_state->fill1=cur->FillStyle1;
				//std::cout << "fill style1" << std::endl;
			}
			if(cur->StateFillStyle0)
			{
				//if(cur->FillStyle0!=1)
				//	throw "fill style0";
				cur_state->validFill0=true;
				cur_state->fill0=cur->FillStyle0;
				
				//std::cout << "fill style0" << std::endl;
			}
		}
		cur=cur->next;
	}
}

void TriangulateMonotone(const std::vector<Vector2>& monotone, Shape& shape);

void TessellatePath(Path& path, Shape& shape)
{
	std::vector<Vector2> unsorted(path.points);
	int size=unsorted.size();

	fixIndex(unsorted);
	std::vector<Vector2> sorted(unsorted);
	sort(sorted.begin(),sorted.end());
	int primo=sorted[size-1].index;

	int prod=crossProd(unsorted[primo]-unsorted[(primo-1+size)%size],unsorted[(primo+1)%size]-unsorted[(primo-1+size)%size]);
//	std::cout << "prod2 " << prod << std::endl;

	int area=0;
	int i;
	for(i=0; i<size-1;i++)
	{
		area+=(unsorted[i].y+unsorted[i+1].y)*(unsorted[i+1].x-unsorted[i].x)/2;
	}
	area+=(unsorted[i].y+unsorted[0].y)*(unsorted[0].x-unsorted[i].x)/2;
//	std::cout << "area " << area << std::endl;

	if(prod>0)
	{
		if(area>=0)
			throw "area>0";
		//std::cout << "reversing" << std::endl;
		int size2=size-1;
		for(int i=0;i<size;i++)
			sorted[i].index=size2-sorted[i].index;
		reverse(unsorted.begin(),unsorted.end());
		fixIndex(unsorted);
		shape.winding=1;
	}
	else
	{
		if(area<=0)
			throw "area<0";
		shape.winding=0;
	}
	std::list<Edge> T;
	std::vector<Numeric_Edge> D;
	std::vector<int> helper(sorted.size(),-1);
	for(int j=size-1;j>=0;j--)
	{
		Vector2* v=&(sorted[j]);
		if(v->index==28)
			char a=0;
		//std::cout << *v;
		VTYPE type=getVertexType(*v,unsorted);
		//std::cout << "Type: " << type << std::endl;
		switch(type)
		{
			case START_VERTEX:
				{
					T.push_back(Edge(*v,unsorted[(v->index-1+unsorted.size())%unsorted.size()],v->index));
					//std::cout << "add edge " << v->index << std::endl;
					helper[v->index]=v->index;
					break;
				}
			case END_VERTEX:
				{
					if(getVertexType(unsorted[helper[(v->index+1)%size]],unsorted)==MERGE_VERTEX)
						throw "merge1";
					//std::cout << "End helper " << helper[(v->index+1)%size] << "at index" << (v->index+1)%size<< std::endl;
					std::list<Edge>::iterator d=std::find(T.begin(),T.end(),(v->index+1)%size);
					T.erase(d);
					break;
				}
			case NORMAL_VERTEX:
				{
					//Edge f(unsorted[(v->index-1+size)%size],unsorted[(v->index+1)%size],-1);
					int32_t dist;
					//if(!f.yIntersect(v->y,dist))
					//	throw "not possible";
					//std::cout << "dist " << dist-v->x << std::endl;

					int count=0;
					int jj;
					for(jj=0;jj<unsorted.size()-1;jj++)
					{
					//	if(jj==v->index || jj+1==v->index)
					//		continue;
						Edge e(unsorted[jj],unsorted[jj+1],-1);
						if(e.yIntersect(v->y,dist,v->x))
						{
							if(dist-v->x>0)
							{
								count++;
								//std::cout << "Impact edge (" << jj << ',' << jj+1 <<')'<< std::endl;
							}
						}
					}
					Edge e(unsorted[jj],unsorted[0],-1);
					if(e.yIntersect(v->y,dist,v->x))
					{
						if(dist-v->x>0)
							count++;
					}
					//std::cout << "count " << count << std::endl;
					if(count%2)
					{
						//std::cout << "right" << std::endl;
						if(getVertexType(unsorted[helper[(v->index+1)%size]],unsorted)==MERGE_VERTEX)
						{
							//std::cout << "Diag4 " << v->index << ' ' << helper[(v->index+1)%size] << std::endl;
							D.push_back(Numeric_Edge(v->index,helper[(v->index+1)%size],size));
						}
						//std::cout << "remove edge " << (v->index+1)%size << std::endl;
						std::list<Edge>::iterator d=std::find(T.begin(),T.end(),(v->index+1)%size);
						T.erase(d);
						//std::cout << "add edge " << v->index << std::endl;
						T.push_back(Edge(*v,unsorted[(v->index-1+unsorted.size())%unsorted.size()],v->index));
						helper[v->index]=v->index;
					}
					else
					{
						//std::cout << "left" << std::endl;
						std::list<Edge>::iterator e=T.begin();
						int32_t dist=0x7fffffff;
						int dist_index=-1;
						for(e;e!=T.end();e++)
						{
							int32_t d2;
							if(e->yIntersect(v->y,d2))
							{
								d2=v->x-d2;
								if(d2>0 && d2<dist)
								{
									dist=d2;
									dist_index=e->index;
								}
							}
						}
						//std::cout << "left index: " << dist_index << std::endl;
						if(getVertexType(unsorted[helper[dist_index]],unsorted)==MERGE_VERTEX)
						{
							//std::cout << "Diag3 " << v->index << ' ' << helper[dist_index] << std::endl;
							D.push_back(Numeric_Edge(v->index,helper[dist_index],size));
						}
						helper[dist_index]=v->index;
					}

					break;
				}
			case MERGE_VERTEX:
				{
					if(getVertexType(unsorted[helper[(v->index+1)%size]],unsorted)==MERGE_VERTEX)
					{
						//std::cout << "Diag4 " << v->index << ' ' << helper[(v->index+1)%size] << std::endl;
						D.push_back(Numeric_Edge(v->index,helper[(v->index+1)%size],size));
					}
					std::list<Edge>::iterator e=std::find(T.begin(),T.end(),(v->index+1)%size);
					T.erase(e);

					e=T.begin();
					int32_t dist=0x7fffffff;
					int dist_index=-1;
					for(e;e!=T.end();e++)
					{
						int32_t d2;
						if(e->yIntersect(v->y,d2))
						{
							d2=v->x-d2;
							if(d2>0 && d2<dist)
							{
								dist=d2;
								dist_index=e->index;
							}
						}
					}
					//std::cout << "left index: " << dist_index << std::endl;
					if(getVertexType(unsorted[helper[dist_index]],unsorted)==MERGE_VERTEX)
					{
						//std::cout << "Diag2 " << v->index << ' ' << helper[dist_index] << std::endl;
						D.push_back(Numeric_Edge(v->index,helper[dist_index],size));
					}
					helper[dist_index]=v->index;
					//std::cout << "helper(" << dist_index << "):" << v->index << std::endl;
					break;
				}
			case SPLIT_VERTEX:
				{
					std::list<Edge>::iterator e=T.begin();

					uint32_t dist=0xffffffff;
					int dist_index=-1;
					for(e;e!=T.end();e++)
					{
						int32_t d2;
						if(e->yIntersect(v->y,d2))
						{
							d2=v->x-d2;
							if(d2>0 && d2<dist)
							{
								dist=d2;
								dist_index=e->index;
							}
						}
					}
					//std::cout << "left index: " << dist_index << std::endl;
					//std::cout << "Diag1 " << v->index << ' ' << helper[dist_index] << std::endl;
					D.push_back(Numeric_Edge(v->index,helper[dist_index],size));
					helper[dist_index]=v->index;
					helper[v->index]=v->index;
					T.push_back(Edge(*v,unsorted[(v->index-1+unsorted.size())%unsorted.size()],v->index));
					break;
				}
			default:
				throw "Unsupported type";
		}
	}
	sort(D.begin(),D.end());
	for(int j=0;j<D.size();j++)
		shape.edges.push_back(Edge(unsorted[D[j].a],unsorted[D[j].b],-1));
	for(int j=0;j<D.size();j++)
	{
		std::vector<Vector2> monotone;
		if(D[j].a<D[j].b)
		{
			std::vector< Vector2 >::iterator a=lower_bound(unsorted.begin(),unsorted.end(),D[j].a);
			std::vector< Vector2 >::iterator b=lower_bound(unsorted.begin(),unsorted.end(),D[j].b);
			std::vector< Vector2 > temp(a,b+1);
			monotone.swap(temp);
			unsorted.erase(a+1,b);
		}
		else
		{
			std::vector< Vector2 >::iterator a=lower_bound(unsorted.begin(),unsorted.end(),D[j].a);
			monotone.insert(monotone.end(),a,unsorted.end());
			unsorted.erase(a+1,unsorted.end());
			std::vector< Vector2 >::iterator b=lower_bound(unsorted.begin(),unsorted.end(),D[j].b);
			monotone.insert(monotone.end(),unsorted.begin(),b+1);
			unsorted.erase(unsorted.begin(),b);
		}
		fixIndex(monotone);
		if(monotone.size()==2)
			abort();

		TriangulateMonotone(monotone,shape);
	}
	fixIndex(unsorted);
	TriangulateMonotone(unsorted,shape);
}

void TriangulateMonotone(const std::vector<Vector2>& monotone, Shape& shape)
{
	std::vector<int> S;
	std::vector<Vector2> sorted(monotone);
	std::vector<Vector2> unsorted(monotone);
	int size=monotone.size();
	std::vector<Vector2>::const_iterator first=max_element(monotone.begin(),monotone.end());
	int cur_index=first->index;
	int cur_y=first->y;
	cur_index++;
	cur_index%=size;
	for(int i=0;i<size;i++)
		sorted[i].chain=1;
	while(1)
	{
		if(cur_y>=sorted[cur_index].y)
		{
			sorted[cur_index].chain=2;
			cur_y=sorted[cur_index].y;
			cur_index++;
			cur_index%=size;
		}
		else
			break;
	}
	sort(sorted.begin(),sorted.end());

	std::vector < Numeric_Edge > edges;

	std::vector < Edge > border;
	for(int i=0;i<size-1;i++)
	{
		border.push_back(Edge(unsorted[i],unsorted[i+1],-1));
	}
	border.push_back(Edge(unsorted[size-1],unsorted[0],-1));

	S.push_back(size-1);
	S.push_back(size-2);
	for(int i=size-3;i>0;i--)
	{
		if(sorted[i].chain!=sorted[S.back()].chain)
		{
			for(int j=1;j<S.size();j++)
			{
				//shape.edges.push_back(Edge(sorted[S[j]],sorted[i],-1));
				edges.push_back(Numeric_Edge(sorted[S[j]].index,sorted[i].index,size));
			}
			S.clear();
			S.push_back(i+1);
			S.push_back(i);
		}
		else
		{
			int last_vertex=S.back();
			S.pop_back();
			while(!S.empty())
			{
				//shape.edges.push_back(Edge(sorted[S[j]],sorted[i],-1));
				Edge e(sorted[S.back()],sorted[i],-1);
				bool stop=false;
				for(int k=0;k<border.size();k++)
				{
					if(border[k].edgeIntersect(e))
					{
						stop=true;
						break;
					}
				}
				if(stop)
					break;
				else
				{
					last_vertex=S.back();
					S.pop_back();
					edges.push_back(Numeric_Edge(sorted[last_vertex].index,sorted[i].index,size));
				}
			}
			S.push_back(last_vertex);
			S.push_back(i);
		}
	}
	for(int j=1;j<S.size()-1;j++)
	{
		//shape.edges.push_back(Edge(sorted[S[j]],sorted[0],-1));
		edges.push_back(Numeric_Edge(sorted[S[j]].index,sorted[0].index,size));
	}
	sort(edges.begin(),edges.end());

	std::vector< Vector2 > backup(unsorted);
	int third;
	for(int i=0;i<edges.size();i++)
	{
		std::vector< Vector2 >::iterator first=find(unsorted.begin(),unsorted.end(),edges[i].a);
		std::vector< Vector2 >::iterator second=find(unsorted.begin(),unsorted.end(),edges[i].b);

		std::vector < Vector2 >::iterator third;
		if(first!=unsorted.end()-1)
			third=first+1;
		else
			third=unsorted.begin();
			
		if(third==second)
			throw "Internal error while triangulating";
		shape.interior.push_back(Triangle(*first,*second,*third));
		unsorted.erase(third);
	}
	if(unsorted.size()!=3)
		throw "Internal error after triangulating";
	else
	{
		shape.interior.push_back(Triangle(unsorted[0],unsorted[1],unsorted[2]));
	}
}

void DefineFont2Tag::Render(int glyph,const RGBA& color,int layer)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	std::vector < Path > paths;
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		//TODO: Shape construtor from path
		shapes.push_back(Shape());
		FromPathToShape(*i,shapes.back());

		//Fill graphic data
		shapes.back().graphic.filled0=true;
		shapes.back().graphic.filled1=true;
		shapes.back().graphic.stroked=false;
		shapes.back().graphic.color0=color;
		shapes.back().graphic.color1=RGB(0,255,0);

		if(i->state->validFill0)
		{
			if(i->state->fill0!=1)
				throw "Wrong fill0";
		}

		if(i->state->validFill1)
		{
			throw "Wrong fill1";
		}

		if(i->state->validStroke)
		{
			if(i->state->stroke)
			{
				shapes.back().graphic.stroked=true;
				throw "Wrong stroke";
//				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
//				std::cout << shapes.back().graphic.stroke_color << std::endl;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		else
			shapes.back().graphic.stroked=false;
	}
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
//		if(it->filled)
			it->Render(layer);
	}
}

void DefineFontTag::Render(int glyph,const RGBA& color, int layer)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	std::vector < Path > paths;
	std::vector < Shape > shapes;
	SHAPERECORD* cur=&(shape.ShapeRecords);

	FromShaperecordListToPaths(cur,paths);
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		//TODO: Shape construtor from path
		shapes.push_back(Shape());
		FromPathToShape(*i,shapes.back());

		std::cout << "PUPPA" << std::endl;

		//Fill graphic data
		shapes.back().graphic.filled0=true;
		shapes.back().graphic.filled1=true;
		shapes.back().graphic.stroked=false;
		shapes.back().graphic.color0=color;
		shapes.back().graphic.color1=RGB(0,255,0);

		if(i->state->validFill0)
		{
			if(i->state->fill0!=1)
				throw "Wrong fill0";
		}

		if(i->state->validFill1)
		{
			throw "Wrong fill1";
		}

		if(i->state->validStroke)
		{
			if(i->state->stroke)
			{
				shapes.back().graphic.stroked=true;
				throw "Wrong stroke";
//				shapes.back().graphic.stroke_color=Shapes.LineStyles.LineStyles[i->state->stroke-1].Color;
//				std::cout << shapes.back().graphic.stroke_color << std::endl;
			}
			else
				shapes.back().graphic.stroked=false;
		}
		else
			shapes.back().graphic.stroked=false;
	}
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
		it->Render(layer);
	}
}

void DefineTextTag::Render(int layer)
{
	std::cout << "Render Text" << std::endl;
	//std::cout << TextMatrix << std::endl;
	glColor3f(0,0,0);
	std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
	int x=0,y=0;//1024;
	std::vector < GLYPHENTRY >::iterator it2;
	FontTag* font=NULL;
	int done=0;
	int cur_height;
	for(it;it!=TextRecords.end();it++)
	{
		std::cout << "Text height " << it->TextHeight << std::endl;
		if(it->StyleFlagsHasFont)
		{
			cur_height=it->TextHeight;
			std::list< RenderTag*>::iterator it3 = dictionary.begin();
			for(it3;it3!=dictionary.end();it3++)
			{
				if((*it3)->getId()==it->FontID)
				{
					//std::cout << "Looking for Font " << it->FontID << std::endl;
					break;
				}
			}
			font=dynamic_cast<FontTag*>(*it3);
			if(font==NULL)
				throw "danni";
		}
		it2 = it->GlyphEntries.begin();
		int x2=x,y2=y;
		x2+=(*it).XOffset;
		y2+=(*it).YOffset;
		for(it2;it2!=(it->GlyphEntries.end());it2++)
		{
			glPushMatrix();
			glTranslatef(x2,y2,0);
			float scale=cur_height;
			scale/=1024;
			glScalef(scale,scale,1);
			font->Render(it2->GlyphIndex, it->TextColor,layer);
			glPopMatrix();
			
			//std::cout << "Character " << it2->GlyphIndex << std::endl;
			x2+=it2->GlyphAdvance;
		}
	}
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout <<"ShowFrame" << std::endl;
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	std::cout << "PlaceObject2" << std::endl;
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
	if(PlaceFlagHasClipAction)
		throw "clip action";
	if(PlaceFlagHasName)
		throw "has name";
	if(PlaceFlagMove && PlaceFlagHasCharacter)
	{
		throw "unsupported has";
	}
	if(PlaceFlagHasCharacter)
	{
		in >> CharacterId;
		std::cout << "new character ID " << CharacterId << std::endl;
	}
	if(PlaceFlagHasMatrix)
		in >> Matrix;
	if(PlaceFlagHasColorTransform)
		in >> ColorTransform;
	if(PlaceFlagHasRatio)
	{
		in >> Ratio;
		std::cout << "Ratio " << Ratio << std::endl;
	}
	if(PlaceFlagHasClipDepth)
	{
		in >> ClipDepth;
		std::cout << "Clip Depth " << ClipDepth << std::endl;
	}
	if(PlaceFlagMove)
	{
		std::list < DisplayListTag*>::const_iterator it=displayList.begin();
		std::cout << "find depth " << Depth << std::endl;
		for(it;it!=displayList.end();it++)
		{
			if((*it)->getDepth()==Depth)
			{
				add_to_list=false;
				PlaceObject2Tag* it2=dynamic_cast<PlaceObject2Tag*>(*it);
				if(PlaceFlagHasMatrix)
					it2->Matrix=Matrix;
				if(PlaceFlagHasClipDepth)
					it2->ClipDepth=ClipDepth;
				if(PlaceFlagHasColorTransform)
					it2->ColorTransform=ColorTransform;
				break;
			}
		}
		if(it==displayList.end())
			throw "has move";
	}
}


void PlaceObject2Tag::Render()
{
	std::cout << "Render Place object 2 ChaID " << CharacterId <<  std::endl;

	//TODO: support clipping
	if(ClipDepth!=0)
		return;

	//std::cout << Matrix << std::endl;
	if(!PlaceFlagHasCharacter)
		throw "modify not supported";
	std::list< RenderTag* >::iterator it=dictionary.begin();
	for(it;it!=dictionary.end();it++)
	{
		//std::cout << "ID " << dynamic_cast<RenderTag*>(*it)->getId() << std::endl;
		if(dynamic_cast<RenderTag*>(*it)->getId()==CharacterId)
			break;
	}
	if(it==dictionary.end())
	{
		throw "Object does not exist";
	}
	if((*it)->getType()!=RENDER_TAG)
		throw "cazzo renderi";
	
	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glMultMatrixf(matrix);
	{
		dynamic_cast<RenderTag*>(*it)->Render(Depth);
	}
}

void SetBackgroundColorTag::execute()
{
	std::cout << "execute SetBG" <<  std::endl;
	glClearColor(BackgroundColor.Red/255.0F,BackgroundColor.Green/255.0F,BackgroundColor.Blue/255.0F,0);
}
