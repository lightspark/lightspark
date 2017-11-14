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
	int32_t width;
	int32_t height;
	vector<_NR<BitmapContainer>> bitmaparray;
public:
	TextureBase(Class_base* c):EventDispatcher(c),textureID(UINT32_MAX),width(0),height(0){ subtype = SUBTYPE_TEXTUREBASE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
};

class Texture: public TextureBase
{
public:
	Texture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_TEXTURE; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadCompressedTextureFromByteArray);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};

class CubeTexture: public TextureBase
{
public:
	CubeTexture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_CUBETEXTURE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadCompressedTextureFromByteArray);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};
class RectangleTexture: public TextureBase
{
public:
	RectangleTexture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_RECTANGLETEXTURE;}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(uploadFromBitmapData);
	ASFUNCTION_ATOM(uploadFromByteArray);
};
class VideoTexture: public TextureBase
{
public:
	VideoTexture(Class_base* c):TextureBase(c){ subtype = SUBTYPE_VIDEOTEXTURE;}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER(int,videoHeight);
	ASPROPERTY_GETTER(int,videoWidth);
	ASFUNCTION_ATOM(attachCamera);
	ASFUNCTION_ATOM(attachNetStream);
};

}
#endif // FLASHDISPLAY3DTEXTURES_H
