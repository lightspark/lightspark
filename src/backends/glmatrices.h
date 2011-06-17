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

#ifndef GLMATRICES_H
#define GLMATRICES_H

#define LSGL_MATRIX_SIZE (16*sizeof(GLfloat))

extern GLfloat lsIdentityMatrix[16];
extern GLfloat lsMVPMatrix[16];
void lsglLoadMatrixf(const GLfloat *m);
void lsglLoadIdentity();
void lsglPushMatrix();
void lsglPopMatrix();
void lsglMultMatrixf(const GLfloat *m);
void lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
void lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ);
void lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);

#endif
