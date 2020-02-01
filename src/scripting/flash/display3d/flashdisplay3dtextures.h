/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2017 Ludger Krämer <dbluelle@onlinehome.de>

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
#ifndef FLASHDISPLAY3DTEXTURES_H
#define FLASHDISPLAY3DTEXTURES_H

#include "compat.h"

#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/display/BitmapContainer.h"

namespace lightspark
{

class TextureBase: public EventDispatcher
{
friend class Context3D;
protected:
	uint32_t textureID;
	uint32_t width;
	uint32_t height;
	vector<vector<uint8_t>> bitmaparray;
	bool needrefresh;
	bool hasalpha;
	Context3D* context;
	void parseAdobeTextureFormat(ByteArray* data, int32_t byteArrayOffset, bool forCubeTexture, bool &hasalpha);
public:
	TextureBase(Class_base* c):EventDispatcher(c)
	  ,textureID(UINT32_MAX),width(0),height(0),needrefresh(false),hasalpha(true),context(NULL)
	{ subtype = SUBTYPE_TEXTUREBASE;}
	TextureBase(Class_base* c,Context3D* _context):EventDispatcher(c)
	  ,textureID(UINT32_MAX),width(0),height(0),needrefresh(false),hasalpha(true),context(_context)
	{ subtype = SUBTYPE_TEXTUREBASE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
};

class Texture: public TextureBase
{
public:
	Texture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_TEXTURE; }
	Texture(Class_base* c,Context3D* _context):TextureBase(c,_context){ subtype = SUBTYPE_TEXTURE; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadCompressedTextureFromByteArray);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};

class CubeTexture: public TextureBase
{
	friend class Context3D;
protected:
	uint32_t max_miplevel;
public:
	CubeTexture(Class_base* c):TextureBase(c),max_miplevel(0) { subtype = SUBTYPE_CUBETEXTURE;}
	CubeTexture(Class_base* c,Context3D* _context):TextureBase(c,_context),max_miplevel(0) { subtype = SUBTYPE_CUBETEXTURE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadCompressedTextureFromByteArray);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};
class RectangleTexture: public TextureBase
{
public:
	RectangleTexture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_RECTANGLETEXTURE;}
	RectangleTexture(Class_base* c,Context3D* _context):TextureBase(c,_context){ subtype = SUBTYPE_RECTANGLETEXTURE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};
class VideoTexture: public TextureBase
{
public:
	VideoTexture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_VIDEOTEXTURE;}
	VideoTexture(Class_base* c,Context3D* _context):TextureBase(c,_context){ subtype = SUBTYPE_VIDEOTEXTURE;}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER(int,videoHeight);
	ASPROPERTY_GETTER(int,videoWidth);
	ASFUNCTION_ATOM(attachCamera);
	ASFUNCTION_ATOM(attachNetStream);
};

}
#endif // FLASHDISPLAY3DTEXTURES_H
