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
#include <fstream>
#include <math.h>
#include <algorithm>
#include <GL/gl.h>
#include "swftypes.h"
#include "logger.h"
#include "geometry.h"

using namespace std;

float colors[][3] = { { 0 ,0 ,0 },
			{1, 0, 0},
			{0, 1, 0},
			{0 ,0, 1},
			{1 ,1, 0},
			{1,0 ,1},
			{0,1,1},
			{0,0,0}};

void Shape::Render(int i) const
{
	if(unsupported)
	{
		LOG(NOT_IMPLEMENTED,"Unsupported shape kind");
		return;
	}

	if(edges.empty())
	{
		LOG(TRACE,"No edges in this shape");
		return;
	}

/*	if(color0!=0 && color0!=1)
		LOG(ERROR,"color0 "<<color0);
	if(color1!=0 && color1!=1)
		LOG(ERROR,"color1"<<color0);*/

	glUseProgram(0);
	bool filled=false;
	if(closed && winding==0)
	{
		LOG(TRACE,"Filling 0");
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFunc(GL_ALWAYS,color,0xff);
		glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		std::vector<Triangle>::const_iterator it2=interior.begin();
		glBegin(GL_TRIANGLES);
		for(it2;it2!=interior.end();it2++)
		{
			glVertex2i(it2->v1.x,it2->v1.y);
			glVertex2i(it2->v2.x,it2->v2.y);
			glVertex2i(it2->v3.x,it2->v3.y);
		}
		glEnd();
		if(color!=0)
			filled=true;
	}
/*	else if(closed && winding==1)
	{
		LOG(TRACE,"Filling 1");
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFunc(GL_ALWAYS,color1,0xff);
		glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		std::vector<Triangle>::const_iterator it2=interior.begin();
		glBegin(GL_TRIANGLES);
		for(it2;it2!=interior.end();it2++)
		{
			glVertex2i(it2->v1.x,it2->v1.y);
			glVertex2i(it2->v2.x,it2->v2.y);
			glVertex2i(it2->v3.x,it2->v3.y);
		}
		glEnd();
		if(color1!=0)
			filled=true;
	}*/


//	if(graphic.stroked || !filled)
	{
		LOG(TRACE,"Line tracing");
		glDisable(GL_STENCIL_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilFunc(GL_NEVER,0,0);
		std::vector<Vector2>::const_iterator it=outline.begin();
		//glColor3ub(graphic.stroke_color.Red,graphic.stroke_color.Green,graphic.stroke_color.Blue);
		glColor3f(colors[i%8][0],colors[i%8][1],colors[i%8][2]);
		if(closed)
			glBegin(GL_LINE_LOOP);
		else
			glBegin(GL_LINE_STRIP);
		for(it;it!=outline.end();it++)
		{
			glVertex2i(it->x,it->y);
		}
		glEnd();
		glEnable(GL_STENCIL_TEST);
	}

	for(int i=0;i<sub_shapes.size();i++)
		sub_shapes[i].Render();
}

bool Edge::yIntersect(const Vector2& p, int& x)
{
	if(p1.y!=p2.y && p.y==highPoint().y)
	{
		x=highPoint().x;
		return true;
	}
	if(p.y<=highPoint().y || p.y>=lowPoint().y)
		return false;
	else
	{
		Vector2 c=lowPoint()-highPoint();
		Vector2 d=p-highPoint();

		x=(c.x*d.y)/c.y+highPoint().x;
		return true;
	}
}

FilterIterator::FilterIterator(const std::vector<Vector2>::const_iterator& i, const std::vector<Vector2>::const_iterator& e, int f):it(i),end(e),filter(f)
{
	if(it!=end && it->index==filter)
		it++;
}
FilterIterator FilterIterator::operator++(int)
{
	FilterIterator copy(*this);
	it++;
	if(it!=end && it->index==filter)
		it++;
	return copy;
}

const Vector2& FilterIterator::operator*()
{
	return *it;
}

bool FilterIterator::operator==(FilterIterator& i)
{
	return i.it==it;
}

bool FilterIterator::operator!=(FilterIterator& i)
{
	return i.it!=it;
}

void Shape::dumpEdges()
{
	ofstream f("edges.dat");

	for(int i=0;i<edges.size();i++)
		f << edges[i].p1.x << ' ' << edges[i].p1.y << endl;
/*			for(int k=0;k<cached[j].sub_shapes.size();k++)
	{
		for(int i=0;i<cached[j].sub_shapes[k].edges.size();i++)
			f << cached[j].sub_shapes[k].edges[i].p1.x << ' ' << cached[j].sub_shapes[k].edges[i].p1.y << endl;
	}*/
	f.close();
	ofstream g("interior.dat");

	for(int i=0;i<interior.size();i++)
	{
		g << interior[i].v1.x << ' ' << interior[i].v1.y << endl;
		g << interior[i].v2.x << ' ' << interior[i].v2.y << endl;
		g << interior[i].v3.x << ' ' << interior[i].v3.y << endl;
		g << interior[i].v1.x << ' ' << interior[i].v1.y << endl;
		g << endl;
	}

	g.close();
}

void Shape::dumpInterior()
{
	ofstream f("interior.dat");

	for(int i=0;i<interior.size();i++)
	{
		f << interior[i].v1.x << ' ' << interior[i].v1.y << endl;
		f << interior[i].v2.x << ' ' << interior[i].v2.y << endl;
		f << interior[i].v3.x << ' ' << interior[i].v3.y << endl;
		f << interior[i].v1.x << ' ' << interior[i].v1.y << endl;
		f << endl;
	}

	f.close();
}

void Shape::SetStyles(FILLSTYLE* styles)
{
	outline.reserve(edges.size());
	for(int i=0;i<edges.size();i++)
		outline.push_back(edges[i].p1);

	if(edges.back().p2==outline.front())
		closed=true;
	else
		closed=false;

	if(styles)
	{
		if(color)
			style=&styles[color-1];
		else
			style=NULL;

	}

/*	if(!coerent)
	{
		LOG(ERROR,"Not coerent shape");
		closed=false;
		unsupported=true;
		return;
	}*/

	//Calculate shape winding
	long area=0;
	int i;
	for(i=0; i<outline.size()-1;i++)
	{
		area+=(outline[i].y+outline[i+1].y)*(outline[i+1].x-outline[i].x)/2;
	}
	area+=(outline[i].y+outline[0].y)*(outline[0].x-outline[i].x)/2;

	if(area<0)
		winding=1;
	else
		winding=0;
}

void Shape::BuildFromEdges(FILLSTYLE* styles, bool normalize)
{
	style=NULL;
	unsupported=false;
	if(edges.empty())
		return;

	//We try to build coerent shapes out of a possible incoerent one
	/*int cur_fill0=edges[0].color0;
	int cur_fill1=edges[0].color1;
	vector<Edge>::iterator it_start=edges.begin();
	vector<Edge>::iterator it_cur=edges.begin();
	do
	{
		if(it_cur->color0!=cur_fill0 || it_cur->color1!=cur_fill1)
		{
			//As the style is changed we create a new Shape;
			sub_shapes.push_back(Shape());
			sub_shapes.back().edges=vector<Edge>(it_start,it_cur);
			sub_shapes.back().color=cur_fill0;
			sub_shapes.back().graphic.stroked=false;
			sub_shapes.back().SetStyles(styles);
			if(sub_shapes.back().closed)
				sub_shapes.back().TessellateSimple();

			edges.erase(it_start,it_cur);
			it_start=edges.begin();
			it_cur=edges.begin();
			cur_fill0=it_cur->color0;
			cur_fill1=it_cur->color1;
		}
		it_cur++;
	}
	while(it_cur!=edges.end());*/

	//color=cur_fill0;

	SetStyles(styles);

	graphic.stroked=false;
	//Tessellate the shape using ear removing algorithm
	//if(closed)
	//	TessellateSimple();
}

bool pointInPolygon(FilterIterator start, FilterIterator end, const Vector2& point)
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
		Edge e(*jj,*jj2,-1,-1);
		if(e.yIntersect(point,dist))
		{
			if(dist>point.x)
				count++;
		}
		jj=jj2++;
	}
	Edge e(*jj,*start,-1,-1);
	if(e.yIntersect(point,dist))
	{
		if(dist>point.x)
			count++;
	}
	return count%2;
}

void fixIndex(std::vector<Vector2>& points)
{
	int size=points.size();
	for(int i=0;i<size;i++)
		points[i].index=i;
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

	return (u >= 0) && (v>= 0) && (u + v <= 1);
}

void Shape::TessellateSimple()
{
	vector<Vector2> P=outline;
	int i=0;
	int count=0;
	while(P.size()>3)
	{
		fixIndex(P);
		int a=(i+1)%P.size();
		int b=(i-1+P.size())%P.size();
		FilterIterator ai(P.begin(),P.end(),i);
		FilterIterator bi(P.end(),P.end(),i);
		bool not_ear=false;

		//Collinearity test
		Vector2 t1=P[b]-P[a];
		Vector2 t2=P[i]-P[a];
		if(t2.y*t1.x==t2.x*t1.y)
		{
			i++;
			i%=P.size();
			continue;
		}

		if(!pointInPolygon(ai,bi,P[i]))
		{
			for(int j=0;j<P.size();j++)
			{
				if(j==i || j==a || j==b)
					continue;
				if(pointInTriangle(P[j],P[a],P[i],P[b]))
				{
					not_ear=true;
					break;
				}
			}
			if(!not_ear)
			{
				interior.push_back(Triangle(P[a],P[i],P[b]));
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

		//Force loop ending after a way too long time.
		if(count>30000)
			break;
	}
	interior.push_back(Triangle(P[0],P[1],P[2]));
}

//Shape are compared using the minimum vertex
bool Shape::operator<(const Shape& r) const
{
	Vector2 vl=*min_element(outline.begin(),outline.end());
	Vector2 vr=*min_element(r.outline.begin(),r.outline.end());

	if(vl<vr)
		return true;
	else
		return false;
}
