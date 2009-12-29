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

#include <fstream>
#include <math.h>
#include <algorithm>
#include <GL/glew.h>
#include "swftypes.h"
#include "logger.h"
#include "geometry.h"

using namespace std;
using namespace lightspark;

/*! \brief Renders the shape interior and outline setting the correct
* *        parameters for the shader
* * \param x Optional x translation
* * \param y Optional y translation */

void GeomShape::Render(int x, int y) const
{
	if(outline.empty())
	{
		LOG(LOG_TRACE,"No edges in this shape");
		return;
	}

	bool filled=false;
	if(closed)
	{
		style->setFragmentProgram();

		for(int i=0;i<triangle_strips.size();i++)
		{
			glBegin(GL_TRIANGLE_STRIP);
			for(int j=0;j<triangle_strips[i].size();j++)
				glVertex2i(triangle_strips[i][j].x+x,triangle_strips[i][j].y+y);
			glEnd();
		}
		filled=true;

		/*char* t=(char*)varray;
		glVertexPointer(2,GL_INT,sizeof(arrayElem),t);
		glColorPointer(3,GL_FLOAT,sizeof(arrayElem),t+2*sizeof(GLint));
		glTexCoordPointer(4,GL_FLOAT,sizeof(arrayElem),t+2*sizeof(GLint)+3*sizeof(GLfloat));

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		int used=0;
		for(int i=0;i<triangle_strips.size();i++)
		{
			glDrawArrays(GL_TRIANGLE_STRIP,used,triangle_strips[i].size());
			used+=triangle_strips[i].size();
		}
		glDisableClientState(GL_VERTEX_ARRAY);*/
	}

	if(/*graphic.stroked ||*/ !filled)
	{
		//LOG(TRACE,"Line tracing");
		FILLSTYLE::fixedColor(0,0,0);
		std::vector<Vector2>::const_iterator it=outline.begin();
		if(closed)
			glBegin(GL_LINE_LOOP);
		else
			glBegin(GL_LINE_STRIP);
		for(it;it!=outline.end();it++)
			glVertex2i(it->x+x,it->y+y);
		glEnd();
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

void GeomShape::dumpEdges()
{
	ofstream f("edges.dat");

	for(int i=0;i<outline.size();i++)
		f << outline[i].x << ' ' << outline[i].y << endl;
	f.close();
}

void GeomShape::dumpInterior()
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

void GeomShape::SetStyles(FILLSTYLE* styles)
{
	static FILLSTYLE* clearStyle=NULL;
	if(!clearStyle)
	{
		clearStyle=new FILLSTYLE;
		clearStyle->FillStyleType=0x00;
		clearStyle->Color=RGBA(0,0,0,0);
	}

	if(styles)
	{
		if(color)
			style=&styles[color-1];
		else
			style=clearStyle;
	}
}

void GeomShape::BuildFromEdges(FILLSTYLE* styles)
{
	style=NULL;
	if(outline.empty())
		return;

	if(outline.front()==outline.back())
	{
		closed=true;
		outline.pop_back();
	}
	else
		closed=false;

	SetStyles(styles);

	//Tessellate the shape using ear removing algorithm
	if(closed)
		TessellateSimple();

	//Try to build triangle strip
	while(!interior.empty())
	{
		std::vector<Vector2> strip[3];
		std::vector<Triangle> triangles[3];

		triangles[0]=interior;
		triangles[1]=interior;
		triangles[2]=interior;

		int j[3]={2,2,2};

		strip[0].push_back(interior[0].v1);
		strip[0].push_back(interior[0].v2);
		strip[0].push_back(interior[0].v3);

		strip[1].push_back(interior[0].v3);
		strip[1].push_back(interior[0].v1);
		strip[1].push_back(interior[0].v2);

		strip[2].push_back(interior[0].v2);
		strip[2].push_back(interior[0].v3);
		strip[2].push_back(interior[0].v1);

		for(int k=0;k<3;k++)
		{
			triangles[k].erase(triangles[k].begin());
			int i=0;
			while(1)
			{
				if(i==triangles[k].size())
					break;

				if(strip[k][j[k]]==triangles[k][i].v1 && strip[k][j[k]-1]==triangles[k][i].v2)
				{
					strip[k].push_back(triangles[k][i].v3);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else if(strip[k][j[k]-1]==triangles[k][i].v1 && strip[k][j[k]]==triangles[k][i].v2)
				{
					strip[k].push_back(triangles[k][i].v3);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else if(strip[k][j[k]]==triangles[k][i].v2 && strip[k][j[k]-1]==triangles[k][i].v3)
				{
					strip[k].push_back(triangles[k][i].v1);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else if(strip[k][j[k]-1]==triangles[k][i].v2 && strip[k][j[k]]==triangles[k][i].v3)
				{
					strip[k].push_back(triangles[k][i].v1);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else if(strip[k][j[k]]==triangles[k][i].v3 && strip[k][j[k]-1]==triangles[k][i].v1)
				{
					strip[k].push_back(triangles[k][i].v2);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else if(strip[k][j[k]-1]==triangles[k][i].v3 && strip[k][j[k]]==triangles[k][i].v1)
				{
					strip[k].push_back(triangles[k][i].v2);
					triangles[k].erase(triangles[k].begin()+i);
					i=0;
					j[k]++;
				}
				else
					i++;
			}
		}

		if(strip[0].size()<=strip[2].size() && 
			strip[1].size()<=strip[2].size())
		{
			triangle_strips.push_back(strip[2]);
			interior=triangles[2];
		}
		else if(strip[0].size()<=strip[1].size() && 
			strip[2].size()<=strip[1].size())
		{
			triangle_strips.push_back(strip[1]);
			interior=triangles[1];
		}
		else if(strip[1].size()<=strip[0].size() && 
			strip[2].size()<=strip[0].size())
		{
			triangle_strips.push_back(strip[0]);
			interior=triangles[0];
		}
	}

/*	int strips_size=0;
	for(int i=0;i<triangle_strips.size();i++)
		strips_size+=triangle_strips[i].size();

	varray=new arrayElem[strips_size];

	int used=0;
	for(int i=0;i<triangle_strips.size();i++)
	{
		for(int j=0;j<triangle_strips[i].size();j++)
		{
			varray[used].coord[0]=triangle_strips[i][j].x;
			varray[used].coord[1]=triangle_strips[i][j].y;
			style->setVertexData(&varray[used]);
			used++;
		}
	}*/
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

void GeomShape::TessellateSimple()
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
bool GeomShape::operator<(const GeomShape& r) const
{
	Vector2 vl=*min_element(outline.begin(),outline.end());
	Vector2 vr=*min_element(r.outline.begin(),r.outline.end());

	if(vl<vr)
		return true;
	else
		return false;
}
