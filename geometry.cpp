#include "swftypes.h"
#include "geometry.h"

void Shape::Render() const
{
	//DEBUG
	/*		std::vector<Edge>::const_iterator it2=edges.begin();
	glBegin(GL_LINES);
	for(it2;it2!=edges.end();it2++)
	{
	glVertex2i(it2->x1,it2->y1);
	glVertex2i(it2->x2,it2->y2);
	}
	glEnd();*/
	if(graphic.filled0)
	{
		std::vector<Triangle>::const_iterator it2=interior.begin();
		glColor3ub(graphic.color0.Red,graphic.color0.Green,graphic.color0.Blue);
		glBegin(GL_TRIANGLES);
		for(it2;it2!=interior.end();it2++)
		{
			glVertex2i(it2->v1.x,it2->v1.y);
			glVertex2i(it2->v2.x,it2->v2.y);
			glVertex2i(it2->v3.x,it2->v3.y);
		}
		glEnd();
	}

	if(graphic.filled1)
	{
		std::cout << "BIG WARNING: fill1" << std::endl;
		std::vector<Triangle>::const_iterator it2=interior.begin();
		glColor3ub(graphic.color1.Red,graphic.color1.Green,graphic.color1.Blue);
		glBegin(GL_TRIANGLES);
		for(it2;it2!=interior.end();it2++)
		{
			glVertex2i(it2->v1.x,it2->v1.y);
			glVertex2i(it2->v2.x,it2->v2.y);
			glVertex2i(it2->v3.x,it2->v3.y);
		}
		glEnd();
	}

	if(graphic.stroked)
	{
		std::vector<Vector2>::const_iterator it=outline.begin();
		glColor3ub(graphic.stroke_color.Red,graphic.stroke_color.Green,graphic.stroke_color.Blue);
		if(closed)
			glBegin(GL_LINE_LOOP);
		else
			glBegin(GL_LINE_STRIP);
		for(it;it!=outline.end();it++)
		{
			glVertex2i(it->x,it->y);
		}
		glEnd();
	}
}

bool Edge::xIntersect(int x,int32_t& d)
{
	int u1,u2;
	u1=min(x1,x2);
	u2=max(x1,x2);
	if((x>=u1) && (x<=u2))
	{
		if(x2==x1)
			d=y1;
		else
		{
			float m=(y2-y1);
			m/=(x2-x1);
			d=m*(x-x1);
			d+=y1;
		}
		return true;
	}
	else
		return false;
}

bool Edge::yIntersect(int y,int32_t& d, int x)
{
	int u1,u2;
	u1=min(y1,y2);
	u2=max(y1,y2);

	if((y>u1) && (y<u2))
	{
		float m=(x2-x1);
		m/=(y2-y1);
		d=m*(y-y1);
		d+=x1;
		return true;
	}
	else if(y==y1 || y==y2)
	{
		if(y1==y2)
		{
			int v1,v2;
			v1=min(x1,x2);
			v2=max(x1,x2);
			if(!(x>=v1 && x<=v2))
			{
				return false;
			}
			if(x1==x)
				d=x2;
			else
				d=x1;
			return true;
		}
		else if(y==u1)
		{
			if(u1==y1)
				d=x1;
			else
				d=x2;
			return true;
		}
		else
		{
/*			if(u1==y1)
				d=x2;
			else
				d=x1;*/
			return false;
		}
	}
/*	else if(y==y1 && y==y2)
	{
		d=x1;
		return true;
	}*/
	else
	{
		return false;
	}
}
