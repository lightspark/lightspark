#ifndef GLTAGS_H
#define GLTAGS_H

#include <GL/gl.h>

class GLObject
{
private:
public:
	GLuint list;
	GLObject();
	~GLObject();
	void start();
	void end();
};

#endif
