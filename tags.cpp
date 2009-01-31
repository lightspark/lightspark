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
	in >> ShapeId >> ShapeBounds >> Shapes;
}

void DefineSpriteTag::Render()
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
	bool validFill;
	bool validStroke;

	GraphicStatus():validFill(false),validStroke(false){}
};

enum VTYPE {START_VERTEX=0,END_VERTEX,NORMAL_VERTEX,SPLIT_VERTEX,MERGE_VERTEX};

class Path
{
public:
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

bool pointInPolygon(const std::vector<Vector2>& poly, const Vector2& point);

VTYPE getVertexType(const Vector2& v,const std::vector<Vector2>& points)
{
	//std::cout << "get vertex type on " << v << std::endl;
	int a=(v.index+1)%points.size();
	int b=(v.index-1+points.size())%points.size();

	Vector2 center=points[a]+points[b]+v;

	std::vector<Vector2> new_points(points);

	new_points.erase(find(new_points.begin(),new_points.end(),v));
	fixIndex(new_points);

	if(points[a].y <= v.y &&
		points[b].y <= v.y)
	{
		if(!pointInPolygon(new_points,v))
			return START_VERTEX;
		else
			return SPLIT_VERTEX;
	}
/*	if(points[a].y <= v.y &&
		points[b].y <= v.y)
	{
		if(pointInPolygon(points,center))
			return START_VERTEX;
		else
			return SPLIT_VERTEX;
	}*/
	else if(points[a].y >= v.y &&
		points[b].y <= v.y)
	{
		return NORMAL_VERTEX;
	}
	else if(points[a].y <= v.y &&
		points[b].y >= v.y)
	{
		return NORMAL_VERTEX;
	}
	else if(points[a].y >= v.y &&
		points[b].y >= v.y)
	{
		if(!pointInPolygon(new_points,v))
			return END_VERTEX;
		else
			return MERGE_VERTEX;
	}
/*	else if(points[a].y >= v.y &&
		points[b].y >= v.y)
	{
		if(pointInPolygon(points,center))
		{
			return END_VERTEX;
		}
		else
		{
			return MERGE_VERTEX;
		}
	}*/
	else
	{
		throw "unknown type";
	}
}

std::ostream& operator<<(std::ostream& s, const GraphicStatus& p)
{
	std::cout << "ValidFill "<< p.validFill << std::endl;
	std::cout << "ValidStroke "<< p.validStroke << std::endl;
}

void TessellateShaperecordList(SHAPERECORD* cur, std::vector< Shape >& shapes);

void DefineShapeTag::Render()
{
	std::cout << "Render Shape" << std::endl;
	int startX=0,startY=0;
	SHAPERECORD* cur=&(Shapes.ShapeRecords);
	std::vector< Shape > shapes;
	TessellateShaperecordList(cur,shapes);
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
		it->Render();
		//std::cout << it->_state << std::endl;
	}
	std::cout << "fine Render Shape" << std::endl;
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

bool pointInPolygon(const std::vector<Vector2>& poly, const Vector2& point)
{
	int count=0;
	int jj;
	int dist;
	for(jj=0;jj<poly.size()-1;jj++)
	{
		Edge e(poly[jj],poly[jj+1],-1);
		if(e.yIntersect(point.y,dist))
		{
			if(dist-point.x>0)
			{
				count++;
				//std::cout << "Impact edge (" << jj << ',' << jj+1 <<')'<< std::endl;
			}
		}
	}
	Edge e(poly[jj],poly[0],-1);
	if(e.yIntersect(point.y,dist))
	{
		if(dist-point.x>0)
			count++;
	}
	//std::cout << "count " << count << std::endl;
	return count%2;
}

void TriangulateMonotone(const std::vector<Vector2>& monotone, Shape& shape);

void TessellateShaperecordList(SHAPERECORD* cur, std::vector<Shape>& shapes)
{
	int startX=0,startY=0;
	GraphicStatus* cur_state=NULL;
	std::vector<Path> paths;

	int vindex=0;
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
				/*				glLineWidth(Shapes.LineStyles.LineStyles[cur->LineStyle].Width);
								glColor3ub(Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Red,
								Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Green,
								Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Blue);
								*/
				if(cur->LineStyle)
				{
					cur_state->validStroke=true;
					//std::cout << "line style" << std::endl;
				}
				else
					cur_state->validStroke=false;
			}
			if(cur->StateFillStyle1)
			{
				/*		glColor4ub(Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Red,
						Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Green,
						Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Blue);*/
				//std::cout << "fill style1" << std::endl;
			}
			if(cur->StateFillStyle0)
			{
				if(cur->FillStyle0!=1)
					throw "fill style0";
				cur_state->validFill=true;
				//std::cout << "fill style0" << std::endl;
			}
		}
		cur=cur->next;
	}
	std::vector < Path >::iterator i=paths.begin();
	for(i;i!=paths.end();i++)
	{
		shapes.push_back(Shape());
		shapes.back().outline=i->points;
		shapes.back().closed=i->closed;
		if(!i->closed)
			continue;

			//Debug
			std::vector<Vector2> debug;
			debug.push_back(Vector2(200,-400));
			debug.push_back(Vector2(0,-200));
			debug.push_back(Vector2(200,0));
			debug.push_back(Vector2(400,-200));
/*			debug.push_back(Vector2(-400,-400));
			debug.push_back(Vector2(0,-400));
			debug.push_back(Vector2(0,0));
			debug.push_back(Vector2(-400,0));*/
			//Debug
		std::vector<Vector2> unsorted(i->points);
//		std::vector<Vector2> unsorted(debug);
		int size=unsorted.size();

		fixIndex(unsorted);
		//Parity break
		for(int j=0;j<size;j++)
		{
			if(unsorted[j].y==unsorted[(j+1)%unsorted.size()].y)
			{
				if(j==0)
					unsorted[(j+1)%unsorted.size()].y+=2;
				else
					unsorted[(j+1)%unsorted.size()].y++;
			}
		}

		std::vector<Vector2> sorted(unsorted);
		sort(sorted.begin(),sorted.end());
		int primo=sorted[size-1].index;

		int prod=crossProd(unsorted[primo]-unsorted[(primo-1+size)%size],unsorted[(primo+1)%size]-unsorted[(primo-1+size)%size]);
		//std::cout << "prod2 " << prod << std::endl;
		if(prod>0)
		{
			//std::cout << "reversing" << std::endl;
			int size2=size-1;
			for(int i=0;i<size;i++)
				sorted[i].index=size2-sorted[i].index;
			reverse(unsorted.begin(),unsorted.end());
			fixIndex(unsorted);
		}
		std::list<Edge> T;
		std::vector<Numeric_Edge> D;
		std::vector<int> helper(sorted.size(),-1);
		for(int j=size-1;j>=0;j--)
		{
			Vector2* v=&(sorted[j]);
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
							if(jj==v->index || jj+1==v->index)
								continue;
							Edge e(unsorted[jj],unsorted[jj+1],-1);
							if(e.yIntersect(v->y,dist))
							{
								if(dist-v->x>0)
								{
									count++;
									//std::cout << "Impact edge (" << jj << ',' << jj+1 <<')'<< std::endl;
								}
							}
						}
						Edge e(unsorted[jj],unsorted[0],-1);
						if(e.yIntersect(v->y,dist))
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
			shapes.back().edges.push_back(Edge(unsorted[D[j].a],unsorted[D[j].b],-1));
		for(int j=0;j<D.size();j++)
		{
			std::vector<Vector2> monotone;
			if(D[j].a<D[j].b)
			{
				std::vector< Vector2 >::iterator a=find(unsorted.begin(),unsorted.end(),D[j].a);
				std::vector< Vector2 >::iterator b=find(unsorted.begin(),unsorted.end(),D[j].b);
				std::vector< Vector2 > temp(a,b+1);
				monotone.swap(temp);
				unsorted.erase(a+1,b);
			}
			else
			{
				std::vector< Vector2 >::iterator a=find(unsorted.begin(),unsorted.end(),D[j].a);
				monotone.insert(monotone.end(),a,unsorted.end());
				unsorted.erase(a+1,unsorted.end());
				std::vector< Vector2 >::iterator b=find(unsorted.begin(),unsorted.end(),D[j].b);
				monotone.insert(monotone.end(),unsorted.begin(),b+1);
				unsorted.erase(unsorted.begin(),b);
			}
			fixIndex(monotone);

			TriangulateMonotone(monotone,shapes.back());
		}
	}
}

void TriangulateMonotone(const std::vector<Vector2>& monotone, Shape& shape)
{
	std::vector<int> S;
	std::vector<Vector2> sorted(monotone);
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
		if(cur_y>sorted[cur_index].y)
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

	S.push_back(size-1);
	S.push_back(size-2);
	for(int i=size-3;i>0;i--)
	{
		if(sorted[i].chain!=sorted[S.back()].chain)
		{
			for(int j=1;j<S.size();j++)
			{
				shape.edges.push_back(Edge(sorted[S[j]],sorted[i],-1));
			}
			S.clear();
			S.push_back(sorted[i+1].index);
			S.push_back(sorted[i].index);
		}
		else
		{
			S.pop_back();
			for(int j=0;j<S.size();j++)
			{
				shape.edges.push_back(Edge(sorted[S[j]],sorted[i],-1));
			}
			S.erase(S.begin()+1,S.end());
			S.push_back(sorted[i].index);
		}
	}
	for(int j=1;j<S.size()-1;j++)
	{
		shape.edges.push_back(Edge(sorted[S[j]],sorted[0],-1));
	}


}

void DefineFont2Tag::Render(int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);
	std::vector< Shape > shapes;
	TessellateShaperecordList(cur,shapes);
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
		//std::cout << "path len " << it->outline.size()<< std::endl;
		//std::vector < Vector2 >::iterator it2=(*it).outline.begin();
		/*				if((*it).state->validFill && (*it).closed)
						{
						glBegin(GL_TRIANGLE_FAN);
						for(it2;it2!=(*it).points.end();it2++)
						glVertex2i((*it2).x,(*it2).y);
						glEnd();
						}*/
		it->Render();
		//std::cout << it->_state << std::endl;
	}
}

void DefineFontTag::Render(int glyph)
{
	SHAPE& shape=GlyphShapeTable[glyph];
	SHAPERECORD* cur=&(shape.ShapeRecords);
	std::vector< Shape > shapes;
	TessellateShaperecordList(cur,shapes);
	std::vector < Shape >::iterator it=shapes.begin();
	for(it;it!=shapes.end();it++)
	{
		//std::cout << "path len " << it->outline.size()<< std::endl;
		std::vector < Vector2 >::iterator it2=(*it).outline.begin();
		/*				if((*it).state->validFill && (*it).closed)
						{
						glBegin(GL_TRIANGLE_FAN);
						for(it2;it2!=(*it).points.end();it2++)
						glVertex2i((*it2).x,(*it2).y);
						glEnd();
						}*/
		it->Render();
		//std::cout << it->_state << std::endl;
	}
}

void DefineTextTag::Render()
{
	std::cout << "Render Text" << std::endl;
	std::cout << TextMatrix << std::endl;
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
			font->Render(it2->GlyphIndex);
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
		in >> ClipDepth;
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
	std::cout << Matrix << std::endl;
	if(!PlaceFlagHasCharacter)
		throw "modify not supported";
	std::list< RenderTag* >::iterator it=dictionary.begin();
	for(it;it!=dictionary.end();it++)
	{
		std::cout << "ID " << dynamic_cast<RenderTag*>(*it)->getId() << std::endl;
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
		dynamic_cast<RenderTag*>(*it)->Render();
	}
}

void SetBackgroundColorTag::execute()
{
	std::cout << "execute SetBG" <<  std::endl;
	glClearColor(BackgroundColor.Red/255.0F,BackgroundColor.Green/255.0F,BackgroundColor.Blue/255.0F,0);
}
