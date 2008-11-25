#include "gltags.h"

GLObject::GLObject()
{
	list=glGenLists(1);
}

GLObject::~GLObject()
{
	glDeleteLists(list,1);
}

void GLObject::start()
{
	glNewList(list,GL_COMPILE);
}

void GLObject::end()
{
	glEndList();
}
