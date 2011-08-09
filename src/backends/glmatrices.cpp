/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

//This file implements a few helpers that should be drop-in replacements for
//the Open GL coordinate matrix handling API. GLES 2.0 does not provide this
//API, so applications need to handle the coordinate transformations and keep
//the state themselves.
//
//The functions have the same signature as the original gl ones but with a ls
//prefix added to make their purpose more clear. The main difference from a
//usage point of view compared to the GL API is that the operations take effect
//- the projection of modelview matrix uniforms sent to the shader - only when
//explicitly calling setMatrixUniform.

#include <string.h>
#include <stack>
#include "glmatrices.h"
#include "../logger.h"

using namespace std;

GLfloat lsIdentityMatrix[16] = {
								1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1
								};

//The matrix stack used by push and pop
stack<GLfloat*> *lsglMatrixStack = new stack<GLfloat*>;

//The current matrix
GLfloat lsMVPMatrix[16];

void lsglLoadMatrixf(const GLfloat *m)
{
	memcpy(lsMVPMatrix, m, LSGL_MATRIX_SIZE);
}

void lsglLoadIdentity()
{
	lsglLoadMatrixf(lsIdentityMatrix);
}

void lsglPushMatrix()
{
	GLfloat *top = new GLfloat[16];
	memcpy(top, lsMVPMatrix, LSGL_MATRIX_SIZE);
	lsglMatrixStack->push(top);
}

void lsglPopMatrix()
{
	if (lsglMatrixStack->size() == 0)
	{
		LOG(LOG_ERROR, "Popping from empty stack!");
		::abort();
	}
	memcpy(lsMVPMatrix, lsglMatrixStack->top(), LSGL_MATRIX_SIZE);
	lsglMatrixStack->pop();
}

void lsglMultMatrixf(const GLfloat *m)
{
	GLfloat tmp[16];
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			GLfloat sum=0;
			for (int k=0;k<4;k++)
			{
				sum += lsMVPMatrix[i+k*4]*m[j*4+k];
			}
			tmp[i+j*4] = sum;
		}
	}
	memcpy(lsMVPMatrix, tmp, LSGL_MATRIX_SIZE);
}

void lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ)
{
	static GLfloat scale[16];

	memcpy(scale, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	scale[0] = scaleX;
	scale[5] = scaleY;
	scale[10] = scaleZ;
	lsglMultMatrixf(scale);
}

void lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ)
{
	static GLfloat trans[16];

	memcpy(trans, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	trans[12] = translateX;
	trans[13] = translateY;
	trans[14] = translateZ;
	lsglMultMatrixf(trans);
}

void lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	GLfloat ortho[16];
	memset(ortho, 0, sizeof(ortho));
	ortho[0] = 2/(r-l);
	ortho[5] = 2/(t-b);
	ortho[10] = 2/(n-f);
	ortho[12] = -(r+l)/(r-l);
	ortho[13] = -(t+b)/(t-b);
	ortho[14] = -(f+n)/(f-n);
	ortho[15] = 1;

	lsglMultMatrixf(ortho);
}

